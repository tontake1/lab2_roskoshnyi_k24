cmake_minimum_required(VERSION 3.20)
project(LabParallel CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lab_release main.cpp)

# Налаштування оптимізації для Release-режиму 
if(MSVC)
    target_compile_options(lab_release PRIVATE /O2)
else()
    target_compile_options(lab_release PRIVATE -O3 -pthread)
    target_link_options(lab_release PRIVATE -pthread)
endif()