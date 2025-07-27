
SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
.ONESHELL:
.DELETE_ON_ERROR:

TARGET_ARCH = -march=x86-64
CXX = clang++
CXXFLAGS = -Wall -Wextra
CXXFLAGS += -std=c++2c -fno-exceptions -fno-rtti
CXXFLAGS += -g
LDFLAGS =
LDLIBS = -luring 
CPPFLAGS = -I./src
CC := $(CXX)

.PHONY: all
all:: build/io build/task

.PRECIOUS: %/.
%/.:
	mkdir -p "$(@D)"

run_%: build/%
	"$<"

test_%: build/test_%
	@printf -- '\e[33m---------- Running test:\e[0m %s\n' '$<'
	"$<"
	printf --  '\e[33m---------- Finished test:\e[0m %s\n' '$<'

.PRECIOUS: build/%.o
build/%.o: src/%.cc | build/.
	$(COMPILE.cc) -MMD -MP -MF build/$*.d -o $@ -c $< 

-include $(patsubst src/%.cc,build/%.d,$(wildcard src/*.cc))

.PRECIOUS: build/%
build/%: build/%.o  | build/.
#	$(LINK.o) $(LDLIBS) -o $@ $^
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) $(LDLIBS) -o $@ $^


.PHONY: clean
clean::
	rm -rf build

.PHONY: run
run:: 

.PHONY: test
#test:: clean
test::
	reset

.PHONY: watch
watch: build/test_dlist build/test_task
	@printf -- '\e[33m---------- Watching for changes...\e[0m\n'
	inotifywait -qmr -e close_write,delete,move ./src | while read -r event; do
		reset
		printf -- '\e[33m---------- Detected change:\e[0m %s\n' "$$event"
		while read -r -t 1.0 debounce_event; do :; done
		make  $^ || true
	done



#----------------------------------------

build/io: build/io.o

build/task: build/main.o

test:: test_dlist
test:: test_task
