//
// Created by 陈子锐 on 2024/2/14.
//

#include "Config.h"

#include <memory>

static YAML::Node root = YAML::LoadFile("/Users/chenzirui/CLionProjects/ServerDemo/bin/conf/test.yml");
static Server::ConfigVar<int>::ptr g_int_config = Server::Config::Lookup<int>("system.port", (int) 8080, "port");
static Server::ConfigVar<float>::ptr g_float_config = Server::Config::Lookup<float>("system.value", 10.0f, "value");
static Server::ConfigVar<std::vector<int>>::ptr g_vec_config = Server::Config::Lookup<std::vector<int>>(
        "system.int_vec",
        std::vector<int>{1, 2},
        "vec");
static Server::ConfigVar<std::list<int>>::ptr g_list_config = Server::Config::Lookup<std::list<int>>(
        "system.int_list",
        std::list<int>{1, 2},
        "list");
static Server::ConfigVar<std::set<int>>::ptr g_set_config = Server::Config::Lookup<std::set<int>>(
        "system.int_set",
        std::set<int>{1, 2},
        "set");
static Server::ConfigVar<std::unordered_set<int>>::ptr g_unordered_set_config = Server::Config::Lookup<std::unordered_set<int>>(
        "system.int_oun_set",
        std::unordered_set<int>{1, 2, 3},
        "unordered_set");
static Server::ConfigVar<std::unordered_map<std::string, int>>::ptr g_unordered_map_config = Server::Config::Lookup<std::unordered_map<std::string, int>>(
        "system.int_oun_map",
        std::unordered_map<std::string, int>{std::make_pair("key1", 1)},
        "unordered_map");

class Person {
public:
    std::string name{};
    int age = 0;
    bool sex = true;
public:
    explicit Person(std::string name, int age, bool sex) : name(std::move(name)), age(age), sex(sex) {}

    bool operator==(const Person &person) const {
        return person.sex == sex && person.age == age && person.name == name;
    }

    bool operator<(const Person &person) const {
        return person.age <= age;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "[ Person name=" << name
           << " age=" << age
           << " sex=" << sex
           << " ]";
        return ss.str();
    }
};

