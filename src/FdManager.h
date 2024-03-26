//
// Created by czr on 24-3-21.
//

#ifndef SERVER_FDMANAGER_H
#define SERVER_FDMANAGER_H

#include <memory>
#include "IOSchedule.h"
#include "Singleton.h"

namespace Server {

    class FdCtx : public std::enable_shared_from_this<FdCtx> {

    public:
        typedef std::shared_ptr<FdCtx> ptr;

        explicit FdCtx(int fd);

        ~FdCtx();

        bool init();

        bool isInit() const { return m_isInit; }

        bool isSocket() const { return m_isSocket; }

        bool isClose() const { return m_isClosed; }

        void setUserNonblock(bool v) { m_userNonBlock = v; }

        bool getUserNonblock() const { return m_userNonBlock; }

        void setSysNonblock(bool v) { m_sysNonBlock = v; }

        bool getSysNonblock() const { return m_sysNonBlock; }

        void setTimeout(int type, uint64_t v);

        uint64_t getTimeout(int type) const;

    private:
        bool m_isInit: 1;
        bool m_isSocket: 1;
        bool m_sysNonBlock: 1;
        bool m_isClosed: 1;
        bool m_userNonBlock: 1;
        int m_fd;

        uint64_t m_recvTimeout;
        uint64_t m_sendTimeout;
        Server::IOSchedule *ioSchedule{};

    };

    class FdManager {
    public:
        typedef RWMutex RWMutexType;

        FdManager();

        FdCtx::ptr get(int fd, bool auto_create = false);

        void del(int fd);

    private:
        RWMutexType m_mutex;
        std::vector<FdCtx::ptr> m_datas;



    };

    typedef Singleton<FdManager> FdMgr;

}


#endif //SERVER_FDMANAGER_H
