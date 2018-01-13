
# Include a local Makefile, if one exists
-include Makefile.local

# C++ compiler
CXX ?= g++

# C++ flags for release and debug builds
CXXFLAGS.release ?= -g -O3 -march=native -DNDEBUG
CXXFLAGS.debug   ?= -g -march=native

CXXFLAGS = -std=c++11 $(CXXFLAGS.$(BUILD))
LIBS =


# Rules
#

debug:	BUILD = debug
debug:	all

release:	BUILD = release
release:	all

all:		test.exe

test.exe:	src/*.cpp src/*.h
			$(CXX) $(CXXFLAGS) -Isrc -o test.exe src/unity_build.cpp $(LIBS)

.PHONY: 	release debug all
