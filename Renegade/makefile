# -----------------------------------------------------------------------------
# Makefile for compiling Renegade
# -----------------------------------------------------------------------------

TARGET    := Renegade
_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
CXX       := clang++
CXXFLAGS  := -std=c++20 -O3 -flto -funroll-loops -fno-exceptions -Wall -Wextra
NATIVE    := -march=native -mtune=native
SOURCES   := $(filter-out Datagen.cpp Classical.cpp, $(wildcard *.cpp))


# System dependent flags -------------------------------------------------------

ifeq ($(OS), Windows_NT)
	SYSTEM_OS := Windows
	SUFFIX := .exe
	CXXFLAGS += -static
else
	SYSTEM_OS := $(shell uname -s)
	SUFFIX :=
	CXXFLAGS += -pthread
	LDFLAGS := -pthread
endif

# Use LLD for Clang on non-macOS systems
CXX_VERSION := $(shell $(CXX) --version)
ifneq ($(SYSTEM_OS), Darwin)
	ifneq (, $(findstring clang, $(CXX_VERSION)))
		LDFLAGS += -fuse-ld=lld
	endif
endif


# Specific builds -------------------------------------------------------------
# (as of 2025, the default release build is bmi2)

ifeq ($(build), debug)
	CXXFLAGS = -std=c++20 -Og -g3 -fno-omit-frame-pointer
	ifeq ($(SYSTEM_OS), Windows)
		CXXFLAGS += -fsanitize=undefined,address
	else
		CXXFLAGS += -fsanitize=undefined,address,leak
	endif
endif

ifeq ($(build), datagen)
	CXXFLAGS += -DRENEGADE_DATAGEN
	SOURCES  += Datagen.cpp
endif

ifeq ($(build), x86-64)
	NATIVE   = -msse -msse2 -mtune=sandybridge
endif

ifeq ($(build), x86-64-bmi2)
	NATIVE   = -march=haswell -mtune=haswell
endif


# Running build commands ------------------------------------------------------
# (Engine.cpp is always recompiled to include the correct date and time) 

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
	$(CXX) $(CXXFLAGS) $(NATIVE) -MMD -MP -c $< -o $@

-include $(DEPENDS)

Engine.o: FORCE
FORCE: