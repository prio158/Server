//
// Created by czr on 24-3-4.
//

#include "IOSchedule.h"
#include "Log.h"
#include <unistd.h>
#include <string>
#include <cstring>
#include <fcntl.h>

namespace Server {

    enum EpollCtlOp {
    };

    static void epoll_error_log(const std::string &epoll_method, int efd, int op, uint32_t event, int ret,
                                const IOSchedule::Event &m_events) {
        LOGE(LOG_ROOT()) << epoll_method
                         << "m_efd:" << efd << (EpollCtlOp) op << ", "
                         << (EPOLL_EVENTS) event << "):"
                         << ret << "(" << errno << ") (" << strerror(errno)
                         << ") fd_ctx->event="
                         << (EPOLL_EVENTS) m_events;
    }

    IOSchedule::IOSchedule(size_t threads, bool use_caller, const std::string &name) :
            Scheduler(threads, use_caller, name) {
        LOGD(LOG_ROOT()) << "IOSchedule::IOSchedule";
        m_epfd = epoll_create(1);
        SERVER_ASSERT(m_epfd > 0)
        int ret = pipe(m_tickleFds);
        SERVER_ASSERT(ret == 0)
        epoll_event event{};
        memset(&event, 0, sizeof(event));
        ///EPOLLET：缓冲区剩余未读尽的数据不会导致epoll_wait返回，只有新的事件满足才会触发
        ///设置监听的事件类型（EPOLLIN：监听读事件，当有读请求事件过来，epoll_wait解除阻塞）
        ///设置监听事件的触发方式：当前新的事件来临时，epoll_wait才解除阻塞
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];
        ret = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        SERVER_ASSERT(ret >= 0)
        ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        SERVER_ASSERT(ret == 0)
        contextArrayResize(64);
        start();
    }


    IOSchedule::~IOSchedule() {
        LOGD(LOG_ROOT()) << "IOSchedule::~IOSchedule";
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);
        for (auto &m_fdContext: m_fdContexts) {
            delete m_fdContext;
        }
    }

    int IOSchedule::addEvent(int fd, IOSchedule::Event event, std::function<void()> callback) {
        FdContext *fdContext = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if (m_fdContexts.size() > fd) {
            fdContext = m_fdContexts[fd];
            lock.unlock();
        } else {
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextArrayResize(fd * 1.5);
            fdContext = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock2(fdContext->mutex);
        ///fdContext->m_events原来存在，新注册的事件event存在
        if (fdContext->m_events & event) {
            LOGE(LOG_ROOT()) << "addEvent assert fd="
                             << fd << "event=" << event
                             << " fd_ctx.event="
                             << fdContext->m_events;
            SERVER_ASSERT(!(fdContext->m_events && event))
        }

        int op = fdContext->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event ep_event{};
        ep_event.data.fd = fd;
        ep_event.data.ptr = fdContext;
        ///注册要监听的事件
        ep_event.events = EPOLLET | fdContext->m_events | event;
        int ret = epoll_ctl(m_epfd, op, fd, &ep_event);
        if (ret < 0) {
            epoll_error_log("epoll_ctl", fd, op, ep_event.events, ret, fdContext->m_events);
            return -1;
        }
        ++m_pendingEventCount;
        fdContext->m_events = (Event) (fdContext->m_events | event);
        FdContext::EventContext &eventContext = fdContext->getContext(event);
        SERVER_ASSERT(!eventContext.scheduler && !eventContext.fiber && !eventContext.cb)
        eventContext.scheduler = Scheduler::GetThis();
        if (callback) {
            eventContext.cb.swap(callback);
        } else {
            eventContext.fiber = Fiber::GetThis();
            SERVER_ASSERT2(eventContext.fiber->getState() == Fiber::EXEC, "state=" << eventContext.fiber->getState())
        }
        return 0;
    }

    bool IOSchedule::removeEvent(int fd, IOSchedule::Event event) {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_fdContexts.size() <= fd) return false;
        auto fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->m_events & event)) return false;
        auto new_event = (Event) (fd_ctx->m_events & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ep_event{};
        ep_event.events = EPOLLET | new_event;
        ep_event.data.ptr = fd_ctx;
        int ret = epoll_ctl(m_epfd, op, fd_ctx->fd, &ep_event);
        if (ret < 0) {
            epoll_error_log("epoll_ctl", fd, op, ep_event.events, ret, fd_ctx->m_events);
            return false;
        }
        --m_pendingEventCount;
        fd_ctx->m_events = new_event;
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    ///找到event事件删除，并强制执行事件
    bool IOSchedule::cancelEvent(int fd, IOSchedule::Event event) {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_fdContexts.size() < fd) return false;
        auto fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        ///　没有找到对应的ｅｖｅｎｔ事件就退出
        if (!(fd_ctx->m_events & event)) return false;
        auto new_event = (Event) (fd_ctx->m_events & ~event);
        int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ep_event{};
        ep_event.events = EPOLLET | new_event;
        ep_event.data.ptr = fd_ctx;
        int ret = epoll_ctl(m_epfd, op, fd_ctx->fd, &ep_event);
        if (ret < 0) {
            epoll_error_log("epoll_ctl", fd, op, ep_event.events, ret, fd_ctx->m_events);
            return false;
        }
        --m_pendingEventCount;
        fd_ctx->triggerEvent(event);
        return true;
    }

    bool IOSchedule::cancelAllEvent(int fd) {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_fdContexts.size() < fd) return false;
        auto fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!fd_ctx->m_events) return false;
        int op = EPOLL_CTL_DEL;
        epoll_event ep_event{};
        ep_event.events = 0;
        ep_event.data.ptr = fd_ctx;
        int ret = epoll_ctl(m_epfd, op, fd, &ep_event);
        if (ret < 0) {
            epoll_error_log("epoll_ctl", fd, op, ep_event.events, ret, fd_ctx->m_events);
            return false;
        }

        if (fd_ctx->m_events & READ) {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }

        if (fd_ctx->m_events & WRITE) {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }
        SERVER_ASSERT(fd_ctx->m_events == 0)
        return true;
    }

    IOSchedule *IOSchedule::GetThis() {
        return dynamic_cast<IOSchedule *>(Scheduler::GetThis());
    }

    void IOSchedule::tickle() {
        /// 如果没有空闲的线程就不发消息，通知有任务来了
        if (!hasIdleThreads()) {
            return;
        }
        /// tickle的作用就是通知有事件请求到来，这里是直接往管道的写端写入数据＂Ｔ＂
        size_t rt = write(m_tickleFds[1], "T", 1);
        SERVER_ASSERT(rt == 1)
    }

    bool IOSchedule::stopping(uint64_t &timeout) {
        return m_pendingEventCount == 0 && Scheduler::stopping();

    }


    bool IOSchedule::stopping() {
        return Scheduler::stopping() && m_pendingEventCount == 0;
    }

    ///如果没有事件（任务）处理，就陷入epoll_wait
    void IOSchedule::idle() {
        LOGD(LOG_ROOT()) << "IOSchedule::idle";
        const uint64_t MAX_EVENTS = 256;
        auto *events = new epoll_event[MAX_EVENTS]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr) {
            delete[] ptr;
        });

        while (true) {
            uint64_t next_timeout = 0;
            if (stopping(next_timeout )) {
                LOGD(LOG_ROOT()) << "name=" << getName() << " idle stopping exit";
                break;
            }
            int rt;
            do {
                static const int MAX_TIME_OUT = 5000;
                /// 陷入到epoll_wait中５ｓ，如果没有事件回来，这里也会唤醒,epoll_wait return wake events
                rt = epoll_wait(m_epfd, shared_events.get(), 64, MAX_TIME_OUT);
                ///https://blog.csdn.net/hnlyyk/article/details/51444617
                ///　预防在没有事件回来时，操作系统强制中断epoll_wait慢系统调用
                if (rt < 0 && errno == EINTR) {
                } else break;
            } while (true);

            /// epoll_wait 返回的触发的事件数,IOSchedule的构造函数里面监听就是m_tickleFds[0]，
            /// m_tickleFds[0]是一个只读的ｆｄ，任务就是起一个通知的作用，报告有事情请求过来了．
            for (int i = 0; i < rt; i++) {
                auto &event = events[i];
                if (event.data.fd == m_tickleFds[0]) {
                    uint8_t dummy[256];
                    /// ＥＴ触发方式，所以m_tickleFds[0]指向的缓冲区可能还有残余数据，
                    /// 这里读干净
                    while (read(m_tickleFds[0], &dummy, 1) > 0);
                    continue;
                }
                auto *fdContext = (FdContext *) event.data.ptr;
                FdContext::MutexType::Lock lock(fdContext->mutex);
                if (event.events & (EPOLLIN | EPOLLOUT)) {
                    event.events |= (EPOLLIN | EPOLLOUT) & fdContext->m_events;
                }
                /// 记录要触发的事件
                int real_events = NONE;
                if (event.events & EPOLLIN) {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT) {
                    real_events |= WRITE;
                }
                ///　没有触发的事件可以处理,就是没有读事件（ＧＥＴ请求），写事件（ｐｏｓｔ请求）
                if ((fdContext->m_events & real_events) == NONE) {
                    continue;
                }

                /// fdContext->m_events - real_events，从当前事件fdContext->m_events上减掉触发的事件real_events
                int remind_events = (fdContext->m_events & ~real_events);
                /// fdContext->m_events 减去 已经触发的real_events后，如果还有剩余事件，就修改，否则就整个直接删除掉
                int op = remind_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                /// 将剩余的事件重新进行监听
                event.events = EPOLLET | remind_events;
                int ret2 = epoll_ctl(m_epfd, op, fdContext->fd, &event);
                if (ret2 < 0) {
                    epoll_error_log("epoll_ctl", fdContext->fd, op, event.events, ret2, fdContext->m_events);
                    continue;
                }
                if (real_events & READ) {
                    fdContext->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE) {
                    fdContext->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }
            ///　到这里说明已经处理完所有的触发事件,让出处理这些事件的协程的执行权
            ///  返回到 idle_fiber->swapIn();（返回到原来的挂起点，继续向下执行）
            Fiber::ptr curFiber = Fiber::GetThis();
            auto raw_ptr = curFiber.get();
            curFiber.reset();
            raw_ptr->swapOut();
        }
    }

    void IOSchedule::contextArrayResize(size_t size) {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < m_fdContexts.size(); i++) {
            if (m_fdContexts[i] == nullptr) {
                auto fdContext = new FdContext;
                fdContext->fd = i;
                m_fdContexts[i] = fdContext;
            }
        }
    }

    IOSchedule::FdContext::EventContext &IOSchedule::FdContext::getContext(IOSchedule::Event event) {
        switch (event) {
            case READ:
                return read;
            case WRITE:
                return write;
            default:
                SERVER_ASSERT2(false, "getContext")
        }
    }

    void IOSchedule::FdContext::resetContext(IOSchedule::FdContext::EventContext &ctx) {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOSchedule::FdContext::triggerEvent(IOSchedule::Event event) {
        SERVER_ASSERT(event & m_events)
        m_events = (Event) (m_events & ~event);
        EventContext &ctx = getContext(event);
        if (ctx.cb) {
            ctx.scheduler->post(ctx.cb);
        } else {
            ctx.scheduler->post(ctx.fiber);
        }
        ctx.scheduler = nullptr;
    }


} // Server