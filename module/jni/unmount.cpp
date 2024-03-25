#include <string>
#include <vector>
#include <array>

#include <sys/mount.h>

#include "zygisk.hpp"
#include "logging.hpp"
#include "mount_parser.hpp"

constexpr std::array<const char *, 4> fsname_list = {"KSU", "APatch", "magisk", "worker"};

static bool shouldUnmount(const mount_entry_t &info)
{
    const auto &mountPoint = info.getMountPoint();
    const auto &type = info.getType();
    const auto &options = info.getOptions();

    // Unmount everything mounted to /data/adb
    if (mountPoint.rfind("/data/adb", 0) == 0)
        return true;

    // Unmount all module overlayfs and tmpfs
    bool doesFsnameMatch = std::find(fsname_list.begin(), fsname_list.end(), info.getFsName()) != fsname_list.end();
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

void do_unmount()
{
    std::vector<std::string> mountPoints;
    for (auto &info : parseMountsFromPath("/proc/self/mounts"))
    {
        if (shouldUnmount(info))
        {
            mountPoints.push_back(info.getMountPoint());
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
