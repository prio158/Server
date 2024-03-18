//
// Created by czr on 24-3-21.
//

#include <sys/stat.h>
#include <fcntl.h>
#include "FdManager.h"
#include "Hook.h"

namespace Server {


    FdManager::FdManager() {
        m_datas.resize(64);
    }

    FdCtx::ptr FdManager::get(int fd, bool auto_create) {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_datas.size() <= fd) {
            if (!auto_create)
                return nullptr;
        } else {
            if (m_datas[fd] || !auto_create) {
                return m_datas[fd];
            }
        }
        lock.unlock();
        RWMutexType::WriteLock writeLock(m_mutex);
        FdCtx::ptr ctx(new FdCtx(fd));
        m_datas[fd] = ctx;
        return ctx;
    }

    void FdManager::del(int fd) {
        RWMutexType::WriteLock writeLock(m_mutex);
        if (m_datas.size() <= fd) {
            return;
        }
        m_datas[fd].reset();
    }


    FdCtx::FdCtx(int fd) : m_isInit(false),
                           m_isSocket(false),
                           m_sysNonBlock(false),
                           m_isClosed(false),
                           m_userNonBlock(false),
                           m_fd(fd),
                           m_recvTimeout(-1),
                           m_sendTimeout(-1) {


    }

    FdCtx::~FdCtx() {

    }

    bool FdCtx::init() {
        if (m_isInit) {
            return true;
        }
        m_recvTimeout = -1;
        m_sendTimeout = -1;

        struct stat fd_stat;
        // 取出fd的状态，查看它是否已经关闭，==-1代表已经关闭了
        if (-1 == fstat(m_fd, &fd_stat)) {
            m_isInit = false;
            m_isSocket = false;
        } else {
            m_isInit = true;
            // 如果没有关闭，再查看这个fd是否是socket
            m_isSocket = S_ISSOCK(fd_stat.st_mode);
        }

        if (m_isSocket) {
            int flags = fcntl_f(m_fd, F_GETFL, 0);
            // 且这个ｆｄ是阻塞类型
            if (!(flags & O_NONBLOCK)) {
                // 设置ｆｄ为非阻塞类型
                fcntl_f(m_fd, F_SETFL, false | O_NONBLOCK);
            }
            m_sysNonBlock = true;
        } else {
            m_sysNonBlock = false;
        }
        m_userNonBlock = false;
        m_isClosed = false;
        return m_isInit;
    }

    void FdCtx::setTimeout(int type, uint64_t v) {
        // socket recv 接收数据超时时间
        if (type == SO_RCVTIMEO) {
            m_recvTimeout = v;
        } else {
            // SO_SNDTIMEO　socket 发送超时
            m_sendTimeout = v;
        }
    }

    uint64_t FdCtx::getTimeout(int type) const {
        if (type == SO_RCVTIMEO) {
            return m_recvTimeout;
        } else {
            return m_sendTimeout;
        }
    }
}