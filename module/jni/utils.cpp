#include <string>
#include <format>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>

#include "map_parser.hpp"
#include "utils.hpp"
#include "android_filesystem_config.h"
#include "zygisk.hpp"
#include "logging.hpp"

bool hookPLTByName(zygisk::Api *api, const std::string &libName, const std::string &symbolName, void *hookFunc, void **origFunc)
{
    for (const auto &map : parseSelfMaps())
    {
        if (map.getPathname().ends_with("/" + libName))
        {
            api->pltHookRegister(map.getDevice(), map.getInode(), symbolName.c_str(), hookFunc, origFunc);
            return true;
        }
    }
    return false;
}

int isUserAppUID(int uid)
{
    int appid = uid % AID_USER_OFFSET;
    if (appid >= AID_APP_START && appid <= AID_APP_END)
        return true;
    if (appid >= AID_ISOLATED_START && appid <= AID_ISOLATED_END)
        return true;
    return false;
}

bool switchMountNS(int pid)
{
    std::string path = std::string("/proc/") + std::to_string(pid) + "/ns/mnt";
    int ret, fd;
    if ((fd = open(path.c_str(), O_RDONLY)) < 0)
    {
        return false;
    }

    ret = setns(fd, 0);
    close(fd);
    return ret == 0;
}
