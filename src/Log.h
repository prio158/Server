//
// Created by 陈子锐 on 2024/2/1.
//

#ifndef Server_LOG_H
#define Server_LOG_H

#include <string>
#include <cstdint>
#include <list>
#include <fstream>
#include <utility>
#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <functional>
#include <memory>
#include <set>
#include <thread>
#include "Util.h"
#include "Singleton.h"
#include "Thread.h"

/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 */
#define LOG_LEVEL(logger, level) \
    if(logger->getLeveL() <= level) \
        Server::LogEventWrap(Server::LogEvent::ptr(new Server::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, Server::GetThreadId(),\
                        Server::GetFiberId(), time(0), "threadName"))).getSS()

#define LOGD(logger)  LOG_LEVEL(logger, Server::LogLevel::DEBUG)

#define LOGI(logger)  LOG_LEVEL(logger, Server::LogLevel::INFO)

#define LOGW(logger)  LOG_LEVEL(logger, Server::LogLevel::WARN)

#define LOGE(logger)  LOG_LEVEL(logger, Server::LogLevel::ERROR)

#define LOGF(logger) LOG_LEVEL(logger,Server::LogLevel::FATAL)

#define LOG_ROOT() Server::LoggerMgr::GetInstance()->getRoot()

/// 指定name找到日志，LOG_ROOT是name=root的日志
#define LOG_NAME(name) Server::LoggerMgr::GetInstance()->getLogger(name)

/**
 * Logger (定义日志类别)
 *   ｜
 *  Formatter(日志输出格式)
 *   ｜
 *  Appender(日志输出的地方)
 * */
class ptr;
namespace Server {
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

        static LogLevel::Level FromString(const std::string &str);

