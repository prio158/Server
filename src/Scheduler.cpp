//
// Created by czr on 24-2-29.
//

#include "Scheduler.h"

#include <utility>
#include "Log.h"

namespace Server {

    static auto logger = LOG_ROOT();

    static thread_local Scheduler *t_scheduler = nullptr; // 当前schedule
    static thread_local Fiber *t_main_schedule_fiber = nullptr; // main schedule fiber

    static bool isStateTermOrExcept(const Fiber::ptr &fiber) {
        return fiber->getState() != Fiber::TERM || fiber->getState() != Fiber::EXCEPT;
    }

    static bool isStateTermAndExcept(const Fiber::ptr &fiber) {
        return fiber->getState() != Fiber::TERM && fiber->getState() != Fiber::EXCEPT;
    }

    Scheduler::Scheduler(size_t threads, bool use_caller, std::string name) : m_name(std::move(name)) {
        SERVER_ASSERT(threads > 0)
        if (use_caller) {
            LOGD(logger) << name << "Scheduler::Scheduler";
            /// init main fiber===>Fiber::t_thread_fiber
            Fiber::GetThis();
            --threads;
            SERVER_ASSERT(GetThis() == nullptr)
            t_scheduler = this;
            /// 这个创建的协程是调度协程，用来调度任务的，fiberId=0, Scheduler::run是调度方法
            /// 那这个Scheduler::run什么时候被调用呢？ 或者更广泛地说，Fiber的callback什么时候被调有呢？
            /// 在执行swapcontext(其他Fiber->m_ctx, currentFiber->m_ctx)之后，就会调用currentFiber　callback
            /// 而swapcontext(其他Fiber->m_ctx, currentFiber->m_ctx)其实就是为了将currentFiber->m_ctx切换到执行栈上，
            /// 为currentFiber　callback的执行做铺垫．
            m_scheduleFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0,true));
            Thread::SetName(m_name);
            t_main_schedule_fiber = m_scheduleFiber.get();
            m_mainThreadId = GetThreadId();
            m_threadIds.emplace_back(m_mainThreadId);
        } else {
            m_mainThreadId = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler() {
        SERVER_ASSERT(m_stopping)
        if (GetThis() == this) {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::GetThis() {
        return t_scheduler;
    }


    Fiber *Scheduler::GetMainScheduleFiber() {
        return t_main_schedule_fiber;
    }

    ///start thread pool, create m_threadCount thread
    void Scheduler::start() {
        MutexType::Lock lock(m_mutex);
        LOGD(logger) << m_name << " Schedule::start";
        if (!m_stopping) {
            return;
        }
        m_stopping = false;
        SERVER_ASSERT(m_threads.empty())
        m_threads.resize(m_threadCount);
        ///创建指定的工作线程，，每个线程都持有调度方法Scheduler::run
        for (size_t index = 0; index < m_threadCount; index++) {
            m_threads[index].reset(new Thread(std::bind(&Scheduler::run, this), m_name + std::to_string(index)));
            m_threadIds.emplace_back(m_threads[index]->getId());
        }
        lock.unlock();
//        if (m_scheduleFiber) {
//            m_scheduleFiber->call();
//        }
    }


    /// cycle wait
    void Scheduler::stop() {
        m_auto_stop = true;
        if (m_scheduleFiber && m_threadCount == 0 &&
            (m_scheduleFiber->getState() == Fiber::TERM || m_scheduleFiber->getState() == Fiber::INIT)) {
            LOGD(logger) << "Scheduler::stop";
            m_stopping = true;
            if (stopping()) {
                return;
            }
        }

        /// is use_caller state
        if (m_mainThreadId != -1) {
            ///保证当前线程是创建Scheduler的那个线程，符合use_caller的定义
            ///因为use_caller是把创建Scheduler的那个线程加入进去Scheduler中
            ///所以这里断言，是保证此处执行的线程是===》创建Scheduler的那个线程
            SERVER_ASSERT(GetThis() == this)
        } else {
            SERVER_ASSERT(GetThis() != this)
        }
        m_stopping = true;
        ///tickle every thread
        for (size_t index = 0; index < m_threadCount; index++) {
            tickle();
        }
        ///schedule fiber start execute runs
        if (m_scheduleFiber) {
            if (!stopping())
                m_scheduleFiber->call();
        }

        std::vector<Thread::ptr> threads;
        {
            MutexType::Lock lock(m_mutex);
            threads.swap(m_threads);
        }
        for (auto &thread: threads) {
            thread->join();
        }

    }

    void Scheduler::tickle() {
        LOGI(logger) << "Scheduler::tickle";
    }

    ///协程调度模块的核心部分：协调协程与线程之间的调度
    void Scheduler::run() {
        LOGD(logger) << m_name << " Scheduler::run";
        ///把当前的Schedule设置到t_scheduler
        setThis();
        if (GetThreadId() != m_mainThreadId) {
            t_main_schedule_fiber = Fiber::GetThis().get();
        }
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        ///注意：这里调用的是Fiber的默认构造函数，状态初始置为EXEC
        Fiber::ptr cb_fiber; //this fiber finish  callback task
        FiberAndThread ft;
        while (true) {
            ft.reset();
            bool tickle_me = false;
            bool is_active = false;
            {
                MutexType::Lock lock(m_mutex);
                ///从消息队列中取出任务
                auto it = m_task_queue.begin();
                while (it != m_task_queue.end()) {
                    /// 不在该任务指定的thread上,那么这个thread就不处理该任务，只是发出通知
                    if (it->threadId != -1 && it->threadId != GetThreadId()) {
                        ++it;
                        /// 通知其他线程处理该任务
                        tickle_me = true;
                        continue;
                    }
                    ///首先要有协程或要执行的任务
                    SERVER_ASSERT(it->fiber || it->cb)
                    ///其次协程不能处于忙碌执行状态
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                        ++it;
                        continue;
                    }
                    ///取出任务
                    ft = *it;
                    is_active = true;
                    ///从消息队列中删除任务
                    m_task_queue.erase(it);
                    ++m_activeThreadCount;
                    break;
                }
                tickle_me |= it != m_task_queue.end();
            }

            if (tickle_me)
                tickle(); ///通知其他线程

            ///如果有要执行的协程,且状态不是结束状态
            if (ft.fiber && isStateTermAndExcept(ft.fiber)) {
                ft.fiber->swapIn();
                --m_activeThreadCount;
                if (ft.fiber->getState() == Fiber::READY) {
                    /// 如果ft.fiber在READY狀態中，丢进消息队列
                    post(ft.fiber);
                } else if (isStateTermAndExcept(ft.fiber)) {
                    ///　ft.fiber如果没有结束，进入暂停状态
                    ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();
                ///如果有要执行的cb,就把这个cb给cb_fiber协程，它就是用来执行cb的一个临时协程
            } else if (ft.cb) {
                if (cb_fiber)
                    cb_fiber->reset(ft.cb);
                else
                    cb_fiber.reset(new Fiber(ft.cb));

                ft.reset();
                cb_fiber->swapIn();
                --m_activeThreadCount;
                ///cb_fiber协程执行完后，回到这里，然后在执行到这里
                if (cb_fiber->getState() == Fiber::READY) {
                    post(cb_fiber);
                    cb_fiber.reset();
                } else if (isStateTermOrExcept(cb_fiber)) {
                    cb_fiber->reset(nullptr);
                } else {
                    ///cb_fiber->getState() != Fiber::TERM 没有结束
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            } else {
                if (is_active) {
                    --m_activeThreadCount;
                    continue;
                }
                /// 走到这里就代表没有可以执行的任务了
                /// 下面进入ＩＤＥＡ状态
                if (idle_fiber->getState() == Fiber::TERM) {
                    LOGD(logger) << "Idle fiber term";
                    break;
                }
                ++m_idleThreadCount;
                //LOGD(logger) << "Idle fiber will swapIn";
                idle_fiber->swapIn();
                --m_idleThreadCount;
                if (isStateTermAndExcept(idle_fiber)) {
                    idle_fiber->m_state = Fiber::HOLD;
                }
            }
        }
    }

    void Scheduler::setThis() {
        t_scheduler = this;
    }

    ///协程没有任务做，且所在的线程又不能终止，那就让它执行idle协程，这个方法
    ///是一个虚函数，具体由子类完成
    void Scheduler::idle() {
        LOGD(logger) << "Idle fiber running...";
//        while (!stopping()) {
//            Fiber::YieldToHold();
//        }
    }

    bool Scheduler::stopping() {
        MutexType::Lock lock(m_mutex);
        return m_auto_stop && m_stopping && m_task_queue.empty() && m_activeThreadCount == 0;
    }

    std::ostream &Scheduler::dump(std::ostream &os) {
        os << "[Scheduler name=" << m_name
           << " size=" << m_threadCount
           << " active_count=" << m_activeThreadCount
           << " idle_count=" << m_idleThreadCount
           << " stopping=" << m_stopping
           << " ]" << std::endl << "    ";
        for (size_t i = 0; i < m_threadIds.size(); ++i) {
            if (i) {
                os << ", ";
            }
            os << m_threadIds[i];
        }
        return os;
    }


}