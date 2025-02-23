#include <fstream>
#include <string.h>
#include <cstdint>
#include <string>
#include <functional>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>
#include <fcntl.h>
#include <endian.h>

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
    if ((fd = open(path.c_str(), O_RDONLY | O_CLOEXEC)) < 0)
    {
        return false;
    }

    ret = setns(fd, 0);
    close(fd);
    return ret == 0;
}

int Utils::forkAndInvoke(const std::function<int()> &lambda)
{
    pid_t pid = fork();
    if (pid == -1)
        return -1;

    if (pid == 0) // Child process
        exit(lambda());

    // Parent process
    int status = -1;
    waitpid(pid, &status, 0);
    return status;
}

constexpr off_t EXT_SUPERBLOCK_OFFSET = 0x400;
constexpr off_t EXT_MAGIC_OFFSET = 0x38;
constexpr off_t EXT_ERRORS_OFFSET = 0x3C;
constexpr uint16_t EXT_MAGIC = 0xEF53;

const char *Utils::getExtErrorsBehavior(const Parsers::mountinfo_entry &entry)
{
    auto fs_type = entry.getFilesystemType();
    if (fs_type != "ext2" && fs_type != "ext3" && fs_type != "ext4")
        return nullptr;

    std::ifstream file(entry.getMountSource(), std::ios::binary);
    if (!file)
        return nullptr;

    uint16_t magic;
    file.seekg(EXT_SUPERBLOCK_OFFSET + EXT_MAGIC_OFFSET, std::ios::beg);
    file.read(reinterpret_cast<char *>(&magic), sizeof(magic));
    if (!file || file.gcount() != sizeof(magic))
        return nullptr;
    magic = le16toh(magic);

    if (magic != EXT_MAGIC)
        return nullptr;

    uint16_t errors;
    file.seekg(EXT_SUPERBLOCK_OFFSET + EXT_ERRORS_OFFSET, std::ios::beg);
    file.read(reinterpret_cast<char *>(&errors), sizeof(errors));
    if (!file || file.gcount() != sizeof(errors))
        return nullptr;
    errors = le16toh(errors);

    switch (errors)
    {
    case 1:
        return "continue";
    case 2:
        return "remount-ro";
    case 3:
        return "panic";
    default:
        return nullptr;
    }
}
