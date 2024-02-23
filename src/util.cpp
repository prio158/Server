//
// Created by 陈子锐 on 2024/2/3.
//

#include "util.h"
#include <pthread.h>

namespace Server {

    uint32_t GetFiberId() {
        return 0;
    }

    pid_t GetThreadId() {
        uint64_t tid;
        pthread_threadid_np(nullptr, &tid);
        return tid;
    }
}