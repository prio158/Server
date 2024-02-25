//
// Created by 陈子锐 on 2024/2/5.
//

#include "Config.h"


namespace Server {

    static void ListAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output) {
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678") !=
            std::string::npos) {
            LOGE(LOG_ROOT()) << "Config invalid name: " << prefix << node;
            return;
        }
        output.emplace_back(prefix, node);
        if (node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); it++) {
                ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + '.' + it->first.Scalar(), it->second,
                              output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node &node) {
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", node, all_nodes);
        for (auto& it: all_nodes) {
            std::string key = it.first;
            if (key.empty()) continue;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);
            if (var) {
                if (it.second.IsScalar()) {
                    var->fromString(it.second.Scalar());
                } else {
                    std::stringstream ss;
                    ss << it.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

    ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetData().find(name);
        return it == GetData().end() ? nullptr : it->second;
    }

    void Config::Visit(const std::function<void(ConfigVarBase::ptr)>& cb) {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap& map = GetData();
        for(const auto& it:map){
            cb(it.second);
        }
    }
}