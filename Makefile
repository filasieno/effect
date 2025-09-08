#!/usr/bin/make

PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
libak_build_dir  := $(PROJECT_DIR)/build/libak
libak_source_dir := $(PROJECT_DIR)/libak/src
libak_test_dir   := $(PROJECT_DIR)/libak/test

# Default values
CONFIG ?= debug
ENABLE_PCH ?= yes

# ------------------------------------------------------------------------------------------------------------------------------
# Configuraion
# ------------------------------------------------------------------------------------------------------------------------------

TARGET_ARCH := -march=x86-64-v3
CXX         ?= clang++
ifneq ($(shell command -v ccache 2>/dev/null),)
  CXX := ccache $(CXX)
endif
CXXFLAGS    := 
CXXFLAGS    += -fno-exceptions -fno-rtti
CXXFLAGS    += -Wall -Wextra -std=c++2c  
CXXFLAGS    += -fdiagnostics-color=always 
CXXFLAGS    += -mavx2 -mbmi -mbmi2 -fPIC
CXXFLAGS    += $(TARGET_ARCH)
CXXFLAGS    += -I$(libak_source_dir)
CXXFLAGS    += -I$(libak_test_dir)

# Optional dependency discovery
PKGCONFIG := $(shell command -v pkg-config 2>/dev/null)
ifneq ($(PKGCONFIG),)
  CPPFLAGS += $(shell pkg-config --cflags liburing 2>/dev/null)
  LDFLAGS  += $(shell pkg-config --libs-only-L liburing 2>/dev/null)
  LDLIBS   += $(shell pkg-config --libs-only-l liburing 2>/dev/null)
endif

ifdef LIBURING_INCLUDE
  CPPFLAGS += -I$(LIBURING_INCLUDE)
endif
ifdef LIBURING_LIB
  LDFLAGS  += -L$(LIBURING_LIB)
endif


# Dependency flags
DEPFLAGS := -MMD -MP

# Configuration
ifeq ($(CONFIG),debug)
  CXXFLAGS += -g -O1
else ifeq ($(CONFIG),release)
  CXXFLAGS += -O3 -flto
  CPPFLAGS += -DNDEBUG
  LDFLAGS  += -flto
endif

# Precompiled headers
ifeq ($(ENABLE_PCH),yes)
  PCH = $(libak_build_dir)/precompiled.pch
  PCH_FLAG = -include-pch $(PCH)
else
  PCH =
  PCH_FLAG =
endif

LDFLAGS += -L$(LIBARGTABLE)
LDLIBS  += -largtable3

# Valgrind support
ifeq ($(RUN_WITH_VALGRIND),yes)
  VALGRIND_CMD := valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
else
  VALGRIND_CMD :=
endif

# ------------------------------------------------------------------------------------------------------------------------------
# Pattern Rules
# ------------------------------------------------------------------------------------------------------------------------------

$(libak_build_dir)/%.o: $(libak_source_dir)/%.cpp $(PCH) | $(libak_build_dir)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PCH_FLAG) $(DEPFLAGS) -c $< -o $@

$(libak_build_dir)/test/%.o: $(libak_test_dir)/%.cpp $(PCH) | $(libak_build_dir)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PCH_FLAG) $(DEPFLAGS) -c $< -o $@

# Directories
$(libak_build_dir):
	mkdir -p $@

# Doxygen
.PHONY: doc
doc: doxygen

.PHONY: doxygen
doxygen: | $(libak_build_dir)/doc
	cd $(PROJECT_DIR)/libak && doxygen Doxyfile

$(libak_build_dir)/doc:
	mkdir -p $@

ifneq ($(PCH),)
$(PCH): $(libak_source_dir)/precompiled.hpp | $(libak_build_dir)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -x c++-header $< -o $@
endif

# ==============================================================================================================================
# Modules 
# ==============================================================================================================================

# Notes:
#   $@ is the target
#   $^ is the dependencies
#   $< is the first dependency
#   $$ is the first argument of the target

# ------------------------------------------------------------------------------------------------------------------------------
# libak base
# ------------------------------------------------------------------------------------------------------------------------------

base_sources := $(shell find $(libak_source_dir)/ak/base -name "*.cpp")
base_objects := $(patsubst $(libak_source_dir)/%.cpp, $(libak_build_dir)/%.o, $(base_sources))
$(libak_build_dir)/libak_base.a: $(base_objects)
	ar rcs $@ $^

