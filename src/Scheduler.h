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
///协程调度：协程在线程之际切换，一个线程中有一堆协程，如果这个线程很繁忙，那么它底下的协程可以切换到其他空闲的线程上执行。
///scheduler ----》 N个线程Thread ----》M个线程Thread ----》 多个协程
///协程调度：让协程在线程之间自由切换，将协程指定到相应的线程上执行 （N：M）

namespace Server {

    class Scheduler {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

    public:
        ///@brief
        ///@threads 创建的线程数
        ///@use_caller true: 在那个线程调用这个构造函数，就会把该线程加入到调度器中
        explicit Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");

        virtual ~Scheduler() = default;

        const std::string &getName() const { return m_name; }

        static Scheduler *GetThis();

        ///each thread have a main fiber ,and many sub fiber
        static Fiber *GetMainFiber();

        void start();

        void stop();

        template<class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1) {
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

        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end) {
                    begin++;
                    need_tickle = scheduleNoLock(&(*begin)) || need_tickle;
                }
                if(need_tickle)
                    tickle();
            }

        }

    protected:
        void tickle();

    private:
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread = -1) {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if (ft.fiber || ft.cb) {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        struct FiberAndThread {
            Fiber::ptr fiber;
            std::function<void()> cb;
            int threadId;

            ///外面在栈上定义的智能指针，传递进来，用这个构造函数，因为栈的生命周期会管理外面在栈上定义的智能指针
            FiberAndThread(Fiber::ptr f, int thread) : fiber(std::move(f)), threadId(thread) {}

            ///当外面在堆上定义智能指针的指针的时候，生命周期是整个程序，那么使用这个构造函数，内部通过swap释放外面的指针智针
            FiberAndThread(Fiber::ptr *f, int thread) : threadId(thread) {
                ///swap之后，f智能指针变为空值，它的引用也就减1
                fiber.swap(*f);
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
        std::vector<Thread::ptr> m_threads;
        std::list<FiberAndThread> m_fibers;
        std::string m_name;


    };
}


#endif //SERVER_SCHEDULER_H
