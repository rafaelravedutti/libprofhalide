# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.7

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build

# Utility rule file for local_laplacian_auto_schedule_lib_gen.

# Include the progress variables for this target.
include CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/progress.make

CMakeFiles/local_laplacian_auto_schedule_lib_gen: genfiles/local_laplacian_auto_schedule/local_laplacian_auto_schedule.a
CMakeFiles/local_laplacian_auto_schedule_lib_gen: genfiles/local_laplacian_auto_schedule/local_laplacian_auto_schedule.h


genfiles/local_laplacian_auto_schedule/local_laplacian_auto_schedule.a: local_laplacian.generator_binary
	/usr/bin/cmake -E echo Running /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build/local_laplacian.generator_binary -e static_library,h -o /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build/./genfiles/local_laplacian_auto_schedule -g local_laplacian -f local_laplacian_auto_schedule -x .s=.s.txt,.cpp=.generated.cpp target=host-no_runtime auto_schedule=true
	/usr/bin/cmake -E env ASAN_OPTIONS=detect_leaks=0 /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build/local_laplacian.generator_binary -e static_library,h -o /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build/./genfiles/local_laplacian_auto_schedule -g local_laplacian -f local_laplacian_auto_schedule -x .s=.s.txt,.cpp=.generated.cpp target=host-no_runtime auto_schedule=true

genfiles/local_laplacian_auto_schedule/local_laplacian_auto_schedule.h: genfiles/local_laplacian_auto_schedule/local_laplacian_auto_schedule.a
	@$(CMAKE_COMMAND) -E touch_nocreate genfiles/local_laplacian_auto_schedule/local_laplacian_auto_schedule.h

local_laplacian_auto_schedule_lib_gen: CMakeFiles/local_laplacian_auto_schedule_lib_gen
local_laplacian_auto_schedule_lib_gen: genfiles/local_laplacian_auto_schedule/local_laplacian_auto_schedule.a
local_laplacian_auto_schedule_lib_gen: genfiles/local_laplacian_auto_schedule/local_laplacian_auto_schedule.h
local_laplacian_auto_schedule_lib_gen: CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/build.make

.PHONY : local_laplacian_auto_schedule_lib_gen

# Rule to build all files generated by this target.
CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/build: local_laplacian_auto_schedule_lib_gen

.PHONY : CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/build

CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/cmake_clean.cmake
.PHONY : CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/clean

CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/depend:
	cd /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build /home/rafael/hdd/repositories/macalan/libpapihalide/tests/local_laplacian/build/CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/local_laplacian_auto_schedule_lib_gen.dir/depend

