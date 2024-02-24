//
// Created by 陈子锐 on 2024/2/23.
//

#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H

#include <thread>
#include <memory>
#include <functional>
#include <pthread.h>
#include <semaphore.h>

namespace Server {

    class Semaphore {
    public:

        /**
         * 信号量：相当于初始化值为count的互斥量，表示每次允许N个线程并发（同时）访问公共数据区域。
         * */
        explicit Semaphore(uint32_t count = 0);

        ~Semaphore();

        void wait();

        void notify();

    public:
        Semaphore(const Semaphore &) = delete;

        Semaphore(const Semaphore &&) = delete;

        Semaphore operator=(const Semaphore &) = delete;

    private:
        sem_t *m_semaphore;

    };

    template<class T>
    struct ScopedLockImpl {
    public:
        explicit ScopedLockImpl(T &mutex) : m_mutex(mutex) {
            m_mutex.lock();
            m_locked = true;
        }

        ~ScopedLockImpl() {
            unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.lock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    class Mutex {
    public:
        typedef ScopedLockImpl<Mutex> Lock;

        explicit Mutex() {
            pthread_mutex_init(&m_lock, nullptr);
        }

        ~Mutex() {
            pthread_mutex_destroy(&m_lock);
        }

        void lock() {
            pthread_mutex_lock(&m_lock);
        }

        void unlock() {
            pthread_mutex_unlock(&m_lock);
        }


    private:
        pthread_mutex_t m_lock{};
    };

    class NullMutex {
    public:
        typedef ScopedLockImpl<NullMutex> Lock;

        NullMutex() {}

        ~NullMutex() {}

        void lock();

        void unlock();
    };

    template<class T>
    struct ReadScopedLockImpl {
    public:
        explicit ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
            m_mutex.rdlock();
            m_locked = true;
        }

        ~ReadScopedLockImpl() {
            m_mutex.unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked;
    };

    template<class T>
    struct WriteScopedLockImpl {
    public:
        explicit WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
            m_mutex.wrlock();
            m_locked = true;
        }

        ~WriteScopedLockImpl() {
            unlock();
        }

        void lock() {
            if (!m_locked) {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock() {
            if (m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked = false;
    };


    class RWMutex {
    public:
        typedef ReadScopedLockImpl<RWMutex> ReadLock;
        typedef WriteScopedLockImpl<RWMutex> WriteLock;

        explicit RWMutex() {
            pthread_rwlock_init(&m_lock, nullptr);
        }

        ~RWMutex() {
            pthread_rwlock_destroy(&m_lock);
        }

        void rdlock() {
            pthread_rwlock_rdlock(&m_lock);
        }

        void wrlock() {
            pthread_rwlock_wrlock(&m_lock);
        }

        void unlock() {
            pthread_rwlock_unlock(&m_lock);
        }

    private:
        pthread_rwlock_t m_lock;
    };

    class NullRWMutex {
    public:
        typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
        typedef WriteScopedLockImpl<NullRWMutex> WriteLock;

        NullRWMutex() {}

        ~NullRWMutex() {}

        void rdlock() {
            //pthread_rwlock_rdlock(&m_lock);
        }

        void wrlock() {
            //pthread_rwlock_wrlock(&m_lock);
        }

        void unlock() {
            //pthread_rwlock_unlock(&m_lock);
        }
    };

    class Thread {
    public:
        typedef std::shared_ptr<Thread> ptr;

        explicit Thread(const std::function<void()> &cb, const std::string &name);

        ~Thread();

        pid_t getId() const { return m_id; }

        const std::string &getName() const { return m_name; }

        void join();

        static Thread *GetThis();

        static const std::string &GetName();

        static void SetName(const std::string &name);

        static void *run(void *args);

    private:
        pid_t m_id = -1;
        pthread_t m_thread = nullptr;
        std::function<void()> m_cb;
        std::string m_name;
        Semaphore m_semaphore;

    public:
        Thread(const Thread &) = delete;

        Thread(const Thread &&) = delete;

        Thread operator=(const Thread &) = delete;
    };


}


#endif //SERVER_THREAD_H
