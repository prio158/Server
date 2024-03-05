//
// Created by czr on 24-2-29.
//

#ifndef SERVER_MUTEX_H
#define SERVER_MUTEX_H


#include <cstdlib>
#include <thread>
#include <memory>
#include <functional>
#include <pthread.h>
#include <semaphore.h>
#include <atomic>
#include <string>

namespace Server {

    class Semaphore {
    public:

        /**
         * 互斥量的初始值为1，表示对共享数据区域，每次只能一个线程去访问，相当于串行访问公共数据区域，虽然安全，但是并发性差。
         * 信号量：相当于初始化值为count的互斥量，表示每次允许N个线程并发（同时）访问公共数据区域。
         * */
        explicit Semaphore(uint32_t count = 0);

        ~Semaphore();

        /**
         * @brief sem_wait: 信号量＞０，则信号量－－，不会阻塞
         * 　　　　　　　　　　信号量＝０，造成线程阻塞
         * */
        void wait();

        /**
         * @brief sem_post: 信号量++，同时唤醒阻塞在信号量上的线程
         * 　　　　　　　　　　
         * */
        void notify();

    public:
        Semaphore(const Semaphore &) = delete;

        Semaphore(const Semaphore &&) = delete;

        Semaphore operator=(const Semaphore &) = delete;

    private:
        sem_t m_semaphore;

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

    /** 互斥锁*/
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

    /** 读写锁*/
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

    /**
     * @brief 空读写锁(用于调试)
     */
    class NullRWMutex {
    public:
        typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
        typedef WriteScopedLockImpl<NullRWMutex> WriteLock;

        NullRWMutex() {}

        ~NullRWMutex() {}

        void rdlock() {}

        void wrlock() {}

        void unlock() {}
    };

    /**
     * @brief 原子锁（compare and swap）
     *  CAS 是一种基于硬件层面的原子操作。它比较内存中的值与给定的期望值，
     *  如果相等，则将该内存位置的值更新为新值。这是一个非阻塞操作，意味着
     *  它不会等待其他线程，而是立即返回结果。
     *
     *  CAS是通过无限循环来获取数据的，若果在第一轮循环中，a线程获取地址里
     *  面的值被b线程修改了，那么a线程需要自旋，到下次循环才有可能机会执行。
     *
     *  在使用上，通常会记录下某块内存中的旧值，通过对旧值进行一系列的操作后得到新值，
     *  然后通过CAS操作将新值与旧值进行交换。如果这块内存的值在这期间内没被修改过，则
     *  旧值会与内存中的数据相同，这时CAS操作将会成功执行 使内存中的数据变为新值。如果
     *  内存中的值在这期间内被修改过，则一般[2]来说旧值会与内存中的数据不同，这时CAS操
     *  作将会失败，新值将不会被写入内存。
     *  比如我们例子中的 count，如果 count 没有被其他线程修改，既 oldValue == *addr
     *  那么不会锁住，这个线程可能对 count 进行操作，如果被其他线程修改了，就锁住。
     *
    // int cas(long *addr, long old, long new)
    //{
    //    if(*addr != old)
    //       return 0;
    //    *addr = new;
    //    return 1;
    //}
     */

    class CASLock {
    public:
        typedef ScopedLockImpl<CASLock> Lock;

        CASLock() {
            m_mutex.clear();
        }

        ~CASLock() = default;

        void lock() {
            /**
             * acquire: 在锁被释放前一直等到，然后获取锁
             *
             * */
            while (std::atomic_flag_test_and_set_explicit(&m_mutex,
                                                          std::memory_order_acquire));

        }

        void unlock() {
            /**
             * release：解锁并唤醒任何等带中的进程
             * */
            std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
        }

    private:
        volatile std::atomic_flag m_mutex{};
    };


}

#endif //SERVER_MUTEX_H
