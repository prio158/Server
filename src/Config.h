//
// Created by 陈子锐 on 2024/2/5.
//

#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <utility>
#include <yaml-cpp/yaml.h>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <list>
#include "Log.h"
#include "Thread.h"

//Config ------->yaml
namespace Server {

    class ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;

        explicit ConfigVarBase(std::string name, std::string desc = "")
                : m_name(std::move(name)),
                  m_desc(std::move(desc)) {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase() = default;

        const std::string &getName() const { return m_name; }

        const std::string &getDesc() const { return m_desc; }

        virtual std::string toString() = 0;

        virtual bool fromString(const std::string &val) = 0;

        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;
        std::string m_desc;

    };

    /**
     * @brief F类型 ----> T类型
     * */
    template<class F, class T>
    class LexicalCast {
    public:
        T operator()(const F &v) {
            return boost::lexical_cast<T>(v);
        }
    };

    /**
     * @brief YAML Node String -----> Vector<T>
     * */
    template<class T>
    class LexicalCast<std::string, std::vector<T>> {
    public:
        std::vector<T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (auto &&i: node) {
                ss.str("");
                ss << i;
                vec.emplace_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * @brief Vector<T>-----> YAML Node String
     * */
    template<class T>
    class LexicalCast<std::vector<T>, std::string> {
    public:
        std::string operator()(const std::vector<T> &vec) {
            YAML::Node node;
            for (auto &&i: vec) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief YAML Node String -----> list<T>
     * */
    template<class T>
    class LexicalCast<std::string, std::list<T>> {
    public:
        std::list<T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (auto &&i: node) {
                ss.str("");
                ss << i;
                vec.emplace_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
 * @brief list<T>-----> YAML Node String
 * */
    template<class T>
    class LexicalCast<std::list<T>, std::string> {
    public:
        std::string operator()(const std::list<T> &vec) {
            YAML::Node node;
            for (auto &&i: vec) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief YAML Node String -----> set<T>
     * */
    template<class T>
    class LexicalCast<std::string, std::set<T>> {
    public:
        std::set<T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (auto &&i: node) {
                ss.str("");
                ss << i;
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * @brief set<T>-----> YAML Node String
     * */
    template<class T>
    class LexicalCast<std::set<T>, std::string> {
    public:
        std::string operator()(const std::set<T> &vec) {
            YAML::Node node;
            for (auto &&i: vec) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
     * @brief YAML Node String -----> unordered_set<T>
     * */
    template<class T>
    class LexicalCast<std::string, std::unordered_set<T>> {
    public:
        std::unordered_set<T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (auto &&i: node) {
                ss.str("");
                ss << i;
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * @brief unordered_set<T>-----> YAML Node String
     * */
    template<class T>
    class LexicalCast<std::unordered_set<T>, std::string> {
    public:
        std::string operator()(const std::unordered_set<T> &vec) {
            YAML::Node node;
            for (auto &&i: vec) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    /**
    * @brief YAML Node String -----> unordered_map<string,T>
    * */
    template<class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>> {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> map;
            std::stringstream ss;
            for (auto &&it: node) {
                ss.str("");
                ss << it.second;
                map.insert(
                        std::make_pair(it.first.Scalar(),
                                       LexicalCast<std::string, T>()(ss.str())));
            }
            return map;
        }
    };

    /**
     * @brief unordered_map<string,T>-----> YAML Node String
     * */
    template<class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string> {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &vec) {
            YAML::Node node(YAML::NodeType::Map);
            for (auto &&it: vec) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(it.second)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };


    /**
     * @brief 类型转换模板类片特化(YAML String 转换成 std::map<std::string, T>)
     */
    template<class T>
    class LexicalCast<std::string, std::map<std::string, T> > {
    public:
        std::map<std::string, T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin();
                 it != node.end(); ++it) {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(),
                                          LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    /**
     * @brief 类型转换模板类片特化(std::map<std::string, T> 转换成 YAML String)
     */
    template<class T>
    class LexicalCast<std::map<std::string, T>, std::string> {
    public:
        std::string operator()(const std::map<std::string, T> &v) {
            YAML::Node node(YAML::NodeType::Map);
            for (auto &i: v) {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };


    /**
     * FromStr---> T operator() (const std::string)
     * ToStr---> std::string operator()(const T&)
     * */
    template<class T,
            class FromStr = LexicalCast<std::string, T>,
            class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase {
    public:
        typedef RWMutex RWMutexType;

        typedef std::shared_ptr<ConfigVar> ptr;

        typedef std::function<void(const T &old_value, const T &new_value)> onChangeCallback;

        explicit ConfigVar(const std::string &name,
                           const T &default_val,
                           const std::string &desc = "")
                : m_val(default_val), ConfigVarBase(name, desc) {}

        std::string toString() override {
            try {
                RWMutexType::ReadLock lock(m_mutex);
                return ToStr()(m_val);
            } catch (std::exception &e) {
                LOGE(LOG_ROOT()) << "ConfigVar::toString exception"
                                 << e.what() << "convert: " << typeid(m_val).name() << " to string";
            }
            return "";
        }

        bool fromString(const std::string &val) override {
            try {
                RWMutexType::WriteLock lock(m_mutex);
                setValue(FromStr()(val));
            } catch (std::exception &e) {
                LOGE(LOG_ROOT()) << "ConfigVar::fromString exception"
                                 << e.what() << "convert: " << typeid(m_val).name() << " to string";
            }
            return true;
        }

        T getValue() {
            RWMutexType::ReadLock lock(m_mutex);
            return m_val;
        }

        /**
         * 如果配置文件发生变化了，代码能否马上感知到变化，并做出调整
         * 配置变更事件：当一个配置项修改的时候，可以反向通知对应的代码
         * onChangeCallback: 配置变更时回调
         * */
        void setValue(const T &val) {
            //加上一个局部域，出局部域时，readLock析构
            {
                RWMutexType::ReadLock readLock(m_mutex);
                if (val == m_val) return;
                for (auto &callback: changeCallbackMap) {

                    callback.second(m_val, val);
                }
            }
            RWMutexType::WriteLock writeLock(m_mutex);
            m_val = val;
        }

        std::string getTypeName() const override {
            return typeid(T).name();
        };

        uint64_t addChangeCallback(onChangeCallback on_change_callback) {
            static uint64_t s_fun_id = 0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            changeCallbackMap[s_fun_id] = std::move(on_change_callback);
            return s_fun_id;
        }

        void deleteChangeCallback(uint64_t key) {
            RWMutexType::WriteLock lock(m_mutex);
            changeCallbackMap.erase(key);
        }

        onChangeCallback getChangeCallback(uint64_t key) {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = changeCallbackMap.find(key);
            return it == changeCallbackMap.end() ? nullptr : it->second;
        }

        void clearCallbackMap() {
            RWMutexType::WriteLock lock(m_mutex);
            changeCallbackMap.clear();
        }

    private:
        RWMutexType m_mutex;
        T m_val;
        ///回调Maps，uint64_t key要求唯一，可以用hash作为key
        std::map<uint64_t, onChangeCallback> changeCallbackMap;
    };


    class Config {
    public:
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        typedef RWMutex RWMutexType; //读多写少，就用读写锁

        /**
         * @brief 获取/创建对应参数名的配置参数
         * @param[in] name 配置参数名称
         * @param[in] default_value 参数默认值
         * @param[in] description 参数描述
         * @details 获取参数名为name的配置参数,如果存在直接返回
         *          如果不存在,创建参数配置并用default_value赋值
         * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
         * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
         */
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(
                const std::string &name,
                const T &default_value,
                const std::string &desc = "") {
            RWMutexType::WriteLock lock(GetMutex());
            auto it = GetData().find(name);
            if (it != GetData().end()) {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                if (tmp) {
                    LOGI(LOG_ROOT()) << "Lookup name = " << it->first << " exists";
                    return tmp;
                } else {
                    LOGE(LOG_ROOT()) << "Lookup name =" << name << " exists,but type: " <<
                                     typeid(T).name() << " not support" << ",real type = " << it->second->getTypeName()
                                     << " " << it->second->toString();
                    return nullptr;
                }
            }
            /** 正向查找在原字符串中第一个与指定字符串（或字符）中的任一字符都不匹配的字符，返回它的位置。若查找失败，则返回npos。*/
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") !=
                std::string::npos) {
                LOGE(LOG_ROOT()) << "Lookup name invalid:" << name;
                throw std::invalid_argument(name);
            }
            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, desc));
            GetData()[name] = v;
            return v;
        }

        /**
         * @brief 查找配置参数
         * @param[in] name 配置参数名称
         * @return 返回配置参数名为name的配置参数
         */
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
            RWMutexType::ReadLock lock(GetMutex());
            auto it = GetData().find(name);
            if (it == GetData().end()) {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        /**
         * @brief 使用YAML::Node初始化配置模块
         */
        static void LoadFromYaml(const YAML::Node &node);

        /**
         * @brief 查找配置参数,返回配置参数的基类
         * @param[in] name 配置参数名称
         */
        static ConfigVarBase::ptr LookupBase(const std::string &name);

        /**
         * @brief 遍历配置模块里面所有配置项
         * @param[in] cb 配置项回调函数
         */
        static void Visit(const std::function<void(ConfigVarBase::ptr)>& cb);

    private:
        /** 这里用静态原因是保证：configVarMap和mutexType的初始化顺序
         * 早于成员对象，也就是说保证在使用它们的时候，它们一定是初始化了的*/
        /**
        * @brief 返回所有的配置项
        */
        static ConfigVarMap &GetData() {
            static ConfigVarMap configVarMap;
            return configVarMap;
        }

        /**
         * @brief 配置项的RWMutex
         */
        static RWMutexType &GetMutex(){
            static RWMutexType mutexType;
            return mutexType;
        };
    };

    struct LogDefine {
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter = "UNKNOW";
        std::vector<LogAppenderDefine> appenders;

        void toString() const {
            std::string typeString;
            for (auto &item: appenders) {
                typeString += std::to_string(item.type);
                typeString += " ";
            }

            std::cout << "[ name=" << name <<
                      " level=" << LogLevel::ToString(level) <<
                      " formatter=" << formatter <<
                      " appenders=" << "[ " << typeString << "]"
                                                             " ]" << std::endl;
        }

        bool operator==(const LogDefine &oth) const {
            return name == oth.name
                   && level == oth.level
                   && formatter == oth.formatter
                   && appenders == appenders;
        }

        bool operator<(const LogDefine &oth) const {
            return name < oth.name;
        }

        bool isValid() const {
            return !name.empty();
        }
    };

    template<>
    class LexicalCast<LogDefine, std::string> {
    public:
        std::string operator()(const LogDefine &i) {
            YAML::Node n;
            n["name"] = i.name;
            if (i.level != LogLevel::UNKNOW) {
                n["level"] = LogLevel::ToString(i.level);
            }
            if (!i.formatter.empty()) {
                n["formatter"] = i.formatter;
            }

            for (auto &a: i.appenders) {
                YAML::Node na;
                if (a.type == 1) {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if (a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }
                if (a.level != LogLevel::UNKNOW) {
                    na["level"] = LogLevel::ToString(a.level);
                }

                if (!a.formatter.empty()) {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            std::stringstream ss;
            ss << n;
            return ss.str();
        }
    };

    template<>
    class LexicalCast<std::string, LogDefine> {
    public:
        LogDefine operator()(const std::string &v) {
            YAML::Node n = YAML::Load(v);
            LogDefine ld;
            if (!n["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << n
                          << std::endl;
                throw std::logic_error("log config name is null");
            }
            ld.name = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            if (n["formatter"].IsDefined()) {
                ld.formatter = n["formatter"].as<std::string>();
            }

            if (n["appenders"].IsDefined()) {
                //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
                for (size_t x = 0; x < n["appenders"].size(); ++x) {
                    auto a = n["appenders"][x];
                    if (!a["type"].IsDefined()) {
                        std::cout << "log config error: appender type is null, " << a
                                  << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if (type == "FileLogAppender") {
                        lad.type = 1;
                        if (!a["file"].IsDefined()) {
                            std::cout << "log config error: fileappender file is null, " << a
                                      << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if (a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else if (type == "StdoutLogAppender") {
                        lad.type = 2;
                        if (a["formatter"].IsDefined()) {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else {
                        std::cout << "log config error: appender type is invalid, " << a
                                  << std::endl;
                        continue;
                    }
                    ld.appenders.push_back(lad);
                }
            }
            return ld;
        }
    };

//    TODO
//    static auto g_log_defines1 =
//            Config::Lookup("logs", std::set<LogDefine>(), "logs config");

//    ///在main函数之前执行
//    struct LogIniter {
//        LogIniter() {
//            ///事件注册
//            g_log_defines->addChangeCallback(12323,
//                                             [](
//                                                     std::set<LogDefine> old_value,
//                                                     std::set<LogDefine> new_value) {
//                                                 LOGI(LOG_ROOT()) << "on_logger_conf_changed";
//                                                 for (auto &i: new_value) {
//                                                     auto it = old_value.find(i);
//                                                     Logger::ptr logger;
//                                                     if (it == old_value.end()) {
//                                                         //新增logger
//                                                         logger = LOG_NAME(i.name);
//                                                     } else {
//                                                         if (!(i == *it)) {
//                                                             //修改的logger
//                                                             logger = LOG_NAME(i.name);
//                                                         } else {
//                                                             continue;
//                                                         }
//                                                     }
//                                                     logger->setLevel(i.level);
//                                                     //std::cout << "** " << i.name << " level=" << i.level
//                                                     //<< "  " << logger << std::endl;
//                                                     if (!i.formatter.empty()) {
//                                                         logger->setFormatter(i.formatter);
//                                                     }
//
//                                                     logger->clearAppenders();
//                                                     for (auto &a: i.appenders) {
//                                                         LogAppender::ptr ap;
//                                                         if (a.type == 1) {
//                                                             ap.reset(new FileLogAppender(a.file));
//                                                         } else if (a.type == 2) {
////                                                             if (!LoggerMgr::GetInstance()->has("d")) {
//                                                             ap.reset(new StdoutLogAppender);
////                                                             } else {
////                                                             continue;
////                                                             }
//                                                         }
//                                                         ap->setLevel(a.level);
//                                                         if (!a.formatter.empty()) {
//                                                             LogFormatter::ptr fmt(new LogFormatter(a.formatter));
//                                                             if (!fmt->isError()) {
//                                                                 ap->setLogFormatter(fmt);
//                                                             } else {
//                                                                 std::cout << "log.name=" << i.name << " appender type="
//                                                                           << a.type
//                                                                           << " formatter=" << a.formatter
//                                                                           << " is invalid" << std::endl;
//                                                             }
//                                                         }
//                                                         logger->addAppender(ap);
//                                                     }
//                                                 }
//
//                                                 for (auto &i: old_value) {
//                                                     auto it = new_value.find(i);
//                                                     if (it == new_value.end()) {
//                                                         //删除logger
//                                                         auto logger = LOG_NAME(i.name);
//                                                         logger->setLevel((LogLevel::Level) 0);
//                                                         logger->clearAppenders();
//                                                     }
//                                                 }
//                                             });
//        }
//    };
//
//    static LogIniter __log_init__;


}


#endif //SERVER_CONFIG_H
