cmake_minimum_required(VERSION 2.8)

project(roughserver)

set(cpp 
    Channel.cpp 
    Condition.cpp 
    CountDownLatch.cpp 
    Epoller.cpp 
    EventLoop.cpp 
    EventLoopThread.cpp 
    EventLoopThreadPool.cpp 
    HTTPData.cpp 
    Main.cpp 
    Server.cpp
    Socket.cpp
    Thread.cpp
    Timer.cpp)

    if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
    -lpthread
)


set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O0")

#string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")


#string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_executable(roughserver ${cpp})
