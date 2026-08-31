cmake_minimum_required(VERSION 3.10.2)
project(compiler_cpp)

include(FetchContent)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG main
)
FetchContent_MakeAvailable(fmt)

set(CMAKE_CXX_STANDARD 20)
include_directories(src src/00_scanner src/01_parser src/02_ast_generation src/03_ast_optimization src/04_bril_generation src/05_bril_optimization src/06_mips_generation)

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=performance-*,clang-analyzer-*,cppcoreguidelines-*;-warnings-as-errors=*")
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# Default to Release when no -DCMAKE_BUILD_TYPE is given, so an existing
# `build/` directory (or a fresh one configured the old way) keeps behaving
# like it always has. Pass -DCMAKE_BUILD_TYPE=Debug for an ASan/UBSan build
# with debug_assert-friendly flags -- or just run ./build.sh, which
# configures and builds both `build/` (Release) and `build-debug/` (Debug).
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Flags common to every build type. -march=native: fine now that build.sh
# pins the compiler to this exact machine's LLVM -- no portability to give up.
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -Wno-error=nan-infinity-disabled -march=native")

# Release: what CMAKE_CXX_FLAGS used to hardcode, unconditionally -- same
# behavior as before this file started distinguishing build types.
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -g -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-O3 -ffast-math -g -flto")

# Debug: ASan + UBSan, no LTO (slow/flaky with sanitizers), debug_assert
# stays on either way (see util.hpp -- it never checked NDEBUG). Code can
# gate debug-only logging on `#ifdef DEBUG_BUILD`.
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g -fno-omit-frame-pointer -fsanitize=address,undefined -DDEBUG_BUILD")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-g -fsanitize=address,undefined")

file(GLOB src_cpp src/*.cpp src/00_scanner/*.cpp src/01_parser/*.cpp src/02_ast_generation/*.cpp src/03_ast_optimization/*.cpp src/04_bril_generation/*.cpp src/05_bril_optimization/*.cpp src/05_bril_optimization/data_flow/*.cpp src/06_mips_generation/*.cpp)
list(APPEND SOURCES ${src_cpp})

add_executable(compile ${SOURCES})
target_link_libraries(compile PRIVATE fmt::fmt-header-only)
