cmake_minimum_required(VERSION 3.7)
project(task_4)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES server.c)
add_executable(task_4 ${SOURCE_FILES})