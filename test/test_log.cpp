//
// Created by 陈子锐 on 2024/2/3.
//
#include <iostream>
#include "Log.h"

int main() {
    /** 1、创建 Logger */
    Server::Logger::ptr logger(new Server::Logger);
    /** 2、创建 LoggerAppender，添加到 logger 中*/
    logger->addAppender(Server::LogAppender::ptr(new Server::StdoutLogAppender));
    logger->addAppender(Server::LogAppender::ptr(new Server::FileLogAppender("./log.txt")));

    /** 3、创建 LoggerEvent */
    Server::LogEvent::ptr event(new Server::LogEvent(
            logger,
            Server::LogLevel::INFO,
            __FILE__,
            __LINE__,
            0,
            Server::GetThreadId(),
            Server::GetFiberId(),
            time(nullptr),
            "TestLog"));

    /** 4、Log 输出的内容*/
    event->getSS() << "XXXXXXXSSSS";

    /** 5、log 打印*/
    logger->log(Server::LogLevel::DEBUG, event);

    /** 6、宏打印 */
    LOGD(logger) << "DDDDDD";
    LOGE(logger) << "SSSSSS";
    LOGI(logger) << "FFFFFF";

    /** 7、单例模式测试*/
    auto& logger2 = Server::LoggerMgr::GetInstance()->getLogger("stdout");
    logger2->log(Server::LogLevel::ERROR,event);

    return 0;
}
