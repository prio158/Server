//
// Created by 陈子锐 on 2024/2/3.
//
#include <iostream>
#include "Log.h"

int main() {
    std::cout << "Test Hello, World!" << std::endl;

    /** 1、创建 Logger */
    Server::Logger::ptr logger(new Server::Logger);
    /** 2、创建 LoggerAppender，添加到 logger 中*/
    logger->addAppender(Server::LogAppender::ptr(new Server::StdoutLogAppender));
    /** 3、创建 LoggerEvent */
    Server::LogEvent::ptr event(new Server::LogEvent(
            logger,
            Server::LogLevel::DEBUG,
            __FILE__,
            __LINE__,
            Server::GetFiberId(),
            Server::GetThreadId(),
            2,
            time(nullptr),
            "TestLog"));

    /** 4、Log 输出的内容*/
    event->getSS() << "XXXXXXX";

    /** 5、log 打印*/
    logger->log(Server::LogLevel::DEBUG, event);

    /** 6、宏打印 */
    LOGD(logger) << "DDDDDD";
    LOGE(logger) << "SSSSSS";

    return 0;
}
