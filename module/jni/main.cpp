#include <unistd.h>
#include <sched.h>
#include <sys/mount.h>

#include <cstdint>
#include <functional>

#include "zygisk.hpp"
#include "logging.hpp"
#include "utils.hpp"
#include "modules.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

static std::function<void()> callbackFunction = []() {};

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
    callbackFunction();
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

/*
 * The reason why we hook setresuid is because so far it has been unconditionally called
 * and we still have CAP_SYS_ADMIN during this call.
 */
DCL_HOOK_FUNC(static int, setresuid, uid_t ruid, uid_t euid, uid_t suid)
{
    callbackFunction();
    return old_setresuid(ruid, euid, suid);
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
        bool isChildZygote = args->is_child_zygote != NULL && *args->is_child_zygote;
        if (isRoot || !isOnDenylist || !isUserAppUID(args->uid))
        {
            LOGD("Skipping ppid=%d uid=%d isChildZygote=%d", getppid(), args->uid, isChildZygote);
            return;
        }
        LOGD("Processing ppid=%d uid=%d isChildZygote=%d", getppid(), args->uid, isChildZygote);

        /*
         * Read the comment above unshare hook.
         */
        ASSERT_EXIT("preAppSpecialize", unshare(CLONE_NEWNS) != -1, return);

        /*
         * Mount the app mount namespace's root as MS_SLAVE, so every mount/umount from
         * Zygote shared pre-specialization namespace is propagated to this one.
         */
        ASSERT_EXIT("preAppSpecialize", mount("rootfs", "/", NULL, (MS_SLAVE | MS_REC), NULL) != -1, return);

        ASSERT_EXIT("preAppSpecialize", hookPLTByName("libandroid_runtime.so", "unshare", new_unshare, &old_unshare), return);
        ASSERT_EXIT("preAppSpecialize", hookPLTByName("libandroid_runtime.so", "setresuid", new_setresuid, &old_setresuid), return);

        int companionFd = -1;
        ASSERT_LOG("preAppSpecialize", (companionFd = api->connectCompanion()) != -1);
        ASSERT_LOG("preAppSpecialize", companionFd != -1 && api->exemptFd(companionFd));

        callbackFunction = [fd = companionFd]()
        {
            bool result = false;
            if (fd != -1)
            {
                do
                {
                    pid_t pid = getpid();
                    ASSERT_EXIT("invokeZygiskCompanion", write(fd, &pid, sizeof(pid)) == sizeof(pid), break);
                    ASSERT_EXIT("invokeZygiskCompanion", read(fd, &result, sizeof(result)) == sizeof(result), break);
                } while (false);
                close(fd);
            }

            if (result)
                LOGD("Invoking the companion was successful.");
            else
            {
                LOGW("Invoking the companion failed. Performing operations in Zygote context!");
                doUnmount();
                doRemount();
            }

            doHideZygisk();

            // Call only once per process.
            callbackFunction = []() {};
        };
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override
    {
        api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override
    {
        if (old_unshare != nullptr)
            ASSERT_LOG("postAppSpecialize", hookPLTByName("libandroid_runtime.so", "unshare", old_unshare));
        if (old_setresuid != nullptr)
            ASSERT_LOG("postAppSpecialize", hookPLTByName("libandroid_runtime.so", "setresuid", old_setresuid));
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

void zygisk_companion_handler(int fd)
{
    bool result = [&]() -> bool
    {
        pid_t pid;
        ASSERT_EXIT("zygisk_companion_handler", read(fd, &pid, sizeof(pid)) == sizeof(pid), return false);
        ASSERT_EXIT("zygisk_companion_handler", unshare(CLONE_NEWNS) != -1, return false);
        ASSERT_EXIT("zygisk_companion_handler", switchMountNS(pid), return false);
        LOGD("zygisk_companion_handler processing namespace of pid=%d", pid);

        doUnmount();
        doRemount();

        return true;
    }();

    ASSERT_LOG("zygisk_companion_handler", write(fd, &result, sizeof(result)) == sizeof(result));
}

REGISTER_ZYGISK_MODULE(ZygiskModule)
REGISTER_ZYGISK_COMPANION(zygisk_companion_handler)
