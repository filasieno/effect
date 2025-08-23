#!/usr/bin/make

# Set the shell to bash for advanced scripting capabilities in recipes.
SHELL := /bin/bash

# Shell flags to make scripts fail fast on errors and enable pipefail.
# Ensures that errors in pipelines are caught, preventing silent failures.
.SHELLFLAGS := -eu -o pipefail -c

# Make flags to warn on undefined variables, disable built-in rules, silence recipes, and sync output.
# Prevents common mistakes, avoids unexpected behavior from built-ins, and improves output readability in parallel builds.
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --silent
MAKEFLAGS += --output-sync=target

# Enable one-shell mode for recipes.
# Allows multi-line recipes to run in a single shell invocation, preserving variables and state.
.ONESHELL:

# Delete targets on recipe failure.
# Prevents partial or corrupt files from being left behind if a build step fails.
.DELETE_ON_ERROR:

# Clear default suffixes to avoid implicit rules.
# We define all rules explicitly for better control.
.SUFFIXES:

# Include configuration file for customizable settings.
# Allows users to override defaults without modifying the Makefile.
include .config.mk

# Compiler and flags setup
# TARGET_ARCH specifies the architecture for optimization.
# Targets modern x86-64 CPUs for better performance.
TARGET_ARCH = -march=x86-64-v3

# Use ccache with clang++ for faster recompilation.
# ccache caches compilation results, speeding up repeated builds.
CXX = ccache clang++

# Base compiler flags: warnings, C++ standard, disable exceptions and RTTI.
# Enables strict warnings for code quality. Disables stupid C++ features such as exceptions and RTTI.
CXXFLAGS = -Wall -Wextra
CXXFLAGS += -std=c++2c -fno-exceptions -fno-rtti

# Color diagnostics for better readability.
# Makes compiler output easier to read in terminals.
CXXFLAGS += -fdiagnostics-color=always

# CPU-specific optimizations.
# Enables instructions for better performance on supported hardware.
CXXFLAGS += -mavx2 -mbmi -mbmi2

# Position-independent code for shared libraries.
# Required for building shared objects (.so files).
CXXFLAGS += -fPIC

# Default configuration is for CONFIG=debug, can be overridden.
CONFIG ?= debug
ifeq ($(CONFIG),debug)
CXXFLAGS += -g
endif
ifeq ($(CONFIG),release)
CXXFLAGS += -O3 -DNDEBUG
endif

# Linker flags and libraries.
# Specifies libraries needed for linking (io_uring for async I/O).
LDFLAGS =
LDLIBS = -luring

# Preprocessor flags: include source directory.
# Allows headers to be found in ./src.
CPPFLAGS = -I./src

# Set CC to CXX since we're using C++.
# Some tools expect CC for C, but we unify under C++ compiler.
CC := $(CXX)

# Archiver for static libraries.
# ar is used to create .a files.
AR := ar

# Color configuration for output.
# Makes build output more visually appealing and easier to scan.
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

# Trace function for logging build steps.
# Provides feedback on what Make is doing, unless in debug/trace mode.
ifeq ($(findstring --debug,$(MAKEFLAGS))$(findstring --trace,$(MAKEFLAGS)),)
trace = printf -- '[$(ANSI_HIWHITE)%s$(ANSI_RESET)] $(ANSI_WHITE)%s$(ANSI_RESET)\n' "$$$$" '$1'
else
trace :=
endif

# Rule to create directories.
# Ensures build directories exist before writing files.
.PRECIOUS: %/.
%/.:
	$(call trace,mkdir -p "$(@D)")
	mkdir -p "$(@D)"

# Pattern rule to run executables.
# Simplifies running built programs with tracing.
run_%: build/%
	$(call trace,"$<")
	"$<"

# Option to run tests with Valgrind.
# Allows memory leak checking during tests.
RUN_TESTS_WITH_VALGRIND ?= no

# Prevent parallel execution of tests.
# Some tests may not be thread-safe or could interfere.
.NOTPARALLEL: test_%

