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
        ../src/Config.cpp
        ../src/Config.h
        ../src/Thread.cpp
        ../src/Thread.h
)

include_directories(../src ${BOOST_PATH}/include)

link_directories(${BOOST_PATH}/lib ${YAML_LIB_PATH})

#add_library(LogConfigLib SHARED ${LOG_LIB_SRC})

#[[日志系统测试]]
#add_executable(
#        TestLog
#        ${LOG_LIB_SRC}
#        ../test/test_log.cpp
#)

#[[YAML配置系统测试]]
#add_executable(想·
#        TestConfigYAML
#        ${LOG_LIB_SRC}
#        ../test/testYamlConfig.cpp想·
#)

#[[Thread系统测试]]
add_executable(
        TestThread
        ${LOG_LIB_SRC}
        ../test/test_thread.cpp
)

target_link_libraries(TestThread yaml-cpp)