        static std::string getLevelColor(LogLevel::Level level);
    };

    /** 日志事件*/
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        /**
         * @brief 构造函数
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] file 文件名
         * @param[in] line 文件行号
         * @param[in] elapse 程序启动依赖的耗时(毫秒)
         * @param[in] thread_id 线程id
         * @param[in] fiber_id 协程id
         * @param[in] time 日志事件(秒)
         * @param[in] thread_name 线程名称
         */
        LogEvent(std::shared_ptr<Logger> logger,
                 LogLevel::Level level,
                 const char *file,
                 uint32_t line,
                 uint32_t elapse,
                 pid_t thread_id,
                 uint32_t fiber_id,
                 uint64_t time,
                 std::string thread_name);

        ~LogEvent() = default;

        /**
         * @brief 返回文件名
         */
        const char *getFile() const { return m_file; }

        /**
         * @brief 返回行号
         */
        int32_t getLine() const { return m_line; }

        /**
         * @brief 返回耗时
         */
        uint32_t gerElapse() const { return m_elapse; }

        /**
         * @brief 返回线程ID
         */
        pid_t getThreadId() const { return m_threadId; }

        /**
         * @brief 返回协程ID
         */
        uint32_t getFiberId() const { return m_fiberId; }

        /**
         * @brief 返回时间
         */
        uint64_t getTime() const { return m_time; }

        /**
         * @brief 返回线程名称
         */
        std::string getThreadName() const { return m_threadName; }

        /**
         * @brief 返回日志内容
         */
        std::string getContent() const { return m_ss.str(); }

        /**
         * @brief 返回日志器
         */
        std::shared_ptr<Logger> getLogger() const { return m_logger; }

        /**
         * @brief 返回日志级别
         */
        LogLevel::Level getLevel() const { return m_level; }

        /**
         * @brief 返回日志内容字符串流
         */
        std::stringstream &getSS() { return m_ss; }

        /**
         * @brief 格式化写入日志内容
         */
        void format(const char *fmt, ...);

        /**
         * @brief 格式化写入日志内容
         */
        void format(const char *fmt, va_list al);


    private:
        /// 文件名
        const char *m_file = nullptr;
        /// 行号
        uint32_t m_line = 0;
        /// 程序启动开始到现在的毫秒数
        uint32_t m_elapse = 0;
        /// 线程 ID
        pid_t m_threadId;
        /// 协程 ID
        uint16_t m_fiberId = 0;
        /// 时间戳
        uint64_t m_time = 0;
        /// 线程名称
        std::string m_threadName;
        /// 日志内容流
        std::stringstream m_ss;
        /// 日志器
        std::shared_ptr<Logger> m_logger;
        /// 日志等级
        LogLevel::Level m_level;
    };

    class LogEventWrap {
    public:
        explicit LogEventWrap(LogEvent::ptr event);

        ~LogEventWrap();

        std::stringstream &getSS();

    private:
        LogEvent::ptr m_event;
    };

    /** 日志格式基类*/
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;

        explicit FormatItem(const std::string &fmt = "") {}

        virtual ~FormatItem() = default;

        virtual void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) = 0;
    };

    /** 日志格式器*/
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        /**
         * @brief 构造函数
         * @param[in] pattern 格式模板
         * @details
         *  %m 消息
         *  %p 日志级别
         *  %r 累计毫秒数
         *  %c 日志名称
         *  %t 线程id
         *  %n 换行
         *  %d 时间
         *  %f 文件名
         *  %l 行号
         *  %T 制表符
         *  %F 协程id
         *  %N 线程名称
         *
         *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
         */
        explicit LogFormatter(std::string pattern);

        /**
         * @brief 返回格式化日志文本
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] event 日志事件
         */
        std::string format(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event);

        std::ostream &
        format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

        /**
         * @brief 初始化,解析日志模板
         */
        void init();

        /**
         * @brief 是否有错误
         */
        bool isError() const { return m_error; }

        /**
         * @brief 返回日志模板
         */
        std::string getPattern() const { return m_pattern; }

    private:
        /// 日志格式模板
        std::string m_pattern;
        /// 日志格式解析后格式
        std::vector<FormatItem::ptr> m_items;
        /// 是否有错误
        bool m_error = false;
    };

    /**
     * @brief 日志输出目标
     */
    class LogAppender {
        friend class Logger;

    public:
        typedef std::shared_ptr<LogAppender> ptr;
        typedef Mutex MutexType;

        virtual ~LogAppender() = default;

        /**
         * @brief 写入日志
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] event 日志事件
         */
        virtual void log(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event) = 0;

        /**
         * @brief 更改日志格式器
         */
        void setLogFormatter(LogFormatter::ptr formatter) {
            MutexType::Lock lock(m_mutex);
            m_formatter = std::move(formatter);
            if (m_formatter) {
                m_hasFormatter = true;
            } else {
                m_hasFormatter = false;
            }
        }

        /**
         * @brief 获取日志格式器
         */
        LogFormatter::ptr getLogFormatter() {
            MutexType::Lock lock(m_mutex);
            return m_formatter;
        }

        /**
         * @brief 获取日志级别
         */
        LogLevel::Level getLevel() const { return m_level; }

        /**
         * @brief 设置日志级别
         */
        void setLevel(LogLevel::Level level) { m_level = level; }

        /**
         * @brief 将日志输出目标的配置转成YAML String
         */
        virtual std::string toYamlString() = 0;

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;
        LogFormatter::ptr m_formatter;
        /// 是否有自己的日志格式器
        bool m_hasFormatter = false;
        MutexType m_mutex;
    };

    /** 日志*/
    class Logger {
        friend class LoggerManager;

    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Mutex MutexType;

        explicit Logger(std::string name = "stdout");

        /**
         * @brief 写日志
         * @param[in] level 日志级别
         * @param[in] event 日志事件
         */
        void log(LogLevel::Level level, const LogEvent::ptr &event);

        /**
         * @brief 写info级别日志
         * @param[in] event 日志事件
         */
        void info(const LogEvent::ptr &event);

        /**
         * @brief 写debug级别日志
         * @param[in] event 日志事件
         */
        void debug(const LogEvent::ptr &event);

        /**
         * @brief 写warn级别日志
         * @param[in] event 日志事件
         */
        void warn(const LogEvent::ptr &event);

        /**
         * @brief 写error级别日志
         * @param[in] event 日志事件
         */
        void error(const LogEvent::ptr &event);

        /**
         * @brief 写fatal级别日志
         * @param[in] event 日志事件
         */
        void fatal(const LogEvent::ptr &event);

        /**
         * @brief 添加日志目标
         * @param[in] appender 日志目标
         */
        void addAppender(const LogAppender::ptr &appender);

        /**
         * @brief 删除日志目标
         * @param[in] appender 日志目标
         */
        void deleteAppender(const LogAppender::ptr &appender);

        /**
         * @brief 清空日志目标
         */
        void clearAppenders();

        /**
         * @brief 设置日志级别
         */
        void setLevel(LogLevel::Level level) { m_level = level; }

        /**
         * @brief 返回日志级别
         */
        LogLevel::Level getLeveL() const { return m_level; }

        /**
         * @brief 返回日志名称
         */
        std::string getName() const { return m_name; }


        /**
         * @brief 设置日志格式器
         */
        void setFormatter(LogFormatter::ptr val);

        /**
         * @brief 设置日志格式模板
         */
        void setFormatter(const std::string &val);

        /**
         * @brief 获取日志格式器
         */
        LogFormatter::ptr getFormatter();

        /**
         * @brief 将日志器的配置转成YAML String
         */
        std::string toYamlString();

    private:
        /// 日志名称
        std::string m_name;
        /// 日志级别
        LogLevel::Level m_level;
        /// 日志目标集合
        std::list<LogAppender::ptr> m_appenderList;
        /// 日志格式器
        LogFormatter::ptr m_formatter;
        /// 主日志器
        Logger::ptr m_root;
        /// 同步锁
        MutexType m_mutex;

    };


    /**
     * @brief 输出到控制台的Appender
     */
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        void log(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event) override;

        std::string toYamlString() override;
    };


    /**日志输出到文件*/
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        explicit FileLogAppender(std::string file_name);

        void log(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event) override;

        /** 重新打开文件，文件打开成功返回 true*/
        bool reopen();

        std::string toYamlString() override;

    private:
        std::string m_filename;
        std::ofstream m_filestream;
        uint64_t m_lastTime = 0;
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
            os << event->getLogger()->getName();
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
        explicit DateTimeFormatItem(std::string format = "%Y-%m-%d %H:%M:%S")
                : m_format(std::move(format)) {
            if (m_format.empty()) {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
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

    class TabFormatItem : public FormatItem {
    public:
        explicit TabFormatItem(const std::string &str = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << "\t";
        }

    private:
        std::string m_string;
    };

    class ThreadNameFormatItem : public FormatItem {
    public:
        explicit ThreadNameFormatItem(const std::string &str = "") {}

        void
        format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::ostream &os, LogEvent::ptr event) override {
            os << event->getThreadName();
        }
    };

    class LoggerManager {
    public:
        typedef Mutex MutexType;

        explicit LoggerManager();

        Logger::ptr getLogger(const std::string &name);

        Logger::ptr getRoot() const { return m_root; };

        /**
         * @brief 将所有的日志器配置转成YAML String
         */
        std::string toYamlString();


    private:
        MutexType m_mutex;
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    /// 日志器管理类单例模式
    typedef Singleton<LoggerManager> LoggerMgr;

    struct LogAppenderDefine {
        int type = 0; //1 File, 2 Stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &oth) const {
            return type == oth.type
                   && level == oth.level
                   && formatter == oth.formatter
                   && file == oth.file;
        }
    };
}

#endif //Server_LOG_H
