#pragma once
#include <string>
#include <errno.h>
#include <functional>
#include "logging.hpp"
#include "zygisk.hpp"
#include "mountinfo_parser.hpp"

#define DCL_HOOK_FUNC(ret, func, ...)         \
    ret (*old_##func)(__VA_ARGS__) = nullptr; \
    ret new_##func(__VA_ARGS__)

#define ASSERT_LOG(tag, expr)                                                                         \
    if (!(expr))                                                                                      \
    {                                                                                                 \
        LOGE("%s:%d Assertion %s failed. %d:%s", #tag, __LINE__, #expr, errno, std::strerror(errno)); \
    }

#define ASSERT_DO(tag, expr, ret)                                                                     \
    if (!(expr))                                                                                      \
    {                                                                                                 \
        LOGE("%s:%d Assertion %s failed. %d:%s", #tag, __LINE__, #expr, errno, std::strerror(errno)); \
        ret;                                                                                          \
    }

namespace Utils
{
    /*
     * Always null terminates dest if dest_size is at least 1.
     * Writes at most dest_size bytes to dest including null terminator.
     * Reads at most dest_size bytes from src.
     * Returns strlen(dest)
     */
    size_t safeStringCopy(char *dest, const char *src, size_t dest_size);

    bool switchMountNS(int pid);
    int isUserAppUID(int uid);
    bool hookPLTByName(zygisk::Api *api, const std::string &libName, const std::string &symbolName, void *hookFunc, void **origFunc);
    int forkAndInvoke(const std::function<int()> &lambda);
    const char *getExtErrorsBehavior(const Parsers::mountinfo_entry &entry);
}
