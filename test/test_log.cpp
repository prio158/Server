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
            0,
            1,
            2,
            time(nullptr),
            "TestLog"));

    event->getSS() << "Hello Crash";

    /** 4、log 打印*/
    logger->log(Server::LogLevel::DEBUG, event);

    return 0;
}
