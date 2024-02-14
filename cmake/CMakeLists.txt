cmake_minimum_required(VERSION 3.27)
project(Server)

set(CMAKE_CXX_STANDARD 11)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../lib)
SET(BOOST_PATH /opt/homebrew/Cellar/boost/1.84.0)
SET(YAML_LIB_PATH ${PROJECT_SOURCE_DIR}/../lib/yaml-cpp)


set(
        LOG_LIB_SRC
        ../src/Log.cpp
        ../src/Log.h
        ../src/util.cpp
        ../src/util.h
        ../src/Singleton.h
)


include_directories(../src ${BOOST_PATH}/include)

link_directories(${BOOST_PATH}/lib ${YAML_LIB_PATH})

add_library(LogLib SHARED ${LOG_LIB_SRC})

#[[日志系统测试]]
#add_executable(TestLog
#        ../test/test_log.cpp
#)

#[[YAML配置系统测试]]
add_executable(TestConfigYAML
        ../src/Config.cpp
        ../src/Config.h
        ../test/testYamlConfig.cpp
)


target_link_libraries(TestConfigYAML LogLib yaml-cpp)

