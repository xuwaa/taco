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
include apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/compiler_depend.make

# Include the progress variables for this target.
include apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/progress.make

# Include the compile flags for this target's objects.
include apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/flags.make

apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o: apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/flags.make
apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o: ../apps/tensor_5_fusion/tensor_transpose.cpp
apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o: apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/xwb/transpose/taco/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o"
	cd /home/xwb/transpose/taco/build/apps/tensor_5_fusion && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o -MF CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o.d -o CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o -c /home/xwb/transpose/taco/apps/tensor_5_fusion/tensor_transpose.cpp

apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.i"
	cd /home/xwb/transpose/taco/build/apps/tensor_5_fusion && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/xwb/transpose/taco/apps/tensor_5_fusion/tensor_transpose.cpp > CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.i

apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.s"
	cd /home/xwb/transpose/taco/build/apps/tensor_5_fusion && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/xwb/transpose/taco/apps/tensor_5_fusion/tensor_transpose.cpp -o CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.s

# Object files for target tensor_5_fusion
tensor_5_fusion_OBJECTS = \
"CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o"

# External object files for target tensor_5_fusion
tensor_5_fusion_EXTERNAL_OBJECTS =

bin/taco-tensor_5_fusion: apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/tensor_transpose.cpp.o
bin/taco-tensor_5_fusion: apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/build.make
bin/taco-tensor_5_fusion: lib/libtaco.so
bin/taco-tensor_5_fusion: apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/xwb/transpose/taco/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ../../bin/taco-tensor_5_fusion"
	cd /home/xwb/transpose/taco/build/apps/tensor_5_fusion && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tensor_5_fusion.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/build: bin/taco-tensor_5_fusion
.PHONY : apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/build

apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/clean:
	cd /home/xwb/transpose/taco/build/apps/tensor_5_fusion && $(CMAKE_COMMAND) -P CMakeFiles/tensor_5_fusion.dir/cmake_clean.cmake
.PHONY : apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/clean

apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/depend:
	cd /home/xwb/transpose/taco/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/xwb/transpose/taco /home/xwb/transpose/taco/apps/tensor_5_fusion /home/xwb/transpose/taco/build /home/xwb/transpose/taco/build/apps/tensor_5_fusion /home/xwb/transpose/taco/build/apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : apps/tensor_5_fusion/CMakeFiles/tensor_5_fusion.dir/depend

