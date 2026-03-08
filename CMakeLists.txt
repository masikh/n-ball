cmake_minimum_required(VERSION 3.20)
project(circles)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(circles main.cpp)
target_link_libraries(circles glfw OpenGL::GL)
