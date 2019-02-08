cmake_minimum_required(VERSION 2.8)

project(DWARFtest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra")

add_executable(DWARFtest main.cpp)

target_link_libraries(DWARFtest libdwarf)