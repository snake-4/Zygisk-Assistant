#pragma once
#include <string>
#include <errno.h>
#include <functional>
#include "logging.hpp"
#include "zygisk.hpp"

#define DCL_HOOK_FUNC(ret, func, ...)         \
    ret (*old_##func)(__VA_ARGS__) = nullptr; \
    ret new_##func(__VA_ARGS__)

#define ASSERT_LOG(tag, expr)                                                                        \
    if (!(expr))                                                                                     \
    {                                                                                                \
        LOGE("%s:%d Assertion %s failed. %d:%s", #tag, __LINE__, #expr, errno, std::strerror(errno)); \
    }

#define ASSERT_DO(tag, expr, ret)                                                                  \
    if (!(expr))                                                                                     \
    {                                                                                                \
        LOGE("%s:%d Assertion %s failed. %d:%s", #tag, __LINE__, #expr, errno, std::strerror(errno)); \
        ret;                                                                                         \
    }

namespace Utils
{
    bool switchMountNS(int pid);
    int isUserAppUID(int uid);
    bool hookPLTByName(zygisk::Api *api, const std::string &libName, const std::string &symbolName, void *hookFunc, void **origFunc);
    int executeLambdaInFork(const std::function<void()> &lambda);
}
