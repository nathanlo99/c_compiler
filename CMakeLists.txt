cmake_minimum_required(VERSION 3.10.2)
project(compiler_cpp)

set(CMAKE_CXX_STANDARD 20)
include_directories(src)

file(GLOB src_cpp src/*.cpp)
list(APPEND SOURCES ${src_cpp})

add_executable(compiler_cpp ${SOURCES})

set_target_properties(
  compiler_cpp
  PROPERTIES
  COMPILE_FLAGS "-O3 -Wno-error -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused"
)
