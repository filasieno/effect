#!/usr/bin/make

SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --silent
MAKEFLAGS += --output-sync=target
.ONESHELL:
.DELETE_ON_ERROR:
.SUFFIXES:

.PHONY: all
all::

include .config.mk

TARGET_ARCH = -march=x86-64-v3
CXX = clang++

CXXFLAGS = -Wall -Wextra
CXXFLAGS += -std=c++2c -fno-exceptions -fno-rtti
CXXFLAGS += -fdiagnostics-color=always
CXXFLAGS += -mavx2 -mbmi -mbmi2

CONFIG ?= debug
ifeq ($(CONFIG),debug)
CXXFLAGS += -g
endif

LDFLAGS =
LDLIBS = -luring 
CPPFLAGS = -I./src
CC := $(CXX)


COLOR ?= yes
ifeq ($(COLOR),yes)
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

RUN_TESTS_WITH_VALGRIND ?= no

.NOTPARALLEL: test_%
test_%: build/test_%
	printf -- '$(ANSI_YELLOW)---------- Running test:$(ANSI_RESET) %s\n' '$<'
ifeq ($(RUN_TESTS_WITH_VALGRIND),yes)	
	valgrind --leak-check=full --show-leak-kinds=all  --track-origins=yes --show-reachable=yes --error-exitcode=1 -- "$<"
else
	"$<" |& cat -n
endif
	printf -- '$(ANSI_YELLOW)---------- Finished test:$(ANSI_RESET) %s\n' '$<'

build/precompiled.pch: src/ak/precompiled.hpp | build/.
	$(call trace,CXX -o $@ -c $<)
	$(COMPILE.cc) -x c++-header -MMD -MP -MF build/precompiled.d -o $@ -c $<

-include build/precompiled.d


.PRECIOUS: build/%.o
build/%.o: src/%.cc build/precompiled.pch | build/.
	$(call trace,CXX -o $@ -c $<)
	$(COMPILE.cc) -MMD -MP -MF build/$*.d -include-pch build/precompiled.pch -o $@ -c $<

build/%.o: src/test/%.cc build/precompiled.pch | build/.
	$(call trace,CXX -o $@ -c $<)
	$(COMPILE.cc) -MMD -MP -MF build/$*.d -include-pch build/precompiled.pch -o $@ -c $<

-include $(patsubst src/%.cc,build/%.d,$(wildcard src/*.cc))
-include $(patsubst src/test/%.cc,build/%.d,$(wildcard src/test/*.cc))

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

ifeq ($(CONFIG),coverage) # coverage support

CXXFLAGS += -g -O0 -mavx2 -mbmi -msse4.2
CXXFLAGS += -fprofile-instr-generate -fcoverage-mapping
LDFLAGS += -fprofile-instr-generate

export LLVM_PROFILE_FILE = build/$@.profraw


build/coverage.profdata:
	llvm-profdata merge -output=$@ build/*.profraw
	for i in build/*.profraw; do echo "-object $${i%.profraw}"; done > $@.binaries

.PHONY: coverage-html
coverage-html: build/coverage.profdata
	$(call trace,Generating HTML coverage report)
	llvm-cov show -instr-profile=$< -use-color -format html -output-dir=build/coverage \
		-Xdemangler c++filt -Xdemangler -n \
		-show-instantiations \
		-show-regions -show-line-counts -show-branches=count -show-mcdc -show-expansions \
		-check-binary-ids\
		$$(cat $<.binaries) -sources src/*.hpp src/*.cc src/test/*.cc


.PHONY: coverage-term
coverage-term: build/coverage.profdata
	$(call trace,Generating coverage report for terminal)
	llvm-cov report -instr-profile=$< -use-color $$(cat $<.binaries) -sources src/*.hpp src/*.cc src/test/*.cc
	
build/lcov.info: build/coverage.profdata
	$(call trace,Generating lcov info file)
	llvm-cov export -instr-profile=$< -format=lcov $$(cat $<.binaries) -sources src/*.hpp src/*.cc src/test/*.cc > $@


.PHONY: coverage
coverage:: coverage-term coverage-html build/lcov.info
	

endif # coverage support

define CONFIG_TEMPLATE
# Configuration for the project: [debug], coverage, release
CONFIG = $(CONFIG)

# Enable or disable color output: [yes], no
COLOR = $(COLOR)

# Run tests with Valgrind: yes, [no]
RUN_TESTS_WITH_VALGRIND = $(RUN_TESTS_WITH_VALGRIND)
endef

.PHONY: config
config:: .config.mk
.config.mk:
	cat <<-'EOF' > $@
	$(CONFIG_TEMPLATE)
	EOF




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

test:: test_dlist
test:: test_ak
test:: test_event
test:: test_file_io
test:: test_alloc
test:: test_freelist_search

all:: test doxygen
