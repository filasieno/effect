CXX = clang++
#CXXFLAGS =  -Wall -Wextra
CXXFLAGS =  -std=c++2c -fno-exceptions -fno-rtti
LDFLAGS = -luring 

.PHONY: all
all:: build/io

%/.:
	mkdir -p $(@D)

.PHONY: run
run:: build/io
	reset
	build/io

build/io: build/io.o | build/.
	$(CXX) $< -o $@ $(LDFLAGS)

build/io.o:  src/io.cc | build/.
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean::
	rm -rf build

