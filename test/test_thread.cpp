//
// Created by 陈子锐 on 2024/2/23.
//
#include <vector>
#include "Thread.h"
#include "Log.h"

Server::Logger::ptr g_logger = LOG_ROOT();

static int count = 0;

static Server::RWMutex mutex;
static Server::Mutex s_mutex;
static Server::CASLock cas_mutex;

void fun1() {
    LOGI(g_logger) << " thread_Name=" << Server::Thread::GetName()
                   << " thread_Id=" << Server::GetThreadId()
                   << " this.Id=" << Server::Thread::GetThis()->getId()
                   << " this.name=" << Server::Thread::GetThis()->getName();

    for (int i = 0; i < 100000; ++i) {
        //Server::RWMutex::WriteLock lock(mutex);
        //Server::Mutex::Lock lock(s_mutex);
        Server::CASLock::Lock lock(cas_mutex);
        count++;
    }
}

void fun2() {

}


int main() {
    Server::Thread::SetName("main");
    LOGI(g_logger) << " thread test begin";
    std::vector<Server::Thread::ptr> thrs;
    for (int index = 0; index < 5; index++) {
        Server::Thread::ptr thr(new Server::Thread(&fun1, "sub_thread_" + std::to_string(index)));
        thrs.push_back(thr);
    }

    for (const auto &thr: thrs) {
        thr->join();
    }
    LOGI(g_logger) << " thread test end";
    LOGI(g_logger) << " count=" << count;
    return 0;
}