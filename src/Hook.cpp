//
// Created by czr on 24-3-17.
//

#include "Hook.h"
#include "Fiber.h"
#include "IOSchedule.h"
#include "dlfcn.h"
#include "Log.h"

namespace Server {

    static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)       \
    XX(usleep)


    void hook_init() {
        static bool is_inited = false;
        if (is_inited) return;

///dlsym 动态加载库文件
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT,#name);
        HOOK_FUN(XX)
#undef XX
    }

    struct _HookIniter {
        _HookIniter() {
            hook_init();
        }
    };

    static _HookIniter s_hook_initer;

    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }


    extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX)
#undef XX

    unsigned int sleep(unsigned int seconds) {
        if (!t_hook_enable) {
            return sleep_f(seconds);
        }
        Fiber::ptr fiber = Server::Fiber::GetThis();
        auto ioSchedule = IOSchedule::GetThis();
        ioSchedule->addTimer(seconds * 1000, std::bind((void (Scheduler::*)
                (Fiber::ptr, int thread)) &IOSchedule::post, ioSchedule, fiber, -1));
        Fiber::YieldToHold();
        return 0;
    }

    int usleep(useconds_t usec) {
        if (!t_hook_enable) {
            return usleep_f(usec);
        }
        Fiber::ptr fiber = Server::Fiber::GetThis();
        fiber->reset([] {
            LOGI(LOG_ROOT()) << "usleep润的飞起";
        });
        auto ioSchedule = IOSchedule::GetThis();
        ioSchedule->addTimer(usec / 1000, std::bind((void (Scheduler::*)
                (Fiber::ptr, int thread)) &IOSchedule::post, ioSchedule, fiber, -1));
        Fiber::YieldToHold();
        return 0;
    }

    }
}