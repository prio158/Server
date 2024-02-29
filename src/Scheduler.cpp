//
// Created by czr on 24-2-29.
//

#include "Scheduler.h"
#include "Log.h"

namespace Server {

    static auto logger = LOG_NAME("system");


    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) {

    }

    Scheduler *Scheduler::GetThis() {
        return nullptr;
    }

    Fiber *Scheduler::GetMainFiber() {
        return nullptr;
    }

    void Scheduler::start() {

    }

    void Scheduler::stop() {

    }

    void Scheduler::tickle() {

    }
}