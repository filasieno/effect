#!/usr/bin/make

build_dir  := build
source_dir := src
test_dir   := test

# Default values
CONFIG ?= debug
ENABLE_PCH ?= yes

# ------------------------------------------------------------------------------------------------------------------------------
# Configuraion
# ------------------------------------------------------------------------------------------------------------------------------

TARGET_ARCH := -march=x86-64-v3
CXX := ccache clang++
CXXFLAGS := 
CXXFLAGS += -fno-exceptions -fno-rtti
CXXFLAGS += -Wall -Wextra -std=c++2c  
CXXFLAGS += -fdiagnostics-color=always 
CXXFLAGS += -mavx2 -mbmi -mbmi2 -fPIC
CXXFLAGS += $(TARGET_ARCH)
CXXFLAGS += -I$(source_dir)
CXXFLAGS += -I$(test_dir)

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
  PCH = $(build_dir)/precompiled.pch
  PCH_FLAG = -include-pch $(PCH)
else
  PCH =
  PCH_FLAG =
endif

# Valgrind support
ifeq ($(RUN_WITH_VALGRIND),yes)
  VALGRIND_CMD := valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
else
  VALGRIND_CMD :=
endif

# ------------------------------------------------------------------------------------------------------------------------------
# Pattern Rules
# ------------------------------------------------------------------------------------------------------------------------------

$(build_dir)/%.o: src/%.cpp $(PCH) | $(build_dir)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PCH_FLAG) $(DEPFLAGS) -c $< -o $@

$(build_dir)/test/%.o: test/%.cpp $(PCH) | $(build_dir)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PCH_FLAG) $(DEPFLAGS) -c $< -o $@

# Directories
$(build_dir):
	mkdir -p $@

# Doxygen
.PHONY: doc
doc: doxygen

.PHONY: doxygen
doxygen: | $(build_dir)/doc
	doxygen Doxyfile

$(build_dir)/doc:
	mkdir -p $@

ifneq ($(PCH),)
$(PCH): $(source_dir)/precompiled.hpp | $(build_dir)
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
# Base
# ------------------------------------------------------------------------------------------------------------------------------

base_sources := $(shell find $(source_dir)/ak/base -name "*.cpp")
base_objects := $(patsubst $(source_dir)/%.cpp, $(build_dir)/%.o, $(base_sources))
$(build_dir)/libak_base.a: $(base_objects)
	ar rcs $@ $^

test_base_sources := $(shell find $(test_dir)/base -name "*.cpp")
test_base_objects := $(patsubst $(test_dir)/%.cpp, $(build_dir)/test/%.o, $(test_base_sources))
$(build_dir)/test_base: $(test_base_objects) $(build_dir)/libak_base.a
	$(CXX) $(LDFLAGS) -L$(build_dir) $^ -lgtest_main -lgtest -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# Alloc
# ------------------------------------------------------------------------------------------------------------------------------

alloc_sources := $(shell find $(source_dir)/ak/alloc -name "*.cpp")
alloc_objects := $(patsubst $(source_dir)/%.cpp, $(build_dir)/%.o, $(alloc_sources))
$(build_dir)/libak_alloc.a: $(alloc_objects)
	ar rcs $@ $^

test_alloc_sources := $(shell find $(test_dir)/alloc -name "*.cpp")
test_alloc_objects := $(patsubst $(test_dir)/%.cpp, $(build_dir)/test/%.o, $(test_alloc_sources))
$(build_dir)/test_alloc: $(test_alloc_objects) $(build_dir)/libak_base.a $(build_dir)/libak_alloc.a
	$(CXX) $(LDFLAGS) -L$(build_dir) $^ -lgtest_main -lgtest -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# Runtime
# ------------------------------------------------------------------------------------------------------------------------------

runtime_sources := $(shell find $(source_dir)/ak/runtime -name "*.cpp")
runtime_objects := $(patsubst $(source_dir)/%.cpp, $(build_dir)/%.o, $(runtime_sources))
$(build_dir)/libak_runtime.a: $(runtime_objects)
	ar rcs $@ $^

test_runtime_sources := $(shell find $(test_dir)/runtime -name "*.cpp")
test_runtime_objects := $(patsubst $(test_dir)/%.cpp, $(build_dir)/test/%.o, $(test_runtime_sources))
$(build_dir)/test_runtime: $(test_runtime_objects) $(build_dir)/libak_base.a $(build_dir)/libak_runtime.a $(build_dir)/libak_alloc.a
	$(CXX) $(LDFLAGS) -L$(build_dir) $^ -lgtest_main -lgtest -luring -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# Sync
# ------------------------------------------------------------------------------------------------------------------------------

