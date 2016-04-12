cmake_minimum_required(VERSION 3.5)
project(Network_Chatting)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES chat_client.c chat_server.c)
add_executable(Network_Chatting ${SOURCE_FILES})