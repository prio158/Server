cmake_minimum_required(VERSION 3.27)
project(Server)

SET(CMAKE_CXX_STANDARD 11)
add_definitions("-Wall -g")

SET(USR_PATH /usr/local/)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../lib)

set(
        LIB_SRC
        ../src/Log.cpp
        ../src/Log.h
        ../src/Util.cpp
        ../src/Util.h
        ../src/Singleton.h
        ../src/Config.cpp
        ../src/Config.h
        ../src/Thread.cpp
        ../src/Thread.h
        ../src/Fiber.h
        ../src/Fiber.cpp
)

include_directories(../src ${USR_PATH}/include)

link_directories(${USR_PATH}/lib)

#add_library(LogConfigLib SHARED ${LOG_LIB_SRC})

#[[日志系统测试]]
#add_executable(
#        TestLog
#        ${LOG_LIB_SRC}
#        ../test/test_log.cpp
#)
#target_link_libraries(TestLog yaml-cpp)

#[[YAML配置系统测试]]
#add_executable(
#        TestConfigYAML
#        ${LOG_LIB_SRC}
#        ../test/testYamlConfig.cpp
#)
#target_link_libraries(TestConfigYAML yaml-cpp)

#[[Thread系统测试]]
#add_executable(
#        TestThread
#        ${LOG_LIB_SRC}
#        ../test/test_thread.cpp
#)
#target_link_libraries(TestThread yaml-cpp)

#[[工具系统测试]]
#add_executable(
#        TestMacro
#        ${LIB_SRC}
#        ../test/test_util.cpp
#)
#target_link_libraries(TestMacro yaml-cpp)

#[[Fiber系统测试]]
add_executable(
        TestFiberCrash
        ${LIB_SRC}
        ../test/test_fiber.cpp
)
target_link_libraries(TestFiberCrash yaml-cpp)


