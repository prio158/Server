//
// Created by 陈子锐 on 2024/2/3.
//

#include "util.h"

namespace Server {

    uint32_t GetFiberId() {
        return 0;
    }

    pthread_t GetThreadId() {
        return pthread_self();
    }
}