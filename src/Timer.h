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
#include <vector>

///enable_shared_from_this的用法：https://blog.csdn.net/breadheart/article/details/112451022
namespace Server {
    class TimerManager;

    ///定时器模块
    class Timer : public std::enable_shared_from_this<Timer> {
        friend class TimerManager;

    public:
        typedef std::shared_ptr<Timer> ptr;

        explicit Timer(uint64_t ms, std::function<void()> cb, TimerManager *manager,bool recurring = false);

        explicit Timer(uint64_t next);

        /* return shared this*/
        ptr GetThis() {
            return shared_from_this();
        }

        bool cancel();

        /* 更新该定时器的执行时间为now + m_ms */
        bool refresh();

        bool reset(uint64_t ms,bool from_now);

    private:
        //是否循环定时器
        bool m_recurring = false;
        //执行周期　　　
        uint64_t m_ms = 0;
        //定期器的执行时间
        uint64_t m_next = 0;
        //定时器要执行的任务
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
        ///std::weak_ptr<void> weak_cond，通过智能指针修饰条件，借助 weak_ptr 类型指针，(不会使这个对象的引用计数＋１)
        ///我们可以获取 shared_ptr 指针的一些状态信息，比如有多少指向相同的 shared_ptr 指针
        ///如果没有指向shared_ptr的指针，代表条件结束．https://c.biancheng.net/view/7918.html
        Timer::ptr addConditionTimer(uint64_t ms, const std::function<void()>& cb, const std::weak_ptr<void>& weak_cond, bool recurring = false);

        ///获取下一个定时器执行的时间
        uint64_t getNextTimer();

        ///返回已经超时的定时器
        void listExpiredTimer(std::vector<std::function<void()>>& cbs);

        bool hasTimer();

    private:
        /**
         * @brief 检测服务器时间是否被调后了
         */
        bool detectClockRollover(uint64_t now_ms);

    protected:
        /// notify
        virtual void onTimerInsertedAtFront() = 0;
        void addTimer(const Timer::ptr& timer);

    private:
        RWMutexType m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers;
        bool m_tickled = false;
        /// 上次执行时间
        uint64_t m_preTime{};
    };
}

#endif //SERVER_TIMER_H
