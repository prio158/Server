//
// Created by 陈子锐 on 2024/2/23.
//

#include "Thread.h"
#include "Log.h"
#include "Util.h"


static Server::Logger::ptr g_logger = LOG_NAME("system");


namespace Server {

    static thread_local Thread *t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOW";

    Server::Thread *Server::Thread::GetThis() {
        return t_thread;
    }

    const std::string &Server::Thread::GetName() {
        return t_thread_name;

    }

    Server::Thread::Thread(const std::function<void()> &cb, const std::string &name)
    : m_cb(cb), m_name(name) {
        if (name.empty()) {
            m_name = "UN_KNOW";
        }
        int rt = pthread_create(&m_thread, nullptr, run, this);
        if (rt < 0) {
            LOGE(g_logger) << "pthread_create thread fail,rt=" << rt << " name=" << name;
            throw std::logic_error("pthread_create error");
        }
        m_semaphore.wait(); //阻塞等待，直到线程运行起来，解除阻塞
    }

    Server::Thread::~Thread() {
        if (m_thread) {
            pthread_detach(m_thread);
        }
    }

    void Server::Thread::join() {
        if (m_thread) {
            int rt = pthread_join(m_thread, nullptr);
            if (rt < 0) {
                LOGE(g_logger) << "pthread_join thread fail,rt=" << rt << " name=" << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }

    void Thread::SetName(const std::string &name) {
        if (t_thread) {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    void *Thread::run(void *args) {
        auto *thread = (Thread *) args;
        t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = GetThreadId();
        pthread_setname_np(t_thread->m_thread, thread->m_name.substr(0, 15).c_str());
        std::function<void()> cb;
        cb.swap(thread->m_cb);
        thread->m_semaphore.notify(); //线程运行起来了，发出通知
        cb();
        return nullptr;
    }


    Semaphore::Semaphore(uint32_t count) {
        if (sem_init(m_semaphore, 0, count)) {
            throw std::logic_error("sem_init error");
        }
    }

    Semaphore::~Semaphore() {
        sem_close(m_semaphore);
    }

    void Semaphore::wait() {
        //sem_wait，会将m_semaphore--，当m_semaphore为 0 时，就会等待
        if (sem_wait(m_semaphore) < 0) {
            throw std::logic_error("sem wait error");
        }
    }

    void Semaphore::notify() {
        //sem_post 会将m_semaphore++
        if (sem_post(m_semaphore) < 0) {
            throw std::logic_error("sem post error");
        }
    }
}