# Rule to run individual tests, with optional Valgrind.
# Provides structured output for test runs, including memory checking if enabled.
test_%: build/test_%
	printf -- '$(ANSI_YELLOW)---------- Running test:$(ANSI_RESET) %s\n' '$<'
ifeq ($(RUN_TESTS_WITH_VALGRIND),yes)	
	valgrind --leak-check=full --show-leak-kinds=all  --track-origins=yes --show-reachable=yes --error-exitcode=1 -- "$<"
else
	"$<" |& cat -n
endif
	printf -- '$(ANSI_YELLOW)---------- Finished test:$(ANSI_RESET) %s\n' '$<'

# Precompiled header rule.
# Speeds up compilation by precompiling common headers.
build/precompiled.pch: src/ak/precompiled.hpp | build/.
	$(call trace,CXX -o $@ -c $<)
	$(COMPILE.cpp) -x c++-header -MMD -MP -MF build/precompiled.d -o $@ -c $<

-include build/precompiled.d

# Preserve object files.
# Prevents Make from deleting intermediate files.
.PRECIOUS: build/%.o

# Compilation rule for source files using precompiled header.
# Generates object files with dependencies, using PCH for speed.
build/%.o: src/%.cpp build/precompiled.pch | build/.
	$(call trace,CXX -o $@ -c $<)
	mkdir -p "$(@D)"
	$(COMPILE.cpp) -MMD -MP -MF build/$*.d -include-pch build/precompiled.pch -o $@ -c $<

# Compilation rule for test files.
build/%.o: test/%.cpp build/precompiled.pch | build/.
	$(call trace,CXX -o $@ -c $<)
	mkdir -p "$(@D)"
	$(COMPILE.cpp) -MMD -MP -MF build/$*.d -include-pch build/precompiled.pch -o $@ -c $<

