//
// Created by czr on 24-3-9.
//

#include "IOSchedule.h"
#include "Log.h"

Server::Logger::ptr g_logger = LOG_ROOT();

void test_fiber(){
    LOGI(g_logger) << "test_fiber running...";
}

/**
 * CALL StackTrace:
 * 1. Scheduler::Scheduler(), create main schedule fiber
 * 2. IOSchedule::IOSchedule(), call Scheduler::start
 * 3. IOSchedule::~IOSchedule(), call Scheduler::stop
 * 4. Scheduler::stop() --->  m_scheduleFiber->call
 * 5. main fiber id=0 --- swap context ---> schedule fiber id=1
 * 6. schedule fiber callback: MainFuncCaller ----> Execute Scheduler::run
 * 7. Scheduler::run ---> create cb_fiber id=3, task callback: test_fiber
 * 8. Fiber::MainFunc Execute --->  execute test_fiber
 * 9. cb_fiber id=3  --- swap out context ---> idle fiber id=2
 * 10.execute idle fiber callback: IOSchedule::idle
 * 11.
 * */
void test1(){
    Server::IOSchedule ioSchedule;
    ioSchedule.post(test_fiber);
}




int main() {


    test1();


}