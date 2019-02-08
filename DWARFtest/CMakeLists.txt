cmake_minimum_required(VERSION 2.8)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")

project(DWARFtest)

find_package(LibDwarf)
if(LibDwarf_FOUND)
  include_directories(${LibDwarf_INCLUDE_DIRS})

  add_executable(DWARFtest main.cpp)

  target_link_libraries(DWARFtest ${LibDwarf_LIBRARIES})
endif()

