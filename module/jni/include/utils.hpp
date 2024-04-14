#pragma once
#include <string>
#include "zygisk.hpp"

#define DCL_HOOK_FUNC(ret, func, ...) \
    ret (*old_##func)(__VA_ARGS__);   \
    ret new_##func(__VA_ARGS__)

int isUserAppUID(int uid);
bool hookPLTByName(zygisk::Api *api, const std::string &libName, const std::string &symbolName, void *hookFunc, void **origFunc);
