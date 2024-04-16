#include <unistd.h>
#include <sched.h>
#include <sys/mount.h>

#include "zygisk.hpp"
#include "logging.hpp"
#include "utils.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

void doUnmount();
void doRemount();

/*
 * [What's the purpose of this hook?]
 * Calling unshare twice invalidates existing FD links, which fails Zygote sanity checks.
 * So we prevent further namespaces by hooking unshare.
 *
 * [Doesn't Android already call unshare?]
 * Whether there's going to be an unshare or not changes with each major Android version
 * so we unconditionally unshare in preAppSpecialize.
 * > Android 5: Conditionally unshares
 * > Android 6: Always unshares
 * > Android 7-11: Conditionally unshares
 * > Android 12-14: Always unshares
 */
DCL_HOOK_FUNC(static int, unshare, int flags)
{
    doUnmount();
    doRemount();

    // Do not allow CLONE_NEWNS.
    flags &= ~(CLONE_NEWNS);
    if (!flags)
    {
        // If CLONE_NEWNS was the only flag, skip the call.
        errno = 0;
        return 0;
    }
    return old_unshare(flags);
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
        if (isRoot || !isOnDenylist || !isUserAppUID(args->uid))
        {
            LOGD("Skipping pid=%d ppid=%d uid=%d", getpid(), getppid(), args->uid);
            return;
        }
        LOGD("Processing pid=%d ppid=%d uid=%d", getpid(), getppid(), args->uid);

        /*
         * Read the comment above unshare hook.
         */
        if (unshare(CLONE_NEWNS) == -1)
        {
            LOGE("unshare(CLONE_NEWNS) returned -1: %d (%s)", errno, strerror(errno));
            return;
        }

        /*
         * Mount the app mount namespace's root as MS_SLAVE, so every mount/umount from
         * Zygote shared pre-specialization namespace is propagated to this one.
         */
        if (mount("rootfs", "/", NULL, (MS_SLAVE | MS_REC), NULL) == -1)
        {
            LOGE("mount(\"rootfs\", \"/\", NULL, (MS_SLAVE | MS_REC), NULL) returned -1: %d (%s)", errno, strerror(errno));
            return;
        }

        if (!hookPLTByName("libandroid_runtime.so", "unshare", new_unshare, &old_unshare))
        {
            LOGE("plt_hook_wrapper(\"libandroid_runtime.so\", \"unshare\", new_unshare, old_unshare) returned false");
            return;
        }
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override
    {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override
    {
        if (old_unshare != nullptr && !hookPLTByName("libandroid_runtime.so", "unshare", old_unshare))
        {
            LOGE("plt_hook_wrapper(\"libandroid_runtime.so\", \"unshare\", old_unshare) returned false");
        }
    }

    template <typename T>
    bool hookPLTByName(const std::string &libName, const std::string &symbolName, T *hookFunction, T **originalFunction = nullptr)
    {
        return ::hookPLTByName(api, libName, symbolName, (void *)hookFunction, (void **)originalFunction) && api->pltHookCommit();
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(ZygiskModule)
