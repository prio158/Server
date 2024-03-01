//
// Created by czr on 24-3-2.
//

#include "Log.h"
#include "Scheduler.h"


void idleFiberScheduleTest() {
    Server::Scheduler::ptr scheduler(new Server::Scheduler());
    scheduler->start();
    scheduler->stop();
}

void testFiberTest() {
    LOGI(LOG_ROOT()) << "testFiberTest running...";
}

void taskFiberSchedule() {
    Server::Scheduler::ptr scheduler(new Server::Scheduler());
    scheduler->start();
    scheduler->schedule(&testFiberTest);
    scheduler->stop();
}

int main() {

    //idleFiberScheduleTest();
    taskFiberSchedule();
}