//
// Created by 陈子锐 on 2024/2/14.
//

#include "Config.h"

static Server::ConfigVar<int>::ptr g_int_config = Server::Config::Lookup<int>("system.port", (int) 8080, "port");
static Server::ConfigVar<float>::ptr g_float_config = Server::Config::Lookup<float>("system.value", 10.0, "value");
static Server::ConfigVar<std::vector<int>>::ptr g_vec_config = Server::Config::Lookup<std::vector<int>>(
        "system.int_vec",
        std::vector<int>{1, 2},
        "vec");

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
    YAML::Node root = YAML::LoadFile("/Users/chenzirui/CLionProjects/ServerDemo/bin/conf/test.yml");
    LOGI(LOG_ROOT()) << "before: " << g_int_config->getValue();
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << g_int_config->getValue();
}

void testConfigFloat() {
    YAML::Node root = YAML::LoadFile("/Users/chenzirui/CLionProjects/ServerDemo/bin/conf/test.yml");
    LOGI(LOG_ROOT()) << "before: " << g_float_config->getValue();
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << g_float_config->getValue();
}

void testConfigVec() {
    YAML::Node root = YAML::LoadFile("/Users/chenzirui/CLionProjects/ServerDemo/bin/conf/test.yml");
    LOGI(LOG_ROOT()) << "before: " << g_vec_config->toString();
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << g_vec_config->toString();
}

int main() {
    //testConfigInt();
    //testConfigFloat();
    testConfigVec();

}