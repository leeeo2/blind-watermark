cmake_minimum_required(VERSION 3.10.0)
project(blind_watermark_decoder)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    set(OpenCV_DIR C:/opencv4/opencv-mingw/build)
    find_package(OpenCV REQUIRED)
else()
    find_package(opencv REQUIRED)
endif()
    


include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ./src/decoder.cpp)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})