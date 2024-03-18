cmake_minimum_required(VERSION 3.27)
project(Server)

SET(CMAKE_CXX_STANDARD 11)
add_definitions("-Wall -g")

SET(USR_PATH /usr/local)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../lib)

SET(
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
        ../src/Scheduler.cpp
        ../src/Scheduler.h
        ../src/Mutex.cpp
        ../src/Mutex.h
        ../src/IOSchedule.cpp
        ../src/IOSchedule.h
        ../src/Timer.cpp
        ../src/Timer.h
        ../src/Hook.cpp
        ../src/Hook.h
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
#        ../test/test_yaml_config.cpp
#)
#target_link_libraries(TestConfigYAML yaml-cpp)

#[[Thread系统测试]]
#add_executable(
#        TestThread
#        ${LIB_SRC}
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
#add_executable(
#        TestFiber
#        ${LIB_SRC}
#        ../test/test_fiber.cpp
#)
#target_link_libraries(TestFiber yaml-cpp)

#[[Schedule系统测试]]
#add_executable(
#        TestSchedule
#        ${LIB_SRC}
#        ../test/test_scheduler.cpp
#)
#target_link_libraries(TestSchedule yaml-cpp)

#[[IO Schedule系统测试]]
#add_executable(
#        TestIOSchedule
#        ${LIB_SRC}
#        ../test/test_io_schedule.cpp
#)
#target_link_libraries(TestIOSchedule yaml-cpp)

#[[定时器系统测试]]
#add_executable(
#        TestTimer
#        ${LIB_SRC}
#        ../test/test_timer.cpp
#)
#target_link_libraries(TestTimer yaml-cpp)

#[[HOOK系统测试]]
add_executable(
        TestHook
        ${LIB_SRC}
        ../test/test_hook.cpp
)
target_link_libraries(TestHook yaml-cpp)

