##############################################################################
# Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.
# Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE
# AGREEMENT). 
##############################################################################
# USER PROJECT BUILD DEFINITION.
##############################################################################

#####################################################################
### set TWELITE model
TWELITE ?= BLUE
#TWELITE = RED

#####################################################################
### set application version (MUST SET THIS.)
VERSION_MAIN = 0
VERSION_SUB  = 1
VERSION_VAR  = 0

#####################################################################
### set an additional source file
###   the default file name is dirname.

### for C++ files compiled with g++ (must have .cpp suffix)
#APPSRC_CXX += my_cpp_file.cpp

### for C files compiled with gcc (must have .c suffix)
#APPSRC += my_c_file.c

### Additional Src/Include Path
# if set, find source files from given dirs.
#
#APP_COMMON_SRC_DIR_ADD1 = ./src_dir
#APP_COMMON_SRC_DIR_ADD2 = ../my_common_dir
#APP_COMMON_SRC_DIR_ADD3 = 
#APP_COMMON_SRC_DIR_ADD4 =

#####################################################################
### set misc option for compiler

### C++ flags passed to g++
# e.g. CXXFLAGS += -DMY_DEFS
#CXXFLAGS +=

### C++/C flags passed to g++/gcc
# e.g. CFLAGS += -DMY_DEFS
#CFLAGS +=

### include opts
# e.g. INCFLAGS += -I../my_common_src/
#INCFLAGS +=

### optimize flag (default is -Os, normally no need to change)
#OPTFLAG=-O2

#####################################################################
### must include mwx.mk (the makefile body part.)
MWSDK_PATH?=$(realpath $(MWSDK_ROOT))
include $(MWSDK_PATH)/MkFiles/mwx.mk
#####################################################################
