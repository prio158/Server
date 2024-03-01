//
// Created by 陈子锐 on 2024/2/25.
//

#ifndef SERVER_FIBER_H
#define SERVER_FIBER_H

#include <memory>
#include <ucontext.h>
#include "Thread.h"
#include <functional>


namespace Server {
    /// 协程
    /// Thread ---> main fiber <-----> sub fiber1
    ///                  |----> sub fiber2
    /// main fiber can switch any sub fiber,but any sub fiber can not switch other sub fiber
    /// only switch main fiber
    /// each thread have a main fiber created by Fiber::GetThis()
    /// [Server::Fiber::ptr fiber(new Server::Fiber(run_in_fiber))] can create a sub fiber
    /// sub_fiber->swapIn(); //switch sub_fiber execute, and suspend main fiber
    /// sub fiber swapOut by [YieldToReady], switch main fiber continue execute from suspend point
    class Fiber : public std::enable_shared_from_this<Fiber> {
        /**继承enable_shared_from_this，会提一个方法，可以获取当前类的智能指针
         * 同时Fiber不能在栈上创建对象了，
         * */
        friend class Scheduler;

    public:
        ///shard_ptr: https://blog.csdn.net/hj605635529/article/details/76984839
        typedef std::shared_ptr<Fiber> ptr;

        /**
         * @brief 协程状态
         */
        enum State {
            /// 初始化状态
            INIT,
            /// 暂停状态
            HOLD,
            /// 执行中状态
            EXEC,
            /// 结束状态
            TERM,
            /// 可执行状态
            READY,
            /// 异常状态
            EXCEPT
        };

    public:
        explicit Fiber();

        explicit Fiber(std::function<void()> cb, size_t stack_size = 0,bool use_caller =false);

        ~Fiber();

        ///重置协程函数，并重置状态为INIT
        void reset(std::function<void()> cb);

        ///切换当前协程执行(main fiber suspend), main fiber ----> sub fiber
        void swapIn();

        ///切换当前协程到后台(sub fiber suspend), sub fiber ----> main fiber
        void swapOut();

        /**
        * @brief 将当前线程切换到执行状态
        * @pre 执行的为当前线程的主协程
        */
        void call();

        /**
         * @biref 执行的为该协程
         * @post 返回到线程的主协程
         */
        void back();

        State getState() const { return m_state; }


    public:
        ///设置当前协程
        static void SetThis(Fiber *f);

        ///获取当前协程
        static Fiber::ptr GetThis();

        /// 协程切换到后台(suspend)，并设置为Ready状态
        static void YieldToReady();

        /// 协程切换到后台(suspend)，并设置为Hold状态
        static void YieldToHold();


        ///总协程数
        static uint64_t TotalFibers();

        ///m_cb execute hook callback
        static void MainFunc();

        static void MainFuncCaller();

        ///get fiber id
        static uint64_t GetFiberId();

    private:
        uint64_t m_id = 0;
        uint64_t m_stack_size = 0;
        State m_state = INIT;
        ucontext_t m_ctx{};
        void *m_stack = nullptr;
        std::function<void()> m_cb;
    };
}


#endif //SERVER_FIBER_H
