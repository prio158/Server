//
// Created by 陈子锐 on 2024/2/1.
//

#ifndef SERVER_DEMO_LOG_H
#define SERVER_DEMO_LOG_H

#include <string>
#include <cstdint>
#include <memory>
#include <list>
#include <fstream>
#include <utility>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <functional>

namespace ServerDemo {

    class Logger;

    /** 日志级别*/
    class LogLevel {
    public:
        enum Level {
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };

        static const char *ToString(LogLevel::Level level);
    };

    /** 日志事件*/
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        LogEvent() = default;

        const char *getFile() const { return m_file; }

        int32_t getLine() const { return m_line; }

        uint32_t gerElapse() const { return m_elapse; }

        uint32_t getThreadId() const { return m_threadId; }

        uint32_t getFiberId() const { return m_fiberId; }

        uint64_t getTime() const { return time; }

        std::string getContent() const { return m_content; }

    private:
        const char *m_file = nullptr; //文件名
        int32_t m_line = 0; //行号
        uint32_t m_elapse = 0; //程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;  //线程 Id
        uint16_t m_fiberId = 0;   //协程 Id
        uint64_t time; //时间戳
        std::string m_content;
    };

    /** 日志格式基类*/
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;

        explicit FormatItem(const std::string &fmt = "") {};

        virtual ~FormatItem() = default;

        virtual void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) = 0;
    };

    /** 日志格式器*/
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        explicit LogFormatter(std::string pattern);

        std::string format(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event);

        void init();

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
    };

    /** 日志输出器*/
    class LogAppender {
    public:
        typedef std::shared_ptr<LogAppender> ptr;

        virtual ~LogAppender() = default;

        virtual void log(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event) = 0;

        void setLogFormatter(LogFormatter::ptr formatter) { m_formatter = std::move(formatter); }

        LogFormatter::ptr getLogFormatter() const { return m_formatter; }

    protected:
        LogLevel::Level m_level;
        LogFormatter::ptr m_formatter;
    };

    /** 日志*/
    class Logger {
    public:
        typedef std::shared_ptr<Logger> ptr;

        explicit Logger(std::string name = "root");

        void log(LogLevel::Level level, const LogEvent::ptr &event);

        void info(const LogEvent::ptr &event);

        void debug(const LogEvent::ptr &event);

        void warn(const LogEvent::ptr &event);

        void error(const LogEvent::ptr &event);

        void fatal(const LogEvent::ptr &event);

        void addAppender(const LogAppender::ptr &appender);

        void deleteAppender(const LogAppender::ptr &appender);

        void setLevel(LogLevel::Level level) { m_level = level; }

        LogLevel::Level getLeveL() const { return m_level; }

        std::string getName() const { return m_name; }


    private:
        std::string m_name;      //日志名称
        LogLevel::Level m_level;  //日志级别
        std::list<LogAppender::ptr> m_appenderList;   //日志输出地集合
    };

    /**日志输出到控制台*/
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        void log(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event) override;
    };

    /**日志输出到文件*/
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        explicit FileLogAppender(std::string file_name);

        void log(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event) override;

        /** 重新打开文件，文件打开成功返回 true*/
        bool ropen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

    /**
     * %m -- 消息体
     * %p -- level
     * %r -- 启动后时间
     * %c -- 日志名称
     * %t -- 线程 Id
     * %n -- 回车换行
     * %d -- 时间
     * %f -- 文件名
     * %l -- 行号
     * */
    class MessageFormatItem : public FormatItem {
    public:
        explicit MessageFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public FormatItem {
    public:
        explicit LevelFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public FormatItem {
    public:
        explicit ElapseFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << event->gerElapse();
        }
    };

    class NameFormatItem : public FormatItem {
    public:
        explicit NameFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << logger->getName();
        }
    };


    class ThreadIdFormatItem : public FormatItem {
    public:
        explicit ThreadIdFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public FormatItem {
    public:
        explicit FiberIdFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
    };

    class DateTimeFormatItem : public FormatItem {
    public:
        explicit DateTimeFormatItem(std::string format = "%Y:%m:%d %H:%M:%S") : m_format(std::move(format)) {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << event->getTime();
        }

    private:
        std::string m_format;

    };

    class FileNameFormatItem : public FormatItem {
    public:
        explicit FileNameFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << event->getFile();
        }
    };

    class LineFormatItem : public FormatItem {
    public:
        explicit LineFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public FormatItem {
    public:
        explicit NewLineFormatItem(const std::string &fmt = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << std::endl;
        }
    };

    class StringFormatItem : public FormatItem {
    public:
        explicit StringFormatItem(std::string fmt = "") : m_string(std::move(fmt)) {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << m_string;
        }

    private:
        std::string m_string;
    };
}

#endif //SERVER_DEMO_LOG_H
