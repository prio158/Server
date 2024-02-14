//
// Created by 陈子锐 on 2024/2/14.
//

#include "Config.h"

Server::ConfigVar<int>::ptr g_int_config = Server::Config::Lookup<int>("system.port",8080,"");

int main(){
    LOGI(LOG_ROOT()) << g_int_config->getValue();
    LOGI(LOG_ROOT()) << g_int_config->toString();
}