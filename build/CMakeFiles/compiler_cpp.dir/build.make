# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.26

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.26.1/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.26.1/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/admin/dev/compiler_cpp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/admin/dev/compiler_cpp/build

# Include any dependencies generated for this target.
include CMakeFiles/compiler_cpp.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/compiler_cpp.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/compiler_cpp.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/compiler_cpp.dir/flags.make

CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o: CMakeFiles/compiler_cpp.dir/flags.make
CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o: /Users/admin/dev/compiler_cpp/src/ast_node.cpp
CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o: CMakeFiles/compiler_cpp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/admin/dev/compiler_cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o -MF CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o.d -o CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o -c /Users/admin/dev/compiler_cpp/src/ast_node.cpp

CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/admin/dev/compiler_cpp/src/ast_node.cpp > CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.i

CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/admin/dev/compiler_cpp/src/ast_node.cpp -o CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.s

CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o: CMakeFiles/compiler_cpp.dir/flags.make
CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o: /Users/admin/dev/compiler_cpp/src/lexer.cpp
CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o: CMakeFiles/compiler_cpp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/admin/dev/compiler_cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o -MF CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o.d -o CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o -c /Users/admin/dev/compiler_cpp/src/lexer.cpp

CMakeFiles/compiler_cpp.dir/src/lexer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/compiler_cpp.dir/src/lexer.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/admin/dev/compiler_cpp/src/lexer.cpp > CMakeFiles/compiler_cpp.dir/src/lexer.cpp.i

CMakeFiles/compiler_cpp.dir/src/lexer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/compiler_cpp.dir/src/lexer.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/admin/dev/compiler_cpp/src/lexer.cpp -o CMakeFiles/compiler_cpp.dir/src/lexer.cpp.s

CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o: CMakeFiles/compiler_cpp.dir/flags.make
CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o: /Users/admin/dev/compiler_cpp/src/parse_node.cpp
CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o: CMakeFiles/compiler_cpp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/admin/dev/compiler_cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o -MF CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o.d -o CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o -c /Users/admin/dev/compiler_cpp/src/parse_node.cpp

CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/admin/dev/compiler_cpp/src/parse_node.cpp > CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.i

CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/admin/dev/compiler_cpp/src/parse_node.cpp -o CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.s

CMakeFiles/compiler_cpp.dir/src/parser.cpp.o: CMakeFiles/compiler_cpp.dir/flags.make
CMakeFiles/compiler_cpp.dir/src/parser.cpp.o: /Users/admin/dev/compiler_cpp/src/parser.cpp
CMakeFiles/compiler_cpp.dir/src/parser.cpp.o: CMakeFiles/compiler_cpp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/admin/dev/compiler_cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/compiler_cpp.dir/src/parser.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/compiler_cpp.dir/src/parser.cpp.o -MF CMakeFiles/compiler_cpp.dir/src/parser.cpp.o.d -o CMakeFiles/compiler_cpp.dir/src/parser.cpp.o -c /Users/admin/dev/compiler_cpp/src/parser.cpp

CMakeFiles/compiler_cpp.dir/src/parser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/compiler_cpp.dir/src/parser.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/admin/dev/compiler_cpp/src/parser.cpp > CMakeFiles/compiler_cpp.dir/src/parser.cpp.i

CMakeFiles/compiler_cpp.dir/src/parser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/compiler_cpp.dir/src/parser.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/admin/dev/compiler_cpp/src/parser.cpp -o CMakeFiles/compiler_cpp.dir/src/parser.cpp.s

CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o: CMakeFiles/compiler_cpp.dir/flags.make
CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o: /Users/admin/dev/compiler_cpp/src/wlp.cpp
CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o: CMakeFiles/compiler_cpp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/admin/dev/compiler_cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o -MF CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o.d -o CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o -c /Users/admin/dev/compiler_cpp/src/wlp.cpp

CMakeFiles/compiler_cpp.dir/src/wlp.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/compiler_cpp.dir/src/wlp.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/admin/dev/compiler_cpp/src/wlp.cpp > CMakeFiles/compiler_cpp.dir/src/wlp.cpp.i

CMakeFiles/compiler_cpp.dir/src/wlp.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/compiler_cpp.dir/src/wlp.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/admin/dev/compiler_cpp/src/wlp.cpp -o CMakeFiles/compiler_cpp.dir/src/wlp.cpp.s

# Object files for target compiler_cpp
compiler_cpp_OBJECTS = \
"CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o" \
"CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o" \
"CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o" \
"CMakeFiles/compiler_cpp.dir/src/parser.cpp.o" \
"CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o"

# External object files for target compiler_cpp
compiler_cpp_EXTERNAL_OBJECTS =

compiler_cpp: CMakeFiles/compiler_cpp.dir/src/ast_node.cpp.o
compiler_cpp: CMakeFiles/compiler_cpp.dir/src/lexer.cpp.o
compiler_cpp: CMakeFiles/compiler_cpp.dir/src/parse_node.cpp.o
compiler_cpp: CMakeFiles/compiler_cpp.dir/src/parser.cpp.o
compiler_cpp: CMakeFiles/compiler_cpp.dir/src/wlp.cpp.o
compiler_cpp: CMakeFiles/compiler_cpp.dir/build.make
compiler_cpp: CMakeFiles/compiler_cpp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/admin/dev/compiler_cpp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Linking CXX executable compiler_cpp"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/compiler_cpp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/compiler_cpp.dir/build: compiler_cpp
.PHONY : CMakeFiles/compiler_cpp.dir/build

CMakeFiles/compiler_cpp.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/compiler_cpp.dir/cmake_clean.cmake
.PHONY : CMakeFiles/compiler_cpp.dir/clean

CMakeFiles/compiler_cpp.dir/depend:
	cd /Users/admin/dev/compiler_cpp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/admin/dev/compiler_cpp /Users/admin/dev/compiler_cpp /Users/admin/dev/compiler_cpp/build /Users/admin/dev/compiler_cpp/build /Users/admin/dev/compiler_cpp/build/CMakeFiles/compiler_cpp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/compiler_cpp.dir/depend

