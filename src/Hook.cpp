//
// Created by czr on 24-3-17.
//

#include "Hook.h"
#include "Fiber.h"
#include "IOSchedule.h"
#include "dlfcn.h"
#include "Log.h"
#include "FdManager.h"
#include <fcntl.h>
#include <cstdarg>
#include <sys/ioctl.h>

namespace Server {

    static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
        XX(sleep)       \
        XX(usleep)       \
        XX(nanosleep)    \
        XX(socket) \
        XX(connect) \
        XX(accept) \
        XX(read) \
        XX(readv) \
        XX(recv) \
        XX(recvfrom) \
        XX(recvmsg) \
        XX(write) \
        XX(writev) \
        XX(send) \
        XX(sendto) \
        XX(sendmsg) \
        XX(close) \
        XX(fcntl) \
        XX(ioctl) \
        XX(getsockopt) \
        XX(setsockopt)

    void hook_init() {
        static bool is_inited = false;
        if (is_inited) return;

///dlsym 动态加载库文件
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT,#name);
        HOOK_FUN(XX)
#undef XX
    }

    struct _HookIniter {
        _HookIniter() {
            hook_init();
        }
    };

    static _HookIniter s_hook_initer;

    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }

    struct timer_info {
        int cancelled = 0;

    };

    // hook　io相关的操作
    template<typename OriginFun, typename ... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                         uint32_t event, int timeout_so, Args &&... args) {
        //如果没有hook，那么就执行originFun
        if (!t_hook_enable) {
            return fun(fd, std::forward<Args>(args)...);
        }

        FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);
        //如果ｆｄ不存在，执行originFun
        if (!ctx) {
            return fun(fd, std::forward<Args>(args)...);
        }
        //如果ｆｄ已经关闭
        if (ctx->isClose()) {
            errno = EBADF;
            return -1;
        }
        // 如果不是ｓｃｏｋｅｔ或为非阻塞
        if (!ctx->isSocket() || ctx->getUserNonblock()) {
            return fun(fd, std::forward<Args>(args)...);
        }

        uint64_t timeout_time = ctx->getTimeout(timeout_so);
        std::shared_ptr<timer_info> tinfo(new timer_info);

        retry:
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        //当在父进程阻塞于慢系统调用时由父进程捕获到了一个有效信号时，内核会致使accept返回一个EINTR错误(被中断的系统调用)
        //这个时候重新执行ｆｕｎ
        while (n == -1 && errno == EINTR) {
            n = fun(fd, std::forward<Args>(args)...);
        }
        // 从字面上来看，是提示再试一次。这个错误经常出现在当应用程序进行一些非阻塞(non-blocking)操作
        // (对文件或socket)的时候。例如，以 O_NONBLOCK的标志打开文件/socket/FIFO，如果你连续做read
        // 操作而没有数据可读，此时程序不会阻塞起来等待数据准备就绪返回，read函数会返回一个错误EAGAIN，提
        // 示你的应用程序现在没有数据可读请稍后再试，也就是ＩＯ未就绪．
        // 问题来了：我什么时候知道ＩＯ就绪了，难道我检查一次 IO 是否就绪，如果没有就绪，那我就直接走了，退出程序吗？
        // 于是应用程程序继续轮询请求，直到对方返回一个正确的响应，即数据拿到了,这种行为实际上还是阻塞在这个 socket 上，而且占用大量 CPU，因为要轮询访问。
        // Reactor：把 IO 的处理转换为对事件的处理。(select/epoll:  只检测 IO 是否就绪的问题，不解决具体IO 的操作)
        if (n == -1 && errno == EAGAIN) {
            //当返回EAGAIN，就代表ｉｏ事件是一个异步非阻塞的操作，这个时候就需要用定时器去处理
            auto ioSchedule = IOSchedule::GetThis();
            Timer::ptr timer;
            //https://c.biancheng.net/view/7918.html，下面是设置一个条件变量
            std::weak_ptr<timer_info> winfo(tinfo);
            //如果设置了超时时间，就放到条件定时器中，等待timeout_time时间后就取消ｆｄ的ｅｖｅｎｔ事件监听
            if (timeout_time != (uint64_t) -1) {
                timer = ioSchedule->addConditionTimer(timeout_time, [winfo, fd, ioSchedule, event]() {
                    //lock:如果当前 weak_ptr 已经过期，则该函数会返回一个空的 shared_ptr 指针；
                    //反之，该函数返回一个和当前 weak_ptr 指向相同的 shared_ptr 指针。
                    auto t = winfo.lock();
                    if (!t || t->cancelled) {
                        return;
                    }
                    //执行到这里就说明timeout_time时间到了，且ＩＯ任务没有被取消，
                    //那么状态置为超时，执行cancelEvent：取消掉事件的监听
                    t->cancelled = ETIMEDOUT;
                    ioSchedule->cancelEvent(fd, static_cast<IOSchedule::Event>(event));
                }, winfo);
            }

            uint64_t now = 0;
            //添加ｅｖｅｎｔ事件到ｆｄ上进行监听
            int rt = ioSchedule->addEvent(fd, static_cast<IOSchedule::Event>(event));
            if (rt != 0) {
                //添加失败
                LOGE(LOG_ROOT()) << hook_fun_name
                                 << " addEvent("
                                 << fd
                                 << ", "
                                 << event
                                 << " used=" << (GetCurrentUS() - now);
                //ｅｖｅｎｔ添加失败，就取消上面的条件定时器任务，然后返回－１
                if (timer) {
                    timer->cancel();
                }
                return -1;
            } else {
                //添加ｅｖｅｎｔ成功后，执行到这里会挂起，然后条件定时器执行回调中的cancelEvent的时候，会唤醒此处的挂起状态
                //以及ioSchedule->addEvent执行后，也会唤醒此处．
                Fiber::YieldToHold();
                if (timer) {
                    timer->cancel();
                }
                //如果是定时任务取消的,返回，不进行ｒｅｔｒｙ　
                if (tinfo->cancelled) {
                    errno = tinfo->cancelled;
                    return -1;
                }
                goto retry;
            }
        }
        return n;
    }

    extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX)
