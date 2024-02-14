//
// Created by 陈子锐 on 2024/2/14.
//

#include "Config.h"

Server::ConfigVar<std::string>::ptr g_desc_config = Server::Config::Lookup<std::string>("logs", "", "");

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

void testYaml() {
    YAML::Node root = YAML::LoadFile("/Users/chenzirui/CLionProjects/ServerDemo/bin/conf/log.yml");
    printYaml(root, 0);
}

void testConfig() {
    LOGI(LOG_ROOT()) << "before: " << g_desc_config->getValue();
    YAML::Node root = YAML::LoadFile("/Users/chenzirui/CLionProjects/ServerDemo/bin/conf/log.yml");
    Server::Config::LoadFromYaml(root);
    LOGI(LOG_ROOT()) << "after: " << std::endl << g_desc_config->getValue();
}

int main() {
//    LOGI(LOG_ROOT()) << g_int_config->getValue();
//    LOGI(LOG_ROOT()) << g_int_config->toString();
//    testYaml();
    testConfig();
}