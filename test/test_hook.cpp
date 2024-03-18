//
// Created by czr on 24-3-17.
//


#include "Hook.h"
#include "IOSchedule.h"
#include "Log.h"


void test_sleep() {
    Server::IOSchedule::ptr ioSchedule(new Server::IOSchedule(1));
    ioSchedule->post([]() {
        sleep(2);
        LOGI(LOG_ROOT()) << "test_sleep2...";
    });
    ioSchedule->post([]() {
        sleep(4);
        LOGI(LOG_ROOT()) << "test_sleep3...";
    });
    LOGI(LOG_ROOT()) << "test_sleep out...";
}

int main() {
    test_sleep();
}