//
// Created by czr on 24-3-14.
//

#include "Timer.h"
#include "Log.h"
#include <utility>

namespace Server {

    Timer::Timer(uint64_t ms, std::function<void()> cb, TimerManager *manager, bool recurring)
            : m_recurring(recurring), m_ms(ms), m_cb(std::move(cb)), m_manager(manager) {

        /* 定期器的启动时间　＋　周期*/
        m_next = Server::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next) {

    }

    TimerManager::TimerManager() {
        m_preTime = GetCurrentMS();
    }

    TimerManager::~TimerManager() {

    }

    bool Timer::cancel() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        LOGD(LOG_ROOT()) << "Timer::cancel";
        if (m_cb) {
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(shared_from_this());
            return true;
        }
        return false;
    }

    bool Timer::reset(uint64_t ms, bool from_now) {
        if (ms == m_ms && !from_now) return true;
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end()) return false;
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        if (from_now) start = GetCurrentMS();
        else start = m_next - m_ms;
        m_ms = ms;
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this());
        return false;
    }

    bool Timer::refresh() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if (!m_cb) return false;
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it == m_manager->m_timers.end()) return false;
        m_manager->m_timers.erase(it);
        m_next = Server::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
        Timer::ptr timer(new Timer(ms, std::move(cb), this, recurring));
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer);
        ///虽然addTimer的作用是添加定时器，但是返回timer的目的是给调用方能够控制定时器
        return timer;
    }

    ///　执行定时任务
    static void onTimer(const std::weak_ptr<void> &weak_cond, const std::function<void()> &cb) {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if (tmp) {
            cb();
        }
    }

    Timer::ptr
    TimerManager::addConditionTimer(uint64_t ms, const std::function<void()> &cb, const std::weak_ptr<void> &weak_cond,
                                    bool recurring) {

        return addTimer(ms, [weak_cond, cb] { return onTimer(weak_cond, cb); }, recurring);
    }

    uint64_t TimerManager::getNextTimer() {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if (m_timers.empty()) return ~0ull;
        auto next_timer = *m_timers.begin();
        auto now_ms = Server::GetCurrentMS();
        /* 现在的时间超过了定时器设定的启动时间，那就马上执行*/
        if (now_ms >= next_timer->m_next) return 0;
        /* 周期　＝　next_timer->m_next（定期器的启动时间　＋　周期）　　－　now_ms（定期器的启动时间）*/
        /* 这里计算出的周期，是要作为epoll_wait中的time_out参数，通过epoll_wait定时超过time_out唤醒来
         * 实现定时器的驱动
         * */
        return next_timer->m_next - now_ms;
    }

    void TimerManager::listExpiredTimer(std::vector<std::function<void()>> &cbs) {
        uint64_t now_ms = Server::GetCurrentMS();
        std::vector<Timer::ptr> expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if (m_timers.empty()) return;
        }
        RWMutexType::WriteLock lock(m_mutex);

        bool rollover = detectClockRollover(now_ms);
        if (!rollover && (m_timers.begin()->get()->m_next == now_ms)) {
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));
        /// lower_bound: 从数组的ｂｅｇｉｎ位置到ｅｎｄ－１位置二分查找到第一个大于等于now_timer的ｉｔｅｒａｔｏｒ
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while (it != m_timers.end() && (*it)->m_next == now_ms) {
            ++it;
        }
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        //cbs.resize(expired.size());

        for (auto &timer: expired) {
            cbs.push_back(timer->m_cb);
            if (timer->m_recurring) {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            } else {
                timer->m_cb = nullptr;
            }
        }
    }

    void TimerManager::addTimer(const Timer::ptr &timer) {
        auto it = m_timers.insert(timer).first;
        /*插入后就立即检查一下:如果插入的定时器排在最前面,代表它的ms(执行周期)处于最小*/
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if (at_front) {
            m_tickled = true;
            /* 新插入timer的ms(执行周期)最小,通知IOSchedule重新设置epoll_wait的超时周期*/
            onTimerInsertedAtFront();
        }
    }

    bool TimerManager::detectClockRollover(uint64_t now_ms) {
        bool rollover = false;
        if (now_ms < (m_preTime - 3600 * 1000)) {
            rollover = true;
        }
        m_preTime = now_ms;
        return rollover;
    }

    bool TimerManager::hasTimer() {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }


}