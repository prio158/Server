cmake_minimum_required(VERSION 3.27)
project(ServerDemo)

set(CMAKE_CXX_STANDARD 11)

add_executable(ServerDemo ../main.cpp
        ../src/Log.cpp
        ../src/Log.h)
