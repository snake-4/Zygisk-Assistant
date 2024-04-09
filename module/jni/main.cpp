#include <unistd.h>
#include <sched.h>
#include <sys/mount.h>

#include "zygisk.hpp"
#include "logging.hpp"
#include "utils.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

void do_unmount();

DCL_HOOK_FUNC(static int, unshare, int flags)
{
    // Do not allow CLONE_NEWNS.
    flags &= ~(CLONE_NEWNS);
    if (!flags)
    {
        // If CLONE_NEWNS was the only flag, skip the call.
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
        if (isRoot || !isOnDenylist || !is_userapp_uid(args->uid))
        {
            LOGD("Skipping pid=%d ppid=%d uid=%d", getpid(), getppid(), args->uid);
            return;
        }

        LOGD("Processing pid=%d ppid=%d uid=%d", getpid(), getppid(), args->uid);

        /*
         * Calling unshare twice invalidates FD hard links, which fails Zygote sanity checks.
         * So we hook unshare to prevent further namespace creations.
         * The logic behind whether there's going to be an unshare or not changes with each major Android version.
         * For maximum compatibility, we will always unshare but prevent further unshare by this Zygote fork in appSpecialize.
         */
        if (!plt_hook_wrapper("libandroid_runtime.so", "unshare", new_unshare, old_unshare))
        {
            LOGE("plt_hook_wrapper(\"libandroid_runtime.so\", \"unshare\", new_unshare, old_unshare) returned false");
            return;
        }

        /*
         * preAppSpecialize is before any possible unshare calls.
         * postAppSpecialize is after seccomp setup.
         * So we unshare here to create an app mount namespace.
         */
        if (old_unshare(CLONE_NEWNS) == -1)
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
            LOGE("mount(\"rootfs\", \"/\", NULL, (MS_SLAVE | MS_REC), NULL) returned -1");
            return;
        }

        do_unmount();
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override
    {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

    template <typename Return, typename... Args>
    bool plt_hook_wrapper(const std::string &libName, const std::string &symbolName, Return (&hookFunction)(Args...), Return (*&originalFunction)(Args...))
    {
        return hook_plt_by_name(api, libName, symbolName, (void *)&hookFunction, (void **)&originalFunction) && api->pltHookCommit();
    }

private:
    Api *api;
    JNIEnv *env;
};

REGISTER_ZYGISK_MODULE(ZygiskModule)