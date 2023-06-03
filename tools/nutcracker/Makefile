.SUFFIXES:

# lazy
TARGET := nutcracker
SOURCES := $(wildcard src/*.cc)
HEADERS :=

CXX ?= g++
CXXFLAGS := -Og -g -std=c++17

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: clean
