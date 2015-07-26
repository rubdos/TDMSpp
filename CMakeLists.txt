project(TDMSpp)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_COLOR_MAKEFILE ON)

include_directories(src)
add_subdirectory(src)

add_subdirectory(tests)
