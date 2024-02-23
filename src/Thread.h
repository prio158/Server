//
// Created by 陈子锐 on 2024/2/23.
//

#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H

#include <thread>
#include <memory>
#include <functional>
#include <pthread.h>

namespace Server {

    class Thread {
    public:
        typedef std::shared_ptr<Thread> ptr;

        explicit Thread(const std::function<void()>& cb, const std::string &name);

        ~Thread();

        pid_t getId() const { return m_id; }

        const std::string &getName() const { return m_name; }

        void join();

        static Thread *GetThis();

        static const std::string &GetName();

        static void SetName(const std::string &name);

        static void* run(void* args);

    private:
        pid_t m_id = -1;
        pthread_t m_thread = 0;
        std::function<void()> m_cb;
        std::string m_name;

    public:
        Thread(const Thread &) = delete;

        Thread(const Thread &&) = delete;

        Thread operator=(const Thread &) = delete;
    };
}


#endif //SERVER_THREAD_H
