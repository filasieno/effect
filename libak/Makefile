#!/usr/bin/make

include linux-clang.mk

build_dir := build
source_dir := src
test_dir := test
dist_dir := dist
DESTDIR := $(dist_dir)
prefix := 


TARGET_ARCH := -march=x86-64-v3
CXX := ccache clang++

CXXFLAGS := -Wall -Wextra
CXXFLAGS += -std=c++2c -fno-exceptions -fno-rtti
CXXFLAGS += -fdiagnostics-color=always
CXXFLAGS += -mavx2 -mbmi -mbmi2
 
CPPFLAGS := -I$(call shell.quote,$(source_dir))
LDFLAGS :=
LDLIBS :=

LDFLAGS  += $(shell pkgconf gtest gtest_main benchmark benchmark_main --libs-only-L)
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

.PHONY: clean.dist
clean.dist::
	[[ ! -e $(call shell.quote,$(dist_dir)) ]] && exit
	$(call shell.title,Clean dist dir)
	$(call shell.trace,rm -r $(call shell.quote,$(dist_dir)))
	rm -r $(call shell.quote,$(dist_dir))




libak_base.sources := $(call shell.find,$(source_dir)/ak/base,-name '*.cpp')
$(call template.eval,$(call template-lib,libak_base))


test_base.sources := $(call shell.find,$(test_dir)/base,-name '*.cpp')
test_base.LDFLAGS := $(libak_base.out.LDFLAGS)
test_base.LDLIBS := -lgtest_main -lgtest -lak_base
$(call template.eval,$(call template-test,test_base))
$(test_base.test-path): $(libak_base.lib-path)


libak_alloc.sources := $(call shell.find,$(source_dir)/ak/alloc,-name '*.cpp')
$(call template.eval,$(call template-lib,libak_alloc))

test_alloc.sources := $(call shell.find,$(test_dir)/alloc,-name '*.cpp')
test_alloc.LDFLAGS := $(libak_base.out.LDFLAGS) $(libak_alloc.out.LDFLAGS)
test_alloc.LDLIBS := -lgtest_main -lgtest -lak_alloc -lak_base
$(call template.eval,$(call template-test,test_alloc))
$(test_alloc.test-path): $(libak_base.lib-path)
$(test_alloc.test-path): $(libak_alloc.lib-path)



libak_runtime.sources := $(call shell.find,$(source_dir)/ak/runtime,-name '*.cpp')
$(call template.eval,$(call template-lib,libak_runtime))


test_runtime.sources := $(call shell.find,$(test_dir)/runtime,-name '*.cpp')
test_runtime.LDFLAGS := $(libak_base.out.LDFLAGS) $(libak_alloc.out.LDFLAGS) $(libak_runtime.out.LDFLAGS)
test_runtime.LDLIBS := -lgtest_main -lgtest -luring -lak_runtime -lak_alloc -lak_base
$(call template.eval,$(call template-test,test_runtime))
$(test_runtime.test-path): $(libak_base.lib-path)
$(test_runtime.test-path): $(libak_alloc.lib-path)
$(test_runtime.test-path): $(libak_runtime.lib-path)


libak_sync.sources := $(call shell.find,$(source_dir)/ak/sync,-name '*.cpp')
$(call template.eval,$(call template-lib,libak_sync))


test_sync.sources := $(call shell.find,$(test_dir)/sync,-name '*.cpp')
test_sync.LDFLAGS := $(libak_base.out.LDFLAGS) $(libak_alloc.out.LDFLAGS) $(libak_runtime.out.LDFLAGS) $(libak_sync.out.LDFLAGS)
test_sync.LDLIBS := -lgtest_main -lgtest -luring -lak_sync -lak_runtime -lak_alloc -lak_base
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

# For solib use a different pch with -fPIC
pch2.CXXFLAGS := $(CXXFLAGS) -fPIC
$(call template.eval,$(call template-pch,pch2))

libak-so.sources := $(libak_base.sources) $(libak_alloc.sources) $(libak_runtime.sources) $(libak_sync.sources)
libak-so.lib-name := libak
libak-so.version := 0.0.1
libak-so.pch := pch2
$(call template.eval,$(call template-solib,libak-so))
all:: libak-so

.PHONY: install.headers
install.headers::
	$(call shell.title,Install headers)
	$(INSTALL) -d $(call shell.quote,$(DESTDIR)$(includedir))
	(cd $(call shell.quote,$(source_dir)) && find . \( -name '*_api.hpp' -o -name '*_api_inl.hpp' -o -name 'ak.hpp' \) -print0) | xargs -0 -I{} -n1 -t -- $(INSTALL_DATA) -D $(call shell.quote,$(source_dir)/{}) $(call shell.quote,$(DESTDIR)$(includedir)/{})

.PHONY: uninstall.headers
uninstall.headers::
	$(call shell.title,Uninstall headers)
	(cd $(call shell.quote,$(source_dir)) && find . \( -name '*_api.hpp' -o -name '*_api_inl.hpp' -o -name 'ak.hpp' \) -print0) | xargs -0 -I{} -n1 -t -- $(RM) $(call shell.quote,$(DESTDIR)$(includedir)/{})

install.test:: bindir := $(exec_prefix)/test
install.test:: install.test_base install.test_alloc install.test_runtime install.test_sync
uninstall.test:: uninstall.test_base uninstall.test_alloc uninstall.test_runtime uninstall.test_sync

install:: install.libak-a 
install:: install.libak-so
install:: install.headers

uninstall:: uninstall.libak-a 
uninstall:: uninstall.libak-so
uninstall:: uninstall.headers


