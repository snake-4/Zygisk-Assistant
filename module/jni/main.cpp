#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>

#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class ZygiskModule : public zygisk::ModuleBase
{
public:
    void onLoad(Api *api, JNIEnv *env) override
    {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override
    {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);

        uint32_t flags = api->getFlags();
        bool isRoot = (flags & zygisk::StateFlag::PROCESS_GRANTED_ROOT) != 0;
        bool isOnDenylist = (flags & zygisk::StateFlag::PROCESS_ON_DENYLIST) != 0;

        if (!isRoot && isOnDenylist && args->uid > 1000)
        {
            api->setOption(zygisk::Option::FORCE_DENYLIST_UNMOUNT);
        }
    }

    void preServerSpecialize(ServerSpecializeArgs *args)
    {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api *api;
    JNIEnv *env;

};

REGISTER_ZYGISK_MODULE(ZygiskModule)
