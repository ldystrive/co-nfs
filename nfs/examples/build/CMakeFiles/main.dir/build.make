# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

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
CMAKE_SOURCE_DIR = /home/ldy/code/co-nfs/nfs/examples

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ldy/code/co-nfs/nfs/examples/build

# Include any dependencies generated for this target.
include CMakeFiles/main.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/main.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/main.dir/flags.make

CMakeFiles/main.dir/nfsclient-async.c.o: CMakeFiles/main.dir/flags.make
CMakeFiles/main.dir/nfsclient-async.c.o: ../nfsclient-async.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ldy/code/co-nfs/nfs/examples/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/main.dir/nfsclient-async.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/main.dir/nfsclient-async.c.o   -c /home/ldy/code/co-nfs/nfs/examples/nfsclient-async.c

CMakeFiles/main.dir/nfsclient-async.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/main.dir/nfsclient-async.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ldy/code/co-nfs/nfs/examples/nfsclient-async.c > CMakeFiles/main.dir/nfsclient-async.c.i

CMakeFiles/main.dir/nfsclient-async.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/main.dir/nfsclient-async.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ldy/code/co-nfs/nfs/examples/nfsclient-async.c -o CMakeFiles/main.dir/nfsclient-async.c.s

CMakeFiles/main.dir/nfsclient-async.c.o.requires:

.PHONY : CMakeFiles/main.dir/nfsclient-async.c.o.requires

CMakeFiles/main.dir/nfsclient-async.c.o.provides: CMakeFiles/main.dir/nfsclient-async.c.o.requires
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/nfsclient-async.c.o.provides.build
.PHONY : CMakeFiles/main.dir/nfsclient-async.c.o.provides

CMakeFiles/main.dir/nfsclient-async.c.o.provides.build: CMakeFiles/main.dir/nfsclient-async.c.o


# Object files for target main
main_OBJECTS = \
"CMakeFiles/main.dir/nfsclient-async.c.o"

# External object files for target main
main_EXTERNAL_OBJECTS =

main: CMakeFiles/main.dir/nfsclient-async.c.o
main: CMakeFiles/main.dir/build.make
main: CMakeFiles/main.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ldy/code/co-nfs/nfs/examples/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable main"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/main.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/main.dir/build: main

.PHONY : CMakeFiles/main.dir/build

CMakeFiles/main.dir/requires: CMakeFiles/main.dir/nfsclient-async.c.o.requires

.PHONY : CMakeFiles/main.dir/requires

CMakeFiles/main.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/main.dir/cmake_clean.cmake
.PHONY : CMakeFiles/main.dir/clean

CMakeFiles/main.dir/depend:
	cd /home/ldy/code/co-nfs/nfs/examples/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ldy/code/co-nfs/nfs/examples /home/ldy/code/co-nfs/nfs/examples /home/ldy/code/co-nfs/nfs/examples/build /home/ldy/code/co-nfs/nfs/examples/build /home/ldy/code/co-nfs/nfs/examples/build/CMakeFiles/main.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/main.dir/depend

