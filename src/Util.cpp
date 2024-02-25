//
// Created by 陈子锐 on 2024/2/3.
//

#include "Util.h"
#include <pthread.h>
#include <execinfo.h>
#include "Log.h"

namespace Server {

    Logger::ptr g_logger = LOG_NAME("system");

    uint32_t GetFiberId() {
        return 0;
    }

    pid_t GetThreadId() {
        uint64_t tid;
        pthread_threadid_np(nullptr, &tid);
        return tid;
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

    std::string BacktraceToString(int size, const std::string &prefix, int skip) {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (const auto &ele: bt) {
            ss << prefix << ele << std::endl;
        }
        return ss.str();
    }
}