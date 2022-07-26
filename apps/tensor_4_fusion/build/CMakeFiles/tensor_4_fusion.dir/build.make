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
CMAKE_SOURCE_DIR = /home/xwb/taco/transpose/taco/apps/tensor_4_fusion

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/xwb/taco/transpose/taco/apps/tensor_4_fusion/build

# Include any dependencies generated for this target.
include CMakeFiles/tensor_4_fusion.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/tensor_4_fusion.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/tensor_4_fusion.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/tensor_4_fusion.dir/flags.make

CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o: CMakeFiles/tensor_4_fusion.dir/flags.make
CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o: ../tensor_transpose.cpp
CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o: CMakeFiles/tensor_4_fusion.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/xwb/taco/transpose/taco/apps/tensor_4_fusion/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o -MF CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o.d -o CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o -c /home/xwb/taco/transpose/taco/apps/tensor_4_fusion/tensor_transpose.cpp

CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/xwb/taco/transpose/taco/apps/tensor_4_fusion/tensor_transpose.cpp > CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.i

CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/xwb/taco/transpose/taco/apps/tensor_4_fusion/tensor_transpose.cpp -o CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.s

# Object files for target tensor_4_fusion
tensor_4_fusion_OBJECTS = \
"CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o"

# External object files for target tensor_4_fusion
tensor_4_fusion_EXTERNAL_OBJECTS =

tensor_4_fusion: CMakeFiles/tensor_4_fusion.dir/tensor_transpose.cpp.o
tensor_4_fusion: CMakeFiles/tensor_4_fusion.dir/build.make
tensor_4_fusion: /home/xwb/cutt/lib/libcutt.so
tensor_4_fusion: /usr/local/cuda-10.1/lib64/libcudnn.so
tensor_4_fusion: /usr/local/cuda-10.1/lib64/libcudart.so
tensor_4_fusion: /home/xwb/taco/transpose/taco/build/lib/libtaco.so
tensor_4_fusion: CMakeFiles/tensor_4_fusion.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/xwb/taco/transpose/taco/apps/tensor_4_fusion/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable tensor_4_fusion"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tensor_4_fusion.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/tensor_4_fusion.dir/build: tensor_4_fusion
.PHONY : CMakeFiles/tensor_4_fusion.dir/build

CMakeFiles/tensor_4_fusion.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/tensor_4_fusion.dir/cmake_clean.cmake
.PHONY : CMakeFiles/tensor_4_fusion.dir/clean

CMakeFiles/tensor_4_fusion.dir/depend:
	cd /home/xwb/taco/transpose/taco/apps/tensor_4_fusion/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/xwb/taco/transpose/taco/apps/tensor_4_fusion /home/xwb/taco/transpose/taco/apps/tensor_4_fusion /home/xwb/taco/transpose/taco/apps/tensor_4_fusion/build /home/xwb/taco/transpose/taco/apps/tensor_4_fusion/build /home/xwb/taco/transpose/taco/apps/tensor_4_fusion/build/CMakeFiles/tensor_4_fusion.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/tensor_4_fusion.dir/depend

