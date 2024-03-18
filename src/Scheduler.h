//
// Created by czr on 24-2-29.
//

#ifndef SERVER_SCHEDULER_H
#define SERVER_SCHEDULER_H

#include "Fiber.h"
#include "Mutex.h"
#include "Thread.h"
#include <list>
#include <utility>
#include <vector>
#include <functional>
#include "Hook.h"

///协程调度：协程在线程之际切换，一个线程中有一堆协程，如果这个线程很繁忙，那么它底下的协程可以切换到其他空闲的线程上执行。
///scheduler ----》 N个线程Thread ----》M个线程Thread ----》 多个协程
///协程调度：让协程在线程之间自由切换，将协程指定到相应的线程上执行 （N：M）
/// t_thread_fiber是main fiber, FiberId = 0
/// m_scheduleFiber是用来做调度的Fiber，FiberId = 1
/// idle_fiber 是调度器空闲下来时执行的Fiber　，　FiberId = 2
/// Fiber Id > 2 的Fiber才是执行任务的Fiber
namespace Server {

    class Scheduler {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

    public:
        /**
         * @brief 构造函数
         * @param[in] threads 线程数量
         * @param[in] use_caller 是否使用当前调用线程 (在那个线程调用这个构造函数，就会把该线程加入到调度器中)
         * @param[in] name 协程调度器名称
         */
        explicit Scheduler(size_t threads = 1, bool use_caller = true, std::string name = "UNKNOW");

        virtual ~Scheduler();

        const std::string &getName() const { return m_name; }

        /**
         * @brief 返回当前协程调度器
         */
        static Scheduler *GetThis();

        ///each thread have a main fiber ,and many sub fiber
        /**
         * @brief 返回当前协程调度器的调度协程
         */
        static Fiber *GetMainScheduleFiber();

        /**
         * @brief 启动协程调度器
         */
        void start();

        /**
         * @brief 停止协程调度器
         */
        void stop();

        std::ostream &dump(std::ostream &os);

        /**
         * @brief 调度协程,将任务压入消息队列
         * @param[in] fc task unit
         * @param[in] thread task unit执行的线程id,-1标识任意线程
         */
        template<class FiberOrCb>
        void post(FiberOrCb fc, int thread = -1) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }
            ///m_fibers is empty
            if (need_tickle) {
                tickle();
            }
        }

        /**
         * @brief 批量调度协程
         * @param[in] begin 协程数组的开始
         * @param[in] end 协程数组的结束
         */
        template<class InputIterator>
        void post(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end) {
                    need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                    begin++;
                }
                if (need_tickle)
                    tickle();
            }

        }

    protected:
        /**
         * @brief 通知协程调度器有任务了
         */
        virtual void tickle();

        /**
         * @brief 协程调度函数
         */
        void run();

        /**
          * @brief 返回是否可以停止
          */
        virtual bool stopping();

        /**
         * @brief 协程无任务可调度时执行idle协程
         */
        virtual void idle();

        /**
         * @brief 设置当前的协程调度器
         */
        void setThis();

        /**
         * @brief 是否有空闲线程
         */
        bool hasIdleThreads() { return m_idleThreadCount > 0; }

    private:
        /**
         * @brief 协程调度启动(无锁)
         */
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread) {
            bool need_tickle = m_task_queue.empty();
            FiberAndThread ft(fc, thread);
            if (ft.fiber || ft.cb) {
                m_task_queue.push_back(ft);
            }
            return need_tickle;
        }

    private:
        /**
         * @brief 协程/函数/线程组
         */
        struct FiberAndThread {
            ///协程
            Fiber::ptr fiber;
            ///协程要执行回调
            std::function<void()> cb;
            ///指定在ThreadId线程上处理该任务
            int threadId;

            ///外面在栈上定义的智能指针，传递进来，用这个构造函数，因为栈的生命周期会管理外面在栈上定义的智能指针
            FiberAndThread(Fiber::ptr f, int thread) : fiber(std::move(f)), threadId(thread) {}

            ///当外面在堆上定义智能指针的指针的时候，生命周期是整个程序，那么使用这个构造函数，内部通过swap释放外面的指针智针
            FiberAndThread(Fiber::ptr *f, int thread) : threadId(thread) {
                ///swap之后，f智能指针变为空值，它的引用也就减1
                fiber.swap(*f);
            }

            FiberAndThread(std::function<void()> f, int thread) : cb(std::move(f)), threadId(thread) {
            }

            FiberAndThread(std::function<void()> *f, int thread) : threadId(thread) {
                cb.swap(*f);
            }

            FiberAndThread() : threadId(-1) {}

            void reset() {
                fiber = nullptr;
                cb = nullptr;
                threadId = -1;
            }
        };

    private:
        MutexType m_mutex;
        /// thread pool
        std::vector<Thread::ptr> m_threads;
        /// FiberAndThread queue, 需要执行的task在这个队列里面，消息队列（待执行的任务队列）
        /// FiberAndThread里面包含了：Fiber、Thread、function ，都可以作为执行的task unit
        std::list<FiberAndThread> m_task_queue;
        /// use_caller为true时有效, 调度协程：main fiber是用来做调度的协程
        Fiber::ptr m_scheduleFiber;
        /// 协程调度器名称
        std::string m_name;

    protected:
        /// 协程下的线程id数组
        std::vector<int> m_threadIds;
        /// 线程数量
        size_t m_threadCount = 0;
        /// 工作线程数量
        std::atomic<size_t> m_activeThreadCount = {0};
        /// 空闲线程数量
        std::atomic<size_t> m_idleThreadCount = {0};
        /// 主线程id(use_caller)
        int m_mainThreadId = 0;
        /// 是否正在停止
        bool m_stopping = true;
        /// 是否自动停止
        bool m_auto_stop = false;
    };
}


#endif //SERVER_SCHEDULER_H