# Include dependency files.
# Automatically handles header dependencies for recompilation.
-include $(patsubst src/%.cpp,build/%.d,$(wildcard src/*.cpp))
-include $(patsubst test/%.cpp,build/%.d,$(wildcard test/*.cpp))

# Linking rule for executables.
# Links object files into binaries.
.PRECIOUS: build/%
build/%: build/%.o  | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS)

# =====================================================================================================================================
# Base module section: Builds the base library and its tests.
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

build/test_base: $(BASE_TEST_OBJECTS) build/libak_base.a | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS) -lgtest -lgtest_main

# =====================================================================================================================================
# Alloc module section: Builds the alloc library and its tests.
# =====================================================================================================================================

# ------------
# Alloc module
# ------------

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
build/test_alloc: $(ALLOC_TEST_OBJECTS) build/libak_alloc.a build/libak_base.a | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS) -lgtest -lgtest_main

# =====================================================================================================================================
# Runtime module section: Builds the runtime library and its tests.
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

# -------------------
# Runtime Module Test
# -------------------

RUNTIME_TEST_SOURCES := $(wildcard test/runtime/*.cpp)
RUNTIME_TEST_OBJECTS := $(patsubst test/%.cpp,build/%.o,$(patsubst test/%.cc,build/%.o,$(RUNTIME_TEST_SOURCES)))

.PRECIOUS: build/test_runtime
build/test_runtime: $(RUNTIME_TEST_OBJECTS) build/libak_runtime.a build/libak_alloc.a build/libak_base.a | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS) -lgtest -lgtest_main

# =====================================================================================================================================
# Sync module section: Builds the sync library and its tests.
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
build/test_sync: $(SYNC_TEST_OBJECTS) build/libak_sync.a build/libak_runtime.a build/libak_alloc.a build/libak_base.a | build/.
	$(call trace,LINK -o $@ $^ $(LDLIBS))
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -o $@ $^ $(LDLIBS) -lgtest -lgtest_main

# =====================================================================================================================================
# Combined AK Library: Static and shared versions.
# Provides both static (.a) and dynamic (.so) libraries for flexibility in linking.
# =====================================================================================================================================

.PHONY: lib
lib:: build/libak.a build/libak.so

# Collect all object files from modules.
ALL_LIB_OBJECTS = $(BASE_LIB_OBJECTS) $(ALLOC_LIB_OBJECTS) $(RUNTIME_LIB_OBJECTS) $(SYNC_LIB_OBJECTS)

.PRECIOUS: build/libak.a
build/libak.a: $(ALL_LIB_OBJECTS) | build/.
	$(call trace,AR -rcs $@ $^)
	$(AR) rcs $@ $^

.PRECIOUS: build/libak.so
build/libak.so: $(ALL_LIB_OBJECTS) | build/.
	$(call trace,LINK -shared -o $@ $^)
	$(CXX) $(LDFLAGS) $(TARGET_ARCH) -shared -o $@ $^ $(LDLIBS)

# =====================================================================================================================================
# Code coverage section: Enabled when CONFIG=coverage.
# =====================================================================================================================================

ifeq ($(CONFIG),coverage) # coverage support

# Additional flags for coverage instrumentation.
# Instruments code to collect coverage data.
CXXFLAGS += -g -O1 -mavx2 -mbmi -msse4.2
CXXFLAGS += -fprofile-instr-generate -fcoverage-mapping
LDFLAGS += -fprofile-instr-generate

# Set profile file pattern.
# Specifies where raw coverage data is written.
export LLVM_PROFILE_FILE = build/$@.profraw

# Rule to merge profile data.
build/coverage.profdata:
	llvm-profdata merge -output=$@ build/*.profraw
	for i in build/*.profraw; do echo "-object $${i%.profraw}"; done > $@.binaries

# Phony target for HTML coverage report.
# Generates a detailed, browsable report.
.PHONY: coverage-html
coverage-html: build/coverage.profdata
	$(call trace,Generating HTML coverage report)
	llvm-cov show -instr-profile=$< -use-color -format html -output-dir=build/coverage \
		-Xdemangler c++filt -Xdemangler -n \
		-show-instantiations \
		-show-regions -show-line-counts -show-branches=count -show-mcdc -show-expansions \
		-check-binary-ids\
		$$(cat $<.binaries) -sources src/*.hpp src/*.cpp src/test/*.cpp

# Terminal coverage report.
.PHONY: coverage-term
coverage-term: build/coverage.profdata
	$(call trace,Generating coverage report for terminal)
	llvm-cov report -instr-profile=$< -use-color $$(cat $<.binaries) -sources src/*.hpp src/*.cpp src/test/*.cpp
	
# LCOV info file.
build/lcov.info: build/coverage.profdata
	$(call trace,Generating lcov info file)
	llvm-cov export -instr-profile=$< -format=lcov $$(cat $<.binaries) -sources src/*.hpp src/*.cpp src/test/*.cpp > $@

# Combined coverage target.
.PHONY: coverage
coverage:: coverage-term coverage-html build/lcov.info
	
endif # coverage support

# =====================================================================================================================================
# Configuration template and generation.
# Provides a way to generate or regenerate the config file with current settings.
# =====================================================================================================================================

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

# =====================================================================================================================================
# Documentation generation.
# =====================================================================================================================================

.PHONY: doc
doc:: doxygen

# Rule to convert markdown to PDF.
# Generates printable docs from markdown sources.
.PRECIOUS: build/%.pdf
build/%.pdf: src/%.md | build/.
	pandoc --pdf-engine=latexmk -o $@  $<

# Doxygen for code documentation.
.PHONY: doxygen
doxygen: | build/doc/.
	doxygen Doxyfile

# Doxygen with XML output.
.PHONY: doxygen-xml
doxygen-xml: | build/doc/.
	{ cat Doxyfile; echo "GENERATE_XML = YES"; echo "EXCLUDE += src/test"; } | doxygen -

.PHONY: docs-xml
docs-xml: doxygen-xml

# =====================================================================================================================================
# Phony targets for common commands.
# Provides high-level targets like clean, test, all for ease of use.
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