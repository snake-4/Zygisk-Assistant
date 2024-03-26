#include <unistd.h>
#include <sched.h>

#include <sys/mount.h>

#include "zygisk.hpp"
#include "logging.hpp"
#include "android_filesystem_config.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

void do_unmount();

static int shouldSkipUid(int uid)
{
    int appid = uid % AID_USER_OFFSET;
    if (appid >= AID_APP_START && appid <= AID_APP_END)
        return false;
    if (appid >= AID_ISOLATED_START && appid <= AID_ISOLATED_END)
        return false;
    return true;
}

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
        if (isRoot || !isOnDenylist || shouldSkipUid(args->uid))
        {
            return;
        }

        LOGD("Unmounting in preAppSpecialize for pid=%d uid=%d", getpid(), args->uid);

        /*
         * preAppSpecialize is before ensureInAppMountNamespace.
         * postAppSpecialize is after seccomp setup.
         * So we unshare here to create a pseudo app mount namespace
         */
        if (unshare(CLONE_NEWNS) == -1)
        {
            LOGE("unshare(CLONE_NEWNS) returned -1: %d (%s)", errno, strerror(errno));
            // Don't unmount anything in global namespace
            return;
        }

        /*
         * Mount the pseudo app mount namespace's root as MS_SLAVE, so every mount/umount from
         * Zygote shared pre-specialization mountspace is propagated to this one.
         */
        if (mount("rootfs", "/", NULL, (MS_SLAVE | MS_REC), NULL) == -1)
        {
            LOGE("mount(\"rootfs\", \"/\", NULL, (MS_SLAVE | MS_REC), NULL) returned -1");
        }

        do_unmount();
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(ZygiskModule)