#undef XX


    ///其实这里Ｈｏｏｋ的睡眠函数，不是调用的操作系统的睡眠函数，而
    ///是让fiber来执行指定的时间，达到睡眠的效果．
    unsigned int sleep(unsigned int seconds) {
        if (!t_hook_enable) {
            return sleep_f(seconds);
        }
        Fiber::ptr fiber = Server::Fiber::GetThis();
        auto ioSchedule = IOSchedule::GetThis();
        ioSchedule->addTimer(seconds * 1000, std::bind((void (Scheduler::*)
                (Fiber::ptr, int thread)) &IOSchedule::post, ioSchedule, fiber, -1));
        Fiber::YieldToHold();
        return 0;
    }


    int usleep(useconds_t usec) {
        if (!t_hook_enable) {
            return usleep_f(usec);
        }
        Fiber::ptr fiber = Server::Fiber::GetThis();
        auto ioSchedule = IOSchedule::GetThis();
        ioSchedule->addTimer(usec / 1000, std::bind((void (Scheduler::*)
                (Fiber::ptr, int thread)) &IOSchedule::post, ioSchedule, fiber, -1));
        Fiber::YieldToHold();
        return 0;
    }

    int nanosleep(const struct timespec *req, struct timespec *rem) {
        if (!t_hook_enable) {
            return nanosleep_f(req, rem);
        }

        int timeout_ms = req->tv_sec * 1000 + rem->tv_nsec / 1000 / 1000;
        Fiber::ptr fiber = Fiber::GetThis();
        auto ioSchedule = IOSchedule::GetThis();
        ioSchedule->addTimer(timeout_ms, [ioSchedule, fiber]() {
            ioSchedule->post(fiber);
        });
        Fiber::YieldToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol) {
        if (!t_hook_enable) {
            return socket_f(domain, type, protocol);
        }
        int fd = socket_f(domain, type, protocol);
        FdMgr::GetInstance()->get(fd, true);
        return fd;
    }

    int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms) {
        if (!t_hook_enable)
            return connect_f(fd, addr, addrlen);
        auto ctx = FdMgr::GetInstance()->get(fd);

        if (!ctx || ctx->isSocket()) {
            errno = EBADF;
            return -1;
        }

        if (!ctx->isSocket())
            return connect_f(fd, addr, addrlen);

        if (!ctx->getUserNonblock()) {
            return connect_f(fd, addr, addrlen);
        }
        int retryNum = 1;
        int now = GetCurrentUS();
        int n = connect_f(fd, addr, addrlen);
        if (n == 0) return 0;
        else if (n != -1 || errno != EINPROGRESS) return n;
        auto ioSchedule = IOSchedule::GetThis();
        Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);
        if (timeout_ms != (uint64_t) -1) {
            timer = ioSchedule->addConditionTimer(timeout_ms, [winfo, fd, ioSchedule]() {
                //lock:如果当前 weak_ptr 已经过期，则该函数会返回一个空的 shared_ptr 指针；
                //反之，该函数返回一个和当前 weak_ptr 指向相同的 shared_ptr 指针。
                auto t = winfo.lock();
                if (!t || t->cancelled) {
                    return;
                }
                //执行到这里就说明timeout_time时间到了，且WAIT任务没有被取消，
                //那么状态置为超时，执行cancelEvent：取消掉事件的监听
                t->cancelled = ETIMEDOUT;
                ioSchedule->cancelEvent(fd, IOSchedule::WRITE);
            }, winfo);
        }
        int rt = ioSchedule->addEvent(fd, IOSchedule::WRITE);
        if (rt != 0) {
            //添加失败
            LOGE(LOG_ROOT()) << connect_with_timeout
                             << " addEvent("
                             << fd
                             << ", "
                             << IOSchedule::WRITE
                             << " used time=" << (GetCurrentUS() - now)
                             << " retry num=" << retryNum;
            //ｅｖｅｎｔ添加失败，就取消上面的条件定时器任务，然后返回－１
            if (timer) {
                timer->cancel();
            }
            return -1;
        } else {
            //添加ｅｖｅｎｔ成功后，执行到这里会挂起，然后条件定时器执行回调中的cancelEvent的时候，会唤醒此处的挂起状态
            //以及ioSchedule->addEvent执行后，也会唤醒此处．
            Fiber::YieldToHold();
            if (timer) {
                timer->cancel();
            }
            //如果是定时任务取消的,返回，不进行ｒｅｔｒｙ　
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
        }
        int error = 0;
        socklen_t len = sizeof(socklen_t);
        if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
            return -1;
        if (!error) return 0;
        else {
            errno = error;
            return -1;
        }
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
        return connect_f(sockfd, addr, addrlen);
    }

    int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
        //先把ｓｏｅｃｋｅｔ挂到ｅｐｏｌｌ的树进行监听
        int fd = do_io(s, accept_f, "accept", IOSchedule::READ, SO_RCVTIMEO, addr, addrlen);
        if (fd >= 0) {
            FdMgr::GetInstance()->get(fd, true);
        }
        return fd;
    }

    ssize_t read(int fd, void *buf, size_t count) {
        return do_io(fd, read_f, "read", IOSchedule::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
        return do_io(fd, readv_f, "readv", IOSchedule::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
        return do_io(sockfd, recv_f, "recv", IOSchedule::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr,
                     socklen_t *addrlen) {
        return do_io(sockfd, recvfrom_f, "recvfrom", IOSchedule::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
        return do_io(sockfd, recvmsg_f, "recvmsg", IOSchedule::READ, SO_RCVTIMEO, msg, flags);
    }

    ssize_t write(int fd, const void *buf, size_t count) {
        return do_io(fd, write_f, "write", IOSchedule::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
        return do_io(fd, writev_f, "writev", IOSchedule::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }


    ssize_t send(int s, const void *msg, size_t len, int flags) {
        return do_io(s, send, "send", IOSchedule::WRITE, SO_SNDTIMEO, msg, len, flags);
    }


    ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to,
                   socklen_t tolen) {
        return do_io(s, sendto_f, "sendto", IOSchedule::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
    }

    ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
        return do_io(s, sendmsg_f, "sendmsg", IOSchedule::WRITE,
                     SO_SNDTIMEO, msg, flags);
    }

    int close_fun(int fd) {
        if (!t_hook_enable) {
            return close_f(fd);
        }
        auto ctx = FdMgr::GetInstance()->get(fd, true);
        if (ctx) {
            auto iom = IOSchedule::GetThis();
            if (iom) {
                iom->cancelAllEvent(fd);
            }
            FdMgr::GetInstance()->del(fd);
        }
        return close_f(fd);
    }


    int fcntl(int fd, int cmd, ... /* arg */ ) {
        // <Step 1> 定义一个 va_list 类型的变量，(假设va_list 类型变量被定义为ap)；
        // <Step 2> 调用va_start ，对ap 进行初始化，让它指向可变参数表里面的第一个参数。
        // <Step 3> 获取参数，并使用参数。
        // <Step 4> 获取所有的参数之后，将 ap 指针关掉。
        va_list va;
        va_start(va, cmd);
        switch (cmd) {
            case F_SETFL: {
                //它的第一个参数是ap，第二个参数是要获取的参数的指定类型。按照指定类型获取当前参数，
                //返回这个指定类型的值，然后把 ap 的位置指向变参表中下一个变量的位置；
                int arg = va_arg(va, int);
                va_end(va); //释放指针ｖａ，并将ｖａ置为ｎｕｌｌ
                FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                // socket fd
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if (ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
                break;
            case F_GETFL: {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if (ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
                break;
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
#ifdef F_SETPIPE_SZ
            case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
                break;
            case F_GETFD:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
#ifdef F_GETPIPE_SZ
            case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
                break;
            case F_SETLK:
            case F_SETLKW:
            case F_GETLK: {
                struct flock *arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
                break;
            case F_GETOWN_EX:
            case F_SETOWN_EX: {
                struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
                break;
            default:
                va_end(va);
                return fcntl_f(fd, cmd);
        }
    }

    //ioctl(input/output control)是设备驱动程序中对设备的I/O通道进行管理的函数。
    //所谓对I/O通道进行管理，就是对设备的一些特性进行控制，例如串口的传输波特率、马达的转速等等。
    //通常，成功返回0。 失败返回 -1。 errno会被设置
    int ioctl(int d, unsigned long int request, ...) {
        va_list va;
        va_start(va, request);
        void *arg = va_arg(va, void*);
        va_end(va);
        //允许或禁止套接口socket的非阻塞模式:如允许非阻塞模式则非零，如禁止非阻塞模式则为零
        if (FIONBIO == request) {
            bool user_nonblock = !!*(int *) arg;
            FdCtx::ptr ctx = FdMgr::GetInstance()->get(d);
            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return ioctl_f(d, request, arg);
            }
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(d, request, arg);
    }


    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }


    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
        if (!t_hook_enable) {
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        }
        //level是被设置的选项的级别，如果想要在套接字级别上设置选项，就必须把level设置为 SOL_SOCKET
        //也就是说只有将ｌｅｖｅｌ设置为了SOL_SOCKET，optname才能设置：
        //SO_RCVTIMEO，设置接收超时时间
        //SO_SNDTIMEO，设置发送超时时间
        if (level == SOL_SOCKET) {
            if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
                auto ctx = FdMgr::GetInstance()->get(sockfd);
                if (ctx) {
                    const auto *v = (const timeval *) optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }


    }

}