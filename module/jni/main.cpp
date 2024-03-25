#include <unistd.h>
#include <sched.h>

#include "zygisk.hpp"
#include "logging.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

void do_unmount();

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
            LOGD("Unmounting in preAppSpecialize for pid=%d uid=%d", getpid(), args->uid);

            /*
             * preAppSpecialize is before ensureInAppMountNamespace.
             * postAppSpecialize is after seccomp setup.
             * So we unshare here to create a pseudo app mount namespace
             */
            if (unshare(CLONE_NEWNS) == 0)
            {
                LOGD("unshare(CLONE_NEWNS) returned 0");
                do_unmount();
            }
            else
            {
                LOGE("unshare(CLONE_NEWNS) returned -1: %d (%s)", errno, strerror(errno));
            }
        }
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(ZygiskModule)