namespace Server {
    template<>
    class LexicalCast<Person, std::string> {
    public:
        std::string operator()(const Person &person) {
            YAML::Node node;
            node["name"] = person.name;
            node["age"] = person.age;
            node["sex"] = person.sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template<>
    class LexicalCast<std::string, Person> {
    public:
        Person operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            try {
                Person p(
                        node["name"].as<std::string>(),
                        node["age"].as<int>(),
                        node["sex"].as<bool>()
                );
                return p;
            } catch (std::exception e) {
                LOGE(LOG_ROOT()) << "LexicalCast Person fail,"
                                 << "[Person name: " << node["name"]
                                 << "  age: " << node["age"]
                                 << "  sex: " << node["sex"] << "]";
            }
        }
    };
}

static Server::ConfigVar<Person>::ptr g_person_config = Server::Config::Lookup<Person>(
        "system.person",
        Person("czr", 12, true),
        "person");

//static Server::ConfigVar<std::set<Person>>::ptr g_vec_person_config = Server::Config::Lookup<std::set<Person>>(
//        "",
//        std::set<Person>(),
//        "vec");

static Server::ConfigVar<std::set<Server::LogDefine>>::ptr g_log_define_test =
        Server::Config::Lookup("logs", std::set<Server::LogDefine>(), "logs config");

template<class T>
void printMap(std::unordered_map<std::string, T> &map) {
    for (auto &&it: map) {
        LOGI(LOG_ROOT()) << it.first << ":" << it.second;
    }
}

void printYaml(const YAML::Node &node, int level) {
    if (node.IsScalar()) {
        LOGI(LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << "-" << level;
    } else if (node.IsNull()) {
        LOGI(LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
    } else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); it++) {
            LOGI(LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - "
                             << level;
            printYaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (size_t it = 0; it != node.size(); ++it) {
            LOGI(LOG_ROOT()) << std::string(level * 4, ' ') << it << " - " << node[it].Type() << " - " << level;
            printYaml(node[it], level + 1);
        }
    }
}

/**YAML Node String 转换为 Int*/
void testConfigInt() {
    LOGI(LOG_ROOT()) << "before: " << g_int_config->getValue();
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << g_int_config->getValue();
}

void testConfigFloat() {
    LOGI(LOG_ROOT()) << "before: " << g_float_config->getValue();
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << g_float_config->getValue();
}

void testConfigVec() {
    LOGI(LOG_ROOT()) << "before: " << g_vec_config->toString();
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << g_vec_config->toString();
}


void testConfigList() {
    LOGI(LOG_ROOT()) << "before: " << g_list_config->toString();
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << g_list_config->toString();
}

void testConfigSet() {
    LOGI(LOG_ROOT()) << "before: " << g_set_config->toString();
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << g_set_config->toString();
}

void testConfigUnorderedSet() {
    LOGI(LOG_ROOT()) << g_unordered_set_config->toString();
    LOGI(LOG_ROOT()) << "-------------------------------";
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << g_unordered_set_config->toString();
}

void testConfigUnorderedMap() {
    auto valueBefore = g_unordered_map_config->getValue();
    printMap(valueBefore);
    LOGI(LOG_ROOT()) << "-------------------------------";
    Server::Config::LoadFromYaml(root);
    auto valueAfter = g_unordered_map_config->getValue();
    printMap(valueAfter);
}

void testConfigUserType() {
    g_person_config->addChangeCallback([](const Person &oldVal, const Person &newVal) {
        LOGI(LOG_ROOT()) << "oldValue:" << oldVal.toString();
        LOGI(LOG_ROOT()) << "newValue:" << newVal.toString();
    });
    auto valueBefore = g_person_config->getValue();
    LOGI(LOG_ROOT()) << valueBefore.toString();
    LOGI(LOG_ROOT()) << "-------------------------------";
    Server::Config::LoadFromYaml(root);
    auto valueAfter = g_person_config->getValue();
    LOGI(LOG_ROOT()) << valueAfter.toString();
}

void testConfigLogDefine() {
    LOGI(LOG_ROOT()) << "-------------------------------";
    Server::Config::LoadFromYaml(root);
    auto logDefineSet = g_log_define_test->getValue();
    for (auto &it: logDefineSet) {
        it.toString();
    }
}

void testToYamlString() {
    std::cout << Server::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node node = YAML::LoadFile("/Users/chenzirui/CLionProjects/ServerDemo/bin/conf/test.yml");
    Server::Config::LoadFromYaml(node);
    std::cout << "==============================" << std::endl;
    std::cout << Server::LoggerMgr::GetInstance()->toYamlString() << std::endl;
}

struct LogIniter {
    LogIniter() {
        ///事件注册
        g_log_define_test->addChangeCallback([](
                std::set<Server::LogDefine> old_value,
                std::set<Server::LogDefine> new_value) {
            LOGI(LOG_ROOT()) << "on_logger_conf_changed";
            for (auto &i: new_value) {
                auto it = old_value.find(i);
                Server::Logger::ptr logger;
                if (it == old_value.end()) {
                    //新增logger
                    logger = LOG_NAME(i.name);
                } else {
                    if (!(i == *it)) {
                        //修改的logger
                        logger = LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                //std::cout << "** " << i.name << " level=" << i.level
                //<< "  " << logger << std::endl;
                if (!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for (auto &a: i.appenders) {
                    Server::LogAppender::ptr ap;
                    if (a.type == 1) {
                        ap = std::make_shared<Server::FileLogAppender>(a.file);
                    } else if (a.type == 2) {
//                                                             if (!LoggerMgr::GetInstance()->has("d")) {
                        ap = std::make_shared<Server::StdoutLogAppender>();
//                                                             } else {
//                                                             continue;
//                                                             }
                    }
                    ap->setLevel(a.level);
                    if (!a.formatter.empty()) {
                        Server::LogFormatter::ptr fmt(
                                new Server::LogFormatter(a.formatter));
                        if (!fmt->isError()) {
                            ap->setLogFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type="
                                      << a.type
                                      << " formatter=" << a.formatter
                                      << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for (auto &i: old_value) {
                auto it = new_value.find(i);
                if (it == new_value.end()) {
                    //删除logger
                    auto logger = LOG_NAME(i.name);
                    logger->setLevel((Server::LogLevel::Level) 0);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init__;

int main() {
    //testConfigInt();
    //testConfigFloat();
    //testConfigVec();
    //testConfigList();
    //testConfigSet(); //排序+去重
    //testConfigUnorderedSet(); //去重
    //testConfigUnorderedMap();
    //testConfigUserType(); //自定义类型
    //testConfigLogDefine();
    testToYamlString();
}