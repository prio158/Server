//
// Created by czr on 24-3-14.
//

#ifndef SERVER_TIMER_H
#define SERVER_TIMER_H

#include <memory>
#include "Thread.h"
#include "Mutex.h"
#include "Util.h"
#include <functional>
#include <set>

///enable_shared_from_this的用法：https://blog.csdn.net/breadheart/article/details/112451022
namespace Server {
    class TimerManager;

    ///定时器模块
    class Timer : public std::enable_shared_from_this<Timer> {
        friend class TimerManager;

    public:
        typedef std::shared_ptr<Timer> ptr;

        explicit Timer(uint64_t ms, std::function<void()> cb, TimerManager *manager,bool recurring = false);

        /* return shared this*/
        ptr GetThis() {
            return shared_from_this();
        }

        /* addTimeTask*/

        /* cancelTimeTask*/

        /* 获取当前定时器触发离现在的时间差*/

        /* 返回当前需要触发的定时器　*/
    private:
        //是否循环定时器
        bool m_recurring = false;
        //执行周期　　　
        uint64_t m_ms = 0;
        //精确的执行时间
        uint64_t m_next = 0;

        std::function<void()> m_cb;

        TimerManager* m_manager = nullptr;

    private:
        struct Comparator {
            bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs) const {
                if (!lhs && !rhs) return false;
                if (!lhs) return true;
                if (!rhs) return false;
                if (lhs->m_next < rhs->m_next) return true;
                if (lhs->m_next > lhs->m_next) return false;
                return lhs.get() < rhs.get();
            }
        };

    };

    class TimerManager {
        friend class Timer;

    public:
        typedef RWMutex RWMutexType;

        explicit TimerManager();

        virtual ~TimerManager();

        ///添加定时器任务
        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

        ///添加条件定时器任务
        ///std::weak_ptr<void> weak_cond，通过智能指针修饰条件，借助 weak_ptr 类型指针，
        ///我们可以获取 shared_ptr 指针的一些状态信息，比如有多少指向相同的 shared_ptr 指针
        ///如果没有指向shared_ptr的指针，代表条件结束．https://c.biancheng.net/view/7918.html
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond, bool recurring = false);

    protected:
        /// notify
        virtual void onTimerInsertedAtFront() = 0;

    private:
        RWMutexType m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers;

    };
}

#endif //SERVER_TIMER_H
