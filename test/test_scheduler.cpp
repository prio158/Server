//
// Created by czr on 24-3-2.
//

#include "Log.h"
#include "Config.h"
#include "Scheduler.h"
#include "Fiber.h"
#include "Thread.h"

void idleFiberScheduleTest() {
    Server::Scheduler::ptr scheduler(new Server::Scheduler());
    scheduler->start();
    scheduler->stop();
}

int main() {

    idleFiberScheduleTest();

}