Loading: init
Loading: setup
[0;33mCONFIG[0m = [0;37mdebug[0m
[0;33mENABLE_PCH[0m = [0;37myes[0m
[0;33mENABLE_COVERAGE[0m = [0;37mno[0m
# GNU Make 4.4.1
# Built for x86_64-pc-linux-gnu
# Copyright (C) 1988-2023 Free Software Foundation, Inc.
# License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
# This is free software: you are free to change and redistribute it.
# There is NO WARRANTY, to the extent permitted by law.

# Make data base, printed on Sun Sep  7 16:30:58 2025

# Variables

# makefile (from 'linux-clang.mk', line 377)
INSTALL_PROGRAM := install
# makefile (from 'Makefile', line 60)
libak_base.out.LDFLAGS := -L'build/libak_base/'
# makefile (from 'linux-clang.mk', line 430)
ansi.hiblack := [1;30m
# environment
MEMORY_PRESSURE_WRITE = c29tZSAyMDAwMDAgMjAwMDAwMAA=
# environment
GDK_BACKEND = x11
# makefile (from '.config.mk', line 14)
ENABLE_COVERAGE := no
# makefile (from 'linux-clang.mk', line 136)
immediate = $(eval $1 := $(value $1))
# makefile (from 'linux-clang.mk', line 268)
shell.install.data = $ $(let src-data,$(call shell.quote.var,$1),$ $(let dst-data,$(call shell.quote.var,$2/$(call std.notdir,$1)),$ $(call shell.install.dir,$2)$(char.nl)$ $(call shell.trace,INSTALL_DATA $(src-data) $(dst-data))$(char.nl)$ $(INSTALL_DATA) $(src-data) $(dst-data)$ )$ )
# makefile (from 'linux-clang.mk', line 377)
AR := ar
# makefile (from 'linux-clang.mk', line 250)
std.sort = $(call mk.unpack,$(sort $(call mk.pack,$1)))
# makefile (from 'linux-clang.mk', line 92)
char.lparen := (
# makefile (from 'Makefile', line 87)
test_runtime.sources := test/runtime/test_alloc.cpp test/runtime/test_ak.cpp test/runtime/test_file_io.cpp
# makefile (from 'Makefile', line 60)
libak_base.lib-dir := build/libak_base/
# makefile (from 'Makefile', line 125)
libak-so.LDLIBS = $(LDLIBS)
# environment
NIX_REMOTE = daemon
# makefile (from 'Makefile', line 60)
libak_base.build_dir = $(build_dir)/libak_base
# makefile (from 'Makefile', line 84)
libak_runtime.CXXFLAGS = $(CXXFLAGS)
# environment
GJS_DEBUG_OUTPUT = stderr
# makefile (from 'linux-clang.mk', line 146)
shell.escape._rec = $(let c r,$2,$(if $c,$(call shell.escape._rec,$(subst $c,\$c,$1),$r),$1))
# makefile (from 'linux-clang.mk', line 375)
default.TARGET_ARCH = $(TARGET_ARCH)
# makefile (from 'Makefile', line 5)
build_dir := build
# makefile (from 'linux-clang.mk', line 428)
ansi.cyan := [0;36m
# makefile (from 'Makefile', line 103)
test_sync.build_dir = build/test_sync
# makefile (from 'linux-clang.mk', line 244)
std.addprefix = $(call mk.unpack,$(addprefix $1,$(call mk.pack,$2)))
# makefile (from 'linux-clang.mk', line 424)
ansi.green := [0;32m
# makefile (from 'Makefile', line 101)
test_sync.LDFLAGS := -L'build/libak_base/' -L'build/libak_alloc/' -L'build/libak_runtime/' -L'build/libak_sync/'
# makefile (from 'Makefile', line 123)
libak-so.version := 0.0.1
# environment
LC_NUMERIC = it_IT.UTF-8
# makefile (from 'linux-clang.mk', line 248)
std.firstword = $(call mk.unpack,$(firstword $(call mk.pack,$1)))
# makefile (from 'linux-clang.mk', line 233)
mk.unpack = $(subst &&,&,$(subst &+,\$(char.space),$1))
# makefile (from 'linux-clang.mk', line 605)
mk.rule.head = $1$(char.nl)
# makefile (from 'Makefile', line 119)
pch2.TARGET_ARCH = $(TARGET_ARCH)
# makefile (from 'Makefile', line 125)
libak-so.lib-path = $(libak-so.build_dir)/$(libak-so.lib-name).so.$(libak-so.version)
# environment
WINDOWPATH = 2
# makefile (from 'Makefile', line 113)
libak-a.sources.objects := 
# makefile (from 'linux-clang.mk', line 128)
immediate-default = $(eval $(if $(findstring $(flavor $1),undefined),$1 := $2,$1 := $1))
# makefile (from 'Makefile', line 97)
libak_sync.objects = $(libak_sync.sources.objects)
# environment
VSCODE_GIT_EDITOR_MAIN = /usr/share/code/resources/app/extensions/git/dist/git-editor-main.js
# makefile (from 'linux-clang.mk', line 164)
shell.quote.var = $(call shell.quote,$(call mk.unescape,$1))
# environment
NODE_VERSIONS = /home/roby/.nvm/versions/node
# makefile (from 'linux-clang.mk', line 378)
prefix := 
# environment
LC_ADDRESS = it_IT.UTF-8
# makefile (from 'linux-clang.mk', line 427)
ansi.magenta := [0;35m
# environment
GDM_LANG = en_US.UTF-8
# default
MAKE_COMMAND := make
# makefile (from 'linux-clang.mk', line 195)
mk.escape = $(subst $(char.space),\$(char.space),$(subst \,\\,$1))
# makefile (from 'Makefile', line 72)
libak_alloc.lib-name = libak_alloc
# makefile (from 'Makefile', line 75)
test_alloc.LDFLAGS := -L'build/libak_base/' -L'build/libak_alloc/'
# environment
GTK3_MODULES = xapp-gtk3-module
# makefile (from 'Makefile', line 77)
test_alloc.CXXFLAGS = -Wall -Wextra -std=c++2c -fno-exceptions -fno-rtti -fdiagnostics-color=always -mavx2 -mbmi -mbmi2 -g -O0
# environment
QT_ACCESSIBILITY = 1
# makefile (from 'Makefile', line 60)
libak_base.CXXFLAGS = $(CXXFLAGS)
# makefile (from 'Makefile', line 77)
test_alloc.build_dir = build/test_alloc
# environment
VSCODE_GIT_ASKPASS_EXTRA_ARGS = 
# environment
GOPATH = /home/roby/.local/lib/go
# makefile (from 'Makefile', line 125)
libak-so.TARGET_ARCH = $(TARGET_ARCH)
# automatic
@D = $(patsubst %/,%,$(dir $@))
# makefile (from 'Makefile', line 59)
libak_base.sources := src/ak/base/base_timer.cpp src/ak/base/base_version.cpp
# makefile (from 'linux-clang.mk', line 389)
define config-file.content
# Configuration for the project: [debug], release, ...
CONFIG := $(CONFIG)

# Enable or disable color output: [yes], no
COLOR := $(COLOR)

# Run tests with Valgrind: yes, [no]
RUN_WITH_VALGRIND := $(RUN_WITH_VALGRIND)

# Enable precompiled header:  [yes], no
ENABLE_PCH := $(ENABLE_PCH)

# Enable coverage:  yes, [no]
ENABLE_COVERAGE := $(ENABLE_COVERAGE)

endef
# makefile (from '.config.mk', line 8)
RUN_WITH_VALGRIND := no
# makefile (from 'Makefile', line 66)
test_base.test-name = test_base
# makefile (from 'Makefile', line 103)
test_sync.TARGET_ARCH = -march=x86-64-v3
# makefile (from 'Makefile', line 124)
libak-so.pch := pch2
# makefile (from 'Makefile', line 83)
libak_runtime.sources := src/ak/runtime/runtime_debug_io.cpp src/ak/runtime/runtime_thread_ops.cpp src/ak/runtime/runtime_thread_context.cpp src/ak/runtime/runtime_debug_task.cpp src/ak/runtime/runtime_boot.cpp src/ak/runtime/runtime_kernel.cpp src/ak/runtime/runtime_io_prep.cpp src/ak/runtime/runtime_task.cpp
# default
.VARIABLES := 
# environment
PWD = /home/roby/Workspace/coro/effect/libak
# automatic
%D = $(patsubst %/,%,$(dir $%))
# makefile (from 'Makefile', line 74)
test_alloc.sources := test/alloc/test_freeblock.cpp test/alloc/test_freeblock_list_search.cpp test/alloc/test_freeblock_list.cpp test/alloc/test_freeblock_tree.cpp test/alloc/test_split.cpp test/alloc/test_defragment.cpp
# makefile (from 'Makefile', line 72)
libak_alloc.objects = $(libak_alloc.sources.objects)
# makefile (from 'Makefile', line 103)
test_sync.sources.objects := /test_event.o
# environment
XDG_DATA_DIRS = /usr/share/gnome:/home/roby/.local/share/flatpak/exports/share:/var/lib/flatpak/exports/share:/usr/local/share/:/usr/share/
# makefile (from 'linux-clang.mk', line 242)
std.basename = $(call mk.unpack,$(basename $(call mk.pack,$1)))
# makefile (from 'Makefile', line 60)
libak_base.lib-name = libak_base
# makefile (from 'linux-clang.mk', line 375)
default.LDFLAGS = $(LDFLAGS)
# makefile (from 'Makefile', line 72)
libak_alloc.CPPFLAGS = $(CPPFLAGS) $($(libak_alloc.pch).out.CPPFLAGS)
# automatic
^D = $(patsubst %/,%,$(dir $^))
# makefile (from 'Makefile', line 77)
test_alloc.TARGET_ARCH = -march=x86-64-v3
# makefile (from 'Makefile', line 90)
test_runtime.objects = 
# automatic
%F = $(notdir $%)
# makefile (from 'Makefile', line 122)
libak-so.lib-name := libak
# makefile (from 'linux-clang.mk', line 104)
  := 
# environment
LANG = en_US.UTF-8
# environment
XAUTHORITY = /run/user/1000/gdm/Xauthority
# default
.LOADED := 
# default
.INCLUDE_DIRS := /usr/local/include /usr/include
# makefile (from 'linux-clang.mk', line 378)
mandir := /share/man
# makefile (from 'linux-clang.mk', line 1149)
define template-solib
# START template-solib
$1.build_dir ?= §(build_dir)/$1
$1.version ?= 1.0.0
$1.lib-name ?= $1
$1.soname ?= §($1.lib-name).so.§(word 1,§(subst ., ,§($1.version)))
$1.lib-path ?= §($1.build_dir)/§($1.lib-name).so.§($1.version)
$1.sources ?=
$1.objects ?=
$1.CXXFLAGS ?= §(CXXFLAGS)
$1.CXXFLAGS += -fPIC
$1.CPPFLAGS ?= §(CPPFLAGS)
$1.LDFLAGS ?= §(LDFLAGS)
$1.LDFLAGS += -shared -Wl,-soname,§($1.soname)
$1.TARGET_ARCH ?= §(TARGET_ARCH)
$1.LDLIBS ?= §(LDLIBS)
$1.pch ?= pch
$1.CPPFLAGS += §(§($1.pch).out.CPPFLAGS)
$1.lib-dir := §(call std.dir,§($1.lib-path))
$1.out.LDFLAGS := -L§(call shell.quote.var,§($1.lib-dir))
$1.out.LDLIBS := -l§(patsubst lib%,%,§($1.lib-name))
$1.link1 := §($1.build_dir)/§($1.lib-name).so
$1.link2 := §($1.build_dir)/§($1.soname)

.PHONY: $1
$1:: §($1.lib-path)

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote.var,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote.var,§($1.build_dir)) == §(call shell.quote.var,§(build_dir)) ]] && exit
	§(call shell.title,Clean shared lib $1)
	§(call shell.rmdir,§($1.build_dir))

$1.sources.objects := §(foreach ps,$ §(call mk.pack,§($1.sources)),$ §(let $ s,§(call mk.unpack,§(ps)),$ §(let $ o,§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(ps))).o),$ §(call template.eval,§(call template-compile.cc,$1,§s,§o))$ §o$ )$ )$ )

ifeq ($(ENABLE_PCH),yes)
§($1.sources.objects): §(§($1.pch).pch-path)
endif

$1.objects += §($1.sources.objects)

.PRECIOUS: §($1.lib-path) §($1.link1) §($1.link2)
§($1.lib-path) §($1.link1) §($1.link2) : §($1.objects) | §(call std.dir,§($1.lib-path)).
	§(call shell.title,LINK -shared -o §(call shell.quote.var,§($1.lib-path)) §(call shell.quote.list,§($1.objects)) §($1.LDLIBS))
	§(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.LDFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF §(call shell.quote.var,§($1.lib-path)).d -o §(call shell.quote.var,§($1.lib-path)) §(call shell.quote.list,§($1.objects)) §($1.LDLIBS) # end
	ln -sfr §(call shell.quote.var,§($1.lib-path)) §(call shell.quote.var,§($1.link1))
	ln -sfr §(call shell.quote.var,§($1.lib-path)) §(call shell.quote.var,§($1.link2))


.PHONY: install.$1
install.$1:: §($1.lib-path) §($1.link1) §($1.link2)
	§(call shell.title,Install $1)
	§(call shell.install.data,§($1.lib-path),§(DESTDIR)§(libdir))
	§(call shell.ln-s,§($1.lib-name).so.§($1.version),§(DESTDIR)§(libdir)/§($1.lib-name).so)
	§(call shell.ln-s,§($1.lib-name).so.§($1.version),§(DESTDIR)§(libdir)/§($1.soname))

.PHONY: uninstall.$1
uninstall.$1::
	§(call shell.title,Uninstall $1)
	§(call shell.rm,§(DESTDIR)§(libdir)/§(call std.notdir,§($1.lib-path)))
	§(call shell.rm,§(DESTDIR)§(libdir)/§($1.lib-name).so)
	§(call shell.rm,§(DESTDIR)§(libdir)/§($1.soname))

# END template-solib
endef
# environment
MEMORY_PRESSURE_WATCH = /sys/fs/cgroup/user.slice/user-1000.slice/user@1000.service/session.slice/org.gnome.Shell@x11.service/memory.pressure
# makefile (from 'Makefile', line 84)
libak_runtime.CPPFLAGS = $(CPPFLAGS) $($(libak_runtime.pch).out.CPPFLAGS)
# makefile
MAKEFLAGS = prRs -Otarget --warn-undefined-variables
# makefile (from 'Makefile', line 66)
test_base.pch = pch
# makefile (from 'Makefile', line 119)
pch2.CPPFLAGS = $(CPPFLAGS) -x c++-header
# environment
GIT_EDITOR = "/usr/share/code/resources/app/extensions/git/dist/git-editor.sh"
# environment
PKG_CONFIG_PATH = /home/roby/.local/lib/pkgconfig
# makefile (from 'linux-clang.mk', line 893)
define template-test
# START template-test $1
$1.build_dir ?= $(build_dir)/$1
$1.test-name ?= $1
$1.test-path ?= $($1.build_dir)/$($1.test-name)
$1.sources ?=
$1.objects ?=
$1.TARGET_ARCH ?= $(TARGET_ARCH)
$1.CXXFLAGS ?= $(CXXFLAGS)
$1.CPPFLAGS ?= $(CPPFLAGS)
$1.LDFLAGS ?= $(LDFLAGS)
$1.LDLIBS ?= $(LDLIBS)
$1.pch ?= pch
$1.CPPFLAGS += $($($1.pch).out.CPPFLAGS)

ifeq ($(ENABLE_COVERAGE),yes)
$1.CXXFLAGS += -fprofile-instr-generate=$(call shell.quote.var,$(coverage.build_dir)/$1-%p.profraw) -fcoverage-mapping -fcoverage-mcdc
endif

.PHONY: $1
ifeq ($(ENABLE_COVERAGE),yes)
$1:: | $(coverage.build_dir)/.
endif
$1:: $($1.test-path)
	§(call shell.title,Running test $1: §(call shell.quote.var,§($1.test-path)))
	declare -i error_code=0
	set +e
ifeq ($(RUN_WITH_VALGRIND),yes)
	valgrind --leak-check=full --show-leak-kinds=all  --track-origins=yes --show-reachable=yes --error-exitcode=1 -- §(call shell.quote.var,§($1.test-path))
else
	§(call shell.quote.var,§($1.test-path))
endif
	error_code=§§?
	set -e
ifeq ($(ENABLE_COVERAGE),yes)
	printf -- '%s' §(call shell.quote.var,§($1.test-path)) > §(call shell.quote.var,§(coverage.build_dir)/§@.profraw.binary)
endif
	if (( error_code == 0 )); then
		§(call shell.trace,§(ansi.green)PASSED §(call shell.quote.var,§($1.test-path))§(ansi.reset))
	else
		§(call shell.trace,§(ansi.red)FAILED (§§error_code) §(call shell.quote.var,§($1.test-path))§(ansi.reset))
	fi

.PHONY: test
test:: $1

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote.var,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote.var,§($1.build_dir)) == §(call shell.quote.var,§(build_dir)) ]] && exit
	§(call shell.title,Clean test $1)
	§(call shell.rmdir,§($1.build_dir))

$1.sources.objects := 

$(foreach ps,$ $(call mk.pack,$($1.sources)),$ $(let $ s,$(call mk.unpack,$(ps)),$ $(let $ o,$($1.build_dir)/$(call mk.unpack,$(basename $(notdir $(ps))).o),$ $(call rule.compile-cc,$1,$o,$s)$ )$ )$ )


$(foreach ps,$ $(call mk.pack,$($1.sources)),$ $(let $ s,$(call mk.unpack,$(ps)),$ $(let $ o,$($1.build_dir)/$(call mk.unpack,$(basename $(notdir $(ps))).o),$ $1.sources.objects += $o$ )$ )$ )


ifeq ($(ENABLE_PCH),yes)
$($1.sources.objects): $($($1.pch).pch-path)
endif

$1.objects += $($1.sources.objects)

§(call template.eval,§(call template-link.cc,$1,§($1.test-path),§($1.objects)))

.PHONY: install.$1
install.$1:: $($1.test-path)
	§(call shell.title,Install $1)
	§(call shell.install.program,§($1.test-path),§(DESTDIR)§(bindir))

.PHONY: uninstall.$1
uninstall.$1::
	§(call shell.title,Uninstall $1)
	§(call shell.rm,§(DESTDIR)§(bindir)/§($1.test-name)


# END template-test $1
endef
# environment
VSCODE_GIT_EDITOR_NODE = /usr/share/code/code
# environment
NUTSHELL_SESSION = nutsh-5646
# makefile (from 'Makefile', line 125)
libak-so.build_dir = $(build_dir)/libak-so
# makefile (from 'Makefile', line 97)
libak_sync.lib-dir := build/libak_sync/
# makefile (from 'Makefile', line 119)
pch2.source = $(source_dir)/precompiled.hpp
# makefile
CURDIR := /home/roby/Workspace/coro/effect/libak
# makefile (from 'linux-clang.mk', line 375)
default.CXX = $(CXX)
# makefile (from 'linux-clang.mk', line 1300)
pch.CPPFLAGS = $(CPPFLAGS) -x c++-header
# makefile (from 'Makefile', line 60)
libak_base.lib-path = $(libak_base.build_dir)/$(libak_base.lib-name).a
# environment
LESSOPEN = | /usr/bin/lesspipe %s
# makefile (from 'linux-clang.mk', line 240)
std.notdir = $(call mk.unpack,$(notdir $(call mk.pack,$1)))
# automatic
*D = $(patsubst %/,%,$(dir $*))
# environment
BUN_INSTALL = /home/roby/.bun
# makefile (from 'Makefile', line 125)
libak-so.lib-dir := build/libak-so/
# makefile (from 'Makefile', line 66)
test_base.sources.objects := /test_dlist.o test_base.sources.objects += /test_timer.o test_base.sources.objects += /test_gtest.o
# environment
MFLAGS = -prRs -Otarget --warn-undefined-variables
# environment
SSH_AUTH_SOCK = /run/user/1000/gcr/ssh
# makefile (from 'linux-clang.mk', line 71)
.SHELLFLAGS := -eu -o pipefail -c
# makefile (from 'Makefile', line 72)
libak_alloc.build_dir = $(build_dir)/libak_alloc
# makefile (from 'Makefile', line 103)
test_sync.test-name = test_sync
# makefile (from 'Makefile', line 97)
libak_sync.out.LDLIBS := -lak_sync
# makefile (from 'Makefile', line 97)
libak_sync.CXXFLAGS = $(CXXFLAGS)
# makefile (from 'Makefile', line 60)
libak_base.out.LDLIBS := -lak_base
# makefile (from 'linux-clang.mk', line 616)
rule.compile-cc = $(let object,$2,$(let source,$3,$(rule.compile-cc._impl)))
# automatic
+D = $(patsubst %/,%,$(dir $+))
# makefile (from 'Makefile', line 97)
libak_sync.lib-name = libak_sync
# environment
GIT_ASKPASS = /usr/share/code/resources/app/extensions/git/dist/askpass.sh
# makefile (from 'Makefile', line 102)
test_sync.LDLIBS := -lgtest_main -lgtest -luring -lak_sync -lak_runtime -lak_alloc -lak_base
# makefile (from 'Makefile', line 119)
pch2.build_dir = $(build_dir)/pch2
# environment
XDG_SESSION_DESKTOP = gnome-xorg
# makefile (from 'linux-clang.mk', line 375)
default.CXXFLAGS = $(CXXFLAGS)
# makefile (from 'build/libak_base/base_version.o.d', line 1)
MAKEFILE_LIST := Makefile linux-clang.mk linux-clang.mk .config.mk build/pch/precompiled.pch.d build/libak_base/base_timer.o.d build/libak_base/base_version.o.d
# makefile (from 'Makefile', line 97)
libak_sync.build_dir = $(build_dir)/libak_sync
# automatic
@F = $(notdir $@)
# makefile (from 'linux-clang.mk', line 105)
\ := 
# makefile (from 'linux-clang.mk', line 377)
TARGET_ARCH := -march=x86-64-v3
# makefile (from 'Makefile', line 88)
test_runtime.LDFLAGS := -L'build/libak_base/' -L'build/libak_alloc/' -L'build/libak_runtime/'
# makefile (from 'Makefile', line 125)
libak-so.out.LDLIBS := -lak
# environment
XDG_SESSION_TYPE = x11
# makefile (from 'Makefile', line 66)
test_base.objects = 
# automatic
?D = $(patsubst %/,%,$(dir $?))
# makefile (from 'linux-clang.mk', line 378)
DESTDIR := dist
# makefile (from 'linux-clang.mk', line 377)
RANLIB := ranlib
# makefile (from 'linux-clang.mk', line 617)
rule.compile-cc._impl = $ $(call mk.rule,$ $(object): $(source) $(shell echo $(CXX) $($1.CXXFLAGS) $($1.CPPFLAGS) $($1.TARGET_ARCH) -MMD -MP -MF \'$(object).d\' -o \'$(object)\' -c \'$(source)\' | cmp -s - $(object).cmd || echo FORCE) | $(call std.dir,$(object))/.,$ $(call shell.compile-cc,$1,$2,$3)$ )
# makefile (from 'linux-clang.mk', line 426)
ansi.blue := [0;34m
# makefile (from 'Makefile', line 121)
libak-so.sources := src/ak/base/base_timer.cpp src/ak/base/base_version.cpp src/ak/alloc/alloc_freeblock_tree.cpp src/ak/alloc/alloc_dump.cpp src/ak/alloc/alloc_freeblock_list.cpp src/ak/alloc/alloc_check_invariants.cpp src/ak/alloc/alloc_table.cpp src/ak/runtime/runtime_debug_io.cpp src/ak/runtime/runtime_thread_ops.cpp src/ak/runtime/runtime_thread_context.cpp src/ak/runtime/runtime_debug_task.cpp src/ak/runtime/runtime_boot.cpp src/ak/runtime/runtime_kernel.cpp src/ak/runtime/runtime_io_prep.cpp src/ak/runtime/runtime_task.cpp src/ak/sync/sync_event.cpp
# makefile (from 'Makefile', line 60)
libak_base.CPPFLAGS = $(CPPFLAGS) $($(libak_base.pch).out.CPPFLAGS)
# makefile (from 'Makefile', line 64)
test_base.LDFLAGS := -L'build/libak_base/'
# makefile (from '.config.mk', line 2)
CONFIG := debug
# makefile (from 'linux-clang.mk', line 377)
INSTALL := install
# makefile (from 'Makefile', line 90)
test_runtime.test-path = /
# makefile (from 'Makefile', line 125)
libak-so.CPPFLAGS = $(CPPFLAGS) $($(libak-so.pch).out.CPPFLAGS)
# makefile (from 'linux-clang.mk', line 608)
mk.undefer = $(subst §,$$,$1)
# makefile (from 'Makefile', line 84)
libak_runtime.lib-path = $(libak_runtime.build_dir)/$(libak_runtime.lib-name).a
# environment
SESSION_MANAGER = local/batman:@/tmp/.ICE-unix/3606,unix/batman:/tmp/.ICE-unix/3606
# makefile (from 'linux-clang.mk', line 145)
shell.escape = $(subst $(char.space),\$(char.space),$(call shell.escape._rec,$1,$(shell.sensitive-chars)))
# automatic
*F = $(notdir $*)
# makefile (from 'linux-clang.mk', line 47)
mk.next-phase.load = $(eval include $(mk.next-phase.makefile))
# makefile (from 'Makefile', line 84)
libak_runtime.build_dir = $(build_dir)/libak_runtime
# makefile (from 'Makefile', line 113)
libak-a.CPPFLAGS = $(CPPFLAGS) $($(libak-a.pch).out.CPPFLAGS)
# makefile (from 'Makefile', line 65)
test_base.LDLIBS := -lgtest_main -lgtest -lak_base
# environment
MANPATH = /home/roby/.local/share/man:/home/roby/.local/man:
# makefile (from 'linux-clang.mk', line 156)
shell.quote = '$(subst ','\'',$1)'
# makefile (from 'Makefile', line 119)
pch2.out.CPPFLAGS = -include-pch $(call shell.quote.var,$(pch2.pch-path))
# environment
QT_QPA_PLATFORMTHEME = qt5ct
# makefile (from 'Makefile', line 113)
libak-a.build_dir = $(build_dir)/libak-a
# environment
CHROME_DESKTOP = code.desktop
# makefile (from 'Makefile', line 71)
libak_alloc.sources := src/ak/alloc/alloc_freeblock_tree.cpp src/ak/alloc/alloc_dump.cpp src/ak/alloc/alloc_freeblock_list.cpp src/ak/alloc/alloc_check_invariants.cpp src/ak/alloc/alloc_table.cpp
# environment
DBUS_SESSION_BUS_ADDRESS = unix:path=/run/user/1000/bus
# makefile (from 'Makefile', line 72)
libak_alloc.sources.objects := build/libak_alloc/alloc_freeblock_tree.o build/libak_alloc/alloc_dump.o build/libak_alloc/alloc_freeblock_list.o build/libak_alloc/alloc_check_invariants.o build/libak_alloc/alloc_table.o
# makefile (from 'linux-clang.mk', line 463)
shell.trace = printf -- '[$(ansi.cyan)%s$(ansi.reset)] %s\n' "$$$$" "$1"
# makefile (from '.config.mk', line 5)
COLOR := yes
# automatic
<D = $(patsubst %/,%,$(dir $<))
# makefile (from 'Makefile', line 90)
test_runtime.TARGET_ARCH = -march=x86-64-v3
# makefile (from 'Makefile', line 77)
test_alloc.pch = pch
# makefile (from 'Makefile', line 113)
libak-a.TARGET_ARCH = $(TARGET_ARCH)
# makefile (from 'Makefile', line 66)
test_base.build_dir = build/test_base
# makefile (from 'Makefile', line 77)
test_alloc.test-name = test_alloc
# makefile (from 'Makefile', line 113)
libak-a.out.LDLIBS := -lak
# default
MAKE_HOST := x86_64-pc-linux-gnu
# environment
ANDROID_HOME = /home/roby/Android/Sdk
# environment
GNOME_DESKTOP_SESSION_ID = this-is-deprecated
# makefile (from 'linux-clang.mk', line 70)
SHELL := /bin/bash
# default
MAKECMDGOALS := build/test_base/test_dlist.o
# environment
XMODIFIERS = @im=ibus
# environment
LD_LIBRARY_PATH = /home/roby/.local/lib
# makefile (from 'linux-clang.mk', line 611)
mk.eval = $(eval $(call mk.undefer,$1))
# makefile (from 'Makefile', line 97)
libak_sync.pch = pch
# makefile (from 'linux-clang.mk', line 347)
insp = $(error $1 = '$($1)' $(origin $1) : $(flavor $1))
# environment
MANGOHUD = 1
# environment
GJS_DEBUG_TOPICS = JS ERROR;JS LOG
# environment
SHLVL = 1
# makefile (from 'linux-clang.mk', line 259)
shell.install.program = $ $(let src-program,$(call shell.quote.var,$1),$ $(let dst-program,$(call shell.quote.var,$2/$(call std.notdir,$1)),$ $(call shell.install.dir,$2)$(char.nl)$ $(call shell.trace,INSTALL_PROGRAM $(src-program) $(dst-program))$(char.nl)$ $(INSTALL_PROGRAM) $(src-program) $(dst-program)$ )$ )
# makefile (from 'Makefile', line 97)
libak_sync.CPPFLAGS = $(CPPFLAGS) $($(libak_sync.pch).out.CPPFLAGS)
# makefile (from 'linux-clang.mk', line 107)
ARGS := 
# makefile (from 'Makefile', line 77)
test_alloc.sources.objects := /test_freeblock.o test_alloc.sources.objects += /test_freeblock_list_search.o test_alloc.sources.objects += /test_freeblock_list.o test_alloc.sources.objects += /test_freeblock_tree.o test_alloc.sources.objects += /test_split.o test_alloc.sources.objects += /test_defragment.o
# environment
GCC_COLORS = error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01
# makefile (from 'Makefile', line 72)
libak_alloc.lib-dir := build/libak_alloc/
# environment
MAKELEVEL := 0
# makefile (from 'linux-clang.mk', line 45)
mk.next-phase.makefile := linux-clang.mk
# makefile (from 'linux-clang.mk', line 95)
char.backslash := \
# makefile (from 'linux-clang.mk', line 388)
config-file := .config.mk
# makefile (from 'Makefile', line 125)
libak-so.out.LDFLAGS := -L'build/libak-so/'
# makefile (from 'linux-clang.mk', line 101)
char.esc := 
# default
MAKE = $(MAKE_COMMAND)
# 'override' directive (from 'Makefile', line 38)
mk.next-phase := setup
# makefile (from 'Makefile', line 97)
libak_sync.out.LDFLAGS := -L'build/libak_sync/'
# environment
PATH = /home/roby/.local/bin:/home/roby/.cargo/bin:/home/roby/Android/Sdk/platform-tools:/home/roby/Android/Sdk/tools:/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games:/home/roby/.config/Code/User/globalStorage/github.copilot-chat/debugCommand
# makefile (from 'linux-clang.mk', line 172)
shell.quote.list = $(foreach i,$(call mk.pack,$1),$(call shell.quote.var,$(call mk.unpack,$i)))
# default
MAKEFILES := 
# environment
VSCODE_GIT_IPC_HANDLE = /run/user/1000/vscode-git-f6d4e44bd1.sock
# environment
LANGUAGE = en_US:en
# environment
VSCODE_GIT_ASKPASS_NODE = /usr/share/code/code
# makefile (from 'linux-clang.mk', line 650)
define template-compile.cc._impl
# START template-compile.cc

.PHONY: clean.$1
clean.$1::
	declare -a deletables=()
	for i in $(call shell.quote.var,$(object)) $(call shell.quote.var,$(object).d); do
		if [[ -e §§i ]]; then
			deletables+=( "§§i" )
		fi
	done
	if [[ §§{#deletables[@]} == 0 ]]; then
		exit
	fi
	§(call shell.rm,$(object) $(object).d)

.PRECIOUS: $(object)
$(object): $(source) | $(call std.dir,$(object)).
	§(call shell.title,CXX -o $(call shell.quote.var,$(object)) -c $(call shell.quote.var,$(source)))
	§(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF $(call shell.quote.var,$(object).d) -o $(call shell.quote.var,$(object)) -c $(call shell.quote.var,$(source))

-include $(object).d

# END template-compile.cc
endef
# environment
LC_MONETARY = it_IT.UTF-8
# automatic
^F = $(notdir $^)
# makefile (from 'Makefile', line 63)
test_base.sources := test/base/test_dlist.cpp test/base/test_timer.cpp test/base/test_gtest.cpp
# makefile (from 'Makefile', line 66)
test_base.CPPFLAGS = -I'src' -DGTEST_HAS_PTHREAD=1 
# makefile (from 'Makefile', line 103)
test_sync.objects = 
# makefile (from 'Makefile', line 125)
libak-so.link1 := build/libak-so/libak.so
# makefile (from 'Makefile', line 103)
test_sync.CPPFLAGS = -I'src' -DGTEST_HAS_PTHREAD=1 
# makefile (from 'linux-clang.mk', line 207)
mk.unescape = $(subst \\,\,$(subst \$(char.space),$(char.space),$1))
# makefile (from 'Makefile', line 72)
libak_alloc.TARGET_ARCH = $(TARGET_ARCH)
# makefile (from 'linux-clang.mk', line 48)
define mk.load
override mk.next-phase := $1
include $(mk.next-phase.makefile)
endef
# makefile (from 'linux-clang.mk', line 379)
LDLIBS := 
# makefile (from 'linux-clang.mk', line 378)
bindir := /bin
# makefile (from 'linux-clang.mk', line 698)
define template-link.cc._impl
# START template-link,$1,$(linked),$(inputs)

.PHONY: clean.$1
clean.$1::
	declare -a deletables=()
	for i in $(call shell.quote.var,$(linked)) $(call shell.quote.var,$(linked).d); do
		if [[ -e §§i ]]; then
			deletables+=( "§§i" )
		fi
	done
	if [[ §§{#deletables[@]} == 0 ]]; then
		exit
	fi
	§(call shell.rm,$(linked) $(linked).d)


.PRECIOUS: $(linked)
$(linked): $(inputs) | $(call std.dir,$(linked)). 
	§(call shell.title,LINK -o $(call shell.quote.var,$(linked)) $(call shell.quote.list,$(inputs)) §($1.LDLIBS))
	§(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.LDFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF $(call shell.quote.var,$(linked).d) -o $(call shell.quote.var,$(linked)) $(call shell.quote.list,$(inputs)) §($1.LDLIBS) # end

-include $(linked).d
# END template-link.cc
endef
# environment
LC_TIME = it_IT.UTF-8
# environment
VSCODE_GIT_ASKPASS_MAIN = /usr/share/code/resources/app/extensions/git/dist/askpass-main.js
# makefile (from 'linux-clang.mk', line 438)
ansi.reset := [0m
# makefile (from 'linux-clang.mk', line 378)
datarootdir := /share
# makefile (from 'linux-clang.mk', line 336)
template.eval = $(eval $(call template.unescape,$1))
# makefile (from 'Makefile', line 125)
libak-so.sources.objects := build/libak-so/base_timer.o build/libak-so/base_version.o build/libak-so/alloc_freeblock_tree.o build/libak-so/alloc_dump.o build/libak-so/alloc_freeblock_list.o build/libak-so/alloc_check_invariants.o build/libak-so/alloc_table.o build/libak-so/runtime_debug_io.o build/libak-so/runtime_thread_ops.o build/libak-so/runtime_thread_context.o build/libak-so/runtime_debug_task.o build/libak-so/runtime_boot.o build/libak-so/runtime_kernel.o build/libak-so/runtime_io_prep.o build/libak-so/runtime_task.o build/libak-so/sync_event.o
# makefile (from 'Makefile', line 90)
test_runtime.CXXFLAGS = -Wall -Wextra -std=c++2c -fno-exceptions -fno-rtti -fdiagnostics-color=always -mavx2 -mbmi -mbmi2 -g -O0
# environment
INVOCATION_ID = ab3908d8ba684d609ab8dd3564c1453d
# makefile (from 'linux-clang.mk', line 1037)
define template-lib
# START template-lib
$1.build_dir ?= §(build_dir)/$1
$1.lib-name ?= $1
$1.lib-path ?= §($1.build_dir)/§($1.lib-name).a
$1.sources ?=
$1.objects ?=
$1.CXXFLAGS ?= §(CXXFLAGS)
$1.CPPFLAGS ?= §(CPPFLAGS)
$1.TARGET_ARCH ?= §(TARGET_ARCH)
$1.pch ?= pch
$1.CPPFLAGS += §(§($1.pch).out.CPPFLAGS)
$1.lib-dir := §(call std.dir,§($1.lib-path))
$1.out.LDFLAGS := -L§(call shell.quote.var,§($1.lib-dir))
$1.out.LDLIBS := -l§(patsubst lib%,%,§($1.lib-name))


.PHONY: $1
$1:: §($1.lib-path)

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote.var,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote.var,§($1.build_dir)) == §(call shell.quote.var,§(build_dir)) ]] && exit
	§(call shell.title,Clean static lib $1)
	§(call shell.rmdir,§($1.build_dir))

$1.sources.objects := §(foreach ps,$ §(call mk.pack,§($1.sources)),$ §(let $ s,§(call mk.unpack,§(ps)),$ §(let $ o,§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(ps))).o),$ §(call template.eval,§(call template-compile.cc,$1,§s,§o))$ §o$ )$ )$ )

ifeq ($(ENABLE_PCH),yes)
§($1.sources.objects): §(§($1.pch).pch-path)
endif

$1.objects += §($1.sources.objects)

.PRECIOUS: §($1.lib-path)
§($1.lib-path): §($1.objects) | §(call std.dir,§($1.lib-path)).
	§(call shell.title,AR rcs §($1.lib-path) §($1.objects))
	§(AR) rcs §($1.lib-path) §($1.objects)
	§(RANLIB) §($1.lib-path)

.PHONY: install.$1
install.$1:: §($1.lib-path)
	§(call shell.title,Install $1)
	§(call shell.install.program,§($1.lib-path),§(DESTDIR)§(libdir))

.PHONY: uninstall.$1
uninstall.$1::
	§(call shell.title,Uninstall $1)
	§(call shell.rm,§(DESTDIR)§(libdir)/§(call std.notdir,§($1.lib-path)))

# END template-lib
endef
# makefile (from 'Makefile', line 113)
libak-a.lib-path = $(libak-a.build_dir)/$(libak-a.lib-name).a
# makefile (from 'linux-clang.mk', line 245)
std.join = $(call mk.unpack,$(join $(call mk.pack,$1),$(call mk.pack,$2)))
# makefile (from 'Makefile', line 125)
libak-so.soname = $(libak-so.lib-name).so.$(word 1,$(subst ., ,$(libak-so.version)))
# environment
NODE_VERSION_PREFIX = v
# environment
USERNAME = roby
# makefile (from 'linux-clang.mk', line 697)
template-link.cc = $(let 1,$1,$(let linked,$2,$(let inputs,$3,$(template-link.cc._impl))))
# environment
TERM_PROGRAM = vscode
# makefile (from 'Makefile', line 90)
test_runtime.pch = pch
# makefile (from 'linux-clang.mk', line 93)
char.rparen := )
# makefile (from 'Makefile', line 6)
source_dir := src
# environment
LESSCLOSE = /usr/bin/lesspipe %s %s
# environment
LC_TELEPHONE = it_IT.UTF-8
# makefile (from 'linux-clang.mk', line 378)
exec_prefix := 
# automatic
?F = $(notdir $?)
# makefile (from 'Makefile', line 97)
libak_sync.sources.objects := build/libak_sync/sync_event.o
# makefile (from 'linux-clang.mk', line 97)
char.nl := $(subst ,,
)
# makefile (from 'Makefile', line 97)
libak_sync.lib-path = $(libak_sync.build_dir)/$(libak_sync.lib-name).a
# makefile (from 'Makefile', line 119)
pch2.pch-path = $(pch2.build_dir)/precompiled.pch
# makefile (from 'Makefile', line 72)
libak_alloc.CXXFLAGS = $(CXXFLAGS)
# environment
XDG_CURRENT_DESKTOP = GNOME
# environment
define NUTSHELL_STACK
 5646 /bin/bash
 5998 /usr/bin/bash

endef
# makefile (from 'Makefile', line 113)
libak-a.sources = 
# makefile (from 'Makefile', line 90)
test_runtime.build_dir = build/test_runtime
# environment
LS_COLORS = rs=0:di=01;34:ln=01;36:mh=00:pi=40;33:so=01;35:do=01;35:bd=40;33;01:cd=40;33;01:or=40;31;01:mi=00:su=37;41:sg=30;43:ca=00:tw=30;42:ow=34;42:st=37;44:ex=01;32:*.7z=01;31:*.ace=01;31:*.alz=01;31:*.apk=01;31:*.arc=01;31:*.arj=01;31:*.bz=01;31:*.bz2=01;31:*.cab=01;31:*.cpio=01;31:*.crate=01;31:*.deb=01;31:*.drpm=01;31:*.dwm=01;31:*.dz=01;31:*.ear=01;31:*.egg=01;31:*.esd=01;31:*.gz=01;31:*.jar=01;31:*.lha=01;31:*.lrz=01;31:*.lz=01;31:*.lz4=01;31:*.lzh=01;31:*.lzma=01;31:*.lzo=01;31:*.pyz=01;31:*.rar=01;31:*.rpm=01;31:*.rz=01;31:*.sar=01;31:*.swm=01;31:*.t7z=01;31:*.tar=01;31:*.taz=01;31:*.tbz=01;31:*.tbz2=01;31:*.tgz=01;31:*.tlz=01;31:*.txz=01;31:*.tz=01;31:*.tzo=01;31:*.tzst=01;31:*.udeb=01;31:*.war=01;31:*.whl=01;31:*.wim=01;31:*.xz=01;31:*.z=01;31:*.zip=01;31:*.zoo=01;31:*.zst=01;31:*.avif=01;35:*.jpg=01;35:*.jpeg=01;35:*.jxl=01;35:*.mjpg=01;35:*.mjpeg=01;35:*.gif=01;35:*.bmp=01;35:*.pbm=01;35:*.pgm=01;35:*.ppm=01;35:*.tga=01;35:*.xbm=01;35:*.xpm=01;35:*.tif=01;35:*.tiff=01;35:*.png=01;35:*.svg=01;35:*.svgz=01;35:*.mng=01;35:*.pcx=01;35:*.mov=01;35:*.mpg=01;35:*.mpeg=01;35:*.m2v=01;35:*.mkv=01;35:*.webm=01;35:*.webp=01;35:*.ogm=01;35:*.mp4=01;35:*.m4v=01;35:*.mp4v=01;35:*.vob=01;35:*.qt=01;35:*.nuv=01;35:*.wmv=01;35:*.asf=01;35:*.rm=01;35:*.rmvb=01;35:*.flc=01;35:*.avi=01;35:*.fli=01;35:*.flv=01;35:*.gl=01;35:*.dl=01;35:*.xcf=01;35:*.xwd=01;35:*.yuv=01;35:*.cgm=01;35:*.emf=01;35:*.ogv=01;35:*.ogx=01;35:*.aac=00;36:*.au=00;36:*.flac=00;36:*.m4a=00;36:*.mid=00;36:*.midi=00;36:*.mka=00;36:*.mp3=00;36:*.mpc=00;36:*.ogg=00;36:*.ra=00;36:*.wav=00;36:*.oga=00;36:*.opus=00;36:*.spx=00;36:*.xspf=00;36:*~=00;90:*#=00;90:*.bak=00;90:*.crdownload=00;90:*.dpkg-dist=00;90:*.dpkg-new=00;90:*.dpkg-old=00;90:*.dpkg-tmp=00;90:*.old=00;90:*.orig=00;90:*.part=00;90:*.rej=00;90:*.rpmnew=00;90:*.rpmorig=00;90:*.rpmsave=00;90:*.swp=00;90:*.tmp=00;90:*.ucf-dist=00;90:*.ucf-new=00;90:*.ucf-old=00;90:
# automatic
+F = $(notdir $+)
# makefile (from 'linux-clang.mk', line 1265)
define template-pch
# START template-pch

$1.build_dir ?= §(build_dir)/$1
$1.source ?= §(source_dir)/precompiled.hpp
$1.pch-path ?= §($1.build_dir)/precompiled.pch
$1.CPPFLAGS ?= §(CPPFLAGS)
$1.CPPFLAGS += -x c++-header
$1.CXXFLAGS ?= §(CXXFLAGS)
$1.TARGET_ARCH ?= §(TARGET_ARCH)

ifeq ($(ENABLE_PCH),yes)
$1.out.CPPFLAGS ?= -include-pch §(call shell.quote.var,§($1.pch-path))
else
$1.out.CPPFLAGS :=
endif

.PHONY: $1
$1:: §($1.pch-path)

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote.var,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote.var,§($1.build_dir)) == §(call shell.quote.var,§(build_dir)) ]] && exit
	§(call shell.title,Clean pch $1)
	§(call shell.rmdir,§($1.build_dir))

§(call template.eval,§(call template-compile.cc,$1,§($1.source),§($1.pch-path)))

# END template-pch
endef
# environment
LESS = -R
# environment
DESKTOP_SESSION = gnome-xorg
# environment
ORIGINAL_XDG_CURRENT_DESKTOP = GNOME
# makefile (from 'linux-clang.mk', line 378)
includedir := /include
# makefile (from 'linux-clang.mk', line 290)
shell.ln-s = $ $(let target,$(call shell.quote.var,$1),$ $(let link-path,$(call shell.quote.var,$2),$ $(call shell.trace,ls -s $(target) $(link-path))$(char.nl)$ ln -sf $(target) $(link-path)$ )$ )
# 'override' directive
GNUMAKEFLAGS := 
# makefile (from 'linux-clang.mk', line 434)
ansi.hiblue := [1;34m
# environment
CPATH = /home/roby/.local/include
# environment
LOGNAME = roby
# makefile (from 'linux-clang.mk', line 328)
template.unescape = $(subst §,$$,$1)
# makefile (from 'linux-clang.mk', line 422)
ansi.black := [0;30m
# makefile (from 'linux-clang.mk', line 247)
std.abspath = $(call mk.unpack,$(abspath $(call mk.pack,$1)))
# makefile (from 'Makefile', line 125)
libak-so.link2 := build/libak-so/libak.so.0
# makefile (from 'Makefile', line 90)
test_runtime.CPPFLAGS = -I'src' -DGTEST_HAS_PTHREAD=1 
# makefile (from 'Makefile', line 84)
libak_runtime.out.LDLIBS := -lak_runtime
# makefile (from 'linux-clang.mk', line 379)
CPPFLAGS := -I'src' -DGTEST_HAS_PTHREAD=1 
# makefile (from 'linux-clang.mk', line 437)
ansi.hiwhite := [1;37m
# environment
GIO_LAUNCHED_DESKTOP_FILE = /usr/share/applications/code.desktop
# makefile (from 'Makefile', line 60)
libak_base.TARGET_ARCH = $(TARGET_ARCH)
# makefile (from 'Makefile', line 72)
libak_alloc.out.LDFLAGS := -L'build/libak_alloc/'
# makefile (from 'Makefile', line 118)
pch2.CXXFLAGS := -Wall -Wextra -std=c++2c -fno-exceptions -fno-rtti -fdiagnostics-color=always -mavx2 -mbmi -mbmi2 -g -O0 -fPIC
# makefile (from 'linux-clang.mk', line 1300)
pch.CXXFLAGS = $(CXXFLAGS)
# makefile (from 'linux-clang.mk', line 77)
.DEFAULT_GOAL := all
# makefile (from 'linux-clang.mk', line 423)
ansi.red := [0;31m
# environment
SYSTEMD_EXEC_PID = 3635
# makefile (from 'linux-clang.mk', line 377)
RM := rm -f
# makefile (from 'linux-clang.mk', line 649)
template-compile.cc = $(let 1,$1,$(let source,$2,$(let object,$3,$(template-compile.cc._impl))))
# makefile (from 'linux-clang.mk', line 253)
shell.install.dir = $ $(let dir-list,$(call shell.quote.list,$1),$ $(call shell.trace,INSTALL -d $(dir-list))$(char.nl)$ $(INSTALL) -d $(dir-list)$ )
# makefile (from 'linux-clang.mk', line 775)
define template-executable
# START template-executable $1
$1.build_dir ?= §(build_dir)/$1
$1.exe-name ?= $1
$1.exe-path ?= §($1.build_dir)/§($1.exe-name)
$1.sources ?=
$1.objects ?=
$1.TARGET_ARCH ?= §(TARGET_ARCH)
$1.CXXFLAGS ?= §(CXXFLAGS)
$1.CPPFLAGS ?= §(CPPFLAGS)
$1.LDFLAGS ?= §(LDFLAGS)
$1.LDLIBS ?= §(LDLIBS)
$1.pch ?= pch
$1.CPPFLAGS += §(§($1.pch).out.CPPFLAGS)

.PHONY: $1
$1:: §($1.exe-path)
	§(call shell.title,Running executable $1: §(call shell.quote.var,§($1.exe-path))
	declare -i error_code=0
	set +e
ifeq (§(RUN_WITH_VALGRIND),yes)
	valgrind --leak-check=full --show-leak-kinds=all  --track-origins=yes --show-reachable=yes --error-exitcode=1 -- §(call shell.quote.var,§($1.exe-path)) §(ARGS)
else
	§(call shell.quote.var,§($1.exe-path)) §(ARGS)
endif
	error_code=§§?
	set -e
	if (( error_code == 0 )); then
		§(call shell.trace,§(ansi.green)SUCCESS §(call shell.quote.var,§($1.exe-path))§(ansi.reset))
	else
		§(call shell.trace,§(ansi.red)FAILURE (§§error_code) §(call shell.quote.var,§($1.exe-path))§(ansi.reset))
	fi

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote.var,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote.var,§($1.build_dir)) == §(call shell.quote.var,§(build_dir)) ]] && exit
	§(call shell.title,Clean executable $1)
	§(call shell.rmdir,§($1.build_dir))

$1.sources.objects := §(foreach ps,$ §(call mk.pack,§($1.sources)),$ §(let $ s,§(call mk.unpack,§(ps)),$ §(let $ o,§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(ps))).o),$ §(call template.eval,§(call template-compile.cc,$1,§s,§o))$ §o$ )$ )$ )

ifeq ($(ENABLE_PCH),yes)
§($1.sources.objects): §(§($1.pch).pch-path)
endif

$1.objects += §($1.sources.objects)

§(call template.eval,§(call template-link.cc,$1,§($1.exe-path),§($1.objects)))

.PHONY: install.$1
install.$1:: §($1.exe-path)
	§(call shell.title,Install $1)
	§(call shell.install.program,§($1.exe-path),§(DESTDIR)§(bindir))

.PHONY: uninstall.$1
uninstall.$1::
	§(call shell.title,Uninstall $1)
	§(call shell.rm,§(DESTDIR)§(bindir)/§($1.exe-name))


# END template-executable
endef
# makefile (from 'Makefile', line 113)
libak-a.out.LDFLAGS := -L'build/libak-a/'
# makefile (from 'Makefile', line 96)
libak_sync.sources := src/ak/sync/sync_event.cpp
# makefile (from 'linux-clang.mk', line 243)
std.addsuffix = $(call mk.unpack,$(addsuffix $1,$(call mk.pack,$2)))
# makefile (from 'linux-clang.mk', line 462)
shell.title = printf -- '[$(ansi.cyan)%s$(ansi.reset)] $(ansi.white)%s$(ansi.reset)\n' "$$$$" "$1"
# makefile (from 'Makefile', line 66)
test_base.CXXFLAGS = -Wall -Wextra -std=c++2c -fno-exceptions -fno-rtti -fdiagnostics-color=always -mavx2 -mbmi -mbmi2 -g -O0
# environment
EDITOR = sensible-editor
# environment
DISPLAY = :1
# environment
USER = roby
# makefile (from 'linux-clang.mk', line 220)
mk.pack = $(subst \$(char.space),&+,$(subst &,&&,$1))
# makefile (from 'Makefile', line 84)
libak_runtime.sources.objects := build/libak_runtime/runtime_debug_io.o build/libak_runtime/runtime_thread_ops.o build/libak_runtime/runtime_thread_context.o build/libak_runtime/runtime_debug_task.o build/libak_runtime/runtime_boot.o build/libak_runtime/runtime_kernel.o build/libak_runtime/runtime_io_prep.o build/libak_runtime/runtime_task.o
# makefile (from 'Makefile', line 100)
test_sync.sources := test/sync/test_event.cpp
# environment
LIBRARY_PATH = /home/roby/.local/lib
# default
MAKE_VERSION := 4.4.1
# 'override' directive
.SHELLSTATUS := 0
# makefile (from 'linux-clang.mk', line 1300)
pch.build_dir = $(build_dir)/pch
# makefile (from 'Makefile', line 103)
test_sync.pch = pch
# makefile (from 'Makefile', line 125)
libak-so.LDFLAGS = $(LDFLAGS) -shared -Wl,-soname,$(libak-so.soname)
# makefile (from 'linux-clang.mk', line 432)
ansi.higreen := [1;32m
# makefile (from '.config.mk', line 11)
ENABLE_PCH := yes
# makefile (from 'linux-clang.mk', line 379)
LDFLAGS := 
# environment
MANAGERPID = 3165
# makefile (from 'linux-clang.mk', line 607)
mk.defer = $(subst $$,§,$1)
# environment
BUN_INSPECT_CONNECT_TO = unix:///tmp/6cfroykgh3.sock
# makefile (from 'linux-clang.mk', line 251)
std.strip = $(call mk.unpack,$(abspath $(strip mk.pack,$1)))
# environment
LC_MEASUREMENT = it_IT.UTF-8
# makefile (from 'Makefile', line 84)
libak_runtime.TARGET_ARCH = $(TARGET_ARCH)
# environment
GIO_LAUNCHED_DESKTOP_FILE_PID = 5551
# environment
VSCODE_GIT_EDITOR_EXTRA_ARGS = 
# makefile (from 'linux-clang.mk', line 147)
shell.sensitive-chars := \ $$ ` ( ) * ? [ ] ' " & | ; < > { } ~
# makefile (from 'linux-clang.mk', line 595)
shell.compile-cc = $ $(let object,$(call shell.quote.var,$2),$ $(let source,$(call shell.quote.var,$3),$ $(call shell.title,CXX -o $(object) -c $(source))$(char.nl)$ $(CXX) $($1.CXXFLAGS) $($1.CPPFLAGS) $($1.TARGET_ARCH) -MMD -MP -MF $(object).d -o $(object) -c $(source)$(char.nl)$ echo $(CXX) $($1.CXXFLAGS) $($1.CPPFLAGS) $($1.TARGET_ARCH) -MMD -MP -MF \'$(object).d\' -o \'$(object)\' -c \'$(source)\' > $(object).cmd$(char.nl)$ )$ )
# makefile (from 'linux-clang.mk', line 429)
ansi.white := [0;37m
# makefile (from 'linux-clang.mk', line 1300)
pch.source = $(source_dir)/precompiled.hpp
# makefile (from 'Makefile', line 60)
libak_base.sources.objects := build/libak_base/base_timer.o build/libak_base/base_version.o
# makefile (from 'Makefile', line 84)
libak_runtime.pch = pch
# makefile (from 'Makefile', line 112)
libak-a.lib-name := libak
# makefile (from 'Makefile', line 111)
libak-a.objects := build/libak_base/base_timer.o build/libak_base/base_version.o build/libak_alloc/alloc_freeblock_tree.o build/libak_alloc/alloc_dump.o build/libak_alloc/alloc_freeblock_list.o build/libak_alloc/alloc_check_invariants.o build/libak_alloc/alloc_table.o build/libak_runtime/runtime_debug_io.o build/libak_runtime/runtime_thread_ops.o build/libak_runtime/runtime_thread_context.o build/libak_runtime/runtime_debug_task.o build/libak_runtime/runtime_boot.o build/libak_runtime/runtime_kernel.o build/libak_runtime/runtime_io_prep.o build/libak_runtime/runtime_task.o build/libak_sync/sync_event.o
# environment
PAGER = less
# environment
_ = /usr/bin/make
# environment
LC_PAPER = it_IT.UTF-8
# makefile (from 'linux-clang.mk', line 91)
char.space := $(subst ,, )
# environment
XDG_RUNTIME_DIR = /run/user/1000
# makefile (from 'linux-clang.mk', line 277)
shell.rm = $ $(let file-list,$(call shell.quote.list,$1),$ $(call shell.trace,RM $(file-list))$(char.nl)$ $(RM) $(file-list)$ )
# environment
GPG_AGENT_INFO = /run/user/1000/gnupg/S.gpg-agent:0:1
# makefile (from 'Makefile', line 76)
test_alloc.LDLIBS := -lgtest_main -lgtest -lak_alloc -lak_base 
# makefile (from 'Makefile', line 66)
test_base.TARGET_ARCH = -march=x86-64-v3
# environment
COLORTERM = truecolor
# makefile (from 'Makefile', line 72)
libak_alloc.out.LDLIBS := -lak_alloc
# makefile (from 'Makefile', line 77)
test_alloc.objects = 
# makefile (from 'linux-clang.mk', line 117)
eq = $(let a,x$1,$(let b,x$2,$(and $(findstring $a,$b),$(findstring $b,$a))))
# makefile (from 'linux-clang.mk', line 375)
default.CPPFLAGS = $(CPPFLAGS)
# makefile (from 'linux-clang.mk', line 435)
ansi.himagenta := [1;35m
# makefile (from 'Makefile', line 113)
libak-a.pch = pch
# environment
JOURNAL_STREAM = 9:26026
# makefile (from 'Makefile', line 60)
libak_base.pch = pch
# makefile (from 'Makefile', line 103)
test_sync.test-path = /
# default
MAKE_TERMERR := /dev/pts/0
# makefile (from 'Makefile', line 84)
libak_runtime.out.LDFLAGS := -L'build/libak_runtime/'
# makefile (from 'Makefile', line 113)
libak-a.CXXFLAGS = $(CXXFLAGS)
# environment
XDG_SESSION_CLASS = user
# makefile (from 'linux-clang.mk', line 96)
char.tab := $(subst ,,	)
# makefile (from 'Makefile', line 8)
dist_dir := dist
# makefile (from 'linux-clang.mk', line 606)
mk.rule.recipe = $(subst $(char.nl),$(char.nl)$(char.tab),$(call mk.defer,$1))$(char.nl)
# makefile (from 'linux-clang.mk', line 90)
nothing := 
# environment
HOME = /home/roby
# environment
QT_IM_MODULE = ibus
# makefile (from 'linux-clang.mk', line 377)
INSTALL_DATA := install -m 644
# makefile (from 'Makefile', line 84)
libak_runtime.objects = $(libak_runtime.sources.objects)
# makefile (from 'linux-clang.mk', line 299)
shell.find = $ $(let dir,$(call shell.quote.var,$1),$ $(let args,$(call shell.quote.list,$2),$ $(shell find $(dir) \( $(args) \) -print | while IFS= read -r f; do printf '%q\n' "$$f"; done)$ )$ )
# makefile (from 'linux-clang.mk', line 182)
mk.add-prereq = $(eval $1: $2)
# makefile (from 'linux-clang.mk', line 375)
default.LDLIBS = $(LDLIBS)
# environment
TERM = xterm-256color
# makefile (from 'Makefile', line 90)
test_runtime.sources.objects := /test_alloc.o test_runtime.sources.objects += /test_ak.o test_runtime.sources.objects += /test_file_io.o
# makefile (from 'linux-clang.mk', line 431)
ansi.hired := [1;31m
# makefile (from 'Makefile', line 77)
test_alloc.test-path = /
# makefile (from 'Makefile', line 77)
test_alloc.CPPFLAGS = -I'src' -DGTEST_HAS_PTHREAD=1 
# makefile (from 'Makefile', line 66)
test_base.test-path = /
# environment
IM_CONFIG_PHASE = 1
# default
.RECIPEPREFIX := 
# automatic
<F = $(notdir $<)
# environment
NIX_PATH = nixpkgs=/nix/var/nix/profiles/per-user/roby/channels/nixpkgs:/nix/var/nix/profiles/per-user/roby/channels
# makefile (from 'linux-clang.mk', line 52)
mk.load.setup = $(eval $(call mk.load,setup))
# makefile (from 'Makefile', line 89)
test_runtime.LDLIBS := -lgtest_main -lgtest -luring -lak_runtime -lak_alloc -lak_base
# makefile (from 'Makefile', line 84)
libak_runtime.lib-dir := build/libak_runtime/
# makefile (from 'linux-clang.mk', line 249)
std.lastword = $(call mk.unpack,$(firstword $(call mk.pack,$1)))
# default
SUFFIXES := 
# makefile (from 'linux-clang.mk', line 239)
std.dir = $(call mk.unpack,$(dir $(call mk.pack,$1)))
# makefile (from 'Makefile', line 125)
libak-so.CXXFLAGS = $(CXXFLAGS) -fPIC
# makefile (from 'Makefile', line 72)
libak_alloc.pch = pch
# makefile (from 'linux-clang.mk', line 433)
ansi.hiyellow := [1;33m
# makefile (from 'Makefile', line 90)
test_runtime.test-name = test_runtime
# makefile (from 'Makefile', line 103)
test_sync.CXXFLAGS = -Wall -Wextra -std=c++2c -fno-exceptions -fno-rtti -fdiagnostics-color=always -mavx2 -mbmi -mbmi2 -g -O0
# makefile (from 'Makefile', line 7)
test_dir := test
# makefile (from 'linux-clang.mk', line 1300)
pch.pch-path = $(pch.build_dir)/precompiled.pch
# makefile (from 'linux-clang.mk', line 425)
ansi.yellow := [0;33m
# makefile (from 'linux-clang.mk', line 379)
CXXFLAGS := -Wall -Wextra -std=c++2c -fno-exceptions -fno-rtti -fdiagnostics-color=always -mavx2 -mbmi -mbmi2 -g -O0
# makefile (from 'linux-clang.mk', line 94)
char.comma := ,
# environment
MANAGERPIDFDID = 3166
# makefile (from 'linux-clang.mk', line 378)
libdir := /lib
# makefile (from 'linux-clang.mk', line 1300)
pch.out.CPPFLAGS = -include-pch $(call shell.quote.var,$(pch.pch-path))
# makefile (from 'Makefile', line 84)
libak_runtime.lib-name = libak_runtime
# default
.FEATURES := target-specific order-only second-expansion else-if shortest-stem undefine oneshell nocomment grouped-target extra-prereqs notintermediate shell-export archives jobserver jobserver-fifo output-sync check-symlink guile load
# makefile (from 'Makefile', line 97)
libak_sync.TARGET_ARCH = $(TARGET_ARCH)
# makefile (from 'linux-clang.mk', line 377)
CXX := ccache clang++
# makefile (from 'linux-clang.mk', line 1300)
pch.TARGET_ARCH = $(TARGET_ARCH)
# makefile (from 'linux-clang.mk', line 604)
mk.rule = $(call mk.rule.head,$1)$(char.tab)$(call mk.rule.recipe,$2)
# makefile (from 'linux-clang.mk', line 246)
std.realpath = $(call mk.unpack,$(realpath $(call mk.pack,$1)))
# makefile (from 'linux-clang.mk', line 284)
shell.rmdir = $ $(let dir-list,$(call shell.quote.list,$1),$ $(call shell.trace,RM -r $(dir-list))$(char.nl)$ $(RM) -r $(dir-list)$ )
# makefile (from 'Makefile', line 113)
libak-a.lib-dir := build/libak-a/
# makefile (from 'Makefile', line 125)
libak-so.objects = $(libak-so.sources.objects)
# environment
XDG_MENU_PREFIX = gnome-
# environment
TERM_PROGRAM_VERSION = 1.103.2
# makefile (from 'Makefile', line 72)
libak_alloc.lib-path = $(libak_alloc.build_dir)/$(libak_alloc.lib-name).a
# makefile (from 'linux-clang.mk', line 241)
std.suffix = $(call mk.unpack,$(suffix $(call mk.pack,$1)))
# makefile (from 'linux-clang.mk', line 436)
ansi.hicyan := [1;36m
# environment
GDMSESSION = gnome-xorg
# makefile (from 'Makefile', line 60)
libak_base.objects = $(libak_base.sources.objects)
# variable set hash-table stats:
# Load=402/1024=39%, Rehash=0, Collisions=4462/6184=72%

# Pattern-specific Variable Values

# No pattern-specific variable values.

# Directories

# . (device 2050, inode 5374419): 15 files, no impossibilities.

# 15 files, no impossibilities in 1 directories.

# Implicit Rules

%/.:
#  recipe to execute (from 'linux-clang.mk', line 489):
	$(call shell.title,MKDIR -p $(call shell.quote,$@))
	mkdir -p $(call shell.quote,$@)

# 1 implicit rules, 0 (0,0%) terminal.
# Files

build/libak-so/libak.so.0.0.1: build/libak-so/base_timer.o build/libak-so/base_version.o build/libak-so/alloc_freeblock_tree.o build/libak-so/alloc_dump.o build/libak-so/alloc_freeblock_list.o build/libak-so/alloc_check_invariants.o build/libak-so/alloc_table.o build/libak-so/runtime_debug_io.o build/libak-so/runtime_thread_ops.o build/libak-so/runtime_thread_context.o build/libak-so/runtime_debug_task.o build/libak-so/runtime_boot.o build/libak-so/runtime_kernel.o build/libak-so/runtime_io_prep.o build/libak-so/runtime_task.o build/libak-so/sync_event.o | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,LINK -shared -o $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.list,$(libak-so.objects)) $(libak-so.LDLIBS))
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.LDFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF $(call shell.quote.var,$(libak-so.lib-path)).d -o $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.list,$(libak-so.objects)) $(libak-so.LDLIBS) # end
	ln -sfr $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.var,$(libak-so.link1))
	ln -sfr $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.var,$(libak-so.link2))

uninstall.test_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 90):
	$(call shell.title,Uninstall test_runtime)
	$(call shell.rm,$(DESTDIR)$(bindir)/$(test_runtime.test-name)

install.headers::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 130):
	$(call shell.title,Install headers)
	$(INSTALL) -d $(call shell.quote.var,$(DESTDIR)$(includedir))
	(cd $(call shell.quote.var,$(source_dir)) && find . \( -name '*_api.hpp' -o -name '*_api_inl.hpp' -o -name 'ak.hpp' \) -print0) | xargs -0 -I{} -n1 -t -- $(INSTALL_DATA) -D $(call shell.quote.var,$(source_dir)/{}) $(call shell.quote.var,$(DESTDIR)$(includedir)/{})

.ONESHELL:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

FORCE:
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'linux-clang.mk', line 614):
	

uninstall.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,Uninstall libak-so)
	$(call shell.rm,$(DESTDIR)$(libdir)/$(call std.notdir,$(libak-so.lib-path)))
	$(call shell.rm,$(DESTDIR)$(libdir)/$(libak-so.lib-name).so)
	$(call shell.rm,$(DESTDIR)$(libdir)/$(libak-so.soname))

# Not a target:
src/ak/alloc/alloc_check_invariants.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
src/ak/runtime/runtime_task.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
build/libak_alloc/alloc_freeblock_list.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak_sync/sync_event.o: src/ak/sync/sync_event.cpp build/pch/precompiled.pch | build/libak_sync/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 97):
	$(call shell.title,CXX -o 'build/libak_sync/sync_event.o' -c 'src/ak/sync/sync_event.cpp')
	$(CXX) $(libak_sync.CXXFLAGS) $(libak_sync.CPPFLAGS) $(libak_sync.TARGET_ARCH) -MMD -MP -MF 'build/libak_sync/sync_event.o.d' -o 'build/libak_sync/sync_event.o' -c 'src/ak/sync/sync_event.cpp'

# Not a target:
src/ak/runtime/runtime_boot.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

install.test_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	$(call shell.title,Install test_alloc)
	$(call shell.install.program,$(test_alloc.test-path),$(DESTDIR)$(bindir))

# Not a target:
test/base/test_gtest.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
test/alloc/test_split.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
src/ak/runtime/runtime_thread_ops.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak-so/alloc_dump.o: src/ak/alloc/alloc_dump.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/alloc_dump.o' -c 'src/ak/alloc/alloc_dump.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/alloc_dump.o.d' -o 'build/libak-so/alloc_dump.o' -c 'src/ak/alloc/alloc_dump.cpp'

# Not a target:
src/ak/alloc/alloc_dump.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

install.libak_sync:: build/libak_sync/libak_sync.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 97):
	$(call shell.title,Install libak_sync)
	$(call shell.install.program,$(libak_sync.lib-path),$(DESTDIR)$(libdir))

# Not a target:
build/libak-so/runtime_kernel.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak-so/runtime_thread_context.o: src/ak/runtime/runtime_thread_context.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/runtime_thread_context.o' -c 'src/ak/runtime/runtime_thread_context.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/runtime_thread_context.o.d' -o 'build/libak-so/runtime_thread_context.o' -c 'src/ak/runtime/runtime_thread_context.cpp'

# Not a target:
build/libak_runtime/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

install.libak_alloc:: build/libak_alloc/libak_alloc.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	$(call shell.title,Install libak_alloc)
	$(call shell.install.program,$(libak_alloc.lib-path),$(DESTDIR)$(libdir))

# Not a target:
test/alloc/test_freeblock_tree.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak-so/runtime_io_prep.o: src/ak/runtime/runtime_io_prep.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/runtime_io_prep.o' -c 'src/ak/runtime/runtime_io_prep.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/runtime_io_prep.o.d' -o 'build/libak-so/runtime_io_prep.o' -c 'src/ak/runtime/runtime_io_prep.cpp'

test_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 90):
	$(call shell.title,Running test test_runtime: $(call shell.quote.var,$(test_runtime.test-path)))
	declare -i error_code=0
	set +e
	$(call shell.quote.var,$(test_runtime.test-path))
	error_code=$$?
	set -e
	if (( error_code == 0 )); then
		$(call shell.trace,$(ansi.green)PASSED $(call shell.quote.var,$(test_runtime.test-path))$(ansi.reset))
	else
		$(call shell.trace,$(ansi.red)FAILED ($$error_code) $(call shell.quote.var,$(test_runtime.test-path))$(ansi.reset))
	fi

build/libak_alloc/alloc_freeblock_list.o: src/ak/alloc/alloc_freeblock_list.cpp build/pch/precompiled.pch | build/libak_alloc/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	$(call shell.title,CXX -o 'build/libak_alloc/alloc_freeblock_list.o' -c 'src/ak/alloc/alloc_freeblock_list.cpp')
	$(CXX) $(libak_alloc.CXXFLAGS) $(libak_alloc.CPPFLAGS) $(libak_alloc.TARGET_ARCH) -MMD -MP -MF 'build/libak_alloc/alloc_freeblock_list.o.d' -o 'build/libak_alloc/alloc_freeblock_list.o' -c 'src/ak/alloc/alloc_freeblock_list.cpp'

# Not a target:
src/ak/alloc/alloc_freeblock_tree.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak_alloc/alloc_table.o: src/ak/alloc/alloc_table.cpp build/pch/precompiled.pch | build/libak_alloc/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	$(call shell.title,CXX -o 'build/libak_alloc/alloc_table.o' -c 'src/ak/alloc/alloc_table.cpp')
	$(CXX) $(libak_alloc.CXXFLAGS) $(libak_alloc.CPPFLAGS) $(libak_alloc.TARGET_ARCH) -MMD -MP -MF 'build/libak_alloc/alloc_table.o.d' -o 'build/libak_alloc/alloc_table.o' -c 'src/ak/alloc/alloc_table.cpp'

src/ak/base/base_version.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
test/runtime/test_ak.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
build/pch/precompiled.pch.d:
#  Implicit rule search has been done.
#  File is secondary (prerequisite of .SECONDARY).
#  Last modified 2025-09-07 14:00:03.749416473
#  File has been updated.
#  Successfully updated.

build/libak-so/libak.so.0: build/libak-so/base_timer.o build/libak-so/base_version.o build/libak-so/alloc_freeblock_tree.o build/libak-so/alloc_dump.o build/libak-so/alloc_freeblock_list.o build/libak-so/alloc_check_invariants.o build/libak-so/alloc_table.o build/libak-so/runtime_debug_io.o build/libak-so/runtime_thread_ops.o build/libak-so/runtime_thread_context.o build/libak-so/runtime_debug_task.o build/libak-so/runtime_boot.o build/libak-so/runtime_kernel.o build/libak-so/runtime_io_prep.o build/libak-so/runtime_task.o build/libak-so/sync_event.o | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,LINK -shared -o $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.list,$(libak-so.objects)) $(libak-so.LDLIBS))
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.LDFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF $(call shell.quote.var,$(libak-so.lib-path)).d -o $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.list,$(libak-so.objects)) $(libak-so.LDLIBS) # end
	ln -sfr $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.var,$(libak-so.link1))
	ln -sfr $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.var,$(libak-so.link2))

build/libak_runtime/runtime_boot.o: src/ak/runtime/runtime_boot.cpp build/pch/precompiled.pch | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,CXX -o 'build/libak_runtime/runtime_boot.o' -c 'src/ak/runtime/runtime_boot.cpp')
	$(CXX) $(libak_runtime.CXXFLAGS) $(libak_runtime.CPPFLAGS) $(libak_runtime.TARGET_ARCH) -MMD -MP -MF 'build/libak_runtime/runtime_boot.o.d' -o 'build/libak_runtime/runtime_boot.o' -c 'src/ak/runtime/runtime_boot.cpp'

.PRECIOUS: %/. build/pch/precompiled.pch build/libak_base/base_timer.o build/libak_base/base_version.o build/libak_base/libak_base.a / build/libak_alloc/alloc_freeblock_tree.o build/libak_alloc/alloc_dump.o build/libak_alloc/alloc_freeblock_list.o build/libak_alloc/alloc_check_invariants.o build/libak_alloc/alloc_table.o build/libak_alloc/libak_alloc.a / build/libak_runtime/runtime_debug_io.o build/libak_runtime/runtime_thread_ops.o build/libak_runtime/runtime_thread_context.o build/libak_runtime/runtime_debug_task.o build/libak_runtime/runtime_boot.o build/libak_runtime/runtime_kernel.o build/libak_runtime/runtime_io_prep.o build/libak_runtime/runtime_task.o build/libak_runtime/libak_runtime.a / build/libak_sync/sync_event.o build/libak_sync/libak_sync.a / build/libak-a/libak.a build/pch2/precompiled.pch build/libak-so/base_timer.o build/libak-so/base_version.o build/libak-so/alloc_freeblock_tree.o build/libak-so/alloc_dump.o build/libak-so/alloc_freeblock_list.o build/libak-so/alloc_check_invariants.o build/libak-so/alloc_table.o build/libak-so/runtime_debug_io.o build/libak-so/runtime_thread_ops.o build/libak-so/runtime_thread_context.o build/libak-so/runtime_debug_task.o build/libak-so/runtime_boot.o build/libak-so/runtime_kernel.o build/libak-so/runtime_io_prep.o build/libak-so/runtime_task.o build/libak-so/sync_event.o build/libak-so/libak.so.0.0.1 build/libak-so/libak.so build/libak-so/libak.so.0
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

install.test_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 90):
	$(call shell.title,Install test_runtime)
	$(call shell.install.program,$(test_runtime.test-path),$(DESTDIR)$(bindir))

install.libak-a:: build/libak-a/libak.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 113):
	$(call shell.title,Install libak-a)
	$(call shell.install.program,$(libak-a.lib-path),$(DESTDIR)$(libdir))

# Not a target:
Makefile:
#  Implicit rule search has been done.
#  File is secondary (prerequisite of .SECONDARY).
#  Last modified 2025-09-06 10:00:19.298090451
#  File has been updated.
#  Successfully updated.

# Not a target:
build/libak-so/runtime_io_prep.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

clean.pch2::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 119):
	[[ ! -e $(call shell.quote.var,$(pch2.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(pch2.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean pch pch2)
	$(call shell.rmdir,$(pch2.build_dir))

clean.pch2::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 119):
	declare -a deletables=()
	for i in 'build/pch2/precompiled.pch' 'build/pch2/precompiled.pch.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/pch2/precompiled.pch build/pch2/precompiled.pch.d)

# Not a target:
build/libak-so/alloc_freeblock_list.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
build/pch/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

/test_ak.o: test/runtime/test_ak.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 90):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_ak.o' -c 'test/runtime/test_ak.cpp'"
	ccache clang++    -MMD -MP -MF '/test_ak.o'.d -o '/test_ak.o' -c 'test/runtime/test_ak.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_ak.o'.d\' -o \''/test_ak.o'\' -c \''test/runtime/test_ak.cpp'\' > '/test_ak.o'.cmd
	

build/libak-so/libak.so: build/libak-so/base_timer.o build/libak-so/base_version.o build/libak-so/alloc_freeblock_tree.o build/libak-so/alloc_dump.o build/libak-so/alloc_freeblock_list.o build/libak-so/alloc_check_invariants.o build/libak-so/alloc_table.o build/libak-so/runtime_debug_io.o build/libak-so/runtime_thread_ops.o build/libak-so/runtime_thread_context.o build/libak-so/runtime_debug_task.o build/libak-so/runtime_boot.o build/libak-so/runtime_kernel.o build/libak-so/runtime_io_prep.o build/libak-so/runtime_task.o build/libak-so/sync_event.o | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,LINK -shared -o $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.list,$(libak-so.objects)) $(libak-so.LDLIBS))
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.LDFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF $(call shell.quote.var,$(libak-so.lib-path)).d -o $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.list,$(libak-so.objects)) $(libak-so.LDLIBS) # end
	ln -sfr $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.var,$(libak-so.link1))
	ln -sfr $(call shell.quote.var,$(libak-so.lib-path)) $(call shell.quote.var,$(libak-so.link2))

build/libak-so/runtime_debug_task.o: src/ak/runtime/runtime_debug_task.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/runtime_debug_task.o' -c 'src/ak/runtime/runtime_debug_task.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/runtime_debug_task.o.d' -o 'build/libak-so/runtime_debug_task.o' -c 'src/ak/runtime/runtime_debug_task.cpp'

/test_freeblock.o: test/alloc/test_freeblock.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_freeblock.o' -c 'test/alloc/test_freeblock.cpp'"
	ccache clang++    -MMD -MP -MF '/test_freeblock.o'.d -o '/test_freeblock.o' -c 'test/alloc/test_freeblock.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_freeblock.o'.d\' -o \''/test_freeblock.o'\' -c \''test/alloc/test_freeblock.cpp'\' > '/test_freeblock.o'.cmd
	

build/libak-so/runtime_debug_io.o: src/ak/runtime/runtime_debug_io.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/runtime_debug_io.o' -c 'src/ak/runtime/runtime_debug_io.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/runtime_debug_io.o.d' -o 'build/libak-so/runtime_debug_io.o' -c 'src/ak/runtime/runtime_debug_io.cpp'

# Not a target:
build/libak-so/runtime_debug_io.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak-so/sync_event.o: src/ak/sync/sync_event.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/sync_event.o' -c 'src/ak/sync/sync_event.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/sync_event.o.d' -o 'build/libak-so/sync_event.o' -c 'src/ak/sync/sync_event.cpp'

# Not a target:
build/libak-so/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
test/alloc/test_freeblock_list.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak_runtime/libak_runtime.a: build/libak_runtime/runtime_debug_io.o build/libak_runtime/runtime_thread_ops.o build/libak_runtime/runtime_thread_context.o build/libak_runtime/runtime_debug_task.o build/libak_runtime/runtime_boot.o build/libak_runtime/runtime_kernel.o build/libak_runtime/runtime_io_prep.o build/libak_runtime/runtime_task.o | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,AR rcs $(libak_runtime.lib-path) $(libak_runtime.objects))
	$(AR) rcs $(libak_runtime.lib-path) $(libak_runtime.objects)
	$(RANLIB) $(libak_runtime.lib-path)

build/libak-so/runtime_thread_ops.o: src/ak/runtime/runtime_thread_ops.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/runtime_thread_ops.o' -c 'src/ak/runtime/runtime_thread_ops.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/runtime_thread_ops.o.d' -o 'build/libak-so/runtime_thread_ops.o' -c 'src/ak/runtime/runtime_thread_ops.cpp'

uninstall.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,Uninstall libak_runtime)
	$(call shell.rm,$(DESTDIR)$(libdir)/$(call std.notdir,$(libak_runtime.lib-path)))

# Not a target:
build/libak_alloc/alloc_check_invariants.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

/test_freeblock_list_search.o: test/alloc/test_freeblock_list_search.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_freeblock_list_search.o' -c 'test/alloc/test_freeblock_list_search.cpp'"
	ccache clang++    -MMD -MP -MF '/test_freeblock_list_search.o'.d -o '/test_freeblock_list_search.o' -c 'test/alloc/test_freeblock_list_search.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_freeblock_list_search.o'.d\' -o \''/test_freeblock_list_search.o'\' -c \''test/alloc/test_freeblock_list_search.cpp'\' > '/test_freeblock_list_search.o'.cmd
	

install.libak_base:: build/libak_base/libak_base.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 60):
	$(call shell.title,Install libak_base)
	$(call shell.install.program,$(libak_base.lib-path),$(DESTDIR)$(libdir))

# Not a target:
build/libak-so/runtime_thread_context.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

test_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	$(call shell.title,Running test test_alloc: $(call shell.quote.var,$(test_alloc.test-path)))
	declare -i error_code=0
	set +e
	$(call shell.quote.var,$(test_alloc.test-path))
	error_code=$$?
	set -e
	if (( error_code == 0 )); then
		$(call shell.trace,$(ansi.green)PASSED $(call shell.quote.var,$(test_alloc.test-path))$(ansi.reset))
	else
		$(call shell.trace,$(ansi.red)FAILED ($$error_code) $(call shell.quote.var,$(test_alloc.test-path))$(ansi.reset))
	fi

.DELETE_ON_ERROR:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

uninstall:: uninstall.libak-a
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

uninstall:: uninstall.libak-so
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

uninstall:: uninstall.headers
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak-so/base_timer.o: src/ak/base/base_timer.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/base_timer.o' -c 'src/ak/base/base_timer.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/base_timer.o.d' -o 'build/libak-so/base_timer.o' -c 'src/ak/base/base_timer.cpp'

# Not a target:
build/libak_alloc/alloc_freeblock_tree.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

/test_alloc.o: test/runtime/test_alloc.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 90):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_alloc.o' -c 'test/runtime/test_alloc.cpp'"
	ccache clang++    -MMD -MP -MF '/test_alloc.o'.d -o '/test_alloc.o' -c 'test/runtime/test_alloc.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_alloc.o'.d\' -o \''/test_alloc.o'\' -c \''test/runtime/test_alloc.cpp'\' > '/test_alloc.o'.cmd
	

# Not a target:
test/runtime/test_alloc.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
build/test_base/test_dlist.o:
#  Command line target.
#  Implicit rule search has been done.
#  File is secondary (prerequisite of .SECONDARY).
#  Last modified 2025-09-07 14:04:21.442282381
#  File has been updated.
#  Successfully updated.

# Not a target:
build/libak-so/alloc_table.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak_runtime/runtime_io_prep.o: src/ak/runtime/runtime_io_prep.cpp build/pch/precompiled.pch | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,CXX -o 'build/libak_runtime/runtime_io_prep.o' -c 'src/ak/runtime/runtime_io_prep.cpp')
	$(CXX) $(libak_runtime.CXXFLAGS) $(libak_runtime.CPPFLAGS) $(libak_runtime.TARGET_ARCH) -MMD -MP -MF 'build/libak_runtime/runtime_io_prep.o.d' -o 'build/libak_runtime/runtime_io_prep.o' -c 'src/ak/runtime/runtime_io_prep.cpp'

# Not a target:
src/ak/sync/sync_event.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
test/alloc/test_freeblock_list_search.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

clean::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'linux-clang.mk', line 496):
	$(call shell.title,Cleaning)
	$(call shell.trace,rm -rf $(call shell.quote.var,$(build_dir)))
	rm -rf $(call shell.quote.var,$(build_dir))

clean:: clean.pch
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.libak_base
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.test_base
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.libak_alloc
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.test_alloc
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.libak_runtime
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.test_runtime
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.libak_sync
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.test_sync
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.libak-a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.pch2
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean:: clean.libak-so
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

clean.pch::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'linux-clang.mk', line 1300):
	[[ ! -e $(call shell.quote.var,$(pch.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(pch.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean pch pch)
	$(call shell.rmdir,$(pch.build_dir))

clean.pch::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'linux-clang.mk', line 1300):
	declare -a deletables=()
	for i in 'build/pch/precompiled.pch' 'build/pch/precompiled.pch.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/pch/precompiled.pch build/pch/precompiled.pch.d)

# Not a target:
build/libak_sync/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
test/base/test_dlist.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	[[ ! -e $(call shell.quote.var,$(libak_runtime.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(libak_runtime.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean static lib libak_runtime)
	$(call shell.rmdir,$(libak_runtime.build_dir))

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	declare -a deletables=()
	for i in 'build/libak_runtime/runtime_debug_io.o' 'build/libak_runtime/runtime_debug_io.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_runtime/runtime_debug_io.o build/libak_runtime/runtime_debug_io.o.d)

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	declare -a deletables=()
	for i in 'build/libak_runtime/runtime_thread_ops.o' 'build/libak_runtime/runtime_thread_ops.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_runtime/runtime_thread_ops.o build/libak_runtime/runtime_thread_ops.o.d)

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	declare -a deletables=()
	for i in 'build/libak_runtime/runtime_thread_context.o' 'build/libak_runtime/runtime_thread_context.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_runtime/runtime_thread_context.o build/libak_runtime/runtime_thread_context.o.d)

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	declare -a deletables=()
	for i in 'build/libak_runtime/runtime_debug_task.o' 'build/libak_runtime/runtime_debug_task.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_runtime/runtime_debug_task.o build/libak_runtime/runtime_debug_task.o.d)

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	declare -a deletables=()
	for i in 'build/libak_runtime/runtime_boot.o' 'build/libak_runtime/runtime_boot.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_runtime/runtime_boot.o build/libak_runtime/runtime_boot.o.d)

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	declare -a deletables=()
	for i in 'build/libak_runtime/runtime_kernel.o' 'build/libak_runtime/runtime_kernel.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_runtime/runtime_kernel.o build/libak_runtime/runtime_kernel.o.d)

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	declare -a deletables=()
	for i in 'build/libak_runtime/runtime_io_prep.o' 'build/libak_runtime/runtime_io_prep.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_runtime/runtime_io_prep.o build/libak_runtime/runtime_io_prep.o.d)

clean.libak_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	declare -a deletables=()
	for i in 'build/libak_runtime/runtime_task.o' 'build/libak_runtime/runtime_task.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_runtime/runtime_task.o build/libak_runtime/runtime_task.o.d)

uninstall.test_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 66):
	$(call shell.title,Uninstall test_base)
	$(call shell.rm,$(DESTDIR)$(bindir)/$(test_base.test-name)

build/libak-a/libak.a: build/libak_base/base_timer.o build/libak_base/base_version.o build/libak_alloc/alloc_freeblock_tree.o build/libak_alloc/alloc_dump.o build/libak_alloc/alloc_freeblock_list.o build/libak_alloc/alloc_check_invariants.o build/libak_alloc/alloc_table.o build/libak_runtime/runtime_debug_io.o build/libak_runtime/runtime_thread_ops.o build/libak_runtime/runtime_thread_context.o build/libak_runtime/runtime_debug_task.o build/libak_runtime/runtime_boot.o build/libak_runtime/runtime_kernel.o build/libak_runtime/runtime_io_prep.o build/libak_runtime/runtime_task.o build/libak_sync/sync_event.o build/libak_base/libak_base.a build/libak_alloc/libak_alloc.a build/libak_runtime/libak_runtime.a build/libak_sync/libak_sync.a | build/libak-a/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 113):
	$(call shell.title,AR rcs $(libak-a.lib-path) $(libak-a.objects))
	$(AR) rcs $(libak-a.lib-path) $(libak-a.objects)
	$(RANLIB) $(libak-a.lib-path)

/test_timer.o: test/base/test_timer.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 66):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_timer.o' -c 'test/base/test_timer.cpp'"
	ccache clang++    -MMD -MP -MF '/test_timer.o'.d -o '/test_timer.o' -c 'test/base/test_timer.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_timer.o'.d\' -o \''/test_timer.o'\' -c \''test/base/test_timer.cpp'\' > '/test_timer.o'.cmd
	

build/libak_runtime/runtime_thread_ops.o: src/ak/runtime/runtime_thread_ops.cpp build/pch/precompiled.pch | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,CXX -o 'build/libak_runtime/runtime_thread_ops.o' -c 'src/ak/runtime/runtime_thread_ops.cpp')
	$(CXX) $(libak_runtime.CXXFLAGS) $(libak_runtime.CPPFLAGS) $(libak_runtime.TARGET_ARCH) -MMD -MP -MF 'build/libak_runtime/runtime_thread_ops.o.d' -o 'build/libak_runtime/runtime_thread_ops.o' -c 'src/ak/runtime/runtime_thread_ops.cpp'

# Not a target:
build/libak-so/sync_event.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
build/libak_runtime/runtime_thread_ops.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

test_sync::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 103):
	$(call shell.title,Running test test_sync: $(call shell.quote.var,$(test_sync.test-path)))
	declare -i error_code=0
	set +e
	$(call shell.quote.var,$(test_sync.test-path))
	error_code=$$?
	set -e
	if (( error_code == 0 )); then
		$(call shell.trace,$(ansi.green)PASSED $(call shell.quote.var,$(test_sync.test-path))$(ansi.reset))
	else
		$(call shell.trace,$(ansi.red)FAILED ($$error_code) $(call shell.quote.var,$(test_sync.test-path))$(ansi.reset))
	fi

# Not a target:
test/sync/test_event.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

src/ak/base/base_api.hpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
src/precompiled.hpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
%/.:
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

clean.libak_sync::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 97):
	[[ ! -e $(call shell.quote.var,$(libak_sync.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(libak_sync.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean static lib libak_sync)
	$(call shell.rmdir,$(libak_sync.build_dir))

clean.libak_sync::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 97):
	declare -a deletables=()
	for i in 'build/libak_sync/sync_event.o' 'build/libak_sync/sync_event.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_sync/sync_event.o build/libak_sync/sync_event.o.d)

uninstall.test_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	$(call shell.title,Uninstall test_alloc)
	$(call shell.rm,$(DESTDIR)$(bindir)/$(test_alloc.test-name)

clean.test_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 66):
	[[ ! -e $(call shell.quote.var,$(test_base.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(test_base.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean test test_base)
	$(call shell.rmdir,$(test_base.build_dir))

clean.test_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 66):
	declare -a deletables=()
	for i in '/' '/.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,/ /.d)

build/libak_runtime/runtime_task.o: src/ak/runtime/runtime_task.cpp build/pch/precompiled.pch | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,CXX -o 'build/libak_runtime/runtime_task.o' -c 'src/ak/runtime/runtime_task.cpp')
	$(CXX) $(libak_runtime.CXXFLAGS) $(libak_runtime.CPPFLAGS) $(libak_runtime.TARGET_ARCH) -MMD -MP -MF 'build/libak_runtime/runtime_task.o.d' -o 'build/libak_runtime/runtime_task.o' -c 'src/ak/runtime/runtime_task.cpp'

# Not a target:
build/libak_base/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

libak-so:: build/libak-so/libak.so.0.0.1
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

install.test_sync::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 103):
	$(call shell.title,Install test_sync)
	$(call shell.install.program,$(test_sync.test-path),$(DESTDIR)$(bindir))

build/libak_sync/libak_sync.a: build/libak_sync/sync_event.o | build/libak_sync/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 97):
	$(call shell.title,AR rcs $(libak_sync.lib-path) $(libak_sync.objects))
	$(AR) rcs $(libak_sync.lib-path) $(libak_sync.objects)
	$(RANLIB) $(libak_sync.lib-path)

doxygen: | build/doc/.
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 47):
	doxygen Doxyfile

build/libak_alloc/alloc_dump.o: src/ak/alloc/alloc_dump.cpp build/pch/precompiled.pch | build/libak_alloc/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	$(call shell.title,CXX -o 'build/libak_alloc/alloc_dump.o' -c 'src/ak/alloc/alloc_dump.cpp')
	$(CXX) $(libak_alloc.CXXFLAGS) $(libak_alloc.CPPFLAGS) $(libak_alloc.TARGET_ARCH) -MMD -MP -MF 'build/libak_alloc/alloc_dump.o.d' -o 'build/libak_alloc/alloc_dump.o' -c 'src/ak/alloc/alloc_dump.cpp'

# Not a target:
build/libak_base/base_version.o.d:
#  Implicit rule search has been done.
#  File is secondary (prerequisite of .SECONDARY).
#  Last modified 2025-09-07 14:00:10.460645377
#  File has been updated.
#  Successfully updated.

# Not a target:
/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

/test_defragment.o: test/alloc/test_defragment.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_defragment.o' -c 'test/alloc/test_defragment.cpp'"
	ccache clang++    -MMD -MP -MF '/test_defragment.o'.d -o '/test_defragment.o' -c 'test/alloc/test_defragment.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_defragment.o'.d\' -o \''/test_defragment.o'\' -c \''test/alloc/test_defragment.cpp'\' > '/test_defragment.o'.cmd
	

# Not a target:
build/libak_alloc/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak-so/runtime_kernel.o: src/ak/runtime/runtime_kernel.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/runtime_kernel.o' -c 'src/ak/runtime/runtime_kernel.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/runtime_kernel.o.d' -o 'build/libak-so/runtime_kernel.o' -c 'src/ak/runtime/runtime_kernel.cpp'

clean.libak_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 60):
	[[ ! -e $(call shell.quote.var,$(libak_base.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(libak_base.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean static lib libak_base)
	$(call shell.rmdir,$(libak_base.build_dir))

clean.libak_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 60):
	declare -a deletables=()
	for i in 'build/libak_base/base_timer.o' 'build/libak_base/base_timer.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_base/base_timer.o build/libak_base/base_timer.o.d)

clean.libak_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 60):
	declare -a deletables=()
	for i in 'build/libak_base/base_version.o' 'build/libak_base/base_version.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_base/base_version.o build/libak_base/base_version.o.d)

# Not a target:
src/ak/runtime/runtime_io_prep.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
src/ak/alloc/alloc_freeblock_list.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/pch2/precompiled.pch: src/precompiled.hpp | build/pch2/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 119):
	$(call shell.title,CXX -o 'build/pch2/precompiled.pch' -c 'src/precompiled.hpp')
	$(CXX) $(pch2.CXXFLAGS) $(pch2.CPPFLAGS) $(pch2.TARGET_ARCH) -MMD -MP -MF 'build/pch2/precompiled.pch.d' -o 'build/pch2/precompiled.pch' -c 'src/precompiled.hpp'

libak_sync:: build/libak_sync/libak_sync.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

# Not a target:
build/libak-a/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

install.libak-so:: build/libak-so/libak.so.0.0.1 build/libak-so/libak.so build/libak-so/libak.so.0
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,Install libak-so)
	$(call shell.install.data,$(libak-so.lib-path),$(DESTDIR)$(libdir))
	$(call shell.ln-s,$(libak-so.lib-name).so.$(libak-so.version),$(DESTDIR)$(libdir)/$(libak-so.lib-name).so)
	$(call shell.ln-s,$(libak-so.lib-name).so.$(libak-so.version),$(DESTDIR)$(libdir)/$(libak-so.soname))

/: build/libak_base/libak_base.a build/libak_base/libak_base.a build/libak_alloc/libak_alloc.a build/libak_base/libak_base.a build/libak_alloc/libak_alloc.a build/libak_runtime/libak_runtime.a build/libak_base/libak_base.a build/libak_alloc/libak_alloc.a build/libak_runtime/libak_runtime.a build/libak_sync/libak_sync.a | /. /. /. /.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 103):
	$(call shell.title,LINK -o '/'  $(test_sync.LDLIBS))
	$(CXX) $(test_sync.CXXFLAGS) $(test_sync.CPPFLAGS) $(test_sync.LDFLAGS) $(test_sync.TARGET_ARCH) -MMD -MP -MF '/.d' -o '/'  $(test_sync.LDLIBS) # end

clean.test_sync::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 103):
	[[ ! -e $(call shell.quote.var,$(test_sync.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(test_sync.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean test test_sync)
	$(call shell.rmdir,$(test_sync.build_dir))

clean.test_sync::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 103):
	declare -a deletables=()
	for i in '/' '/.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,/ /.d)

build/libak_runtime/runtime_debug_task.o: src/ak/runtime/runtime_debug_task.cpp build/pch/precompiled.pch | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,CXX -o 'build/libak_runtime/runtime_debug_task.o' -c 'src/ak/runtime/runtime_debug_task.cpp')
	$(CXX) $(libak_runtime.CXXFLAGS) $(libak_runtime.CPPFLAGS) $(libak_runtime.TARGET_ARCH) -MMD -MP -MF 'build/libak_runtime/runtime_debug_task.o.d' -o 'build/libak_runtime/runtime_debug_task.o' -c 'src/ak/runtime/runtime_debug_task.cpp'

test_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 66):
	$(call shell.title,Running test test_base: $(call shell.quote.var,$(test_base.test-path)))
	declare -i error_code=0
	set +e
	$(call shell.quote.var,$(test_base.test-path))
	error_code=$$?
	set -e
	if (( error_code == 0 )); then
		$(call shell.trace,$(ansi.green)PASSED $(call shell.quote.var,$(test_base.test-path))$(ansi.reset))
	else
		$(call shell.trace,$(ansi.red)FAILED ($$error_code) $(call shell.quote.var,$(test_base.test-path))$(ansi.reset))
	fi

.DEFAULT:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

uninstall.libak-a::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 113):
	$(call shell.title,Uninstall libak-a)
	$(call shell.rm,$(DESTDIR)$(libdir)/$(call std.notdir,$(libak-a.lib-path)))

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	[[ ! -e $(call shell.quote.var,$(libak-so.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(libak-so.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean shared lib libak-so)
	$(call shell.rmdir,$(libak-so.build_dir))

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/base_timer.o' 'build/libak-so/base_timer.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/base_timer.o build/libak-so/base_timer.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/base_version.o' 'build/libak-so/base_version.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/base_version.o build/libak-so/base_version.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/alloc_freeblock_tree.o' 'build/libak-so/alloc_freeblock_tree.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/alloc_freeblock_tree.o build/libak-so/alloc_freeblock_tree.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/alloc_dump.o' 'build/libak-so/alloc_dump.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/alloc_dump.o build/libak-so/alloc_dump.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/alloc_freeblock_list.o' 'build/libak-so/alloc_freeblock_list.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/alloc_freeblock_list.o build/libak-so/alloc_freeblock_list.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/alloc_check_invariants.o' 'build/libak-so/alloc_check_invariants.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/alloc_check_invariants.o build/libak-so/alloc_check_invariants.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/alloc_table.o' 'build/libak-so/alloc_table.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/alloc_table.o build/libak-so/alloc_table.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/runtime_debug_io.o' 'build/libak-so/runtime_debug_io.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/runtime_debug_io.o build/libak-so/runtime_debug_io.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/runtime_thread_ops.o' 'build/libak-so/runtime_thread_ops.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/runtime_thread_ops.o build/libak-so/runtime_thread_ops.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/runtime_thread_context.o' 'build/libak-so/runtime_thread_context.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/runtime_thread_context.o build/libak-so/runtime_thread_context.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/runtime_debug_task.o' 'build/libak-so/runtime_debug_task.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/runtime_debug_task.o build/libak-so/runtime_debug_task.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/runtime_boot.o' 'build/libak-so/runtime_boot.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/runtime_boot.o build/libak-so/runtime_boot.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/runtime_kernel.o' 'build/libak-so/runtime_kernel.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/runtime_kernel.o build/libak-so/runtime_kernel.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/runtime_io_prep.o' 'build/libak-so/runtime_io_prep.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/runtime_io_prep.o build/libak-so/runtime_io_prep.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/runtime_task.o' 'build/libak-so/runtime_task.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/runtime_task.o build/libak-so/runtime_task.o.d)

clean.libak-so::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	declare -a deletables=()
	for i in 'build/libak-so/sync_event.o' 'build/libak-so/sync_event.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak-so/sync_event.o build/libak-so/sync_event.o.d)

pch2:: build/pch2/precompiled.pch
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

# makefile (from 'Makefile', line 139)
install.test: bindir := /test
install.test:: install.test_base install.test_alloc install.test_runtime install.test_sync
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
# variable set hash-table stats:
# Load=1/32=3%, Rehash=0, Collisions=0/3=0%

clean.dist::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 51):
	[[ ! -e $(call shell.quote.var,$(dist_dir)) ]] && exit
	$(call shell.title,Clean dist dir)
	$(call shell.trace,rm -r $(call shell.quote.var,$(dist_dir)))
	rm -r $(call shell.quote.var,$(dist_dir))

install.libak_runtime:: build/libak_runtime/libak_runtime.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,Install libak_runtime)
	$(call shell.install.program,$(libak_runtime.lib-path),$(DESTDIR)$(libdir))

clean.test_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 90):
	[[ ! -e $(call shell.quote.var,$(test_runtime.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(test_runtime.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean test test_runtime)
	$(call shell.rmdir,$(test_runtime.build_dir))

clean.test_runtime::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 90):
	declare -a deletables=()
	for i in '/' '/.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,/ /.d)

build/libak_base/libak_base.a: build/libak_base/base_timer.o build/libak_base/base_version.o | build/libak_base/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 60):
	$(call shell.title,AR rcs $(libak_base.lib-path) $(libak_base.objects))
	$(AR) rcs $(libak_base.lib-path) $(libak_base.objects)
	$(RANLIB) $(libak_base.lib-path)

all::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

all:: libak-a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

all:: libak-so
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

.config.mk:
#  Implicit rule search has not been done.
#  File is secondary (prerequisite of .SECONDARY).
#  Last modified 2025-09-05 12:07:29.512286195
#  File has been updated.
#  Successfully updated.
#  recipe to execute (from 'linux-clang.mk', line 415):
	cat <<-'EOF' > $(call shell.quote.var,$(config-file))
	$(config-file.content)
	EOF

# Not a target:
build/libak_runtime/runtime_kernel.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

/test_freeblock_list.o: test/alloc/test_freeblock_list.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_freeblock_list.o' -c 'test/alloc/test_freeblock_list.cpp'"
	ccache clang++    -MMD -MP -MF '/test_freeblock_list.o'.d -o '/test_freeblock_list.o' -c 'test/alloc/test_freeblock_list.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_freeblock_list.o'.d\' -o \''/test_freeblock_list.o'\' -c \''test/alloc/test_freeblock_list.cpp'\' > '/test_freeblock_list.o'.cmd
	

run::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

doc:: doxygen
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

# Not a target:
build/libak-so/alloc_dump.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak-so/alloc_check_invariants.o: src/ak/alloc/alloc_check_invariants.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/alloc_check_invariants.o' -c 'src/ak/alloc/alloc_check_invariants.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/alloc_check_invariants.o.d' -o 'build/libak-so/alloc_check_invariants.o' -c 'src/ak/alloc/alloc_check_invariants.cpp'

# Not a target:
test/base/test_timer.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

uninstall.test:: uninstall.test_base uninstall.test_alloc uninstall.test_runtime uninstall.test_sync
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak-so/runtime_boot.o: src/ak/runtime/runtime_boot.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/runtime_boot.o' -c 'src/ak/runtime/runtime_boot.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/runtime_boot.o.d' -o 'build/libak-so/runtime_boot.o' -c 'src/ak/runtime/runtime_boot.cpp'

watch:
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'linux-clang.mk', line 518):
	printf -- '%b---------- Watching for changes...%b\n' '$(ansi.yellow)' '$(ansi.reset)'
	inotifywait -qmr -e close_write,delete,move $(call mk.shell.escape,$(call mk.unescape,$(source_dir))) | while read -r event; do
		reset
		printf -- '%b---------- Detected change:%b %s\n' '$(ansi.yellow)' "$$event" '$(ansi.reset)'
		while read -r -t 1.0 debounce_event; do :; done
		declare -i exit_code=0
		if ! $(MAKE) all; then
			printf -- '%b---------- Build failed%b\n' '$(ansi.red)' '$(ansi.reset)'
		\else
			printf -- '%b---------- Build successful%b\n' '$(ansi.green)' '$(ansi.reset)'
		fi
	done

# Not a target:
build/libak_base/base_timer.o.d:
#  Implicit rule search has been done.
#  File is secondary (prerequisite of .SECONDARY).
#  Last modified 2025-09-07 14:00:09.92372421
#  File has been updated.
#  Successfully updated.

# Not a target:
build/libak_runtime/runtime_debug_task.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
build/libak-so/alloc_check_invariants.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak-so/alloc_table.o: src/ak/alloc/alloc_table.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/alloc_table.o' -c 'src/ak/alloc/alloc_table.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/alloc_table.o.d' -o 'build/libak-so/alloc_table.o' -c 'src/ak/alloc/alloc_table.cpp'

/test_gtest.o: test/base/test_gtest.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 66):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_gtest.o' -c 'test/base/test_gtest.cpp'"
	ccache clang++    -MMD -MP -MF '/test_gtest.o'.d -o '/test_gtest.o' -c 'test/base/test_gtest.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_gtest.o'.d\' -o \''/test_gtest.o'\' -c \''test/base/test_gtest.cpp'\' > '/test_gtest.o'.cmd
	

libak_alloc:: build/libak_alloc/libak_alloc.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

# Not a target:
build/libak_alloc/alloc_table.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
build/libak-so/runtime_debug_task.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak_base/base_version.o: src/ak/base/base_version.cpp /home/roby/Workspace/coro/effect/libak/src/precompiled.hpp src/ak/base/base_version.cpp src/ak/base/base_api.hpp build/pch/precompiled.pch | build/libak_base/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 60):
	$(call shell.title,CXX -o 'build/libak_base/base_version.o' -c 'src/ak/base/base_version.cpp')
	$(CXX) $(libak_base.CXXFLAGS) $(libak_base.CPPFLAGS) $(libak_base.TARGET_ARCH) -MMD -MP -MF 'build/libak_base/base_version.o.d' -o 'build/libak_base/base_version.o' -c 'src/ak/base/base_version.cpp'

libak-a:: build/libak-a/libak.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

test::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

test:: test_base
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

test:: test_alloc
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

test:: test_runtime
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

test:: test_sync
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

# Not a target:
src/ak/runtime/runtime_debug_task.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

uninstall.libak_sync::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 97):
	$(call shell.title,Uninstall libak_sync)
	$(call shell.rm,$(DESTDIR)$(libdir)/$(call std.notdir,$(libak_sync.lib-path)))

# Not a target:
/home/roby/Workspace/coro/effect/libak/src/precompiled.hpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

clean.test_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	[[ ! -e $(call shell.quote.var,$(test_alloc.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(test_alloc.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean test test_alloc)
	$(call shell.rmdir,$(test_alloc.build_dir))

clean.test_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	declare -a deletables=()
	for i in '/' '/.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,/ /.d)

uninstall.libak_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 60):
	$(call shell.title,Uninstall libak_base)
	$(call shell.rm,$(DESTDIR)$(libdir)/$(call std.notdir,$(libak_base.lib-path)))

# Not a target:
build/libak_sync/sync_event.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

pch:: build/pch/precompiled.pch
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

# Not a target:
build/libak_alloc/alloc_dump.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak_runtime/runtime_kernel.o: src/ak/runtime/runtime_kernel.cpp build/pch/precompiled.pch | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,CXX -o 'build/libak_runtime/runtime_kernel.o' -c 'src/ak/runtime/runtime_kernel.cpp')
	$(CXX) $(libak_runtime.CXXFLAGS) $(libak_runtime.CPPFLAGS) $(libak_runtime.TARGET_ARCH) -MMD -MP -MF 'build/libak_runtime/runtime_kernel.o.d' -o 'build/libak_runtime/runtime_kernel.o' -c 'src/ak/runtime/runtime_kernel.cpp'

/test_file_io.o: test/runtime/test_file_io.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 90):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_file_io.o' -c 'test/runtime/test_file_io.cpp'"
	ccache clang++    -MMD -MP -MF '/test_file_io.o'.d -o '/test_file_io.o' -c 'test/runtime/test_file_io.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_file_io.o'.d\' -o \''/test_file_io.o'\' -c \''test/runtime/test_file_io.cpp'\' > '/test_file_io.o'.cmd
	

# Not a target:
build/libak-so/runtime_task.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

src/ak/base/base_timer.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
src/ak/runtime/runtime_debug_io.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

/test_dlist.o: test/base/test_dlist.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 66):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_dlist.o' -c 'test/base/test_dlist.cpp'"
	ccache clang++    -MMD -MP -MF '/test_dlist.o'.d -o '/test_dlist.o' -c 'test/base/test_dlist.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_dlist.o'.d\' -o \''/test_dlist.o'\' -c \''test/base/test_dlist.cpp'\' > '/test_dlist.o'.cmd
	

/test_split.o: test/alloc/test_split.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_split.o' -c 'test/alloc/test_split.cpp'"
	ccache clang++    -MMD -MP -MF '/test_split.o'.d -o '/test_split.o' -c 'test/alloc/test_split.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_split.o'.d\' -o \''/test_split.o'\' -c \''test/alloc/test_split.cpp'\' > '/test_split.o'.cmd
	

.NOTPARALLEL: clean run test install
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

uninstall.headers::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 136):
	$(call shell.title,Uninstall headers)
	(cd $(call shell.quote.var,$(source_dir)) && find . \( -name '*_api.hpp' -o -name '*_api_inl.hpp' -o -name 'ak.hpp' \) -print0) | xargs -0 -I{} -n1 -t -- $(RM) $(call shell.quote.var,$(DESTDIR)$(includedir)/{})

uninstall.test_sync::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 103):
	$(call shell.title,Uninstall test_sync)
	$(call shell.rm,$(DESTDIR)$(bindir)/$(test_sync.test-name)

build/libak_alloc/libak_alloc.a: build/libak_alloc/alloc_freeblock_tree.o build/libak_alloc/alloc_dump.o build/libak_alloc/alloc_freeblock_list.o build/libak_alloc/alloc_check_invariants.o build/libak_alloc/alloc_table.o | build/libak_alloc/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	$(call shell.title,AR rcs $(libak_alloc.lib-path) $(libak_alloc.objects))
	$(AR) rcs $(libak_alloc.lib-path) $(libak_alloc.objects)
	$(RANLIB) $(libak_alloc.lib-path)

libak_base:: build/libak_base/libak_base.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

# Not a target:
build/pch2/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

install.test_base::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 66):
	$(call shell.title,Install test_base)
	$(call shell.install.program,$(test_base.test-path),$(DESTDIR)$(bindir))

build/pch/precompiled.pch: src/precompiled.hpp src/precompiled.hpp | build/pch/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'linux-clang.mk', line 1300):
	$(call shell.title,CXX -o 'build/pch/precompiled.pch' -c 'src/precompiled.hpp')
	$(CXX) $(pch.CXXFLAGS) $(pch.CPPFLAGS) $(pch.TARGET_ARCH) -MMD -MP -MF 'build/pch/precompiled.pch.d' -o 'build/pch/precompiled.pch' -c 'src/precompiled.hpp'

# Not a target:
build/libak_runtime/runtime_debug_io.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

install::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'linux-clang.mk', line 512):
	$(call shell.title,Install)

install:: install.libak-a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

install:: install.libak-so
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

install:: install.headers
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

# Not a target:
build/libak-so/runtime_thread_ops.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
build/libak_runtime/runtime_boot.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

clean.libak-a::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 113):
	[[ ! -e $(call shell.quote.var,$(libak-a.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(libak-a.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean static lib libak-a)
	$(call shell.rmdir,$(libak-a.build_dir))

build/libak_base/base_timer.o: src/ak/base/base_timer.cpp /home/roby/Workspace/coro/effect/libak/src/precompiled.hpp src/ak/base/base_timer.cpp src/ak/base/base_api.hpp build/pch/precompiled.pch | build/libak_base/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 60):
	$(call shell.title,CXX -o 'build/libak_base/base_timer.o' -c 'src/ak/base/base_timer.cpp')
	$(CXX) $(libak_base.CXXFLAGS) $(libak_base.CPPFLAGS) $(libak_base.TARGET_ARCH) -MMD -MP -MF 'build/libak_base/base_timer.o.d' -o 'build/libak_base/base_timer.o' -c 'src/ak/base/base_timer.cpp'

build/libak-so/alloc_freeblock_tree.o: src/ak/alloc/alloc_freeblock_tree.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/alloc_freeblock_tree.o' -c 'src/ak/alloc/alloc_freeblock_tree.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/alloc_freeblock_tree.o.d' -o 'build/libak-so/alloc_freeblock_tree.o' -c 'src/ak/alloc/alloc_freeblock_tree.cpp'

uninstall.libak_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	$(call shell.title,Uninstall libak_alloc)
	$(call shell.rm,$(DESTDIR)$(libdir)/$(call std.notdir,$(libak_alloc.lib-path)))

# Not a target:
linux-clang.mk:
#  Implicit rule search has been done.
#  File is secondary (prerequisite of .SECONDARY).
#  Last modified 2025-09-07 16:30:07.177994031
#  File has been updated.
#  Successfully updated.

build/libak-so/runtime_task.o: src/ak/runtime/runtime_task.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/runtime_task.o' -c 'src/ak/runtime/runtime_task.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/runtime_task.o.d' -o 'build/libak-so/runtime_task.o' -c 'src/ak/runtime/runtime_task.cpp'

# Not a target:
test/alloc/test_defragment.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak-so/alloc_freeblock_list.o: src/ak/alloc/alloc_freeblock_list.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/alloc_freeblock_list.o' -c 'src/ak/alloc/alloc_freeblock_list.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/alloc_freeblock_list.o.d' -o 'build/libak-so/alloc_freeblock_list.o' -c 'src/ak/alloc/alloc_freeblock_list.cpp'

# Not a target:
build/libak_runtime/runtime_io_prep.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
build/libak-so/base_version.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
//.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
/.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
build/libak-so/runtime_boot.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak_runtime/runtime_debug_io.o: src/ak/runtime/runtime_debug_io.cpp build/pch/precompiled.pch | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,CXX -o 'build/libak_runtime/runtime_debug_io.o' -c 'src/ak/runtime/runtime_debug_io.cpp')
	$(CXX) $(libak_runtime.CXXFLAGS) $(libak_runtime.CPPFLAGS) $(libak_runtime.TARGET_ARCH) -MMD -MP -MF 'build/libak_runtime/runtime_debug_io.o.d' -o 'build/libak_runtime/runtime_debug_io.o' -c 'src/ak/runtime/runtime_debug_io.cpp'

clean.libak_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	[[ ! -e $(call shell.quote.var,$(libak_alloc.build_dir)) ]] && exit
	[[ $(call shell.quote.var,$(libak_alloc.build_dir)) == $(call shell.quote.var,$(build_dir)) ]] && exit
	$(call shell.title,Clean static lib libak_alloc)
	$(call shell.rmdir,$(libak_alloc.build_dir))

clean.libak_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	declare -a deletables=()
	for i in 'build/libak_alloc/alloc_freeblock_tree.o' 'build/libak_alloc/alloc_freeblock_tree.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_alloc/alloc_freeblock_tree.o build/libak_alloc/alloc_freeblock_tree.o.d)

clean.libak_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	declare -a deletables=()
	for i in 'build/libak_alloc/alloc_dump.o' 'build/libak_alloc/alloc_dump.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_alloc/alloc_dump.o build/libak_alloc/alloc_dump.o.d)

clean.libak_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	declare -a deletables=()
	for i in 'build/libak_alloc/alloc_freeblock_list.o' 'build/libak_alloc/alloc_freeblock_list.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_alloc/alloc_freeblock_list.o build/libak_alloc/alloc_freeblock_list.o.d)

clean.libak_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	declare -a deletables=()
	for i in 'build/libak_alloc/alloc_check_invariants.o' 'build/libak_alloc/alloc_check_invariants.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_alloc/alloc_check_invariants.o build/libak_alloc/alloc_check_invariants.o.d)

clean.libak_alloc::
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	declare -a deletables=()
	for i in 'build/libak_alloc/alloc_table.o' 'build/libak_alloc/alloc_table.o.d'; do
		if [[ -e $$i ]]; then
			deletables+=( "$$i" )
		fi
	done
	if [[ $${#deletables[@]} == 0 ]]; then
		exit
	fi
	$(call shell.rm,build/libak_alloc/alloc_table.o build/libak_alloc/alloc_table.o.d)

# Not a target:
build/libak_runtime/runtime_thread_context.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
build/doc/.:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak_alloc/alloc_check_invariants.o: src/ak/alloc/alloc_check_invariants.cpp build/pch/precompiled.pch | build/libak_alloc/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	$(call shell.title,CXX -o 'build/libak_alloc/alloc_check_invariants.o' -c 'src/ak/alloc/alloc_check_invariants.cpp')
	$(CXX) $(libak_alloc.CXXFLAGS) $(libak_alloc.CPPFLAGS) $(libak_alloc.TARGET_ARCH) -MMD -MP -MF 'build/libak_alloc/alloc_check_invariants.o.d' -o 'build/libak_alloc/alloc_check_invariants.o' -c 'src/ak/alloc/alloc_check_invariants.cpp'

.SUFFIXES:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak_runtime/runtime_thread_context.o: src/ak/runtime/runtime_thread_context.cpp build/pch/precompiled.pch | build/libak_runtime/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 84):
	$(call shell.title,CXX -o 'build/libak_runtime/runtime_thread_context.o' -c 'src/ak/runtime/runtime_thread_context.cpp')
	$(CXX) $(libak_runtime.CXXFLAGS) $(libak_runtime.CPPFLAGS) $(libak_runtime.TARGET_ARCH) -MMD -MP -MF 'build/libak_runtime/runtime_thread_context.o.d' -o 'build/libak_runtime/runtime_thread_context.o' -c 'src/ak/runtime/runtime_thread_context.cpp'

libak_runtime:: build/libak_runtime/libak_runtime.a
#  Phony target (prerequisite of .PHONY).
#  Implicit rule search has not been done.
#  File does not exist.
#  File has not been updated.

.PHONY: all clean run test install watch FORCE pch clean clean.pch clean.pch doc doxygen clean.dist libak_base clean clean.libak_base clean.libak_base clean.libak_base install.libak_base uninstall.libak_base test_base test clean clean.test_base clean.test_base install.test_base uninstall.test_base libak_alloc clean clean.libak_alloc clean.libak_alloc clean.libak_alloc clean.libak_alloc clean.libak_alloc clean.libak_alloc install.libak_alloc uninstall.libak_alloc test_alloc test clean clean.test_alloc clean.test_alloc install.test_alloc uninstall.test_alloc libak_runtime clean clean.libak_runtime clean.libak_runtime clean.libak_runtime clean.libak_runtime clean.libak_runtime clean.libak_runtime clean.libak_runtime clean.libak_runtime clean.libak_runtime install.libak_runtime uninstall.libak_runtime test_runtime test clean clean.test_runtime clean.test_runtime install.test_runtime uninstall.test_runtime libak_sync clean clean.libak_sync clean.libak_sync install.libak_sync uninstall.libak_sync test_sync test clean clean.test_sync clean.test_sync install.test_sync uninstall.test_sync libak-a clean clean.libak-a install.libak-a uninstall.libak-a pch2 clean clean.pch2 clean.pch2 libak-so clean clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so clean.libak-so install.libak-so uninstall.libak-so install.headers uninstall.headers
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

build/libak_alloc/alloc_freeblock_tree.o: src/ak/alloc/alloc_freeblock_tree.cpp build/pch/precompiled.pch | build/libak_alloc/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 72):
	$(call shell.title,CXX -o 'build/libak_alloc/alloc_freeblock_tree.o' -c 'src/ak/alloc/alloc_freeblock_tree.cpp')
	$(CXX) $(libak_alloc.CXXFLAGS) $(libak_alloc.CPPFLAGS) $(libak_alloc.TARGET_ARCH) -MMD -MP -MF 'build/libak_alloc/alloc_freeblock_tree.o.d' -o 'build/libak_alloc/alloc_freeblock_tree.o' -c 'src/ak/alloc/alloc_freeblock_tree.cpp'

# Not a target:
build/libak_runtime/runtime_task.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# Not a target:
src/ak/alloc/alloc_table.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
test/runtime/test_file_io.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
build/libak-so/base_timer.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

/test_event.o: test/sync/test_event.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 103):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_event.o' -c 'test/sync/test_event.cpp'"
	ccache clang++    -MMD -MP -MF '/test_event.o'.d -o '/test_event.o' -c 'test/sync/test_event.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_event.o'.d\' -o \''/test_event.o'\' -c \''test/sync/test_event.cpp'\' > '/test_event.o'.cmd
	

# Not a target:
src/ak/runtime/runtime_kernel.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
test/alloc/test_freeblock.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
build/libak-so/alloc_freeblock_tree.o.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

build/libak-so/base_version.o: src/ak/base/base_version.cpp build/pch2/precompiled.pch | build/libak-so/.
#  Precious file (prerequisite of .PRECIOUS).
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 125):
	$(call shell.title,CXX -o 'build/libak-so/base_version.o' -c 'src/ak/base/base_version.cpp')
	$(CXX) $(libak-so.CXXFLAGS) $(libak-so.CPPFLAGS) $(libak-so.TARGET_ARCH) -MMD -MP -MF 'build/libak-so/base_version.o.d' -o 'build/libak-so/base_version.o' -c 'src/ak/base/base_version.cpp'

/test_freeblock_tree.o: test/alloc/test_freeblock_tree.cpp FORCE | //.
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.
#  recipe to execute (from 'Makefile', line 77):
	printf -- '[[0;36m%s[0m] [0;37m%s[0m\n' "$$" "CXX -o '/test_freeblock_tree.o' -c 'test/alloc/test_freeblock_tree.cpp'"
	ccache clang++    -MMD -MP -MF '/test_freeblock_tree.o'.d -o '/test_freeblock_tree.o' -c 'test/alloc/test_freeblock_tree.cpp'
	echo ccache clang++    -MMD -MP -MF \''/test_freeblock_tree.o'.d\' -o \''/test_freeblock_tree.o'\' -c \''test/alloc/test_freeblock_tree.cpp'\' > '/test_freeblock_tree.o'.cmd
	

# Not a target:
src/ak/runtime/runtime_thread_context.cpp:
#  Implicit rule search has not been done.
#  Modification time never checked.
#  File has not been updated.

# Not a target:
build/pch2/precompiled.pch.d:
#  Implicit rule search has been done.
#  File does not exist.
#  File has been updated.
#  Failed to be updated.

# files hash-table stats:
# Load=240/1024=23%, Rehash=0, Collisions=184/1219=15%
# VPATH Search Paths

# No 'vpath' search paths.

# No general ('VPATH' variable) search path.

# strcache buffers: 1 (0) / strings = 253 / storage = 5296 B / avg = 20 B
# current buf: size = 8162 B / used = 5296 B / count = 253 / avg = 20 B

# strcache performance: lookups = 1342 / hit rate = 81%
# hash-table stats:
# Load=253/8192=3%, Rehash=0, Collisions=12/1342=1%
# Finished Make data base on Sun Sep  7 16:30:58 2025

