//
// Created by 陈子锐 on 2024/2/23.
//

#include "Thread.h"
#include "Log.h"
#include "util.h"

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

    Server::Thread::Thread(const std::function<void()> &cb, const std::string &name) {
        if (name.empty()) {
            m_name = "UNKNOW";
        }
        m_cb = cb;
        m_name = name;

        int rt = pthread_create(&m_thread, nullptr, run, this);
        if (rt < 0) {
            LOGE(g_logger) << "pthread_create thread fail,rt=" << rt << " name=" << name;
            throw std::logic_error("pthread_create error");
        }
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
            m_thread = nullptr;
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
        thread->m_id = GetThreadId();
        pthread_setname_np(thread->m_name.substr(0, 15).c_str());
        std::function<void()> cb;
        cb.swap(thread->m_cb);
        cb();
        return nullptr;
    }


}