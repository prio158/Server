//
// Created by 陈子锐 on 2024/2/25.
//
#include "Log.h"

static Server::Logger::ptr g_logger = LOG_ROOT();

void test_assert() {
    LOGI(g_logger) << std::endl << Server::BacktraceToString(10, "   ") << std::endl;
    SERVER_ASSERT2(false,12)
    SERVER_ASSERT(true)
}


int main() {


    test_assert();
}