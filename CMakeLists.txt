cmake_minimum_required(VERSION 3.22.1)
project(Webserver)

set(CMAKE_CXX_STANDARD 20)

add_executable(Webserver
        server.cpp
)