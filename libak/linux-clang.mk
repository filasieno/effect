# x86_64-linux-clang.mk: An includable makefile for building C++ projects with
# Clang on x86_64 Linux.
#
# This Makefile provides a modular build system with support for executables,
# libraries, tests, precompiled headers, and code coverage analysis.
#
# By design, it explicitly avoids:
# - Implicit rules
# - Automatic variables
# - Recursive makefiles
#
# Overview:
#
# - Uses a multi-phase loading mechanism (init, setup) for configuration and
#   rules.
# - Supports colored output, Valgrind, precompiled headers, and coverage.
# - Includes utility functions for string manipulation and shell escaping.
# - Provides templates for compiling, linking, and building executables,
#   static/shared libraries, and tests.
# - Ensures space-safe operations and proper dependency tracking.
#
# Usage:
#
# 1. Include this makefile at the beginning (init phase).
# 2. Set `build_dir` and `source_dir` and other custom defaults.
# 3. Execute $(mk.load.setup), that is the setup phase.
# 4. Define targets using templates (e.g., `template-executable`,
#    `template-lib`, `template-solib`, `template-test`).
# 5. Run `make` or `make all` to build, `make clean` to clean, `make test` to
#    run tests, or `make coverage` for coverage reports.
#
# Phases:
# - **init**: Initializes configuration, creates `.config.mk` if missing, reads
#   it and sets defaults.
# - **setup**: Sets up build rules.
#
# Configuration:
#
# Variables like `CONFIG`, `COLOR`, `RUN_WITH_VALGRIND`,
# `ENABLE_PCH`, and `ENABLE_COVERAGE` can be overridden in
#   `.config.mk`.
# -----------------------------------------------------------------------------

ifeq ($(flavor mk.next-phase.makefile),undefined)
mk.next-phase.makefile := $(lastword $(MAKEFILE_LIST))
mk.next-phase := init
mk.next-phase.load = $(eval include $(mk.next-phase.makefile))
define mk.load
override mk.next-phase := $1
include $(mk.next-phase.makefile)
endef
mk.load.setup = $(eval $(call mk.load,setup))
endif

 # loading phases
ifeq ($(mk.next-phase),init)
# ▗    ▗ ▐        ▌
# ▄ ▛▀▖▄ ▜▀    ▛▀▖▛▀▖▝▀▖▞▀▘▞▀▖
# ▐ ▌ ▌▐ ▐ ▖   ▙▄▘▌ ▌▞▀▌▝▀▖▛▀
# ▀▘▘ ▘▀▘ ▀    ▌  ▘ ▘▝▀▘▀▀ ▝▀▘
# toilet -f smblock -W -t init prelude rules
$(info Loading: init)

# Makefile defaults
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables
MAKEFLAGS += --silent
MAKEFLAGS += --output-sync=target
SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
.ONESHELL:
.DELETE_ON_ERROR:
.SUFFIXES:
.DEFAULT:

.DEFAULT_GOAL := all
.PHONY: all
all::

# Special chars
nothing :=#nothing
char.space := $(nothing) $(nothing)#space
char.lparen := (
char.rparen := )
char.lsquare := [
char.rsquare := ]
char.lcurly := {
char.rcurly := }
char.comma := ,
char.backslash := \$(nothing)
char.dollar := $$
char.tab := $(nothing)	$(nothing)#tab
define char.nl :=
$(nothing)
$(nothing)
endef
define char.hash :=
#
endef
char.esc := $(shell printf '\e')

# Useful for indenting expressions with the $\ trick
$(char.space) :=#nothing
$(char.backslash) :=#nothing
# Optional arguments to pass to executable targets
ARGS :=


# -----------------------------------------------------------------------------
# Function: eq
# - $1 The first string to compare
# - $2 The second string to compare
#
# Returns a non-empty string (true) if the 2 arguments are equals, otherwise
# returns an empty string (false).
# -----------------------------------------------------------------------------
eq = $(let a,x$1,$(let b,x$2,$(and $(findstring $a,$b),$(findstring $b,$a))))

