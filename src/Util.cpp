//
// Created by 陈子锐 on 2024/2/3.
//

#include "Util.h"
#include <pthread.h>
#include <execinfo.h>
#include "Log.h"
#include "Fiber.h"
#include <sys/syscall.h>

namespace Server {

    Logger::ptr g_logger = LOG_NAME("system");

    uint32_t GetFiberId() {
        return Fiber::GetFiberId();
    }

    pid_t GetThreadId() {
        return syscall(SYS_gettid);
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip) {
        void **array = (void **) malloc(sizeof(void *) * size);
        int s = ::backtrace(array, size);

        char **strings = backtrace_symbols(array, s);
        if (strings == nullptr) {
            LOGE(g_logger) << "backtrace_symbols error";
            return;
        }
        for (size_t i = skip; i < s; i++) {
            bt.emplace_back(strings[i]);
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, std::string &prefix, int skip) {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (const auto &ele: bt) {
            ss << prefix << ele << std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMS() {
        struct timeval tv{};
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
    }

    uint64_t GetCurrentUS() {
        struct timeval tv{};
        gettimeofday(&tv, nullptr);
        return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
    }
}