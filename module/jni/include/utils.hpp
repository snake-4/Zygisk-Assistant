#pragma once
#include <string>
#include "zygisk.hpp"

#define DCL_HOOK_FUNC(ret, func, ...) \
    ret (*old_##func)(__VA_ARGS__);   \
    ret new_##func(__VA_ARGS__)

int is_userapp_uid(int uid);
bool hook_plt_by_name(zygisk::Api *api, const std::string &libName, const std::string &symbolName, void *hookFunc, void **origFunc);
