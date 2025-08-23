#!/usr/bin/make

include x86_64-linux-clang.mk

build_dir := build
source_dir := src
test_dir := test

TARGET_ARCH := -march=x86-64-v3
CXX := ccache clang++

CXXFLAGS := -Wall -Wextra
CXXFLAGS += -std=c++2c -fno-exceptions -fno-rtti
CXXFLAGS += -fdiagnostics-color=always
CXXFLAGS += -mavx2 -mbmi -mbmi2
CXXFLAGS += -fPIC
 
LDFLAGS :=
LDFLAGS += $(shell pkgconf gtest gtest_main benchmark benchmark_main --libs)

LDLIBS :=
CPPFLAGS := -I$(call shell.quote.var,$(source_dir))
CPPFLAGS += $(shell pkgconf gtest gtest_main benchmark benchmark_main --cflags)


ifeq ($(CONFIG),debug)
CXXFLAGS += -g -O0
else ifeq ($(CONFIG),release)
CXXFLAGS += -O3 -flto
CPPFLAGS += -DNDEBUG
LDFLAGS  += -flto
endif


#------------------------------------------
$(mk.load.setup)
#------------------------------------------

.PHONY: doc
doc:: doxygen


.PHONY: doxygen
doxygen: | $(build_dir)/doc/.
	doxygen Doxyfile




libak_base.sources := $(shell find $(source_dir)/ak/base -name "*.cpp")
$(call template.eval,$(call template-lib,libak_base))


test_base.sources := $(shell find $(test_dir)/base -name "*.cpp")
test_base.LDFLAGS := -L$(call std.dir,$(libak_base.lib-path))
test_base.LDLIBS := -lgtest_main -lgtest -lak_base
$(call template.eval,$(call template-test,test_base))
$(test_base.test-path): $(libak_base.lib-path)


libak_alloc.sources := $(shell find $(source_dir)/ak/alloc -name "*.cpp")
$(call template.eval,$(call template-lib,libak_alloc))

test_alloc.sources := $(shell find $(test_dir)/alloc -name "*.cpp")
test_alloc.LDFLAGS := -L$(call std.dir,$(libak_base.lib-path)) -L$(call std.dir,$(libak_alloc.lib-path))
test_alloc.LDLIBS := -lgtest_main -lgtest -lak_base -lak_alloc
$(call template.eval,$(call template-test,test_alloc))
$(test_alloc.test-path): $(libak_base.lib-path)
$(test_alloc.test-path): $(libak_alloc.lib-path)



libak_runtime.sources := $(shell find $(source_dir)/ak/runtime -name "*.cpp")
$(call template.eval,$(call template-lib,libak_runtime))


test_runtime.sources := $(shell find $(test_dir)/runtime -name "*.cpp")
test_runtime.LDFLAGS := -L$(call std.dir,$(libak_base.lib-path)) -L$(call std.dir,$(libak_alloc.lib-path)) -L$(call std.dir,$(libak_runtime.lib-path))
test_runtime.LDLIBS := -lgtest_main -lgtest -lak_base -lak_alloc -lak_runtime -luring
$(call template.eval,$(call template-test,test_runtime))
$(test_runtime.test-path): $(libak_base.lib-path)
$(test_runtime.test-path): $(libak_alloc.lib-path)
$(test_runtime.test-path): $(libak_runtime.lib-path)


libak_sync.sources := $(shell find $(source_dir)/ak/sync -name "*.cpp")
$(call template.eval,$(call template-lib,libak_sync))


test_sync.sources := $(shell find $(test_dir)/sync -name "*.cpp")
test_sync.LDFLAGS := -L$(call std.dir,$(libak_base.lib-path))
test_sync.LDFLAGS += -L$(call std.dir,$(libak_alloc.lib-path))
test_sync.LDFLAGS += -L$(call std.dir,$(libak_runtime.lib-path))
test_sync.LDFLAGS += -L$(call std.dir,$(libak_sync.lib-path))
test_sync.LDLIBS := -lak_sync -lak_runtime -lak_alloc -lak_base -luring -lgtest_main -lgtest 
$(call template.eval,$(call template-test,test_sync))
$(test_sync.test-path): $(libak_base.lib-path)
$(test_sync.test-path): $(libak_alloc.lib-path)
$(test_sync.test-path): $(libak_runtime.lib-path)
$(test_sync.test-path): $(libak_sync.lib-path)



libak-a.objects := $(libak_base.objects) $(libak_alloc.objects) $(libak_runtime.objects) $(libak_sync.objects)
libak-a.lib-name := libak
$(call template.eval,$(call template-lib,libak-a))
$(libak-a.lib-path): $(libak_base.lib-path) $(libak_alloc.lib-path) $(libak_runtime.lib-path) $(libak_sync.lib-path)
all:: libak-a


libak-so.sources := $(libak_base.sources) $(libak_alloc.sources) $(libak_runtime.sources) $(libak_sync.sources)
libak-so.lib-name := libak
libak-so.version := 0.0.1
$(call template.eval,$(call template-solib,libak-so))
all:: libak-so
