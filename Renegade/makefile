# -----------------------------------------------------------------------------
# Makefile for compiling Renegade
# -----------------------------------------------------------------------------

TARGET    := Renegade
_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
CXX       := clang++
CXXFLAGS  := -std=c++20 -O3 -flto -funroll-loops -fno-exceptions -DNDEBUG -Wall -Wextra
NATIVE    := -march=native -mtune=native


# System dependent flags -------------------------------------------------------

ifeq ($(OS), Windows_NT)
	SYSTEM_OS := Windows
	SUFFIX := .exe
	CXXFLAGS += -static
else
	SYSTEM_OS := $(shell uname -s)
	SUFFIX :=
	LDFLAGS := -pthread
endif

# Use LLD for Clang on non-macOS systems
CXX_VERSION := $(shell $(CXX) --version)
ifneq ($(DETECTED_OS), Darwin)
	ifneq (, $(findstring clang, $(CXX_VERSION)))
		LDFLAGS += -fuse-ld=lld
	endif
endif


# Specific builds -------------------------------------------------------------
# (as of 2025, the default release build is bmi2)

ifeq ($(build), native)
	NATIVE   = -march=native -mtune=native
endif

ifeq ($(build), debug)
	CXXFLAGS = -std=c++20 -O3 -g3 -fno-omit-frame-pointer -fsanitize=address -fsanitize=leak -fsanitize=undefined
	NATIVE   = -march=haswell -mtune=haswell
	LDFLAGS  = -pthread -lstdc++
endif

ifeq ($(build), x86-64)
	NATIVE   = -msse -msse2 -mtune=sandybridge
endif

ifeq ($(build), x86-64-bmi2)
	NATIVE   = -march=haswell -mtune=haswell
endif


# Running build commands ------------------------------------------------------
# (Engine.cpp is always recompiled to include the correct date and time) 

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))
EXE     := $(TARGET)

all: $(TARGET)

clean:
ifneq ($(SYSTEM_OS), Windows)
	@rm -rf *.o *.d
else
	@del *.o *.d *.exe
endif

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(NATIVE) -MMD -MP -o $(EXE)$(SUFFIX) $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(NATIVE) -MMD -MP -c $< -o $@ $(LDFLAGS)

-include $(DEPENDS)

Engine.o: FORCE
FORCE: