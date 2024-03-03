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

static int s_count1 = 5;
static int s_count2 = 5;

void testFiberTest() {

    if (--s_count1>0) {
        // post task
        sleep(1);
        LOGI(LOG_ROOT()) << "testFiberTest1 running " << s_count1;
        Server::Scheduler::GetThis()->post(&testFiberTest);
    }
}

void testFiberTest2() {

    if (--s_count2>0) {
        // post task
        sleep(1);
        LOGI(LOG_ROOT()) << "testFiberTest2 running "<< s_count2;
        Server::Scheduler::GetThis()->post(&testFiberTest2);
    }
}

void taskFiberSchedule() {
    Server::Scheduler::ptr scheduler(new Server::Scheduler(1));
    scheduler->start();
    scheduler->post(&testFiberTest);
    scheduler->post(&testFiberTest2);
    scheduler->stop();
}

int main() {

    //idleFiberScheduleTest();
    taskFiberSchedule();
}