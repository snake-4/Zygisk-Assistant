#include <string>

#include "map_parser.hpp"
#include "utils.hpp"
#include "android_filesystem_config.h"
#include "zygisk.hpp"

bool hook_plt_by_name(zygisk::Api *api, const std::string &libName, const std::string &symbolName, void *hookFunc, void **origFunc)
{
    for (const auto &map : parseMapsFromPath("/proc/self/maps"))
    {
        if (map.getPathname().ends_with("/" + libName))
        {
            api->pltHookRegister(map.getDevice(), map.getInode(), symbolName.c_str(), hookFunc, origFunc);
            return true;
        }
    }
    return false;
}

int is_userapp_uid(int uid)
{
    int appid = uid % AID_USER_OFFSET;
    if (appid >= AID_APP_START && appid <= AID_APP_END)
        return true;
    if (appid >= AID_ISOLATED_START && appid <= AID_ISOLATED_END)
        return true;
    return false;
}
