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
    class Fiber : public std::enable_shared_from_this<Fiber> {
        /**继承enable_shared_from_this，会提一个方法，可以获取当前类的智能指针
         * 同时Fiber不能在栈上创建对象了，
         * */
    public:
        typedef std::shared_ptr<Fiber> ptr;

        enum State {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY
        };


    public:
        explicit Fiber();

        explicit Fiber(std::function<void()> cb, size_t stack_size = 0);

        ~Fiber();

        ///重置协程函数，并重置状态为INIT
        void reset(std::function<void()> cb);

        ///切换当前协程执行
        void swapIn();

        ///切换当前协程到后台
        void swapOut();

    public:
        ///设置当前协程
        static void SetThis(Fiber *f);

        ///获取当前协程
        static Fiber::ptr GetThis();

        /// 协程切换到后台，并设置为Ready状态
        static void YieldToReady();

        /// 协程切换到后台，并设置为Hold状态
        static void YieldToHold();

        ///总协程数
        static uint64_t TotalFibers();

        static void MainFunc();

    private:
        uint64_t m_id = 0;
        uint64_t m_stack_size = 0;
        State m_state = INIT;
        ucontext_t m_ctx{};
        void *m_stack = nullptr;
        std::function<void()> cb;
    };
}


#endif //SERVER_FIBER_H
