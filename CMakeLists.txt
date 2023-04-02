cmake_minimum_required(VERSION 3.10.2)
project(compiler_cpp)

set(CMAKE_CXX_STANDARD 20)
include_directories(src src/00_scanner src/01_parser src/02_ast_generation src/03_ast_optimization src/04_code_generation)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_FLAGS "-Ofast -g -fsanitize=address,undefined -Wall -Wextra -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused")
set(CMAKE_EXE_LINKER_FLAGS "-Ofast -g -fsanitize=address,undefined -flto -ffast-math")

file(GLOB src_cpp src/*.cpp src/00_scanner/*.cpp src/01_parser/*.cpp src/02_ast_generation/*.cpp src/03_ast_optimization/*.cpp src/04_code_generation/*.cpp)
list(APPEND SOURCES ${src_cpp})

add_executable(compiler_cpp ${SOURCES})
