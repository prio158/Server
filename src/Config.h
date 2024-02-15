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
#include <list>
#include "Log.h"

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
        typedef std::shared_ptr<ConfigVar> ptr;

        explicit ConfigVar(const std::string &name,
                           const T &default_val,
                           const std::string &desc = "")
                : m_val(default_val), ConfigVarBase(name, desc) {}

        std::string toString() override {
            try {
                return ToStr()(m_val);
            } catch (std::exception &e) {
                LOGE(LOG_ROOT()) << "ConfigVar::toString exception"
                                 << e.what() << "convert: " << typeid(m_val).name() << " to string";
            }
            return "";
        }

        bool fromString(const std::string &val) override {
            try {
                //m_val = boost::lexical_cast<T>(val);
                setValue(FromStr()(val));
            } catch (std::exception &e) {
                LOGE(LOG_ROOT()) << "ConfigVar::fromString exception"
                                 << e.what() << "convert: " << typeid(m_val).name() << " to string";
            }
        }

        T getValue() const { return m_val; }

        void setValue(T val) { m_val = val; }

    private:
        T m_val;
    };


    class Config {
    public:
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        template<class T>
        static typename ConfigVar<T>::ptr Lookup(
                const std::string &name,
                const T &default_value,
                const std::string &desc = "") {
            auto tmp = Lookup<T>(name);
            if (tmp) {
                LOGI(LOG_ROOT()) << "Lookup name =" << name << " exists";
                return tmp;
            }
            /** 正向查找在原字符串中第一个与指定字符串（或字符）中的任一字符都不匹配的字符，返回它的位置。若查找失败，则返回npos。*/
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") !=
                std::string::npos) {
                LOGE(LOG_ROOT()) << "Lookup name invalid:" << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, desc));
            configVarMap[name] = v;
            return v;
        }

        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
            auto it = configVarMap.find(name);
            if (it == configVarMap.end()) {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }

        static void LoadFromYaml(const YAML::Node &node);

        static ConfigVarBase::ptr LookupBase(const std::string &name);

    private:
        static ConfigVarMap configVarMap;
    };
}


#endif //SERVER_CONFIG_H
