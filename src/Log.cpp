//
// Created by 陈子锐 on 2024/2/1.
//

#include "Log.h"

#include <utility>

const char *ServerDemo::LogLevel::ToString(ServerDemo::LogLevel::Level level) {
    switch (level) {
#define XX(name) \
        case ServerDemo::LogLevel::name: \
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


void ServerDemo::LogAppender::log(const std::shared_ptr<Logger> &logger, ServerDemo::LogLevel::Level level,
                                  const ServerDemo::LogEvent::ptr &event) {

}


ServerDemo::LogFormatter::LogFormatter(std::string pattern) : m_pattern(std::move(pattern)) {}

std::string ServerDemo::LogFormatter::format(const std::shared_ptr<Logger> &logger, LogLevel::Level level,
                                             const ServerDemo::LogEvent::ptr &event) {
    std::stringstream ss;
    for (auto &item: m_items) {
        item->format(logger, level, ss, event);
    }
    return ss.str();
}

/** 解析字符串m_pattern: %xxx{xxx} %%*/
void ServerDemo::LogFormatter::init() {
    //string,format,type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    //size_t last_pos = 0;
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); i++) {

        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if (i + 1 < m_pattern.size()) {
            if (m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        std::string str;
        std::string fmt;
        size_t fmt_begin = 0;
        while (n < m_pattern.size()) {
            if (isspace(m_pattern[n])) {
                break;
            }
            if (fmt_status == 0) {
                if (m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1; //解析格式
                    n++;
                    fmt_begin = n;
                    continue;
                }
            }
            if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 2;
                    break;
                }
            }
        }
        if (fmt_status == 0) {
            if (!nstr.empty())
                vec.emplace_back(nstr, "", 0);
            nstr = m_pattern.substr(i + 1, n - i - 1);
            vec.emplace_back(nstr, fmt, 1);
            i = n;
        } else if (fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << "-" << m_pattern.substr(i) << std::endl;
            vec.emplace_back("<pattern error>", fmt, 0);
        } else {
            if (!nstr.empty())
                vec.emplace_back(nstr, "", 0);
            vec.emplace_back(nstr, fmt, 1);
            i = n;
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
            {#str, [](const std::string &fmt) { return FormatItem::ptr(new ServerDemo::C(fmt));}},

            XX("m", MessageFormatItem)
            XX("p", LevelFormatItem)
            XX("r", ElapseFormatItem)
            XX("c", NameFormatItem)
            XX("t", ThreadIdFormatItem)
            XX("n", NewLineFormatItem)
            XX("d", DateTimeFormatItem)
            XX("f", FileNameFormatItem)
            XX("l", LineFormatItem)

#undef XX
    };

    for (auto &i: vec) {
        if (std::get<2>(i) == 0) {
            m_items.emplace_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                m_items.emplace_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
            } else {
                m_items.emplace_back(it->second(std::get<1>(i)));
            }
        }
        std::cout << std::get<0>(i) << "-" << std::get<1>(i) << "-" << std::get<2>(i) << std::endl;
    }
}


ServerDemo::Logger::Logger(std::string name):m_name(std::move(name)) {}

void ServerDemo::Logger::log(ServerDemo::LogLevel::Level level, const ServerDemo::LogEvent::ptr &event) {
    if (level >= m_level) {
        for (const auto &it: m_appenderList) {
            it->log(std::shared_ptr<Logger>(this), level, event);
        }
    }
}

void ServerDemo::Logger::info(const ServerDemo::LogEvent::ptr &event) {
    log(LogLevel::INFO, event);
}

void ServerDemo::Logger::debug(const ServerDemo::LogEvent::ptr &event) {
    log(LogLevel::DEBUG, event);
}

void ServerDemo::Logger::warn(const ServerDemo::LogEvent::ptr &event) {
    log(LogLevel::WARN, event);
}

void ServerDemo::Logger::error(const ServerDemo::LogEvent::ptr &event) {
    log(LogLevel::ERROR, event);
}

void ServerDemo::Logger::fatal(const ServerDemo::LogEvent::ptr &event) {
    log(LogLevel::FATAL, event);
}

void ServerDemo::Logger::addAppender(const ServerDemo::LogAppender::ptr &appender) {
    m_appenderList.push_back(appender);
}

void ServerDemo::Logger::deleteAppender(const ServerDemo::LogAppender::ptr &appender) {
    for (auto it = m_appenderList.begin(); it != m_appenderList.end(); ++it) {
        if (*it == appender) {
            m_appenderList.erase(it);
            break;
        }
    }
}

ServerDemo::FileLogAppender::FileLogAppender(std::string
                                             file_name) : m_filename(std::move(file_name)) {}


void ServerDemo::StdoutLogAppender::log(const std::shared_ptr<Logger> &logger, ServerDemo::LogLevel::Level level,
                                        const ServerDemo::LogEvent::ptr &event) {
    if (level >= m_level) {
        std::cout << m_formatter->format(logger, level, event);
    }
}


void ServerDemo::FileLogAppender::log(const std::shared_ptr<Logger> &logger, ServerDemo::LogLevel::Level level,
                                      const ServerDemo::LogEvent::ptr &event) {
    if (level >= m_level) {
        m_filestream << m_formatter->format(logger, level, event);
    }
}

bool ServerDemo::FileLogAppender::ropen() {
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}


