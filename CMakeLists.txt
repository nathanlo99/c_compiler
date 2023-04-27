cmake_minimum_required(VERSION 3.10.2)
project(compiler_cpp)

include(FetchContent)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

set(CMAKE_CXX_STANDARD 20)
include_directories(src src/00_scanner src/01_parser src/02_ast_generation src/03_ast_optimization src/04_bril_generation src/05_bril_optimization src/06_mips_generation)

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=performance-*,clang-analyzer-*,cppcoreguidelines-*;-warnings-as-errors=*")
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_FLAGS "-Ofast -g -fsanitize=address,undefined -Werror -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "-Ofast -g -fsanitize=address,undefined -flto -ffast-math")

file(GLOB src_cpp src/*.cpp src/00_scanner/*.cpp src/01_parser/*.cpp src/02_ast_generation/*.cpp src/03_ast_optimization/*.cpp src/04_bril_generation/*.cpp src/05_bril_optimization/*.cpp src/05_bril_optimization/data_flow/*.cpp src/06_mips_generation/*.cpp)
list(APPEND SOURCES ${src_cpp})

add_executable(compile ${SOURCES})
target_link_libraries(compile PRIVATE fmt::fmt-header-only)
