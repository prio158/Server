//
// Created by czr on 24-3-4.
//

#ifndef SERVER_IOSCHEDULE_H
#define SERVER_IOSCHEDULE_H

#include "Scheduler.h"
#include <sys/epoll.h>
#include <csignal>

namespace Server {

    ///线程池：有一个消息队列，然后所有的线程从里面收发消息
    ///一般使用信号量来通知线程，消息队列里面有消息了．
    ///互斥量的初始值为1，表示对共享数据区域，每次只能一个线程去访问，相当于串行访问公共数据区域，虽然安全，但是并发性差。
    ///信号量：相当于初始化值为N的互斥量，表示每次允许N个线程并发（同时）访问公共数据区域。
    ///　　pushMessage(msg,smapthone); // sem_post 信号量 +1
    ///   pollMessage(msg,smapthone); // sem_wait 信号量 -1, 如果信号量＝０，造成线程阻塞，这不好，效率低
    ///更高效的做法：
    ///　　epoll_wait: 线程陷入epoll_wait会阻塞等待对发发消息，我们希望在epoll_wait等待对发消息唤醒的时候，
    ///  　如果消息队列中有消息，　它能够被唤醒起来处理这个消息．任务处理完，就再次陷入epoll_wait
    ///所以此时唤醒epoll_wait的方式有两种：
    /// 　［１］等待的消息返回了
    /// 　［２］消息队列中来消息了　　　
    class IOSchedule : public Scheduler {

    public:
        typedef std::shared_ptr<IOSchedule> ptr;
        typedef RWMutex RWMutexType;

        /**
         * @brief IO事件类型
         */
        enum Event {
            NONE = 0x0,
            READ = 0x1,
            WRITE = 0x2
        };

    public:
        explicit IOSchedule(size_t threads = 1, bool use_caller = true, const std::string &name = "");

        ~IOSchedule() override ;

        /**
         * @brief register new event to listen
         * @return 0:success, -1:error
         * */
        int addEvent(int fd, Event event, std::function<void()> read_callback = nullptr);

        bool removeEvent(int fd, Event event);

        bool cancelEvent(int fd, Event event);

        bool cancelAllEvent(int fd);

        static IOSchedule *GetThis();

        bool stopping(uint64_t& timeout) ;

    protected:
        void tickle() override;

        bool stopping() override;

        void idle() override;

        void contextArrayResize(size_t size);

    public:
        /**
         * @brief Socket事件上线文类
         */
        struct FdContext {
            typedef Mutex MutexType;

            /**
             * @brief 事件
             */
            struct EventContext {
                /// 事件执行的调度器
                Scheduler *scheduler;
                /// 事件协程
                Fiber::ptr fiber;
                /// 事件的回调函数
                std::function<void()> cb;
            };

            /**
             * @brief 触发事件
             * @param[in] event 事件类型
             */
            void triggerEvent(Event event);

            /**
             * @brief 获取事件上下文类
             * @param[in] event 事件类型
             * @return 返回对应事件的上线文
             */
            EventContext &getContext(Event event);

            /**
             * @brief 重置事件上下文
             * @param[in, out] ctx 待重置的上下文类
             */
            void resetContext(EventContext &ctx);

            ~FdContext() = default;

            /// 事件关联的句柄
            int fd = 0;
            /// 读事件上下文
            EventContext read;
            /// 写事件上下文
            EventContext write;
            /// 当前的事件
            Event m_events = NONE;
            /// 事件的Mutex
            MutexType mutex;
        };

    private:
        int m_epfd = 0;
        int m_tickleFds[2];
        ///等待执行的事件数,剩余要执行的任务数
        std::atomic<size_t> m_pendingEventCount = {0};
        RWMutexType m_mutex{};
        std::vector<FdContext *> m_fdContexts;
    };

} // Server

#endif //SERVER_IOSCHEDULE_H
