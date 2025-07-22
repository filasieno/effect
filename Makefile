CXX = clang++
#CXXFLAGS =  -Wall -Wextra
CXXFLAGS =  -std=c++2c -fno-exceptions -fno-rtti
LDFLAGS = -luring 

.PHONY: all
all:: build/io

.PHONY: run
run:: build/io
	valgrind ./io

build/io: build/io.o
	$(CXX) $< -o $@ $(LDFLAGS)

build/io.o:  src/io.cc
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean::
	rm -rf build

