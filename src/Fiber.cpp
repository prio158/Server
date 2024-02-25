//
// Created by 陈子锐 on 2024/2/25.
//

#include "Fiber.h"
#include "Config.h"
#include <atomic>

namespace Server {

    static std::atomic<uint64_t> s_fiber_id(0);
    static std::atomic<uint64_t> s_fiber_count(0);
    ///定义一个线程局部变量
    ///对象的存储在线程开始时分配，而在线程结束时自动释放。
    ///线程之间就不会因为访问同一全局对象而引起资源竞争导致性能下降
    static thread_local Fiber *t_fiber = nullptr;
    static thread_local Fiber::ptr t_thread_fiber = nullptr;

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


    Fiber::Fiber(std::function<void()> cb, size_t stack_size) {

    }

    Fiber::Fiber() {
        m_state = EXEC;
        SetThis(this);
//        if(getcontext()){
//
//        }
    }

    void Fiber::SetThis(Fiber *f) {

    }

    Fiber::ptr Fiber::GetThis() {
        return Server::Fiber::ptr();
    }

}
