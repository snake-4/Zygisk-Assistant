#include <string>
#include <vector>
#include <array>

#include <sys/mount.h>

#include "zygisk.hpp"
#include "logging.hpp"
#include "mount_parser.hpp"
#include "mountinfo_parser.hpp"

constexpr std::array<const char *, 4> fsname_list = {"KSU", "APatch", "magisk", "worker"};

static bool shouldUnmount(const mountinfo_entry_t &mount_info)
{
    const auto &root = mount_info.getRoot();

    // Unmount all module bind mounts
    if (root.rfind("/adb/", 0) == 0)
        return true;

    return false;
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
    bool doesFsnameMatch = std::find(fsname_list.begin(), fsname_list.end(), mount.getFsName()) != fsname_list.end();
    if ((type == "overlay" || type == "tmpfs") && doesFsnameMatch)
        return true;

    // Unmount all overlayfs with lowerdir/upperdir/workdir starting with /data/adb
    if (type == "overlay")
    {
        const auto &lowerdir = options.find("lowerdir");
        const auto &upperdir = options.find("upperdir");
        const auto &workdir = options.find("workdir");

        if (lowerdir != options.end() && lowerdir->second.rfind("/data/adb", 0) == 0)
            return true;

        if (upperdir != options.end() && upperdir->second.rfind("/data/adb", 0) == 0)
            return true;

        if (workdir != options.end() && workdir->second.rfind("/data/adb", 0) == 0)
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
            LOGE("umount2(\"%s\", MNT_DETACH) returned -1: %d (%s)", mountPoint.c_str(), errno, strerror(errno));
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
        if (options.find("errors") != options.end() && options.at("errors") == "remount-ro")
        {
            if (mount(NULL, "/data", NULL, MS_REMOUNT, "errors=continue") == 0)
            {
                LOGD("mount(NULL, \"/data\", NULL, MS_REMOUNT, \"errors=continue\") returned 0");
            }
            else
            {
                LOGE("mount(NULL, \"/data\", NULL, MS_REMOUNT, \"errors=continue\") returned -1: %d (%s)", errno, strerror(errno));
            }
        }
    }
}
