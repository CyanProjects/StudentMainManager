cmake_minimum_required(VERSION 3.23)
project(StudentMainManager)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O2")

set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(StudentMainManager main.cpp)
