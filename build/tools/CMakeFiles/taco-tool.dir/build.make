# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.21

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
CMAKE_COMMAND = /home/xwb/cmake-3.21.4-linux-x86_64/bin/cmake

# The command to remove a file.
RM = /home/xwb/cmake-3.21.4-linux-x86_64/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/xwb/transpose/taco

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/xwb/transpose/taco/build

# Include any dependencies generated for this target.
include tools/CMakeFiles/taco-tool.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include tools/CMakeFiles/taco-tool.dir/compiler_depend.make

# Include the progress variables for this target.
include tools/CMakeFiles/taco-tool.dir/progress.make

# Include the compile flags for this target's objects.
include tools/CMakeFiles/taco-tool.dir/flags.make

tools/CMakeFiles/taco-tool.dir/taco.cpp.o: tools/CMakeFiles/taco-tool.dir/flags.make
tools/CMakeFiles/taco-tool.dir/taco.cpp.o: ../tools/taco.cpp
tools/CMakeFiles/taco-tool.dir/taco.cpp.o: tools/CMakeFiles/taco-tool.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/xwb/transpose/taco/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tools/CMakeFiles/taco-tool.dir/taco.cpp.o"
	cd /home/xwb/transpose/taco/build/tools && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT tools/CMakeFiles/taco-tool.dir/taco.cpp.o -MF CMakeFiles/taco-tool.dir/taco.cpp.o.d -o CMakeFiles/taco-tool.dir/taco.cpp.o -c /home/xwb/transpose/taco/tools/taco.cpp

tools/CMakeFiles/taco-tool.dir/taco.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/taco-tool.dir/taco.cpp.i"
	cd /home/xwb/transpose/taco/build/tools && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/xwb/transpose/taco/tools/taco.cpp > CMakeFiles/taco-tool.dir/taco.cpp.i

tools/CMakeFiles/taco-tool.dir/taco.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/taco-tool.dir/taco.cpp.s"
	cd /home/xwb/transpose/taco/build/tools && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/xwb/transpose/taco/tools/taco.cpp -o CMakeFiles/taco-tool.dir/taco.cpp.s

# Object files for target taco-tool
taco__tool_OBJECTS = \
"CMakeFiles/taco-tool.dir/taco.cpp.o"

# External object files for target taco-tool
taco__tool_EXTERNAL_OBJECTS =

bin/taco: tools/CMakeFiles/taco-tool.dir/taco.cpp.o
bin/taco: tools/CMakeFiles/taco-tool.dir/build.make
bin/taco: lib/libtaco.so
bin/taco: tools/CMakeFiles/taco-tool.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/xwb/transpose/taco/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../bin/taco"
	cd /home/xwb/transpose/taco/build/tools && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/taco-tool.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tools/CMakeFiles/taco-tool.dir/build: bin/taco
.PHONY : tools/CMakeFiles/taco-tool.dir/build

tools/CMakeFiles/taco-tool.dir/clean:
	cd /home/xwb/transpose/taco/build/tools && $(CMAKE_COMMAND) -P CMakeFiles/taco-tool.dir/cmake_clean.cmake
.PHONY : tools/CMakeFiles/taco-tool.dir/clean

tools/CMakeFiles/taco-tool.dir/depend:
	cd /home/xwb/transpose/taco/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/xwb/transpose/taco /home/xwb/transpose/taco/tools /home/xwb/transpose/taco/build /home/xwb/transpose/taco/build/tools /home/xwb/transpose/taco/build/tools/CMakeFiles/taco-tool.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : tools/CMakeFiles/taco-tool.dir/depend
