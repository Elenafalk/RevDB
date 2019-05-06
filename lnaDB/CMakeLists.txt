cmake_minimum_required(VERSION 2.8)

project(lnaDB)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -ldwarf -lelf")

add_executable(lnaDB main.cpp debugger.cpp breakpoint.cpp)