test_base_sources := $(shell find $(libak_test_dir)/base -name "*.cpp")
test_base_objects := $(patsubst $(libak_test_dir)/%.cpp, $(libak_build_dir)/test/%.o, $(test_base_sources))
$(libak_build_dir)/test_base: $(test_base_objects) $(libak_build_dir)/libak_base.a
	$(CXX) $(LDFLAGS) -L$(libak_build_dir) $^ -lgtest_main -lgtest -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# libak alloc
# ------------------------------------------------------------------------------------------------------------------------------

alloc_sources := $(shell find $(libak_source_dir)/ak/alloc -name "*.cpp")
alloc_objects := $(patsubst $(libak_source_dir)/%.cpp, $(libak_build_dir)/%.o, $(alloc_sources))
$(libak_build_dir)/libak_alloc.a: $(alloc_objects)
	ar rcs $@ $^

test_alloc_sources := $(shell find $(libak_test_dir)/alloc -name "*.cpp")
test_alloc_objects := $(patsubst $(libak_test_dir)/%.cpp, $(libak_build_dir)/test/%.o, $(test_alloc_sources))
$(libak_build_dir)/test_alloc: $(test_alloc_objects) $(libak_build_dir)/libak_base.a $(libak_build_dir)/libak_alloc.a
	$(CXX) $(LDFLAGS) -L$(libak_build_dir) $^ -lgtest_main -lgtest -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# libak runtime
# ------------------------------------------------------------------------------------------------------------------------------

runtime_sources := $(shell find $(libak_source_dir)/ak/runtime -name "*.cpp")
runtime_objects := $(patsubst $(libak_source_dir)/%.cpp, $(libak_build_dir)/%.o, $(runtime_sources))
$(libak_build_dir)/libak_runtime.a: $(runtime_objects)
	ar rcs $@ $^

test_runtime_sources := $(shell find $(libak_test_dir)/runtime -name "*.cpp")
test_runtime_objects := $(patsubst $(libak_test_dir)/%.cpp, $(libak_build_dir)/test/%.o, $(test_runtime_sources))
$(libak_build_dir)/test_runtime: $(test_runtime_objects) $(libak_build_dir)/libak_base.a $(libak_build_dir)/libak_runtime.a $(libak_build_dir)/libak_alloc.a
	$(CXX) $(LDFLAGS) -L$(libak_build_dir) $^ -lgtest_main -lgtest -luring -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# libak sync
# ------------------------------------------------------------------------------------------------------------------------------

sync_sources := $(shell find $(libak_source_dir)/ak/sync -name "*.cpp")
sync_objects := $(patsubst $(libak_source_dir)/%.cpp, $(libak_build_dir)/%.o, $(sync_sources))
$(libak_build_dir)/libak_sync.a: $(sync_objects)
	ar rcs $@ $^

test_sync_sources := $(shell find $(libak_test_dir)/sync -name "*.cpp")
test_sync_objects := $(patsubst $(libak_test_dir)/%.cpp, $(libak_build_dir)/test/%.o, $(test_sync_sources))
$(libak_build_dir)/test_sync: $(test_sync_objects) $(libak_build_dir)/libak_base.a $(libak_build_dir)/libak_runtime.a $(libak_build_dir)/libak_alloc.a $(libak_build_dir)/libak_sync.a
	$(CXX) $(LDFLAGS) -L$(libak_build_dir) $^ -lgtest_main -lgtest -luring -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# libak json
# ------------------------------------------------------------------------------------------------------------------------------

json_sources := $(shell find $(libak_source_dir)/ak/json -name "*.cpp")
json_objects := $(patsubst $(libak_source_dir)/%.cpp, $(libak_build_dir)/%.o, $(json_sources))
$(libak_build_dir)/libak_json.a: $(json_objects)
	ar rcs $@ $^

test_json_sources := $(shell find $(libak_test_dir)/json -name "*.cpp")
test_json_objects := $(patsubst $(libak_test_dir)/%.cpp, $(libak_build_dir)/test/%.o, $(test_json_sources))
$(libak_build_dir)/test_json: $(test_json_objects) $(libak_build_dir)/libak_base.a $(libak_build_dir)/libak_runtime.a $(libak_build_dir)/libak_alloc.a $(libak_build_dir)/libak_sync.a $(libak_build_dir)/libak_json.a
	$(CXX) $(LDFLAGS) -L$(libak_build_dir) $^ -lgtest_main -lgtest -luring -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# libak storage
