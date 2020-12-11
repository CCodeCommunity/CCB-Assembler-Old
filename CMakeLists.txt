cmake_minimum_required(VERSION 3.17)
project(CCB_Assembler C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(CCB_Assembler
        assembler.h
        main.c)
