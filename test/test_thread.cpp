//
// Created by 陈子锐 on 2024/2/23.
//
#include <vector>
#include "Thread.h"
#include "Log.h"

Server::Logger::ptr g_logger = LOG_ROOT();

void fun1() {
    LOGI(g_logger) << " thread_Name=" << Server::Thread::GetName()
                   << " thread_Id=" << Server::GetThreadId()
                   << " this.Id=" << Server::Thread::GetThis()->getId()
                   << " this.name=" << Server::Thread::GetThis()->getName();
}


int main() {
    LOGI(g_logger) << " thread test begin";
    std::vector<Server::Thread::ptr> thrs;
    for (int index = 0; index < 5; index++) {
        Server::Thread::ptr thr(new Server::Thread(&fun1, "name_" + std::to_string(index)));
        thrs.push_back(thr);
    }

    for (const auto &thr: thrs) {
        thr->join();
    }
    LOGI(g_logger) << " thread test end";
    return 0;
}