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
#include "Log.h"

//Config ------->yaml
namespace Server {

    class ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;

        explicit ConfigVarBase(std::string name, std::string desc = "")
                : m_name(std::move(name)),
                  m_desc(std::move(desc)) {}

        virtual ~ConfigVarBase() = default;

        const std::string &getName() const { return m_name; }

        const std::string &getDesc() const { return m_desc; }

        virtual std::string toString() = 0;

        virtual bool fromString(const std::string &val) = 0;

    protected:
        std::string m_name;
        std::string m_desc;

    };


    template<class T>
    class ConfigVar : public ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;

        explicit ConfigVar(const std::string &name,
                           const T &default_val,
                           const std::string &desc = "")
                : m_val(default_val), ConfigVarBase(name, desc) {}

        std::string toString() override {
            try {
                return boost::lexical_cast<std::string>(m_val);
            } catch (std::exception &e) {
                LOGE(LOG_ROOT()) << "ConfigVar::toString exception"
                                 << e.what() << "convert: " << typeid(m_val).name() << " to string";
            }
            return "";
        }

        bool fromString(const std::string &val) override {
            try {
                m_val = boost::lexical_cast<T>(val);
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
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGIJKLMNOPQRSTUVWXYZ._012345678") !=
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

    private:
        static ConfigVarMap configVarMap;
    };
}


#endif //SERVER_CONFIG_H