sync_sources := $(shell find $(source_dir)/ak/sync -name "*.cpp")
sync_objects := $(patsubst $(source_dir)/%.cpp, $(build_dir)/%.o, $(sync_sources))
$(build_dir)/libak_sync.a: $(sync_objects)
	ar rcs $@ $^

test_sync_sources := $(shell find $(test_dir)/sync -name "*.cpp")
test_sync_objects := $(patsubst $(test_dir)/%.cpp, $(build_dir)/test/%.o, $(test_sync_sources))
$(build_dir)/test_sync: $(test_sync_objects) $(build_dir)/libak_base.a $(build_dir)/libak_runtime.a $(build_dir)/libak_alloc.a $(build_dir)/libak_sync.a
	$(CXX) $(LDFLAGS) -L$(build_dir) $^ -lgtest_main -lgtest -luring -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# JSON
# ------------------------------------------------------------------------------------------------------------------------------

json_sources := $(shell find $(source_dir)/ak/json -name "*.cpp")
json_objects := $(patsubst $(source_dir)/%.cpp, $(build_dir)/%.o, $(json_sources))
$(build_dir)/libak_json.a: $(json_objects)
	ar rcs $@ $^

test_json_sources := $(shell find $(test_dir)/json -name "*.cpp")
test_json_objects := $(patsubst $(test_dir)/%.cpp, $(build_dir)/test/%.o, $(test_json_sources))
$(build_dir)/test_json: $(test_json_objects) $(build_dir)/libak_base.a $(build_dir)/libak_runtime.a $(build_dir)/libak_alloc.a $(build_dir)/libak_sync.a $(build_dir)/libak_json.a
	$(CXX) $(LDFLAGS) -L$(build_dir) $^ -lgtest_main -lgtest -luring -o $@

# ------------------------------------------------------------------------------------------------------------------------------
# Storage
# ------------------------------------------------------------------------------------------------------------------------------

storage_sources := $(shell find $(source_dir)/ak/storage -name "*.cpp")
storage_objects := $(patsubst $(source_dir)/%.cpp, $(build_dir)/%.o, $(storage_sources))
$(build_dir)/libak_storage.a: $(storage_objects)
	ar rcs $@ $^

test_storage_sources := $(shell find $(test_dir)/storage -name "*.cpp")
test_storage_objects := $(patsubst $(test_dir)/%.cpp, $(build_dir)/test/%.o, $(test_storage_sources))
$(build_dir)/test_storage: $(test_storage_objects) $(build_dir)/libak_base.a $(build_dir)/libak_alloc.a $(build_dir)/libak_runtime.a $(build_dir)/libak_sync.a $(build_dir)/libak_json.a $(build_dir)/libak_storage.a
	$(CXX) $(LDFLAGS) -L$(build_dir) $^ -lgtest_main -lgtest -luring -o $@

# ==============================================================================================================================
# Test
# ==============================================================================================================================

define run_test
  @echo "Running $$@"
  $(VALGRIND_CMD) ./$<
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
test_base: $(build_dir)/test_base
	$(run_test)

test_alloc: $(build_dir)/test_alloc
	$(run_test)

test_runtime: $(build_dir)/test_runtime
	$(run_test)

test_sync: $(build_dir)/test_sync
	$(run_test)

test_json: $(build_dir)/test_json
	$(run_test)

test_storage: $(build_dir)/test_storage
	$(run_test)

# Dependency includes
all_test_objects := $(test_base_objects) $(test_alloc_objects) $(test_runtime_objects) $(test_sync_objects) $(test_json_objects) $(test_storage_objects)
all_objects := $(base_objects) $(alloc_objects) $(runtime_objects) $(sync_objects) $(json_objects) $(storage_objects)

-include $(all_objects:.o=.d)
-include $(all_test_objects:.o=.d)

# ==============================================================================================================================
# Libraries
# ==============================================================================================================================

# ------------------------------------------------------------------------------------------------------------------------------
# static lib
# ------------------------------------------------------------------------------------------------------------------------------

$(build_dir)/libak.a: $(all_objects)
	ar rcs $@ $^

# ------------------------------------------------------------------------------------------------------------------------------
# Shared lib
# ------------------------------------------------------------------------------------------------------------------------------

$(build_dir)/libak.so: $(all_objects)
	$(CXX) $(LDFLAGS) -shared -Wl,-soname,libak.so.0 $^ -o $@ -luring

# ==============================================================================================================================
# Phony
# ==============================================================================================================================

.PHONY: all
all: $(build_dir)/libak.a $(build_dir)/libak.so

.PHONY: clean
clean::
	rm -rf $(build_dir)