# ------------------------------------------------------------------------------------------------------------------------------

storage_sources := $(shell find $(libak_source_dir)/ak/storage -name "*.cpp")
storage_objects := $(patsubst $(libak_source_dir)/%.cpp, $(libak_build_dir)/%.o, $(storage_sources))
$(libak_build_dir)/libak_storage.a: $(storage_objects)
	ar rcs $@ $^

test_storage_sources := $(shell find $(libak_test_dir)/storage -name "*.cpp")
test_storage_objects := $(patsubst $(libak_test_dir)/%.cpp, $(libak_build_dir)/test/%.o, $(test_storage_sources))
$(libak_build_dir)/test_storage: $(test_storage_objects) $(libak_build_dir)/libak_base.a $(libak_build_dir)/libak_alloc.a $(libak_build_dir)/libak_runtime.a $(libak_build_dir)/libak_sync.a $(libak_build_dir)/libak_json.a $(libak_build_dir)/libak_storage.a
	$(CXX) $(LDFLAGS) -L$(libak_build_dir) $^ -lgtest_main -lgtest -luring -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# libak lspd
# ------------------------------------------------------------------------------------------------------------------------------

lspd_sources := $(shell find $(libak_source_dir)/ak/lspd -name "*.cpp")
lspd_objects := $(patsubst $(libak_source_dir)/%.cpp, $(libak_build_dir)/%.o, $(lspd_sources))

$(libak_build_dir)/lspd: $(lspd_objects) $(libak_build_dir)/libak_base.a $(libak_build_dir)/libak_runtime.a $(libak_build_dir)/libak_alloc.a $(libak_build_dir)/libak_sync.a
	$(CXX) $(LDFLAGS) -L$(libak_build_dir) $^ -luring -largtable3 -o $@

.PHONY: lspd
lspd:: $(libak_build_dir)/lspd

# ==============================================================================================================================
# Test
# ==============================================================================================================================

define run_test
  @echo "Running $<"
  $(VALGRIND_CMD) "$<"
endef

.PHONY: test
test:: test_base
test:: test_alloc
test:: test_runtime
test:: test_sync
test:: test_json
test:: test_storage

# ------------------------------------------------------------------------------------------------------------------------------
# Individual test targets
# ------------------------------------------------------------------------------------------------------------------------------

test_base: $(libak_build_dir)/test_base
	$(run_test)

test_alloc: $(libak_build_dir)/test_alloc
	$(run_test)

test_runtime: $(libak_build_dir)/test_runtime
	$(run_test)

test_sync: $(libak_build_dir)/test_sync
	$(run_test)

test_json: $(libak_build_dir)/test_json | $(libak_build_dir)/test_output/json
	@echo "Running $<"
	AK_TEST_DATA_DIR=$(libak_test_dir)/json/data AK_TEST_OUTPUT_DIR=$(libak_build_dir)/test_output/json $(VALGRIND_CMD) "$<"

$(libak_build_dir)/test_output/json:
	mkdir -p $@

test_storage: $(libak_build_dir)/test_storage
	$(run_test)

# Dependency includes
all_test_objects := $(test_base_objects) $(test_alloc_objects) $(test_runtime_objects) $(test_sync_objects) $(test_json_objects) $(test_storage_objects)
all_objects := $(base_objects) $(alloc_objects) $(runtime_objects) $(sync_objects) $(json_objects) $(storage_objects) $(lspd_objects)

-include $(all_objects:.o=.d)
-include $(all_test_objects:.o=.d)

# ==============================================================================================================================
# Libraries
# ==============================================================================================================================

# ------------------------------------------------------------------------------------------------------------------------------
# static lib
# ------------------------------------------------------------------------------------------------------------------------------

$(libak_build_dir)/libak.a: $(all_objects)
	ar rcs $@ $^

# ------------------------------------------------------------------------------------------------------------------------------
# Shared lib
# ------------------------------------------------------------------------------------------------------------------------------

$(libak_build_dir)/libak.so: $(all_objects)
	$(CXX) $(LDFLAGS) -shared -Wl,-soname,libak.so.0 $^ -o $@ -luring

# ==============================================================================================================================
# Phony
# ==============================================================================================================================

.PHONY: all
all:: $(libak_build_dir)/libak.a

.PHONY: clean
clean::
	rm -rf $(libak_build_dir)

