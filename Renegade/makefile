# Makefile for compiling Renegade
# Might have some issues, hopefully it works well enough

TARGET    := Renegade
_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
CXX       := clang++
CXXFLAGS  :=  -Wall -Wcast-qual -fno-exceptions -std=c++20 -pedantic -Wextra -Wdouble-promotion -Wformat=2 -Wnull-dereference -Wunused -Wundef -DNDEBUG -fpermissive
NATIVE     = -march=native
# added -fpermissive for the time being

NETFLAGS :=
ifdef EVALFILE
	NETFLAGS = -DNETWORK_NAME=\"$(EVALFILE)\"
endif

$(warning Compiling via makefile is experimental and may have bugs)


# System dependent flags ----------------------------------

# Windows
ifeq ($(OS), Windows_NT)
	uname_S := Windows
	SUFFIX  := .exe

	ifeq ($(CXX),g++)
  		FLAGS    = -pthread -lstdc++ -static
		CXXFLAGS += -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	else ifeq ($(CXX),clang++)
  		CXXFLAGS += -static -fuse-ld=lld
	endif
	
else

	SUFFIX  :=
	uname_S := $(shell uname -s)

	ifeq ($(CXX),g++)
		FLAGS   = -lpthread -lstdc++
	else ifeq ($(CXX),clang++)
  		CXXFLAGS += -static
		# -fuse-ld=lld here?
	endif

endif

# macOS
ifeq ($(uname_S), Darwin)
	NATIVE = -mcpu=apple-a14	
	FLAGS = 
endif


# Specific builds -----------------------------------------

# Remove native for builds
ifdef build
	NATIVE =
endif

ifeq ($(build), native)
	NATIVE   = -march=native
	ARCH     = -x86-64-native
	CXXFLAGS += -w
endif

ifeq ($(build), profile)
	NATIVE   = -march=native
	ARCH     = -x86-64-native
	CXXFLAGS += -w -pg
endif

ifeq ($(build), x86-64)
	NATIVE       = -mtune=znver1
	INSTRUCTIONS = -msse -msse2 -mpopcnt
	ARCH         = -x86-64
endif

# This is the default target for release
# Note: Zen 2 and earlier have very slow pdep & pext
ifeq ($(build), x86-64-bmi2)
	NATIVE       = -march=haswell
	INSTRUCTIONS =
	ARCH         = -x86-64-bmi2
endif

ifeq ($(build), debug)
	CXXFLAGS = -g3 -fno-omit-frame-pointer -std=c++20 -fsanitize=address
	NATIVE   = -msse -msse3 -mpopcnt -mavx -mavx2
	FLAGS    = -lpthread -lstdc++
endif


# Commands ------------------------------------------------

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))
EXE     := $(TARGET)$(SUFFIX)

all: $(TARGET)

clean:
	rm -rf *.o $(DEPENDS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(EXE) $^ $(CXXFLAGS) ${NETFLAGS} $(NATIVE) $(INSTRUCTIONS) $(FLAGS) -flto

-include $(DEPENDS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) ${NETFLAGS} $(NATIVE) $(INSTRUCTIONS) -funroll-loops -O3 -flto -MMD -MP -c -o $@ $< $(FLAGS) -w