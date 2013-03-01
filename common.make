SRCROOT     := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
HOST_ARCH    = $(shell uname -m | sed s,i[3456789]86,ia32,)
TARGET_ARCH := $(shell uname -m | sed s,i[3456789]86,ia32,)
HOST_OS      = $(shell uname -s)

# Dependencies
DEPS_ROOT           := $(SRCROOT)/deps
DEPS_PREFIX         := $(DEPS_ROOT)/build
DEPS_INCLUDEDIR     := $(DEPS_PREFIX)/include
DEPS_LIBDIR         := $(DEPS_PREFIX)/lib

# Build target type
ifeq ($(strip $(DEBUG)),1)
	BUILD_TARGET_TYPE = debug
else
	BUILD_TARGET_TYPE = release
endif

# Build directories
BASE_BUILD_PREFIX     := $(SRCROOT)/build
BUILD_PREFIX          := $(BASE_BUILD_PREFIX)/$(BUILD_TARGET_TYPE)
TESTS_BUILD_PREFIX    := $(BASE_BUILD_PREFIX)/test
INCLUDE_BUILD_PREFIX  := $(BUILD_PREFIX)/include
LIB_BUILD_PREFIX      := $(BUILD_PREFIX)/lib
BIN_BUILD_PREFIX      := $(BUILD_PREFIX)/bin
DEBUG_BUILD_PREFIX    := $(BUILD_PREFIX)/debug

OBJECT_DIR := $(BUILD_PREFIX)/.objs

# Tools
#CC = clang
#CXXC = clang++
CC = cc
CXXC = c++
LD = clang
AR = ar

# $(call UpperCase,foo_bar) -> FOO_BAR
UpperCase = $(shell echo $(1) | tr a-z A-Z)

# Compiler and Linker flags for all targets
CFLAGS    += -Wall -std=c11 -I$(INCLUDE_BUILD_PREFIX) -I$(DEPS_INCLUDEDIR) -arch $(TARGET_ARCH) -fno-common
CXXFLAGS  += -std=c++11 -stdlib=libc++ -fno-rtti -fno-exceptions -fvisibility-inlines-hidden
LDFLAGS   += -arch $(TARGET_ARCH) -L"$(LIB_BUILD_PREFIX)" -L"$(DEPS_LIBDIR)"
XXLDFLAGS += -lc++ #-lstdc++

# Compiler and Linker flags for release and debug targets (e.g. make DEBUG=1)
ifeq ($(strip $(DEBUG)),1)
	CFLAGS += -O0 -g -DLUM_DEBUG=1
	#LDFLAGS +=
else
	CFLAGS += -O3 -DLUM_DEBUG=0 -DNDEBUG
	#LDFLAGS +=
endif

# Functions

# $(call PubHeaderNames,<header_pub_dir>,<list of header files>) -> <list of pub header files>
PubHeaderNames = $(patsubst %,$(1)/%,$(2))

# $(call FileDirs,<list of files>) -> <unique list of dirs>
FileDirs = $(sort $(foreach fn,$(1),$(dir $(fn))))

# $(call SrcToObjects,<objdir>,<list of source files>) -> <list of objects>
SrcToObjects = $(patsubst %,$(1)/%,$(2))
