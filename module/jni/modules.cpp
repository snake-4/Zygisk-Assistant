#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <cstdint>
#include <sys/mount.h>
#include <elfio/elfio.hpp>

#include "zygisk.hpp"
#include "logging.hpp"
#include "map_parser.hpp"
#include "mount_parser.hpp"
#include "mountinfo_parser.hpp"
#include "utils.hpp"

static const std::set<std::string> fsname_list = {"KSU", "APatch", "magisk", "worker"};
static const std::unordered_map<std::string, int> mount_flags_procfs = {
    {"nosuid", MS_NOSUID},
    {"nodev", MS_NODEV},
    {"noexec", MS_NOEXEC},
    {"noatime", MS_NOATIME},
    {"nodiratime", MS_NODIRATIME},
    {"relatime", MS_RELATIME},
    {"nosymfollow", MS_NOSYMFOLLOW}};

static bool shouldUnmount(const mountinfo_entry_t &mount_info)
{
    const auto &root = mount_info.getRoot();

    // Unmount all module bind mounts
    return root.starts_with("/adb/");
}

static bool shouldUnmount(const mount_entry_t &mount)
{
    const auto &mountPoint = mount.getMountPoint();
    const auto &type = mount.getType();
    const auto &options = mount.getOptions();

    // Unmount everything mounted to /data/adb
    if (mountPoint.rfind("/data/adb", 0) == 0)
        return true;

    // Unmount all module overlayfs and tmpfs
    if ((type == "overlay" || type == "tmpfs") && fsname_list.contains(mount.getFsName()))
        return true;

    // Unmount all overlayfs with lowerdir/upperdir/workdir starting with /data/adb
    if (type == "overlay")
    {
        if (options.contains("lowerdir") && options.at("lowerdir").starts_with("/data/adb"))
            return true;

        if (options.contains("upperdir") && options.at("upperdir").starts_with("/data/adb"))
            return true;

        if (options.contains("workdir") && options.at("workdir").starts_with("/data/adb"))
            return true;
    }

    return false;
}

void doUnmount()
{
    std::vector<std::string> mountPoints;

    // Check mounts first
    for (auto &mount : parseMountsFromPath("/proc/self/mounts"))
    {
        if (shouldUnmount(mount))
        {
            mountPoints.push_back(mount.getMountPoint());
        }
    }

    // Check mountinfos so that we can find bind mounts as well
    for (auto &mount_info : parseMountinfosFromPath("/proc/self/mountinfo"))
    {
        if (shouldUnmount(mount_info))
        {
            mountPoints.push_back(mount_info.getMountPoint());
        }
    }

    // Sort by string lengths, descending
    std::sort(mountPoints.begin(), mountPoints.end(), [](const auto &lhs, const auto &rhs)
              { return lhs.size() > rhs.size(); });

    for (const auto &mountPoint : mountPoints)
    {
        if (umount2(mountPoint.c_str(), MNT_DETACH) == 0)
        {
            LOGD("umount2(\"%s\", MNT_DETACH) returned 0", mountPoint.c_str());
        }
        else
        {
            LOGW("umount2(\"%s\", MNT_DETACH) returned -1: %d (%s)", mountPoint.c_str(), errno, strerror(errno));
        }
    }
}

void doRemount()
{
    std::vector<mount_entry_t> mounts = parseMountsFromPath("/proc/self/mounts");
    auto data_mount_it = std::find_if(mounts.begin(), mounts.end(), [](const mount_entry_t &mount)
                                      { return mount.getMountPoint() == "/data"; });
    if (data_mount_it != mounts.end())
    {
        const auto &options = data_mount_it->getOptions();

        // If errors=remount-ro, remount it with errors=continue
        if (options.contains("errors") && options.at("errors") == "remount-ro")
        {
            unsigned long flags = MS_REMOUNT;
            for (const auto &flagName : mount_flags_procfs)
            {
                if (options.contains(flagName.first))
                    flags |= flagName.second;
            }

            if (mount(NULL, "/data", NULL, flags, "errors=continue") == 0)
            {
                LOGD("mount(NULL, \"/data\", NULL, 0x%lx, \"errors=continue\") returned 0", flags);
            }
            else
            {
                LOGW("mount(NULL, \"/data\", NULL, 0x%lx, \"errors=continue\") returned -1: %d (%s)", flags, errno, strerror(errno));
            }
        }
    }
}

/*
 * Is it guaranteed to work? No.
 * At least it has lots of error checking so if something goes wrong
 * the state should remain relatively safe.
 */
void doHideZygisk()
{
    using namespace ELFIO;

    elfio reader;
    std::string filePath;
    uintptr_t startAddress = 0, bssAddress = 0;

    for (const auto &map : parseMapsFromPath("/proc/self/maps"))
    {
        if (map.getPathname().ends_with("/libnativebridge.so") && map.getPerms() == "r--p")
        {
            // First ro page should be the ELF header
            filePath = map.getPathname();
            startAddress = map.getAddressStart();
            break;
        }
    }

    ASSERT_EXIT("doHideZygisk", startAddress != 0, return);
    ASSERT_EXIT("doHideZygisk", reader.load(filePath), return);

    size_t bssSize = 0;
    for (const auto &sec : reader.sections)
    {
        if (sec->get_name() == ".bss")
        {
            bssAddress = startAddress + sec->get_address();
            bssSize = static_cast<size_t>(sec->get_size());
            break;
        }
    }

    ASSERT_EXIT("doHideZygisk", bssAddress != 0, return);
    LOGD("Found .bss for \"%s\" at 0x%" PRIxPTR " sized %" PRIuPTR " bytes.", filePath.c_str(), bssAddress, bssSize);

    uint8_t *pHadError = reinterpret_cast<uint8_t *>(memchr(reinterpret_cast<void *>(bssAddress), 0x01, bssSize));
    ASSERT_EXIT("doHideZygisk", pHadError != nullptr, return);
    *pHadError = 0;
}
