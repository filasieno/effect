#!/usr/bin/make

SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --silent
MAKEFLAGS += --output-sync=target
.ONESHELL:
.DELETE_ON_ERROR:

TARGET_ARCH = -march=x86-64
CXX = clang++

CXXFLAGS = -Wall -Wextra
CXXFLAGS += -std=c++2c -fno-exceptions -fno-rtti
CXXFLAGS += -fdiagnostics-color=always

CONFIG ?= debug
ifeq ($(CONFIG),debug)
CXXFLAGS += -g
endif

LDFLAGS =
LDLIBS = -luring 
CPPFLAGS = -I./src
CC := $(CXX)

.PHONY: all
all:: build/test_dlist build/test_ak

COLOR ?= yes
ifdef COLOR
ESC := $(shell printf '\033')
ANSI_RED := $(ESC)[0;31m
ANSI_GREEN := $(ESC)[0;32m
ANSI_YELLOW := $(ESC)[0;33m
ANSI_WHITE := $(ESC)[0;37m
ANSI_HIWHITE := $(ESC)[1;37m
ANSI_RESET := $(ESC)[0m
else
ESC :=
ANSI_RED :=
ANSI_GREEN :=
ANSI_YELLOW :=
ANSI_WHITE :=
ANSI_HIWHITE :=
ANSI_RESET :=
endif


ifeq ($(findstring --debug,$(MAKEFLAGS))$(findstring --trace,$(MAKEFLAGS)),)
trace = printf -- '[$(ANSI_HIWHITE)%s$(ANSI_RESET)] $(ANSI_WHITE)%s$(ANSI_RESET)\n' "$$$$" '$1'
else
trace :=
endif

.PRECIOUS: %/.
%/.:
	$(call trace,mkdir -p "$(@D)")
	mkdir -p "$(@D)"

run_%: build/%
	$(call trace,"$<")
	"$<"

.NOTPARALLEL: test_%
test_%: build/test_%
	printf -- '$(ANSI_YELLOW)---------- Running test:$(ANSI_RESET) %s\n' '$<'
	"$<" |& cat -n
	printf -- '$(ANSI_YELLOW)---------- Finished test:$(ANSI_RESET) %s\n' '$<'

build/precompiled.pch: src/precompiled.hpp | build/.
	$(call trace,CXX -o $@ -c $<)
	$(COMPILE.cc) -x c++-header -MMD -MP -MF build/precompiled.d -o $@ -c $<

-include build/precompiled.d


.PRECIOUS: build/%.o
build/%.o: src/%.cc build/precompiled.pch | build/.
	$(call trace,CXX -o $@ -c $<)
	$(COMPILE.cc) -MMD -MP -MF build/$*.d -include-pch build/precompiled.pch -o $@ -c $<

-include $(patsubst src/%.cc,build/%.d,$(wildcard src/*.cc))

.PRECIOUS: build/%
build/%: build/%.o  | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS)


.PHONY: clean
clean::
	$(call trace,rm -rf build)
	rm -rf build

.PHONY: run
run:: 

.PHONY: test
.NOTPARALLEL: test
test::
	reset

.PHONY: watch
watch: 
	printf -- '%b---------- Watching for changes...%b\n' '$(ANSI_YELLOW)' '$(ANSI_RESET)'
	inotifywait -qmr -e close_write,delete,move ./src | while read -r event; do
		reset
		printf -- '%b---------- Detected change:%b %s\n' '$(ANSI_YELLOW)' "$$event" '$(ANSI_RESET)'
		while read -r -t 1.0 debounce_event; do :; done
		declare -i exit_code=0
		if ! $(MAKE) all; then
			printf -- '%b---------- Build failed%b\n' '$(ANSI_RED)' '$(ANSI_RESET)'
		else
			printf -- '%b---------- Build successful%b\n' '$(ANSI_GREEN)' '$(ANSI_RESET)'
		fi
	done

.PHONY: doc
doc:: doxygen

.PRECIOUS: build/%.pdf
build/%.pdf: src/%.md | build/.
	pandoc --pdf-engine=latexmk -o $@  $<

.PHONY: doxygen
doxygen: | build/doc/.
	doxygen Doxyfile


#----------------------------------------

build/io: build/io.o

test:: test_dlist

test:: build/test_ak build/test_event
	valgrind --leak-check=full --show-leak-kinds=all build/test_event
	valgrind --leak-check=full --show-leak-kinds=all build/test_ak