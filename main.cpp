#include <cstdio>
#include <string>
#include <string_view>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "hack.h"
#include "zygisk.hpp"
#include "log.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;

namespace {
bool isTargetProcess(int moduleDir, std::string_view processName) {
    if (moduleDir < 0 || processName.empty()) {
        return false;
    }

    int fd = openat(moduleDir, "target.txt", O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) {
        return false;
    }

    struct stat info{};
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode)) {
        close(fd);
        return false;
    }

    FILE *file = fdopen(fd, "r");
    if (!file) {
        close(fd);
        return false;
    }

    char buffer[512];
    size_t length = fread(buffer, 1, sizeof(buffer), file);
    bool readable = length < sizeof(buffer) && !ferror(file);
    fclose(file);
    if (!readable) {
        return false;
    }

    std::string_view target(buffer, length);
    if (target.starts_with("\xEF\xBB\xBF")) {
        target.remove_prefix(3);
    }
    auto start = target.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return false;
    }
    auto end = target.find_last_not_of(" \t\r\n");
    return target.substr(start, end - start + 1) == processName;
}
}

class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        if (!args->nice_name || !args->app_data_dir) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        const char *processName = env->GetStringUTFChars(args->nice_name, nullptr);
        if (!processName) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
            return;
        }

        int moduleDir = api->getModuleDir();
        if (isTargetProcess(moduleDir, processName)) {
            const char *appDataDir = env->GetStringUTFChars(args->app_data_dir, nullptr);
            if (appDataDir) {
                gameDataDir = appDataDir;
                env->ReleaseStringUTFChars(args->app_data_dir, appDataDir);
                if (!gameDataDir.empty()) {
                    LOGI("detect game: %s", processName);
                    prepareNativeBridge(moduleDir);
                }
            }
        }
        if (moduleDir >= 0) {
            close(moduleDir);
        }
        env->ReleaseStringUTFChars(args->nice_name, processName);

        if (gameDataDir.empty()) {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (!gameDataDir.empty()) {
            std::thread hack_thread(hack_prepare, gameDataDir.c_str(), data, length);
            hack_thread.detach();
        }
    }

private:
    Api *api;
    JNIEnv *env;
    std::string gameDataDir;
    void *data = nullptr;
    size_t length = 0;

    void prepareNativeBridge([[maybe_unused]] int moduleDir) {
#if defined(__i386__)
        auto path = "zygisk/armeabi-v7a.so";
#endif
#if defined(__x86_64__)
        auto path = "zygisk/arm64-v8a.so";
#endif
#if defined(__i386__) || defined(__x86_64__)
        int fd = openat(moduleDir, path, O_RDONLY | O_CLOEXEC);
        if (fd != -1) {
            struct stat sb{};
            if (fstat(fd, &sb) == 0 && sb.st_size > 0) {
                length = sb.st_size;
                void *mapping = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
                if (mapping != MAP_FAILED) {
                    data = mapping;
                }
            }
            close(fd);
        }
        if (!data) {
            LOGW("Unable to map ARM module");
        }
#endif
    }
};

REGISTER_ZYGISK_MODULE(MyModule)
