#include <string.h>
#include <string>
#include <functional>
#include <format>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>
#include <fcntl.h>

#include "map_parser.hpp"
#include "utils.hpp"
#include "android_filesystem_config.h"
#include "zygisk.hpp"
#include "logging.hpp"

using namespace Utils;

size_t Utils::safeStringCopy(char *dest, const char *src, size_t dest_size)
{
    if (dest_size < 1)
        return 0;

    *dest = 0;
    // strncpy does not null terminate, strlcpy fails on non-terminated src
    // strncat is relatively safe but may write count+1 because of the null terminator
    strncat(dest, src, dest_size - 1);

    // strlen is safe here because dest is now null-terminated
    return strlen(dest);
}

bool Utils::hookPLTByName(zygisk::Api *api, const std::string &libName, const std::string &symbolName, void *hookFunc, void **origFunc)
{
    for (const auto &map : Parsers::parseSelfMaps())
    {
        if (map.getPathname().ends_with("/" + libName))
        {
            api->pltHookRegister(map.getDevice(), map.getInode(), symbolName.c_str(), hookFunc, origFunc);
            return true;
        }
    }
    return false;
}

int Utils::isUserAppUID(int uid)
{
    int appid = uid % AID_USER_OFFSET;
    if (appid >= AID_APP_START && appid <= AID_APP_END)
        return true;
    if (appid >= AID_ISOLATED_START && appid <= AID_ISOLATED_END)
        return true;
    return false;
}

bool Utils::switchMountNS(int pid)
{
    std::string path = "/proc/" + std::to_string(pid) + "/ns/mnt";
    int ret, fd;
    if ((fd = open(path.c_str(), O_RDONLY)) < 0)
    {
        return false;
    }

    ret = setns(fd, 0);
    close(fd);
    return ret == 0;
}

int Utils::executeLambdaInFork(const std::function<void()> &lambda)
{
    pid_t pid = fork();
    ASSERT_DO(executeLambdaInFork, pid != -1, return -1);

    if (pid == 0)
    {
        // Child process
        lambda();
        exit(EXIT_SUCCESS);
    }
    else
    {
        // Parent process
        int status = -1;
        waitpid(pid, &status, 0);
        return status;
    }
}
