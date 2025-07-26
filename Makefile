
SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
.ONESHELL:
.DELETE_ON_ERROR:

TARGET_ARCH = -march=x86-64
CXX = clang++
CXXFLAGS =  -Wall -Wextra
CXXFLAGS +=  -std=c++2c -fno-exceptions -fno-rtti
CXXFLAGS +=  -g
LDFLAGS = -luring 
CPPFLAGS = -I./src

.PHONY: all
all:: build/io build/task

.PRECIOUS: %/.
%/.:
	mkdir -p "$(@D)"

run_%: build/%
	"$<"

test_%: build/test_%
	@echo "----------" "Running test: $<" 
	"$<"
	echo  "----------" "Finished test: $<"
	
-include $(patsubst src/%.cc,build/%.d,$(wildcard src/*.cc))

build/%.o: src/%.cc | build/.
	$(COMPILE.cc) -MMD -MP -MF build/$*.d -o $@ -c $< 

# build/%: build/%.o | build/.
# 	$(LINK.o) $(LOADLIBES) $(LDLIBS)  -o $@ $^

.PHONY: clean
clean::
	rm -rf build

.PHONY: run
run:: 


.PHONY: test
#test:: clean
test::
	reset

build/io: build/io.o | build/.
	$(CXX) $(LDFLAGS) $< -o $@ 

build/task: build/main.o | build/.
	$(CXX) $(LDFLAGS) $< -o $@ 

build/test_dlist: build/test_dlist.o | build/.
	$(CXX) $(LDFLAGS) $< -o $@ 
test:: test_dlist

build/test_task: build/test_task.o | build/.
	$(CXX) $(LDFLAGS) $< -o $@ 
test:: test_task
