//
// Created by 陈子锐 on 2024/2/1.
//

#include "Log.h"

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
    return "UNKOWN";
}


void ServerDemo::LogAppender::log(ServerDemo::LogLevel::Level level, ServerDemo::LogEvent::ptr event) {

}


ServerDemo::LogFormatter::LogFormatter(std::string pattern) : m_pattern(std::move(pattern)) {}

std::string ServerDemo::LogFormatter::format(LogLevel::Level level, ServerDemo::LogEvent::ptr event) {
    std::stringstream ss;
    for (auto &item: m_items) {
        item->format(level, ss, event);
    }
    return ss.str();
}

/** 解析字符串m_pattern: %xxx{xxx} %%*/
void ServerDemo::LogFormatter::init() {
    //string,format,type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    size_t last_pos = 0;
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
        } else if (fmt_status == 2) {
            if (!nstr.empty())
                vec.emplace_back(nstr, "", 0);
            vec.emplace_back(nstr, fmt, 1);
            i = n;
        }
    }

    if (!nstr.empty()) {
        vec.emplace_back(nstr, "", 0);
    }


}


ServerDemo::Logger::Logger(const std::string &name) {

}

void ServerDemo::Logger::log(ServerDemo::LogLevel::Level level, ServerDemo::LogEvent::ptr evnet) {
    if (level >= m_level) {
        for (const auto &it: m_appenderList) {
            it->log(level, evnet);
        }
    }
}

void ServerDemo::Logger::info(ServerDemo::LogEvent::ptr event) {
    log(LogLevel::INFO, std::move(event));
}

void ServerDemo::Logger::debug(ServerDemo::LogEvent::ptr event) {
    log(LogLevel::DEBUG, std::move(event));
}

void ServerDemo::Logger::warn(ServerDemo::LogEvent::ptr event) {
    log(LogLevel::WARN, std::move(event));
}

void ServerDemo::Logger::error(ServerDemo::LogEvent::ptr event) {
    log(LogLevel::ERROR, std::move(event));
}

void ServerDemo::Logger::fatal(ServerDemo::LogEvent::ptr event) {
    log(LogLevel::FATAL, std::move(event));
}

void ServerDemo::Logger::addAppender(ServerDemo::LogAppender::ptr appender) {
    m_appenderList.push_back(appender);
}

void ServerDemo::Logger::deleteAppender(ServerDemo::LogAppender::ptr appender) {
    for (auto it = m_appenderList.begin(); it != m_appenderList.end(); ++it) {
        if (*it == appender) {
            m_appenderList.erase(it);
            break;
        }
    }
}

ServerDemo::FileLogAppender::FileLogAppender(std::string
                                             file_name) : m_filename(std::move(file_name)) {}


void ServerDemo::StdoutLogAppender::log(ServerDemo::LogLevel::Level level, ServerDemo::LogEvent::ptr event) {
    if (level >= m_level) {
        std::cout << m_formatter->format(level, event);
    }
}


void ServerDemo::FileLogAppender::log(ServerDemo::LogLevel::Level level, ServerDemo::LogEvent::ptr event) {
    if (level >= m_level) {
        m_filestream << m_formatter->format(level, event);
    }
}

bool ServerDemo::FileLogAppender::ropen() {
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}


