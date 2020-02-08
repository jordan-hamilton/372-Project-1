cmake_minimum_required(VERSION 2.8)
project(chatclient)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(chatclient
        chatclient.c)
