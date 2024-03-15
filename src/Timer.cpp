//
// Created by czr on 24-3-14.
//

#include "Timer.h"

#include <utility>

namespace Server {

    Timer::Timer(uint64_t ms, std::function<void()> cb, TimerManager *manager, bool recurring)
            : m_recurring(recurring), m_ms(ms), m_cb(std::move(cb)), m_manager(manager) {

        m_next = Server::GetCurrentMS() + m_ms;
    }

    TimerManager::TimerManager() {

    }

    TimerManager::~TimerManager() {

    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
        Timer::ptr timer(new Timer(ms, std::move(cb), this, recurring));
        RWMutexType::WriteLock lock(m_mutex);
        auto it = m_timers.insert(timer).first;
        /*插入后就立即检查一下:如果插入的定时器排在最前面,代表它的ms(执行周期)处于最小*/
        bool at_front = (it == m_timers.begin());
        lock.unlock();
        if (at_front) {
            /* 新插入timer的ms(执行周期)最小,通知IOSchedule重新设置epoll_wait的超时周期*/
            onTimerInsertedAtFront();
        }
        return timer;
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond,
                                               bool recurring) {
        return Server::Timer::ptr();
    }
}