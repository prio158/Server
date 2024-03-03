//
// Created by 陈子锐 on 2024/2/25.
//

#include "Fiber.h"
#include "Config.h"
#include <atomic>
#include <utility>
#include "Scheduler.h"

namespace Server {

    static std::atomic<uint64_t> s_fiber_id(0);
    static std::atomic<uint64_t> s_fiber_count(0);
    ///定义一个线程局部变量
    ///对象的存储在线程开始时分配，而在线程结束时自动释放。
    ///线程之间就不会因为访问同一全局对象而引起资源竞争导致性能下降
    static thread_local Fiber *t_fiber = nullptr; //　临时　sub fiber
    static thread_local Fiber::ptr t_thread_fiber = nullptr; //main_fiber

    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
            Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024,
                                     "fiber stack size");

    class MallocStackAllocator {
    public:
        static void *Alloc(size_t size) {
            return malloc(size);
        }

        static void Dealloc(void *vp, size_t size) {
            return free(vp);
        }
    };

    using StackAllocator = MallocStackAllocator;


    Fiber::Fiber(std::function<void()> cb, size_t stack_size, bool use_caller) :
            m_id(++s_fiber_id), m_cb(std::move(cb)) {
        ++s_fiber_count;
        m_stack_size = stack_size ? stack_size : g_fiber_stack_size->getValue();
        m_stack = StackAllocator::Alloc(m_stack_size);
        if (getcontext(&m_ctx)) {
            SERVER_ASSERT2(false, "getcontext");
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_size = m_stack_size;
        m_ctx.uc_stack.ss_sp = m_stack;
        if (!use_caller) {
            /// 这里没有直接将cb作为this fiber执行的回调，而是静态设置为MainFunc，在MainFunc中再执行cb, Hook operation
            makecontext(&m_ctx, MainFunc, 0);
        } else {
            makecontext(&m_ctx, MainFuncCaller, 0);
        }
    }

    Fiber::Fiber() {
        m_state = EXEC;
        SetThis(this);
        if (getcontext(&m_ctx)) {
            SERVER_ASSERT2(false, "getcontext");
        }
        s_fiber_count++;
    }

    void Fiber::SetThis(Fiber *f) {
        t_fiber = f;
    }

    Fiber::ptr Fiber::GetThis() {
        if (t_fiber) {
            return t_fiber->shared_from_this();
        }
        Fiber::ptr main_fiber(new Fiber);
        SERVER_ASSERT(t_fiber == main_fiber.get());
        t_thread_fiber = main_fiber;
        return t_fiber->shared_from_this();
    }


    Fiber::~Fiber() {
        --s_fiber_count;
        LOGD(LOG_ROOT()) << "~Fiber,fiberId=" << m_id;
        if (m_stack) {
            SERVER_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT)
            StackAllocator::Dealloc(m_stack, m_stack_size);
        } else {
            SERVER_ASSERT(!m_cb);
            SERVER_ASSERT(m_state == EXEC);
            Fiber *cur = t_fiber;
            if (cur == this) {
                SetThis(nullptr);
            }
        }
    }

    void Fiber::reset(std::function<void()> cb) {
        ///  m_stack must be exist when reset
        SERVER_ASSERT(m_stack);
        /// m_state must be TERN or INIT or EXCEPT when reset, m_state in running can not be reset.
        SERVER_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);

        m_cb = std::move(cb);
        if (getcontext(&m_ctx)) {
            SERVER_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_size = m_stack_size;
        m_ctx.uc_stack.ss_sp = m_stack;
        /// 这里没有直接将cb作为this fiber执行的回调，而是静态设置为MainFunc，在MainFunc中再执行ｃｂ
        makecontext(&m_ctx, MainFunc, 0);
        m_state = INIT;
    }

    void Fiber::swapIn() {
        SetThis(this);
        SERVER_ASSERT(m_state != EXEC)
        m_state = EXEC;
        /// old fiber context ----> new fiber context
//        LOGD(LOG_ROOT()) << "Fiber::swapIn==>" << "[FiberId:" <<
//                         Scheduler::GetMainScheduleFiber()->m_id << "]" << "--->" << "[FiberId:" << m_id << "]";
        if (swapcontext(&(Scheduler::GetMainScheduleFiber()->m_ctx), &m_ctx)) {
            SERVER_ASSERT2(false, "swapcontext");
        }
    }

    void Fiber::swapOut() {
        /// old fiber context ----> new fiber context
        SetThis(Scheduler::GetMainScheduleFiber());
//        LOGD(LOG_ROOT()) << "Fiber::swapOut==>" << "[FiberId:" << m_id << "]" << "--->" << "[FiberId:"
//                         << Scheduler::GetMainScheduleFiber()->m_id << "]";
        if (swapcontext(&m_ctx, &(Scheduler::GetMainScheduleFiber()->m_ctx))) {
            SERVER_ASSERT2(false, "swap context")
        }
    }

    void Fiber::call() {
        SetThis(this);
        m_state = EXEC;
        LOGD(LOG_ROOT()) << "Fiber::call==>" << "[FiberId:" << t_thread_fiber->m_id << "]"
                         << "--->" << "[FiberId:" << m_id << "]";
        if (swapcontext(&t_thread_fiber->m_ctx, &m_ctx)) {
            SERVER_ASSERT2(false, "swap context");
        }
    }

    void Fiber::back() {
        SetThis(t_thread_fiber.get());
        LOGD(LOG_ROOT()) << "Fiber::back==>" << "[FiberId:" << m_id << "]" << "--->"
                         << "[FiberId:" << t_thread_fiber->m_id << "]";
        if (swapcontext(&m_ctx, &t_thread_fiber->m_ctx)) {
            SERVER_ASSERT2(false, "swap context");
        }
    }

    void Fiber::YieldToReady() {
        /// get current fiber
        Fiber::ptr curFiber = GetThis();
        curFiber->m_state = READY;
        /// current fiber swap out to background
        curFiber->swapOut();
    }

    void Fiber::YieldToHold() {
        /// get current fiber
        Fiber::ptr curFiber = GetThis();
        curFiber->m_state = HOLD;
        /// current fiber swap out to background
        curFiber->swapOut();
    }

    uint64_t Fiber::TotalFibers() {
        return s_fiber_count;
    }

    void Fiber::MainFunc() {
        Fiber::ptr currentFiber = GetThis();
        SERVER_ASSERT(currentFiber)
        LOGD(LOG_ROOT()) << "Id=" << currentFiber->m_id << " Fiber::MainFunc Execute";
        try {
            currentFiber->m_cb();
            currentFiber->m_cb = nullptr;
            currentFiber->m_state = TERM;
        } catch (std::exception &e) {
            currentFiber->m_state = EXCEPT;
            LOGE(LOG_ROOT()) << "Fiber Except" << e.what() << std::endl << BacktraceToString();
        } catch (...) {
            currentFiber->m_state = EXCEPT;
            LOGE(LOG_ROOT()) << "Fiber Except";
        }
        auto raw_ptr = currentFiber.get();
        ///release this currentFiber reference, decrease a reference
        currentFiber.reset();
        /// current fiber execute finish , return last fiber continue execute
        LOGD(LOG_ROOT()) << "Fiber will swapOut From MainFunc";
        raw_ptr->swapOut();
    }

    void Fiber::MainFuncCaller() {
        Fiber::ptr currentFiber = GetThis();
        SERVER_ASSERT(currentFiber)
        LOGD(LOG_ROOT()) << "Id=" << currentFiber->m_id << " Fiber::MainFuncCaller Execute";
        try {
            currentFiber->m_cb();
            currentFiber->m_cb = nullptr;
            currentFiber->m_state = TERM;
        } catch (std::exception &e) {
            currentFiber->m_state = EXCEPT;
            LOGE(LOG_ROOT()) << "Fiber Except" << e.what() << std::endl << BacktraceToString();
        } catch (...) {
            currentFiber->m_state = EXCEPT;
            LOGE(LOG_ROOT()) << "Fiber Except";
        }
        auto raw_ptr = currentFiber.get();
        ///release this currentFiber reference, decrease a reference
        currentFiber.reset();
        /// current fiber execute finish , return last fiber continue execute
        LOGD(LOG_ROOT()) << "Fiber will back From MainFuncCaller";
        raw_ptr->back();
    }

    uint64_t Fiber::GetFiberId() {
        if (t_fiber) return t_fiber->m_id;
        return 0;
    }


}
