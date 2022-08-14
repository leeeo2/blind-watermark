cmake_minimum_required(VERSION 3.10.0)
project(blind_water_mark)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} ./src/main.cpp)