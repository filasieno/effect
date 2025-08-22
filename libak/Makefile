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
LDLIBS = -luring -lgtest -lgtest_main
CPPFLAGS = -I./src


CC := $(CXX)

AR := ar


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
	$(COMPILE.cpp) -x c++-header -MMD -MP -MF build/precompiled.d -o $@ -c $<

-include build/precompiled.d


.PRECIOUS: build/%.o

build/%.o: src/%.cpp build/precompiled.pch | build/.
	$(call trace,CXX -o $@ -c $<)
	mkdir -p "$(@D)"
	$(COMPILE.cpp) -MMD -MP -MF build/$*.d -include-pch build/precompiled.pch -o $@ -c $<

build/%.o: test/%.cpp build/precompiled.pch | build/.
	$(call trace,CXX -o $@ -c $<)
	mkdir -p "$(@D)"
	$(COMPILE.cpp) -MMD -MP -MF build/$*.d -include-pch build/precompiled.pch -o $@ -c $<

-include $(patsubst src/%.cpp,build/%.d,$(wildcard src/*.cpp))
-include $(patsubst test/%.cpp,build/%.d,$(wildcard test/*.cpp))

.PRECIOUS: build/%
build/%: build/%.o  | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS)

# =====================================================================================================================================
# Base
# =====================================================================================================================================

# -------------
# Base module
# -------------

BASE_LIB_SOURCES := $(shell find src/ak/base -maxdepth 1 -name '*.cpp')
BASE_LIB_OBJECTS := $(patsubst src/%.cpp,build/%.o,$(BASE_LIB_SOURCES))

.PRECIOUS: build/libak_base.a
build/libak_base.a: $(BASE_LIB_OBJECTS) | build/.
	$(call trace,AR -rcs $@ $^)
	$(AR) rcs $@ $^

# ----------------
# Base Module Test
# ----------------

BASE_TEST_SOURCES := $(wildcard test/base/*.cpp)
BASE_TEST_OBJECTS := $(patsubst test/%.cpp,build/%.o,$(BASE_TEST_SOURCES))

.PRECIOUS: build/test_base
build/test_base: $(BASE_TEST_OBJECTS) build/libak_base.a | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS)

# =====================================================================================================================================
# Alloc
# =====================================================================================================================================

# ------------
# Alloc module
# ------------

# Alloc static library (module-only sources)
ALLOC_LIB_SOURCES := $(shell find src/ak/alloc -maxdepth 1 -name '*.cpp')
ALLOC_LIB_OBJECTS := $(patsubst src/%.cpp,build/%.o,$(ALLOC_LIB_SOURCES))

.PRECIOUS: build/libak_alloc.a
build/libak_alloc.a: $(ALLOC_LIB_OBJECTS) | build/.
	$(call trace,AR -rcs $@ $^)
	$(AR) rcs $@ $^

# -----------------
# Alloc Module Test
# -----------------

ALLOC_TEST_SOURCES := $(wildcard test/alloc/*.cpp)
ALLOC_TEST_OBJECTS := $(patsubst test/%.cpp,build/%.o,$(ALLOC_TEST_SOURCES))

.PRECIOUS: build/test_alloc
build/test_alloc: $(ALLOC_TEST_OBJECTS) build/libak_base.a build/libak_alloc.a | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS)

# =====================================================================================================================================
# Runtime
# =====================================================================================================================================

# --------------
# Runtime module
# --------------

RUNTIME_LIB_SOURCES := $(shell find src/ak/runtime -maxdepth 1 -name '*.cpp')
RUNTIME_LIB_OBJECTS := $(patsubst src/%.cpp,build/%.o,$(RUNTIME_LIB_SOURCES))

.PRECIOUS: build/libak_runtime.a
build/libak_runtime.a: $(RUNTIME_LIB_OBJECTS) | build/.
	$(call trace,AR -rcs $@ $^)
	$(AR) rcs $@ $^

# -----------------
# Runtime Module Test
# -----------------

RUNTIME_TEST_SOURCES := $(wildcard test/runtime/*.cpp)
RUNTIME_TEST_OBJECTS := $(patsubst test/%.cpp,build/%.o,$(patsubst test/%.cc,build/%.o,$(RUNTIME_TEST_SOURCES)))

.PRECIOUS: build/test_runtime
build/test_runtime: $(RUNTIME_TEST_OBJECTS) build/libak_base.a build/libak_alloc.a build/libak_runtime.a | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -Wl,--start-group build/libak_base.a build/libak_alloc.a build/libak_runtime.a $(RUNTIME_TEST_OBJECTS) -Wl,--end-group $(LDLIBS) -o $@

# =====================================================================================================================================
# Sync
# =====================================================================================================================================

# --------------
# Sync module
# --------------

SYNC_LIB_SOURCES := $(shell find src/ak/sync -maxdepth 1 -name '*.cpp')
SYNC_LIB_OBJECTS := $(patsubst src/%.cpp,build/%.o,$(SYNC_LIB_SOURCES))

.PRECIOUS: build/libak_sync.a
build/libak_sync.a: $(SYNC_LIB_OBJECTS) | build/.
	$(call trace,AR -rcs $@ $^)
	$(AR) rcs $@ $^

# -----------------
# Sync Module Test
# -----------------

SYNC_TEST_SOURCES := $(wildcard test/sync/*.cpp)
SYNC_TEST_OBJECTS := $(patsubst test/%.cpp,build/%.o,$(SYNC_TEST_SOURCES))

.PRECIOUS: build/test_sync
build/test_sync: $(SYNC_TEST_OBJECTS) build/libak_base.a build/libak_alloc.a build/libak_runtime.a build/libak_sync.a | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -Wl,--start-group build/libak_base.a build/libak_alloc.a build/libak_runtime.a build/libak_sync.a $(SYNC_TEST_OBJECTS) -Wl,--end-group $(LDLIBS) -o $@

# =====================================================================================================================================
# Static AK Library (combine modules)
# =====================================================================================================================================

.PHONY: lib
lib:: build/libak.a

COMBINED_LIBS = build/libak_base.a build/libak_alloc.a build/libak_runtime.a build/libak_sync.a

.PRECIOUS: build/libak.a
build/libak.a: $(COMBINED_LIBS) | build/.
	$(call trace,AR -rcs $@ $^)
	$(AR) rcs $@ $^


# =====================================================================================================================================
# Code coverage
# =====================================================================================================================================

ifeq ($(CONFIG),coverage) # coverage support

CXXFLAGS += -g -O1 -mavx2 -mbmi -msse4.2
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
		$$(cat $<.binaries) -sources src/*.hpp src/*.cpp src/test/*.cpp


.PHONY: coverage-term
coverage-term: build/coverage.profdata
	$(call trace,Generating coverage report for terminal)
	llvm-cov report -instr-profile=$< -use-color $$(cat $<.binaries) -sources src/*.hpp src/*.cpp src/test/*.cpp
	
build/lcov.info: build/coverage.profdata
	$(call trace,Generating lcov info file)
	llvm-cov export -instr-profile=$< -format=lcov $$(cat $<.binaries) -sources src/*.hpp src/*.cpp src/test/*.cpp > $@


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

# =====================================================================================================================================
# Doc
# =====================================================================================================================================

.PHONY: doc
doc:: doxygen

.PRECIOUS: build/%.pdf
build/%.pdf: src/%.md | build/.
	pandoc --pdf-engine=latexmk -o $@  $<

.PHONY: doxygen
doxygen: | build/doc/.
	doxygen Doxyfile

.PHONY: doxygen-xml
doxygen-xml: | build/doc/.
	{ cat Doxyfile; echo "GENERATE_XML = YES"; echo "EXCLUDE += src/test"; } | doxygen -

.PHONY: docs-xml
docs-xml: doxygen-xml

# =====================================================================================================================================
# Phony targets: clean run test all
# =====================================================================================================================================

.PHONY: clean
clean::
	$(call trace,rm -rf build)
	rm -rf build

.PHONY: run
run:: 

.PHONY: test
.NOTPARALLEL: test
test:: build/test_base build/test_alloc build/test_runtime build/test_sync
	$(MAKE) -s test_base
	$(MAKE) -s test_alloc
	$(MAKE) -s test_runtime
	$(MAKE) -s test_sync

.PHONY: all
all:: test 
all:: lib
all:: doc