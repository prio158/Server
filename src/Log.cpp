//
// Created by 陈子锐 on 2024/2/1.
//

#include "Log.h"

#include <memory>
#include <utility>


namespace Server {

    const char *LogLevel::ToString(LogLevel::Level level) {
        switch (level) {
#define XX(name) \
        case LogLevel::name: \
             return #name;               \
             break;

            XX(DEBUG)
            XX(INFO)
            XX(WARN)
            XX(ERROR)
            XX(FATAL)
#undef XX
            default:
                return "UNKOWN";
        }
    }

    std::string LogLevel::getLevelColor(LogLevel::Level level) {
        switch (level) {
            case LogLevel::DEBUG:
                return BLUE;
            case LogLevel::ERROR:
                return RED;
            case LogLevel::WARN:
                return YELLOW;
            case LogLevel::INFO:
                return GREEN;
            case LogLevel::UNKNOW:
                return RED;
            case LogLevel::FATAL:
                return RED;
        }
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, uint32_t line,
                       uint32_t elapse, pthread_t thread_id, uint32_t fiber_id, uint64_t time,
                       std::string thread_name)
            : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time),
              m_threadName(std::move(thread_name)), m_logger(std::move(logger)), m_level(level) {
    }

    void LogEvent::format(const char *fmt, ...) {}

    void LogEvent::format(const char *fmt, va_list al) {}


    LogFormatter::LogFormatter(std::string pattern) : m_pattern(std::move(pattern)) {
        init();
    }

    std::string LogFormatter::format(const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                                     const LogEvent::ptr &event) {
        std::stringstream ss;
        //ss << LogLevel::getLevelColor(level);
        for (auto &item: m_items) {
            item->format(logger, level, ss, event);
        }
        return ss.str();
    }

    /** 解析字符串m_pattern: %xxx{ xxx} %%*/
    /**eg: "%d [%p] %f %m %n"*/
    void LogFormatter::init() {
        //string,format,type
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); i++) {
            if (m_pattern[i] != '%') {
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size()) {
                if (m_pattern[i + 1] == '%') {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;
            std::string str;
            std::string fmt;
            while (n < m_pattern.size()) {
                if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                                    && m_pattern[n] != '}')) {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }

                if (fmt_status == 0) {
                    if (m_pattern[n] == '{') {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1;
                        fmt_begin = n;
                        n++;
                        continue;
                    }
                } else if (fmt_status == 1) {
                    if (m_pattern[n] == '}') {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if (n == m_pattern.size()) {
                    if (str.empty()) {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }

            if (fmt_status == 0) {
                if (!nstr.empty()) {
                    vec.emplace_back(nstr, "", 0);
                    nstr.clear();
                }
                vec.emplace_back(str, fmt, 1);
                i = n - 1;
            } else if (fmt_status == 1) {
                std::cout << "pattern parse error: " << m_pattern << "-" << m_pattern.substr(i) << std::endl;
                vec.emplace_back("<<pattern error>>", fmt, 0);
                m_error = true;
            }
        }

        if (!nstr.empty()) {
            vec.emplace_back(nstr, "", 0);
        }

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
        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C) \
            {#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt));}},

                XX(m, MessageFormatItem)     //m:消息
                XX(p, LevelFormatItem)       //p:日志级别
                XX(r, ElapseFormatItem)      //r:累计毫秒数
                XX(c, NameFormatItem)        //c:日志名称
                XX(t, ThreadIdFormatItem)    //t:线程id
                XX(n, NewLineFormatItem)     //n:换行
                XX(d, DateTimeFormatItem)    //d:时间
                XX(f, FileNameFormatItem)    //f:文件名
                XX(l, LineFormatItem)        //l:行号
                XX(T, TabFormatItem)         //T:Tab
                XX(F, FiberIdFormatItem)     //F:协程id
                XX(N, ThreadNameFormatItem)  //N:线程名称
#undef XX
        };

        //string,format,type
        for (auto &i: vec) {
            if (std::get<2>(i) == 0) {
                m_items.emplace_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            } else {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end()) {
                    m_error = true;
                    m_items.emplace_back(
                            FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                } else {
                    m_items.emplace_back(it->second(std::get<1>(i)));
                }
            }
//            std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")"
//                      << std::endl;
        }
    }

    std::ostream &
    LogFormatter::format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level,
                         LogEvent::ptr event) {
        return ofs;
    }


    Logger::Logger(std::string name) : m_name(std::move(name)), m_level(LogLevel::Level::DEBUG) {
        m_formatter = std::make_shared<LogFormatter>("[%d{%Y-%m-%d %H:%M:%S}][%t][%F][%p][%f:%l]:%m%n");
    }

    void Logger::log(LogLevel::Level level, const LogEvent::ptr &event) {
        if (level >= m_level) {
            for (const auto &it: m_appenderList) {
                it->setLogFormatter(this->m_formatter);
                it->log(Logger::ptr(nullptr), level, event);
            }
        }
    }

    void Logger::info(const LogEvent::ptr &event) {
        log(LogLevel::INFO, event);
    }

    void Logger::debug(const LogEvent::ptr &event) {
        log(LogLevel::DEBUG, event);
    }

    void Logger::warn(const LogEvent::ptr &event) {
        log(LogLevel::WARN, event);
    }

    void Logger::error(const LogEvent::ptr &event) {
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(const LogEvent::ptr &event) {
        log(LogLevel::FATAL, event);
    }


    void Logger::deleteAppender(const LogAppender::ptr &appender) {
        for (auto it = m_appenderList.begin(); it != m_appenderList.end(); ++it) {
            if (*it == appender) {
                m_appenderList.erase(it);
                break;
            }
        }
    }

    void Logger::setFormatter(LogFormatter::ptr val) {

    }

    void Logger::setFormatter(const std::string &val) {

    }

    LogFormatter::ptr Logger::getFormatter() {
        return m_formatter;
    }

    std::string Logger::toYamlString() {
        return "";
    }

    void Logger::clearAppenders() {
        m_appenderList.clear();
    }

    void Logger::addAppender(const LogAppender::ptr &appender) {
        m_appenderList.push_back(appender);
    }

    FileLogAppender::FileLogAppender(std::string
                                     file_name) : m_filename(std::move(file_name)) {
        reopen();
    }


    std::string StdoutLogAppender::toYamlString() {
        return std::string();
    }

    void
    StdoutLogAppender::log(const std::shared_ptr<Logger> &logger, LogLevel::Level level, const LogEvent::ptr &event) {
        if (level >= m_level) {
            std::cout << LogLevel::getLevelColor(level) << m_formatter->format(logger, level, event);
        }
    }


    void FileLogAppender::log(const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                              const LogEvent::ptr &event) {
        if (level >= m_level) {
            m_filestream << m_formatter->format(logger, level, event);
        }
    }

    bool FileLogAppender::reopen() {
        if (m_filestream) {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }

    std::string FileLogAppender::toYamlString() {
        return "";
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr event) : m_event(std::move(event)) {}

    std::stringstream &LogEventWrap::getSS() {
        return m_event->getSS();
    }

    LogEventWrap::~LogEventWrap() {
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }

    LoggerManager::LoggerManager() {
        m_root = std::make_shared<Logger>();
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        m_loggers[m_root->getName()] = m_root;
        init();
    }

    Logger::ptr& LoggerManager::getLogger(const std::string &name) {
        auto it = m_loggers.find(name);
        if (it != m_loggers.end()) {
            return it->second;
        }
        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root;
        m_loggers[name] = logger;
        return logger;
    }

    void LoggerManager::init() {}
}

