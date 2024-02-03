cmake_minimum_required(VERSION 3.27)
project(Server)

set(CMAKE_CXX_STANDARD 11)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../lib)

set(
        LIB_SRC
        ../src/Log.cpp
        ../src/Log.h
        ../src/util.cpp
        ../src/util.h
)

include_directories(../src)

add_library(LogLib SHARED ${LIB_SRC})

add_executable(TestLog
        ../test/test_log.cpp
)

target_link_libraries(TestLog LogLib)

