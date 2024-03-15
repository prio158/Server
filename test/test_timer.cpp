//
// Created by czr on 24-3-14.
//

#include "Timer.h"
#include "IOSchedule.h"
#include "Log.h"

void test1() {
    Server::IOSchedule ios(2);
    auto timer = ios.addTimer(500, []() {
        LOGI(LOG_ROOT()) << "timer emitting";
    }, true);

    //timer reset test passed
    sleep(3);
    LOGI(LOG_ROOT()) << "3ｓ后开始Timer reset 2000 period";
    timer->reset(2000, true);

    //timer cancel test passed
    sleep(3);
    LOGI(LOG_ROOT()) << "3ｓ后开始取消Ｔｉｍｅｒ";
    timer->cancel();
}



int main() {
    test1();

}