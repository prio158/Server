cmake_minimum_required(VERSION 3.27)
project(Server)

set(CMAKE_CXX_STANDARD 11)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../lib)
SET(BOOST_PATH /opt/homebrew/Cellar/boost/1.84.0)
SET(USR_INCLUDE_PATH /usr/local/include/)
SET(USR_LIB_PATH /usr/local/lib)

set(
        LIB_SRC
        ../src/Log.cpp
        ../src/Log.h
        ../src/util.cpp
        ../src/util.h
        ../src/Singleton.h
        ../src/Config.cpp
        ../src/Config.h
)


include_directories(../src ${BOOST_PATH}/include ${USR_INCLUDE_PATH})

link_directories(${BOOST_PATH}/lib ${USR_LIB_PATH}/yaml-cpp)

add_library(LogLib SHARED ${LIB_SRC})

#[[日志测试]]
#add_executable(TestLog
#        ../test/test_log.cpp
#)

#[[日志Config测试]]
add_executable(TestConfigLog
        ../test/testConfigLog.cpp
)

target_link_libraries(TestConfigLog LogLib yaml-cpp)