# -----------------------------------------------------------------------------
# Function: shell.escape.from-raw
# - $1 The raw string to escape
#
# Escape a raw string for the shell, prefixing a backslash for every sensitive char
# with special meaning in the shell.
# -----------------------------------------------------------------------------
shell.escape.from-raw = $(subst $(char.space),\$(char.space),$(call shell.escape._rec,$1,$(shell.sensitive-chars)))
shell.escape._rec = $(let c r,$2,$(if $c,$(call shell.escape._rec,$(subst $c,\$c,$1),$r),$1))
shell.sensitive-chars := $(char.backslash) $$ ` ( ) * ? [ ] ' " & | ; < > { } ~

# -----------------------------------------------------------------------------
# Function: shell.escape
# - $1 The string to escape
#
# Escape a string for the shell, prefixing a backslash for every sensitive char
# with special meaning in the shell.
# -----------------------------------------------------------------------------
shell.escape = $(call shell.escape.from-raw,$(call mk.unescape,$1))


# -----------------------------------------------------------------------------
# Function: shell.quote.from-raw
# - $1 A raw string to quote
#
# Quote a raw string for the shell. The raw string is then quoted with a single quotation
# mark, according to bash rules
# -----------------------------------------------------------------------------
shell.quote.from-raw = '$(subst ','\'',$1)'

# -----------------------------------------------------------------------------
# Function: shell.quote
# - $1 A "possibly escaped" makefile string
#
# See `shell.quote`
# -----------------------------------------------------------------------------
shell.quote = $(call shell.quote.from-raw,$(call mk.unescape,$1))

shell.unquote.to-raw = $(subst '\'',',$(call shell.unquote._unpack,$(patsubst '%',%,$(call shell.unquote._pack,$1))))
shell.unquote._pack   = $(subst $(char.space),%2,$(subst %,%0,$1))
shell.unquote._unpack = $(subst %0,%,$(subst %2,$(char.space),$1))

shell.unquote = $(call mk.escape,$(call shell.unquote.to-raw,$1))


# -----------------------------------------------------------------------------
# Function: shell.quote.list
# - $1 A list of "possibly escaped" makefile strings
#
# See `shell.quote`
# -----------------------------------------------------------------------------
shell.quote.list = $(foreach i,$(call mk.pack-list,$1),$(call shell.quote,$(call mk.unpack-list,$i)))


# -----------------------------------------------------------------------------
# Function: mk.add-prereq
# - $1 The target
# - $2 One or more prerequisites to be added to the target
#
# Add a prerequisite or a list of prerequisites to the specified target.
# -----------------------------------------------------------------------------
mk.add-prereq = $(eval $1: $2)

# -----------------------------------------------------------------------------
# Function: mk.escape
# - $1 A raw string to escape
#
# Returns a escaped string that is valid for makefile such as in targets and
# prereqs.
# -----------------------------------------------------------------------------
mk.escape = $(subst $$,$$$$,$(subst #,\#,$(subst :,\:,$(subst $(char.space),\$(char.space),$(subst \,\\,$1)))))

# -----------------------------------------------------------------------------
# Function: mk.unescape
# - $1 A valid makefile string to unescape
#
#  Returns an unescaped string.
# -----------------------------------------------------------------------------
mk.unescape = $(subst \\,\,$(subst \$(char.space),$(char.space),$(subst \:,:,$(subst \#,#,$(subst $$$$,$$,$1)))))

# -----------------------------------------------------------------------------
# Function: mk.pack-list
# - $1 A valid "possibly escaped" makefile list to pack
#
# Converts a list to a format in which spaces are univocally used as item
# separator in a list.
# -----------------------------------------------------------------------------
mk.pack-list = $(subst \$(char.space),%2,$(subst \\,%1,$(subst %,%0,$1)))

# -----------------------------------------------------------------------------
# Function: mk.unpack-list
# - $1 A packed list to unpack
#
# Reverts transormation done by `mk.pack-list`. The returned string is a valid
# possibly escaped list.
# -----------------------------------------------------------------------------
mk.unpack-list = $(subst %0,%,$(subst %1,\\,$(subst %2,\$(char.space),$1)))



# -----------------------------------------------------------------------------
# The following functions are a space-safe version of some standard makefile
# functions.
# -----------------------------------------------------------------------------
std.dir = $(call mk.unpack-list,$(dir $(call mk.pack-list,$1)))
std.notdir = $(call mk.unpack-list,$(notdir $(call mk.pack-list,$1)))
std.suffix = $(call mk.unpack-list,$(suffix $(call mk.pack-list,$1)))
std.basename = $(call mk.unpack-list,$(basename $(call mk.pack-list,$1)))
std.addsuffix = $(call mk.unpack-list,$(addsuffix $1,$(call mk.pack-list,$2)))
std.addprefix = $(call mk.unpack-list,$(addprefix $1,$(call mk.pack-list,$2)))
std.join = $(call mk.unpack-list,$(join $(call mk.pack-list,$1),$(call mk.pack-list,$2)))
std.realpath = $(call mk.unpack-list,$(realpath $(call mk.pack-list,$1)))
std.abspath = $(call mk.unpack-list,$(abspath $(call mk.pack-list,$1)))
std.firstword = $(call mk.unpack-list,$(firstword $(call mk.pack-list,$1)))
std.lastword = $(call mk.unpack-list,$(firstword $(call mk.pack-list,$1)))
std.sort = $(call mk.unpack-list,$(sort $(call mk.pack-list,$1)))
std.strip = $(call mk.unpack-list,$(abspath $(strip mk.pack-list,$1)))

shell.install.dir = $\
	$(let dir-list,$(call shell.quote.list,$1),$\
		$(call shell.trace,INSTALL -d $(dir-list))$(char.nl)$\
		$(INSTALL) -d $(dir-list)$\
	)

shell.install.program = $\
	$(let src-program,$(call shell.quote,$1),$\
		$(let dst-program,$(call shell.quote,$2/$(call std.notdir,$1)),$\
			$(call shell.install.dir,$2)$(char.nl)$\
			$(call shell.trace,INSTALL_PROGRAM $(src-program) $(dst-program))$(char.nl)$\
			$(INSTALL_PROGRAM) $(src-program) $(dst-program)$\
		)$\
	)

shell.install.data = $\
	$(let src-data,$(call shell.quote,$1),$\
		$(let dst-data,$(call shell.quote,$2/$(call std.notdir,$1)),$\
			$(call shell.install.dir,$2)$(char.nl)$\
			$(call shell.trace,INSTALL_DATA $(src-data) $(dst-data))$(char.nl)$\
			$(INSTALL_DATA) $(src-data) $(dst-data)$\
		)$\
	)

shell.rm = $\
	$(let file-list,$(call shell.quote.list,$1),$\
		$(call shell.trace,RM $(file-list))$(char.nl)$\
		$(RM) $(file-list)$\
	)


shell.rmdir = $\
	$(let dir-list,$(call shell.quote.list,$1),$\
		$(call shell.trace,RM -r $(dir-list))$(char.nl)$\
		$(RM) -r $(dir-list)$\
	)

shell.ln-s = $\
	$(let target,$(call shell.quote,$1),$\
		$(let link-path,$(call shell.quote,$2),$\
			$(call shell.trace,ls -s $(target) $(link-path))$(char.nl)$\
			ln -sf $(target) $(link-path)$\
		)$\
	)


shell.find = $(call shell.findx,.,$1,$2)
shell.findx = $\
	$(let cwd,$(call shell.quote,$1),$\
	$(let dir,$(call shell.quote,$2),$\
	$(let args,$3,$\
	$(shell cd $(cwd) && find $(dir) \( $(args) \) -print | sed 's/\\/\\\\/g;s/ /\\ /g;s/:/\\:/g;s/#/\\#/g;s/\$$/\$$\$$/g'))))



# Template
# -----------------------------------------------------------------------------
# This makefile uses templates to generate parts in the user makefile.
#
# In this makefile, templates are multiline variables that get evaluated when
# the template is instantiated.
#
# Conventionally the first parameter `$1` is the prefix/namespace from which
# inputs are read (or outputs are written).
#
# In the template the $ char is replaced by the § char to delay expansions
# until the evaluation that instantiates it.


# -----------------------------------------------------------------------------
# Function: template.unescape
# - $1 An escaped template
#
# Reverts an unescaped template, ready to be evaluated.
# -----------------------------------------------------------------------------
template.unescape = $(subst §,$$,$1)

# -----------------------------------------------------------------------------
# Function: template.eval
# - $1 An escaped template
#
# Unescapes the template then evaluates it.
# -----------------------------------------------------------------------------
template.eval = $(eval $(call template.unescape,$1))

template.instantiate = $(let template.name,$1,$(let 1,$2,$(eval $(value $(template.name)))))


## TODO: document
reverse = $(let first rest,$1,$(if $(rest),$(call reverse,$(rest))$(char.space))$(first))
enumerate = $(let first rest,$1,$(if $(rest),$(call enumerate,$(rest))$(char.space))$(words $1))
lambda = $(let expr,$2,$(call lambda._rec,$1,$(call enumerate,$1)))
lambda._rec = $\
	$(let var var-rest,$1,$\
	$(let num num-rest,$2,$\
	$(if $(var),$\
		$$(let $(var),$$$(num),$(call lambda._rec,$(var-rest),$(num-rest))),$\
		$(expr)$\
	)))
def.fun = $(eval $1 = $(call lambda,$2,$3))
def.fun-esc = $(eval $1 = $(subst §,$$,$(call lambda,$2,$3)))


mk.rule = $(call mk.rule.head,$1)$(char.tab)$(call mk.rule.recipe,$2)
mk.rule.head = $1$(char.nl)
mk.rule.recipe =$(subst $(char.nl),$(char.nl)$(char.tab),$1)$(char.nl)




# defaults
TARGET_ARCH ?= -march=x86-64
CXX ?= clang++
CC ?= clang
AR ?= ar
RANLIB ?= ranlib
RM ?= rm -v -f

INSTALL ?= install
INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA ?= $(INSTALL) -m 644

prefix      ?= /usr/local
exec_prefix ?= $(prefix)
bindir      ?= $(exec_prefix)/bin
libdir      ?= $(exec_prefix)/lib
includedir  ?= $(prefix)/include
datarootdir ?= $(prefix)/share
mandir      ?= $(datarootdir)/man
DESTDIR     ?= 

CXXFLAGS ?=
LDFLAGS  ?=
LDLIBS   ?=
CPPFLAGS ?=

# Each default var is aliased in a `default` namespace
default.TARGET_ARCH = $(TARGET_ARCH)
default.CXX         = $(CXX)
default.CXXFLAGS    = $(CXXFLAGS)
default.LDFLAGS     = $(LDFLAGS)
default.LDLIBS      = $(LDLIBS)
default.CPPFLAGS    = $(CPPFLAGS)


# Config defaults
CONFIG ?= debug
COLOR ?= yes
RUN_WITH_VALGRIND ?= no
ENABLE_PCH ?= yes
ENABLE_COVERAGE ?= no
ENABLE_CMD_CMP ?= yes


# The config file
config-file := .config.mk
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

# Enable command comparison:  [yes], no
ENABLE_CMD_CMP := $(ENABLE_CMD_CMP)

endef

# -----------------------------------------------------------------------------
# This rule create a default `config.mk` file.
#
# The `config.mk` is meant to allow developers to persist some configuration
# values locally without modifing the upstrema makefile. The `config.mk` is
# read early in the init phase.
# -----------------------------------------------------------------------------
$(config-file):
	cat <<-'EOF' > $(call shell.quote,$(config-file))
	$(config-file.content)
	EOF

include $(config-file)


else ifeq ($(mk.next-phase),setup)
#       ▐              ▌
# ▞▀▘▞▀▖▜▀ ▌ ▌▛▀▖   ▛▀▖▛▀▖▝▀▖▞▀▘▞▀▖
# ▝▀▖▛▀ ▐ ▖▌ ▌▙▄▘   ▙▄▘▌ ▌▞▀▌▝▀▖▛▀
# ▀▀ ▝▀▘ ▀ ▝▀▘▌     ▌  ▘ ▘▝▀▘▀▀ ▝▀▘
$(info Loading: setup)

insp = $(error $1 = '$($1)' $(origin $1) : $(flavor $1))

TARGET_ARCH := $(TARGET_ARCH)
CXX := $(CXX)
CC := $(CC)
AR := $(AR)
RANLIB := $(RANLIB)
RM := $(RM)

INSTALL := $(INSTALL)
INSTALL_PROGRAM := $(INSTALL_PROGRAM)
INSTALL_DATA := $(INSTALL_DATA)

prefix := $(prefix)
exec_prefix := $(exec_prefix)
bindir := $(bindir)
libdir := $(libdir)
includedir := $(includedir)
datarootdir := $(datarootdir)
mandir := $(mandir)
DESTDIR := $(DESTDIR)

CXXFLAGS := $(CXXFLAGS)
LDFLAGS := $(LDFLAGS)
LDLIBS := $(LDLIBS)
CPPFLAGS := $(CPPFLAGS)

CONFIG := $(CONFIG)
COLOR := $(COLOR)
RUN_WITH_VALGRIND := $(RUN_WITH_VALGRIND)
ENABLE_PCH := $(ENABLE_PCH)
ENABLE_COVERAGE := $(ENABLE_COVERAGE)
ENABLE_CMD_CMP := $(ENABLE_CMD_CMP)



ifeq ($(COLOR),yes)
ansi.black     := $(char.esc)[0;30m
ansi.red       := $(char.esc)[0;31m
ansi.green     := $(char.esc)[0;32m
ansi.yellow    := $(char.esc)[0;33m
ansi.blue      := $(char.esc)[0;34m
ansi.magenta   := $(char.esc)[0;35m
ansi.cyan      := $(char.esc)[0;36m
ansi.white     := $(char.esc)[0;37m
ansi.hiblack   := $(char.esc)[1;30m
ansi.hired     := $(char.esc)[1;31m
ansi.higreen   := $(char.esc)[1;32m
ansi.hiyellow  := $(char.esc)[1;33m
ansi.hiblue    := $(char.esc)[1;34m
ansi.himagenta := $(char.esc)[1;35m
ansi.hicyan    := $(char.esc)[1;36m
ansi.hiwhite   := $(char.esc)[1;37m
ansi.reset     := $(char.esc)[0m
else
ansi.black     :=
ansi.red       :=
ansi.green     :=
ansi.yellow    :=
ansi.blue      :=
ansi.magenta   :=
ansi.cyan      :=
ansi.white     :=
ansi.hiblack   :=
ansi.hired     :=
ansi.higreen   :=
ansi.hiyellow  :=
ansi.hiblue    :=
ansi.himagenta :=
ansi.hicyan    :=
ansi.hiwhite   :=
ansi.reset     :=
endif # COLOR



ifeq ($(findstring --debug,$(MAKEFLAGS))$(findstring --trace,$(MAKEFLAGS)),)
shell.title = printf -- '[$(ansi.cyan)%s$(ansi.reset)] $(ansi.white)%s$(ansi.reset)\n' "$$$$" "$1"
shell.trace = printf -- '[$(ansi.cyan)%s$(ansi.reset)] %s\n' "$$$$" "$1"
else
shell.title := : -----------------------------------------------------------------------------
shell.trace := : -----------------------------------------------------------------------------
endif

$(info $(ansi.yellow)CONFIG$(ansi.reset) = $(ansi.white)$(CONFIG)$(ansi.reset))
$(info $(ansi.yellow)ENABLE_PCH$(ansi.reset) = $(ansi.white)$(ENABLE_PCH)$(ansi.reset))
$(info $(ansi.yellow)ENABLE_COVERAGE$(ansi.reset) = $(ansi.white)$(ENABLE_COVERAGE)$(ansi.reset))
$(info $(ansi.yellow)ENABLE_CMD_CMP$(ansi.reset) = $(ansi.white)$(ENABLE_CMD_CMP)$(ansi.reset))

ifeq ($(flavor build_dir),undefined)
$(error You must define build_dir before setup phase)
endif

ifeq ($(flavor source_dir),undefined)
$(error You must define source_dir before setup phase)
endif

# -----------------------------------------------------------------------------
# Rule: Create directory rule.
#
# This implicit rule is allowed because the target is a directory without any
# ambiguities.
# -----------------------------------------------------------------------------
.PRECIOUS: %/.
%/.:
	$(call shell.title,MKDIR -p $(call shell.quote,$@))
	mkdir -p $(call shell.quote,$@)


.NOTPARALLEL: clean
.PHONY: clean
clean::
	$(call shell.title,Cleaning)
	$(call shell.trace,rm -rf $(call shell.quote,$(build_dir)))
	rm -rf $(call shell.quote,$(build_dir))


.NOTPARALLEL: run
.PHONY: run
run:: 

.NOTPARALLEL: test
.PHONY: test
test::

.NOTPARALLEL: install
.PHONY: install
install::
	$(call shell.title,Install)



.PHONY: watch
watch: 
	printf -- '%b---------- Watching for changes...%b\n' '$(ansi.yellow)' '$(ansi.reset)'
	inotifywait -qmr -e close_write,delete,move $(call shell.quote,$(source_dir)) | while read -r event; do
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




ifeq ($(ENABLE_COVERAGE),yes)

coverage.build_dir ?= $(build_dir)/coverage
coverage.report_dir ?= $(build_dir)/coverage-report

.PHONY: $(coverage.build_dir)/coverage.profdata
$(coverage.build_dir)/coverage.profdata:
	if ! compgen -G $(call shell.quote,$(coverage.build_dir)/*.profraw) > /dev/null; then
		printf '$(ansi.red)Missing coverage data. Unable to create `coverage.profdata` file.$(ansi.reset)\n'
		exit 1
	fi
	$(call shell.trace,Merging coverage data)
	llvm-profdata merge -output=$(call shell.quote,$(coverage.build_dir)/coverage.profdata) $(call shell.quote,$(coverage.build_dir))/*.profraw
	{
		find $(call shell.quote,$(coverage.build_dir)) -name '*.profraw.binary' -print0 | while IFS= read -r -d '' line; do
			printf '%s\n' '-object'
			cat "$${line}"
			printf '\n'
		done
		printf -- '-sources\n'
		find $(call shell.quote,$(source_dir)) \( -name '*.hpp' -or -name '*.cpp' -or -name '*.cc' \) -print
	} > $(call shell.quote,$(coverage.build_dir)/coverage.profdata).options

.PHONY: coverage-html
coverage-html: $(coverage.build_dir)/coverage.profdata
	$(call shell.trace,Generating HTML coverage report)
	declare -a options
	readarray -t options < $(call shell.quote,$(coverage.build_dir)/coverage.profdata.options)
	llvm-cov show -instr-profile=$(call shell.quote,$(coverage.build_dir)/coverage.profdata) -use-color -format html -output-dir=$(call shell.quote,$(coverage.report_dir)) \
		-Xdemangler c++filt -Xdemangler -n \
		-show-instantiations \
		-show-regions -show-line-counts -show-branches=count -show-mcdc -show-expansions \
		-check-binary-ids\
		"$${options[@]}"
	xdg-open $(call shell.quote,$(coverage.report_dir))/index.html


.PHONY: coverage-term
coverage-term: $(coverage.build_dir)/coverage.profdata
	$(call shell.trace,Generating coverage report for terminal)
	declare -a options
	readarray -t options < $(call shell.quote,$(coverage.build_dir)/coverage.profdata.options)
	llvm-cov report -show-instantiation-summary -instr-profile=$(call shell.quote,$(coverage.build_dir)/coverage.profdata) -use-color "$${options[@]}"
	
$(coverage.build_dir)/lcov.info: $(coverage.build_dir)/coverage.profdata
	$(call shell.trace,Generating lcov info file)
	declare -a options
	readarray -t options < $(call shell.quote,$(coverage.build_dir)/coverage.profdata.options)
	llvm-cov export -instr-profile=$(call shell.quote,$(coverage.build_dir)/coverage.profdata) -format=lcov "$${options[@]}" > $(call shell.quote,$(coverage.build_dir)/lcov.info)


.PHONY: coverage
coverage:: coverage-term coverage-html $(coverage.build_dir)/lcov.info

.PHONY: clean.coverage
clean.coverage:
	rm -r $(call shell.quote,$(coverage.build_dir))

endif # ENABLE_COVERAGE

.PHONY: FORCE
FORCE:;

shell.compile-cc = $\
	$(let object,$(call shell.quote,$2),$\
		$(let source,$(call shell.quote,$3),$\
			$(call shell.title,CXX -o $(object) -c $(source))$(char.nl)$\
			$(CXX) $($1.CXXFLAGS) $($1.CPPFLAGS) $($1.TARGET_ARCH) -MMD -MP -MF $(object).d -o $(object) -c $(source)$(char.nl)$\
			echo $(CXX) $($1.CXXFLAGS) $($1.CPPFLAGS) $($1.TARGET_ARCH) -MMD -MP -MF \'$(object).d\' -o \'$(object)\' -c \'$(source)\' > $(object).cmd$(char.nl)$\
		)$\
	)


rule.compile-cc = $(let object,$2,$(let source,$3,$(rule.compile-cc._impl)))
rule.compile-cc._impl = $\
	$(call mk.rule,$\
		$(object) $(object).d $(object).cmd : $(source) $(shell echo $(CXX) $($1.CXXFLAGS) $($1.CPPFLAGS) $($1.TARGET_ARCH) -MMD -MP -MF \'$(object).d\' -o \'$(object)\' -c \'$(source)\' | cmp -s - $(object).cmd || echo FORCE) | $(call std.dir,$(object)).,$(call shell.compile-cc,$1,$2,$3)$\
	)



# ▐            ▜    ▐
# ▜▀ ▞▀▖▛▚▀▖▛▀▖▐ ▝▀▖▜▀ ▞▀▖▞▀▘
# ▐ ▖▛▀ ▌▐ ▌▙▄▘▐ ▞▀▌▐ ▖▛▀ ▝▀▖
#  ▀ ▝▀▘▘▝ ▘▌   ▘▝▀▘ ▀ ▝▀▘▀▀


# -----------------------------------------------------------------------------
# Helper template: template-compile.cc
# - $1 the namespace
# - $2 the source file
# - $3 the object file to produce
#
# From the namespace (i.e., from the variables prefixed with the namespace
# followed by the character “.”) the following standard variables are
# retrieved:
# - CXXFLAGS
# - CPPFLAGS
# - TARGET_ARCH
#
# This template:
# - generates the compilation rule for the source file to create the object
#   file and a dependency file that is then included.
# - adds to the current target clean rules the cleaning of these artifacts
# -----------------------------------------------------------------------------
template-compile.cc = $\
	$(let 1,$1,$\
	$(let object,$2,$\
	$(let source,$3,$\
	$(let depfile,$(object).d,$\
	$(let cmdfile,$(object).cmd,$\
	$(let object.quoted,$(call shell.quote,$(object)),$\
	$(let source.quoted,$(call shell.quote,$(source)),$\
	$(let depfile.quoted,$(call shell.quote,$(depfile)),$\
	$(let cmdfile.quoted,$(call shell.quote,$(cmdfile)),$\
	$(template-compile.cc._impl)$\)))))))))
define template-compile.cc._impl
# START template-compile.cc

.PHONY: clean.$1
clean.$1::
	declare -a deletables=()
	for i in $(object.quoted) $(depfile.quoted) $(cmdfile.quoted); do
		if [[ -e §§i ]]; then
			deletables+=( "§§i" )
		fi
	done
	if [[ §§{#deletables[@]} == 0 ]]; then
		exit
	fi
	§(call shell.rm,$(object.quoted) $(depfile.quoted) $(cmdfile.quoted))

.PRECIOUS: $(object)
ifeq ($(ENABLE_CMD_CMP),yes)
$(object): §(shell echo §(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF \'$(depfile.quoted)\' -o \'$(object.quoted)\' -c \'$(source.quoted)\' | cmp -s - $(cmdfile.quoted) || echo FORCE)
endif
$(object): $(source) | $(call std.dir,$(object)).
	§(call shell.title,CXX -o $(object.quoted) -c $(source.quoted))
	§(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF $(depfile.quoted) -o $(object.quoted) -c $(source.quoted)
	echo §(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF \'$(depfile.quoted)\' -o \'$(object.quoted)\' -c \'$(source.quoted)\' > $(cmdfile.quoted)

-include $(depfile)

# END template-compile.cc
endef # template-compile.cc._impl


# -----------------------------------------------------------------------------
# Helper template: template-link.cc
# - $1 the namespace
# - $2 the target to generate
# - $3 the object or source files to link
#
# From the namespace (i.e., from the variables prefixed with the namespace
# followed by the character “.”) the following standard variables are
# retrieved:
# - CXXFLAGS
# - CPPFLAGS
# - LDFLAGS
# - TARGET_ARCH
# - LDLIBS
#
# This template:
# - generates the linking rule for the object files to create the target file
#   and a dependencies file that is then included.
# - adds to the current target clean rules the cleaning of these artifacts
# -----------------------------------------------------------------------------
template-link.cc = $(let 1,$1,$(let linked,$2,$(let inputs,$3,$(template-link.cc._impl))))
define template-link.cc._impl
# START template-link,$1,$(linked),$(inputs)

.PHONY: clean.$1
clean.$1::
	declare -a deletables=()
	for i in $(call shell.quote,$(linked)) $(call shell.quote,$(linked).d); do
		if [[ -e §§i ]]; then
			deletables+=( "§§i" )
		fi
	done
	if [[ §§{#deletables[@]} == 0 ]]; then
		exit
	fi
	§(call shell.rm,$(linked) $(linked).d)


.PRECIOUS: $(linked)
ifeq ($(ENABLE_CMD_CMP),yes)
$(linked): §(shell echo $\
	§(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.LDFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF $(call shell.quote,$(linked).d) -o $(call shell.quote,$(linked)) $(call shell.quote.list,$(inputs)) §($1.LDLIBS) | cmp -s - $(linked).cmd || echo FORCE)
endif
$(linked): $(inputs) | $(call std.dir,$(linked)). 
	§(call shell.title,LINK -o $(call shell.quote,$(linked)) $(call shell.quote.list,$(inputs)) §($1.LDLIBS))
	§(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.LDFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF $(call shell.quote,$(linked).d) -o $(call shell.quote,$(linked)) $(call shell.quote.list,$(inputs)) §($1.LDLIBS)
	echo $\
	§(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.LDFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF $(call shell.quote,$(linked).d) -o $(call shell.quote,$(linked)) $(call shell.quote.list,$(inputs)) §($1.LDLIBS) > $(linked).cmd


-include $(linked).d
# END template-link.cc
endef # template-link.cc._impl





# -----------------------------------------------------------------------------
# Main template: template-executable
# - $1 the namespace
#
# From the namespace (i.e., from the variables prefixed with the namespace
# followed by the character “.”) the following standard variables are
# retrieved:
# - $1.CXXFLAGS    ?= $(CXXFLAGS)
# - $1.CPPFLAGS    ?= $(CPPFLAGS)
# - $1.LDFLAGS     ?= $(LDFLAGS)
# - $1.TARGET_ARCH ?= $(TARGET_ARCH)
# - $1.LDLIBS      ?= $(LDLIBS)
#
# These variables are optional. Their default value fallbacks to the value of
# the variable in the default namespace.
#
# The following custom variables provide specific information for the target:
# - $1.build_dir ?= $(build_dir)/$1
# - $1.exe-path  ?= $($1.build_dir)/$1
# - $1.sources   ?=
# - $1.objects   ?=
#
# These variables are optional, but at leaast one file name in `sources` or
# `objects` should be given.
#
# Additionally the following variable may specify a different precompiled
# header target name, that would be used instead of the default one (pch):
# - $1.pch ?= pch
#
# This template:
# - calls the compile.cc helper template (see `compile-link.cc`) for every
#   source file
# - calls the link.cc helper template (see `template-link.cc`)
# - adds to the current target clean rules the removing of the target
#   `$1.build_dir` (if different from the global `build_dir`)
# - creates a phony target that depends on the executable file and execute it,
#   if called
# -----------------------------------------------------------------------------
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
	§(call shell.title,Running executable $1: §(call shell.quote,§($1.exe-path))
	declare -i error_code=0
	set +e
ifeq (§(RUN_WITH_VALGRIND),yes)
	valgrind --leak-check=full --show-leak-kinds=all  --track-origins=yes --show-reachable=yes --error-exitcode=1 -- §(call shell.quote,§($1.exe-path)) §(ARGS)
else
	§(call shell.quote,§($1.exe-path)) §(ARGS)
endif
	error_code=§§?
	set -e
	if (( error_code == 0 )); then
		§(call shell.trace,§(ansi.green)SUCCESS §(call shell.quote,§($1.exe-path))§(ansi.reset))
	else
		§(call shell.trace,§(ansi.red)FAILURE (§§error_code) §(call shell.quote,§($1.exe-path))§(ansi.reset))
	fi

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote,§($1.build_dir)) == §(call shell.quote,§(build_dir)) ]] && exit
	§(call shell.title,Clean executable $1)
	§(call shell.rmdir,§($1.build_dir))

$1.sources.objects := §(foreach ps,$\
	§(call mk.pack-list,§($1.sources)),$\
	§(let $\
		s,§(call mk.unpack-list,§(ps)),$\
		§(let $\
			o,§($1.build_dir)/§(call mk.unpack-list,§(basename §(notdir §(ps))).o),$\
			§(call template.eval,§(call template-compile.cc,$1,§o,§s))$\
			§o$\
		)$\
	)$\
)

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
endef # template-executable



# -----------------------------------------------------------------------------
# Main template: template-test
# - $1 the namespace
#
# From the namespace (i.e., from the variables prefixed with the namespace
# followed by the character “.”) the following standard variables are
# retrieved:
# - $1.CXXFLAGS    ?= $(CXXFLAGS)
# - $1.CPPFLAGS    ?= $(CPPFLAGS)
# - $1.LDFLAGS     ?= $(LDFLAGS)
# - $1.TARGET_ARCH ?= $(TARGET_ARCH)
# - $1.LDLIBS      ?= $(LDLIBS)
#
# These variables are optional. Their default value fallbacks to the value of
# the variable in the default namespace.
#
# The following custom variables provide specific information for the target:
# - $1.build_dir ?= $(build_dir)/$1
# - $1.test-path  ?= $($1.build_dir)/$1
# - $1.sources   ?=
# - $1.objects   ?=
#
# These variables are optional, but at leaast one file name in `sources` or
# `objects` should be given.
#
# Additionally the following variable may specify a different precompiled
# header target name, that would be used instead of the default one (pch):
# - $1.pch ?= pch
#
# This template:
# - calls the compile.cc helper template (see `compile-link.cc`) for every
#   source file
# - calls the link.cc helper template (see `template-link.cc`)
# - adds to the current target clean rules the removing of the target
#   `$1.build_dir` (if different from the global `build_dir`)
# - creates a phony target that depends on the test file and execute it, if
#   called. The execution supports test coverage.
# - adds current test to the `test` target
# -----------------------------------------------------------------------------
define template-test
# START template-test $1
$1.build_dir ?= §(build_dir)/$1
$1.build_dir := §($1.build_dir)

$1.test-name ?= $1
$1.test-name := §($1.test-name)

$1.test-path ?= §($1.build_dir)/§($1.test-name)
$1.test-path := §($1.test-path)

$1.sources ?=
$1.sources := §($1.sources)

$1.objects ?=
$1.objects := §($1.objects)

$1.TARGET_ARCH ?= §(TARGET_ARCH)
$1.TARGET_ARCH := §($1.TARGET_ARCH)

$1.CXXFLAGS ?= §(CXXFLAGS)
$1.CXXFLAGS := §($1.CXXFLAGS)

$1.CPPFLAGS ?= §(CPPFLAGS)
$1.CPPFLAGS := §($1.CPPFLAGS)

$1.LDFLAGS ?= §(LDFLAGS)
$1.LDFLAGS := §($1.LDFLAGS)

$1.LDLIBS ?= §(LDLIBS)
$1.LDLIBS := §($1.LDLIBS)

$1.pch ?= pch
$1.pch := §($1.pch)

$1.CPPFLAGS += §(§($1.pch).out.CPPFLAGS)


ifeq ($(ENABLE_COVERAGE),yes)
$1.CXXFLAGS += -fprofile-instr-generate=§(call shell.quote,§(coverage.build_dir)/$1-%p.profraw) -fcoverage-mapping -fcoverage-mcdc
endif

.PHONY: $1
ifeq ($(ENABLE_COVERAGE),yes)
$1:: | §(coverage.build_dir)/.
endif
$1:: §($1.test-path)
	§(call shell.title,Running test $1: §(call shell.quote,§($1.test-path)))
	declare -i error_code=0
	set +e
ifeq (§(RUN_WITH_VALGRIND),yes)
	valgrind --leak-check=full --show-leak-kinds=all  --track-origins=yes --show-reachable=yes --error-exitcode=1 -- §(call shell.quote,§($1.test-path))
else
	§(call shell.quote,§($1.test-path))
endif
	error_code=§§?
	set -e
ifeq ($(ENABLE_COVERAGE),yes)
	printf -- '%s' §(call shell.quote,§($1.test-path)) > §(call shell.quote,§(coverage.build_dir)/§@.profraw.binary)
endif
	if (( error_code == 0 )); then
		§(call shell.trace,§(ansi.green)PASSED §(call shell.quote,§($1.test-path))§(ansi.reset))
	else
		§(call shell.trace,§(ansi.red)FAILED (§§error_code) §(call shell.quote,§($1.test-path))§(ansi.reset))
	fi

.PHONY: test
test:: $1

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote,§($1.build_dir)) == §(call shell.quote,§(build_dir)) ]] && exit
	§(call shell.title,Clean test $1)
	§(call shell.rmdir,§($1.build_dir))

$1.sources.objects := §(foreach ps,$\
	§(call mk.pack-list,§($1.sources)),$\
	§(let $\
		s,§(call mk.unpack-list,§(ps)),$\
		§(let $\
			o,§($1.build_dir)/§(call mk.unpack-list,§(basename §(notdir §(ps))).o),$\
			§(call template.eval,§(call template-compile.cc,$1,§o,§s))$\
			§o$\
		)$\
	)$\
)



ifeq ($(ENABLE_PCH),yes)
§($1.sources.objects): §(§($1.pch).pch-path)
endif

$1.objects += §($1.sources.objects)

§(call template.eval,§(call template-link.cc,$1,§($1.test-path),§($1.objects)))

.PHONY: install.$1
install.$1:: §($1.test-path)
	§(call shell.title,Install $1)
	§(call shell.install.program,§($1.test-path),§(DESTDIR)§(bindir))

.PHONY: uninstall.$1
uninstall.$1::
	§(call shell.title,Uninstall $1)
	§(call shell.rm,§(DESTDIR)§(bindir)/§($1.test-name)


# END template-test $1
endef # template-test




# -----------------------------------------------------------------------------
# Main template: template-lib
# - $1 the namespace
#
# From the namespace (i.e., from the variables prefixed with the namespace
# followed by the character “.”) the following standard variables are
# retrieved:
# - $1.CXXFLAGS    ?= $(CXXFLAGS)
# - $1.CPPFLAGS    ?= $(CPPFLAGS)
# - $1.TARGET_ARCH ?= $(TARGET_ARCH)
#
# These variables are optional. Their default value fallbacks to the value of
# the variable in the default namespace.
#
# The following custom variables provide specific information for the target:
# - $1.build_dir ?= $(build_dir)/$1
# - $1.lib-name  ?= $1
# - $1.lib-path  ?= $($1.build_dir)/$($1.lib-name).a
# - $1.sources   ?=
# - $1.objects   ?=
#
# These variables are optional, but at leaast one file name in `sources` or
# `objects` should be given.
#
# Additionally the following variable may specify a different precompiled
# header target name, that would be used instead of the default one (pch):
# - $1.pch ?= pch
#
# This template:
# - calls the compile.cc helper template (see `compile-link.cc`) for every
#   source file
# - generates the archiving rule for the object files to create the target
#   static lib
# - adds to the current target clean rules the removing of the target
#   `$1.build_dir` (if different from the global `build_dir`)
# - creates a phony target that depends on the lib file .
# -----------------------------------------------------------------------------
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
$1.out.LDFLAGS := -L§(call shell.quote,§($1.lib-dir))
$1.out.LDLIBS := -l§(patsubst lib%,%,§($1.lib-name))


.PHONY: $1
$1:: §($1.lib-path)

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote,§($1.build_dir)) == §(call shell.quote,§(build_dir)) ]] && exit
	§(call shell.title,Clean static lib $1)
	§(call shell.rmdir,§($1.build_dir))

$1.sources.objects := §(foreach ps,$\
	§(call mk.pack-list,§($1.sources)),$\
	§(let $\
		s,§(call mk.unpack-list,§(ps)),$\
		§(let $\
			o,§($1.build_dir)/§(call mk.unpack-list,§(basename §(notdir §(ps))).o),$\
			§(call template.eval,§(call template-compile.cc,$1,§o,§s))$\
			§o$\
		)$\
	)$\
)

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
endef # template-lib





# -----------------------------------------------------------------------------
# Main template: template-solib
# - $1 the namespace
#
# From the namespace (i.e., from the variables prefixed with the namespace
# followed by the character “.”) the following standard variables are
# retrieved:
# - $1.CXXFLAGS    ?= $(CXXFLAGS)
# - $1.CPPFLAGS    ?= $(CPPFLAGS)
# - $1.LDFLAGS     ?= $(LDFLAGS)
# - $1.TARGET_ARCH ?= $(TARGET_ARCH)
# - $1.LDLIBS      ?= $(LDLIBS)
#
# These variables are optional. Their default value fallbacks to the value of
# the variable in the default namespace.
#
# The following custom variables provide specific information for the target:
# - $1.build_dir ?= $(build_dir)/$1
# - $1.version   ?= 1.0.0
# - $1.lib-name  ?= $1
# - $1.lib-path  ?= $($1.build_dir)/$($1.lib-name).so.$($1.version)
# - $1.soname    ?= $($1.lib-name).so.<MAYOR-NUMBER>
# - $1.sources   ?=
# - $1.objects   ?=
#
# These variables are optional, but at leaast one file name in `sources` or
# `objects` should be given.
#
# Additionally the following variable may specify a different precompiled
# header target name, that would be used instead of the default one (pch):
# - $1.pch ?= pch
#
# This template:
# - calls the compile.cc helper template (see `compile-link.cc`) for every
#   source file
# - generates the linking rule for the object files to create the target shared
#   lib, its 2 symbolic links and a dependencies file that is then included. 
# - adds to the current target clean rules the removing of the target
#   `$1.build_dir` (if different from the global `build_dir`)
# - creates a phony target that depends on the shared lib file
# -----------------------------------------------------------------------------
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
$1.out.LDFLAGS := -L§(call shell.quote,§($1.lib-dir))
$1.out.LDLIBS := -l§(patsubst lib%,%,§($1.lib-name))
$1.link1 := §($1.build_dir)/§($1.lib-name).so
$1.link2 := §($1.build_dir)/§($1.soname)

.PHONY: $1
$1:: §($1.lib-path)

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote,§($1.build_dir)) == §(call shell.quote,§(build_dir)) ]] && exit
	§(call shell.title,Clean shared lib $1)
	§(call shell.rmdir,§($1.build_dir))

$1.sources.objects := §(foreach ps,$\
	§(call mk.pack-list,§($1.sources)),$\
	§(let $\
		s,§(call mk.unpack-list,§(ps)),$\
		§(let $\
			o,§($1.build_dir)/§(call mk.unpack-list,§(basename §(notdir §(ps))).o),$\
			§(call template.eval,§(call template-compile.cc,$1,§o,§s))$\
			§o$\
		)$\
	)$\
)

ifeq ($(ENABLE_PCH),yes)
§($1.sources.objects): §(§($1.pch).pch-path)
endif

$1.objects += §($1.sources.objects)

.PRECIOUS: §($1.lib-path) §($1.link1) §($1.link2)
§($1.lib-path) §($1.link1) §($1.link2) : §($1.objects) | §(call std.dir,§($1.lib-path)).
	§(call shell.title,LINK -shared -o §(call shell.quote,§($1.lib-path)) §(call shell.quote.list,§($1.objects)) §($1.LDLIBS))
	§(CXX)\
		§($1.CXXFLAGS)\
		§($1.CPPFLAGS)\
		§($1.LDFLAGS)\
		§($1.TARGET_ARCH)\
		-MMD -MP -MF §(call shell.quote,§($1.lib-path)).d\
		-o §(call shell.quote,§($1.lib-path))\
		§(call shell.quote.list,§($1.objects))\
		§($1.LDLIBS)\
		# end
	ln -sfr §(call shell.quote,§($1.lib-path)) §(call shell.quote,§($1.link1))
	ln -sfr §(call shell.quote,§($1.lib-path)) §(call shell.quote,§($1.link2))


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
endef # template-solib

# -----------------------------------------------------------------------------
# Main template: template-pch
# - $1 the namespace
#
# From the namespace (i.e., from the variables prefixed with the namespace
# followed by the character “.”) the following standard variables are
# retrieved:
# - CXXFLAGS
# - CPPFLAGS
# - TARGET_ARCH
#
# These variables are optional. Their default value fallbacks to the value of
# the variable in the default namespace.
#
# The following custom variables provide specific information for the target:
# - $1.build_dir ?= $(build_dir)/$1
# - $1.source    ?= $(source_dir)/precompiled.hpp
# - $1.pch-path  ?= $($1.build_dir)/precompiled.pch
#
# These variables are optional.
#
# This template:
# - calls the compile.cc helper template (see `compile-link.cc`) for the source
#   header file to create the precompiled header file and a dependency file
#   that is then included.
# - creates a phony target that depends on the pch file.
# -----------------------------------------------------------------------------
define template-pch
# START template-pch

$1.build_dir ?= §(build_dir)/$1
$1.build_dir := §($1.build_dir)

$1.source ?= §(source_dir)/precompiled.hpp
$1.source := §($1.source)

$1.pch-path ?= §($1.build_dir)/precompiled.pch
$1.pch-path := §($1.pch-path)

$1.CPPFLAGS ?= §(CPPFLAGS)
$1.CPPFLAGS := §($1.CPPFLAGS)
$1.CPPFLAGS += -x c++-header

$1.CXXFLAGS ?= §(CXXFLAGS)
$1.CXXFLAGS := §($1.CXXFLAGS)

$1.TARGET_ARCH ?= §(TARGET_ARCH)
$1.TARGET_ARCH := §($1.TARGET_ARCH)

ifeq ($(ENABLE_PCH),yes)
$1.out.CPPFLAGS ?= -include-pch §(call shell.quote,§($1.pch-path))
else
$1.out.CPPFLAGS :=
endif
$1.out.CPPFLAGS := §($1.out.CPPFLAGS)


.PHONY: $1
$1:: §($1.pch-path)

.PHONY: clean
clean:: clean.$1

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote,§($1.build_dir)) == §(call shell.quote,§(build_dir)) ]] && exit
	§(call shell.title,Clean pch $1)
	§(call shell.rmdir,§($1.build_dir))

§(call template.eval,§(call template-compile.cc,$1,§($1.pch-path),§($1.source)))

# END template-pch
endef # template-pch

$(call template.eval,$(call template-pch,pch))



else
$(error There are no more phases!)

endif # loading phases



## Micro makefile tutorial
#
## # Simple assignemt
# var-name := value
# 
# # Recursive assignment
# var-name = value
# 
# # Recursive assignment with default value
# var-name ?= value
# 
# # Simple or Recursive assignment added to the previous value
# var-name += value
# 
# 
# # Single colon rule, can appear many times but recipe can appear at most 1 time
# target1 target2 ... targetN : prereq1 prereq2 ... | order-only-prereq1 ...
# <TAB>shell recipe...
# 
# # Double colon rule, can appear many times and have many recipe executed in order
# target1 target2 ... targetN :: prereq1 prereq2 ... | order-only-prereq1 ...
# <TAB>shell recipe...
# 
# # Evaluation order
# immediate = deferred
# immediate ?= deferred
# immediate := immediate
# immediate += deferred or immediate
# 
# immediate : immediate
# <TAB>deferred
