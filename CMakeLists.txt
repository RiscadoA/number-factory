cmake_minimum_required(VERSION 3.16)
project(jam
  VERSION 0.1.0
  LANGUAGES C
)

find_package(SDL3 REQUIRED)

add_executable(jam src/main.c)
target_link_libraries(jam PRIVATE SDL3::SDL3)
