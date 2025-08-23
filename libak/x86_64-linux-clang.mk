
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




ifeq ($(mk.next-phase),init)
# ▗    ▗ ▐        ▌
# ▄ ▛▀▖▄ ▜▀    ▛▀▖▛▀▖▝▀▖▞▀▘▞▀▖
# ▐ ▌ ▌▐ ▐ ▖   ▙▄▘▌ ▌▞▀▌▝▀▖▛▀
# ▀▘▘ ▘▀▘ ▀    ▌  ▘ ▘▝▀▘▀▀ ▝▀▘
# toilet -f smblock -W -t init prelude rules
$(info Loading: init)

SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables
MAKEFLAGS += --silent
MAKEFLAGS += --output-sync=target
.ONESHELL:
.DELETE_ON_ERROR:
.SUFFIXES:
.DEFAULT:

.DEFAULT_GOAL := all
.PHONY: all
all::


CONFIG ?= debug
COLOR ?= yes
RUN_WITH_VALGRIND ?= no
ENABLE_PRECOMPILED_HEADER ?= yes
ENABLE_COVERAGE ?= no

# special chars
nothing :=#nothing
char.space := $(nothing) $(nothing)#space
char.lparen := (
char.rparen := )
char.comma := ,
char.backslash := \$(nothing)
char.tab := $(nothing)	$(nothing)#tab
define char.nl :=
$(nothing)
$(nothing)
endef
char.esc := $(shell printf '\e')

# Useful for intending expressions with the $\ trick
$(char.space) :=#nothing



config-file := .config.mk
define config-file.content
# Configuration for the project: [debug], release
CONFIG := $(CONFIG)

# Enable or disable color output: [yes], no
COLOR := $(COLOR)

# Run tests with Valgrind: yes, [no]
RUN_WITH_VALGRIND = $(RUN_WITH_VALGRIND)

# Enable precompiled header:  [yes], no
ENABLE_PRECOMPILED_HEADER = $(ENABLE_PRECOMPILED_HEADER)

# Enable coverage:  yes, [no]
ENABLE_COVERAGE = $(ENABLE_COVERAGE)
$(nothing)
endef

$(config-file):
	cat <<-'EOF' > $@
	$(config-file.content)
	EOF

include $(config-file)




eq = $(let a,x $1,$(let b,x$2,$(and $(findstring $a,$b),$(findstring $b,$a))))
immediate-default = $(eval $(if $(findstring $(flavor $1),undefined),$1 := $2,$1 := $1))
#get_all_dirs = $(let dirs,$(patsubst %/,%,$(wildcard $1/*/)),$(dirs) $(foreach d,$(dirs),$(call get_all_dirs,$d)))
#get_all_files = $(let dirs,$(call get_all_dirs,$1),$(filter-out $(dirs),$(foreach d,$1 $(dirs),$(wildcard $(addprefix $d/,$2)))))


shell.sensitive-chars := $(char.backslash) $$ ` ( ) * ? [ ] ' " & | ; < > { } ~
shell.escape._rec = $(let c r,$2,$(if $c,$(call shell.escape._rec,$(subst $c,\$c,$1),$r),$1))
shell.escape = $(subst $(char.space),\$(char.space),$(call shell.escape._rec,$1,$(shell.sensitive-chars)))

shell.quote = '$(subst ','\'',$1)'
shell.quote.var = $(call shell.quote,$(call mk.unescape,$1))
shell.quote.list = $(foreach i,$(call mk.pack,$1),$(call shell.quote.var,$(call mk.unpack,$i)))

# Escape unescaped file path
##       \ -> \\           #
## (space) -> \(space)     #
mk.escape = $(subst $(char.space),\$(char.space),$(subst \,\\,$1))

# Unescape escaped file path
## \(space) -> (space)     #
##       \\ -> \           #
mk.unescape = $(subst \\,\,$(subst \$(char.space),$(char.space),$1))

# Pack escaped file path list
##        & -> &&
## \(space) -> &+
mk.pack   = $(subst \$(char.space),&+,$(subst &,&&,$1))

# Unpack packed file path list
## &+ -> \(space)
## && -> &
mk.unpack = $(subst &&,&,$(subst &+,\$(char.space),$1))

std.dir = $(call mk.unpack,$(dir $(call mk.pack,$1)))
std.notdir = $(call mk.unpack,$(notdir $(call mk.pack,$1)))
std.suffix = $(call mk.unpack,$(suffix $(call mk.pack,$1)))
std.basename = $(call mk.unpack,$(basename $(call mk.pack,$1)))
std.addsuffix = $(call mk.unpack,$(addsuffix $1,$(call mk.pack,$2)))
std.addprefix = $(call mk.unpack,$(addprefix $1,$(call mk.pack,$2)))
std.join = $(call mk.unpack,$(join $(call mk.pack,$1),$(call mk.pack,$2)))
std.realpath = $(call mk.unpack,$(realpath $(call mk.pack,$1)))
std.abspath = $(call mk.unpack,$(abspath $(call mk.pack,$1)))
std.firstword = $(call mk.unpack,$(firstword $(call mk.pack,$1)))
std.lastword = $(call mk.unpack,$(firstword $(call mk.pack,$1)))
std.sort = $(call mk.unpack,$(sort $(call mk.pack,$1)))
std.strip = $(call mk.unpack,$(abspath $(strip mk.pack,$1)))


# filenames with spaces
# note: single backslash chars are not supported

# this sequence is not allowed to appear in file names
mk.prereq.sep := {}

# Pack escaped file path list into a list with separator
##        % -> %%
## \(space) -> %+
##  (space) -> (space){}(space)
##       %+ -> \(space)
##       %% -> %
mk.prereq = $(call mk.unpack,$(subst $(char.space),$(char.space)$(mk.prereq.sep)$(char.space),$(strip $(call mk.pack,$1))))

# Support pseudo automatic var
##        % -> %%
##        \ -> %.
##  (space) -> %+  <--- Unsplitting
mk.prereq.pack = $(subst $(char.space),%+,$(subst $(char.backslash),%.,$(subst %,%%,$1)))
##  %+ -> (space)  <--- Splitting   #
##        %. -> \                   #
##        %% -> %                   #
mk.prereq.unpack = $(subst %%,%,$(subst %+,$(char.space),$(subst %.,$(char.backslash),$1)))

# note: All automatic variables lost escaping
auto.target = $(call shell.quote,$@)
auto.prereq1 = $(call shell.quote,$<)

##        % -> %%
##        \ -> %.
##  (space) -> %+       <--- Unsplitting
##   %+{}%+ -> (space)  <--- Splitting on separator
# and foreach
##  %+ -> (space)  #
##  %. -> \        #
##  %% -> %        #
## and then shell.quote
auto.prereqs = $(foreach i,$(subst %+$(mk.prereq.sep)%+,$(char.space),$(call mk.prereq.pack,$+)),$(call shell.quote,$(call mk.prereq.unpack,$i)))
auto.prereqs-uniq = $(foreach i,$(sort $(subst %+$(mk.prereq.sep)%+,$(char.space),$(call mk.prereq.pack,$+))),$(call shell.quote,$(call mk.prereq.unpack,$i)))
auto.prereqs-new = $(foreach i,$(subst %+$(mk.prereq.sep)%+,$(char.space),$(call mk.prereq.pack,$?)),$(call shell.quote,$(call mk.prereq.unpack,$i)))
auto.oo-prereqs  = $(foreach i,$(subst %+$(mk.prereq.sep)%+,$(char.space),$(call mk.prereq.pack,$|)),$(call shell.quote,$(call mk.prereq.unpack,$i)))


# template
template.unescape = $(subst §,$$,$1)
template.eval = $(eval $(call template.unescape,$1))
generate      = $(eval $(call template.unescape,$(let template,template-$1,$(call $(template),$2))))
generate      = $(call template.eval,$(let template,template-$1,$(call $(template),$2)))




TARGET_ARCH := -march=x86-64
CXX := clang++
CC := $(CXX)
AR := ar
RANLIB := ranlib

CXXFLAGS :=
LDFLAGS  :=
LDLIBS   :=
CPPFLAGS :=


$(foreach i,TARGET_ARCH CXX CC CXXFLAGS LDFLAGS LDLIBS CPPFLAGS,$(eval default.$i = $$($i)))





else ifeq ($(mk.next-phase),setup)
#       ▐              ▌
# ▞▀▘▞▀▖▜▀ ▌ ▌▛▀▖   ▛▀▖▛▀▖▝▀▖▞▀▘▞▀▖
# ▝▀▖▛▀ ▐ ▖▌ ▌▙▄▘   ▙▄▘▌ ▌▞▀▌▝▀▖▛▀
# ▀▀ ▝▀▘ ▀ ▝▀▘▌     ▌  ▘ ▘▝▀▘▀▀ ▝▀▘
$(info Loading: setup)


ifeq ($(flavor build_dir),undefined)
$(error You must define build_dir before prelude phase)
endif

ifeq ($(flavor source_dir),undefined)
$(error You must define source_dir before prelude phase)
endif

.PHONY: $(mk.prereq.sep)
$(mk.prereq.sep):;


# Create directory rule.
# This implicit rule is allowed because the target is a directory without any ambiguities
.PRECIOUS: %/.
%/.:
	$(call shell.title,MKDIR -p $(auto.target))
	mkdir -p $(auto.target)

run_%: $(build_dir)/%
	$(call shell.title,"$<")
	"$<"


.PHONY: clean
clean::
	$(call shell.title,Cleaning)
	$(call shell.trace,rm -rf $(call shell.quote.var,$(build_dir)))
	rm -rf $(call shell.quote.var,$(build_dir))


.PHONY: run
run:: 

.PHONY: test
.NOTPARALLEL: test
test::


.PHONY: watch
watch: 
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

ifeq ($(ENABLE_PRECOMPILED_HEADER),yes)
precompiled_pch ?= $(build_dir)/precompiled.pch
precompiled_pch.source ?= $(source_dir)/precompiled.hpp
precompiled_pch.depfile ?= $(precompiled_pch).d
precompiled_pch.CPPFLAGS ?= -include-pch $(call shell.quote.var,$(precompiled_pch))

$(precompiled_pch): private CPPFLAGS := $(CPPFLAGS)
$(precompiled_pch): $(precompiled_pch.source) | $(build_dir)/.
	$(call shell.title,CXX -o $(auto.target) -c $(auto.prereq1))
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -x c++-header -MMD -MP -MF $(call shell.quote.var,$(precompiled_pch.depfile)) -o $(auto.target) -c $(auto.prereq1)


CPPFLAGS := $(CPPFLAGS)
override CPPFLAGS += $(precompiled_pch.CPPFLAGS)

-include $(precompiled_pch.depfile)
else
precompiled_pch :=
precompiled_pch.CPPFLAGS :=
endif




ifeq ($(ENABLE_COVERAGE),yes)

CXXFLAGS += -fprofile-instr-generate -fcoverage-mapping
LDFLAGS += -fprofile-instr-generate

#coverage.sources ?= $(call get_all_files,$(source_dir),*.hpp *.cc)
#coverage.sources ?= $(shell find $(call shell.quote.var,$(source_dir)) \( -name '*.hpp' -or -name '*.cc' \) -exec printf '%q ' {} \;)
coverage.build_dir ?= $(build_dir)/coverage
coverage.report_dir ?= $(build_dir)/coverage-report

export LLVM_PROFILE_FILE = $(if $(call eq,$(flavor @),undefined),,$(call mk.unescape,$(coverage.build_dir)/$@.profraw))


.PHONY: $(coverage.build_dir)/coverage.profdata
$(coverage.build_dir)/coverage.profdata:
	$(call shell.trace,Merging coverage data)
	llvm-profdata merge -output=$(auto.target) $(coverage.build_dir)/*.profraw
	{
		find $(call shell.quote.var,$(coverage.build_dir)) -name "*.profraw.binary" -print0 | while IFS= read -r -d '' line; do
			printf '%s\n' '-object'
			cat "$${line}"
			printf '\n'
		done
		printf -- '-sources\n'
		find $(call shell.quote.var,$(source_dir)) \( -name '*.hpp' -or -name '*.cpp' -or -name '*.cc' \) -print
	} > $(auto.target).options

.PHONY: coverage-html
coverage-html: $(coverage.build_dir)/coverage.profdata
	$(call shell.trace,Generating HTML coverage report)
	declare -a options
	readarray -t options < $(auto.prereq1).options
	llvm-cov show -instr-profile=$(auto.prereq1) -use-color -format html -output-dir=$(call shell.quote.var,$(coverage.report_dir)) \
		-Xdemangler c++filt -Xdemangler -n \
		-show-instantiations \
		-show-regions -show-line-counts -show-branches=count -show-mcdc -show-expansions \
		-check-binary-ids\
		"$${options[@]}"


.PHONY: coverage-term
coverage-term: $(coverage.build_dir)/coverage.profdata
	$(call shell.trace,Generating coverage report for terminal)
	declare -a options
	readarray -t options < $(auto.prereq1).options
	llvm-cov report -instr-profile=$(auto.prereq1) -use-color "$${options[@]}"
	
$(coverage.build_dir)/lcov.info: $(coverage.build_dir)/coverage.profdata
	$(call shell.trace,Generating lcov info file)
	declare -a options
	readarray -t options < $(auto.prereq1).options
	llvm-cov export -instr-profile=$(auto.prereq1) -format=lcov "$${options[@]}" > $(coverage.build_dir)/lcov.info


.PHONY: coverage
coverage:: coverage-term coverage-html $(coverage.build_dir)/lcov.info

.PHONY: clean.coverage
clean.coverage:
	rm -r $(call shell.quote.var,$(coverage.build_dir))

endif # coverage



ifeq ($(CONFIG),debug)
CXXFLAGS += -g
endif



ifeq ($(COLOR),yes)
ansi.normal    := $(char.esc)[m
ansi.hi        := $(char.esc)[1m
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
ansi.normal    :=
ansi.bold      :=
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

endif


ifeq ($(findstring --debug,$(MAKEFLAGS))$(findstring --trace,$(MAKEFLAGS)),)
shell.title = printf -- '[$(ansi.hiwhite)%s$(ansi.reset)] $(ansi.white)%s$(ansi.reset)\n' "$$$$" "$1"
shell.trace = printf -- '[$(ansi.hiwhite)%s$(ansi.reset)] %s\n' "$$$$" "$1"
else
shell.title := :
shell.trace := :
endif



# ▐            ▜    ▐
# ▜▀ ▞▀▖▛▚▀▖▛▀▖▐ ▝▀▖▜▀ ▞▀▖▞▀▘
# ▐ ▖▛▀ ▌▐ ▌▙▄▘▐ ▞▀▌▐ ▖▛▀ ▝▀▖
#  ▀ ▝▀▘▘▝ ▘▌   ▘▝▀▘ ▀ ▝▀▘▀▀

define _template-compile.cc
# START template-compile.cc
.PRECIOUS: $(object)

$(object): $(source) §(precompiled_pch) | $(call std.dir,$(object)).
	§(call shell.title,CXX -o $(call shell.quote.var,$(object)) -c $(call shell.quote.var,$(source)))
	§(CXX) §($1.CXXFLAGS) §($1.CPPFLAGS) §($1.TARGET_ARCH) -MMD -MP -MF $(call shell.quote.var,$(object)).d -o $(call shell.quote.var,$(object)) -c $(call shell.quote.var,$(source))


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
	§(call shell.trace,rm §§{deletables[@]})
	rm "§§{deletables[@]}"

.PHONY: clean
clean:: clean.$1

-include $(object).d
# END template-compile.cc
endef # _template-compile.cc
template-compile.cc = $(let 1,$1,$(let source,$2,$(let object,$3,$(_template-compile.cc))))

#▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

define _template-link.cc
# START template-link,$1,$(target),$(objects)


.PRECIOUS: $(target)
$(target): $(objects) | $(call std.dir,$(target)). $(precompiled_pch)
	§(call shell.title,LINK -o $(call shell.quote.var,$(target)) $(call shell.quote.list,$(objects)) §($1.LDLIBS))
	declare -a opt=(                                       \
		§($1.CXXFLAGS)                                     \
		§($1.CPPFLAGS)                                     \
		§($1.LDFLAGS)                                      \
		§($1.TARGET_ARCH)                                  \
		-MMD -MP -MF $(call shell.quote.var,$(target)).d   \
		-o $(call shell.quote.var,$(target))               \
		$(call shell.quote.list,$(objects))                \
		§($1.LDLIBS)                                       \
	)
	§(CXX) "§§{opt[@]}"

.PHONY: clean.$1
clean.$1::
	[[ ! -e $(call shell.quote.var,$(target)) ]] && exit
	$(call shell.trace,$(call shell.quote.var,$(target)))
	rm $(call shell.quote.var,$(target))

.PHONY: clean
clean:: clean.$1

# END template-link
endef # template-link
template-link.cc = $(let 1,$1,$(let target,$2,$(let objects,$3,$(_template-link.cc))))



#▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀


define template-executable
# START template-executable


# END template-executable
endef # template-executable

#▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀


define template-test
# START template-test $1
$1.build_dir ?= §(build_dir)/$1
$1.test-path ?= §($1.build_dir)/§(call mk.escape,$1)
$1.sources ?=
$1.objects ?=
$1.TARGET_ARCH ?= §(TARGET_ARCH)
$1.CXXFLAGS ?= §(CXXFLAGS)
$1.CPPFLAGS ?= §(CPPFLAGS)
$1.LDFLAGS ?= §(LDFLAGS)
$1.LDLIBS ?= §(LDLIBS)


$1.objects += §(foreach s,$\
	§(call mk.pack,§($1.sources)),$\
	§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(call mk.pack,§s))).o)$\
)


§(foreach s,$\
	§(call mk.pack,§($1.sources)),$\
	§(call template.eval,$\
		§(call template-compile.cc,$\
			$1,$\
			§(call mk.unpack,§s),$\
			§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(call mk.pack,§s))).o)$\
		)$\
	)$\
)


§(call template.eval,§(call template-link.cc,$1,§($1.test-path),§($1.objects)))


.PHONY: $1
ifeq (§(ENABLE_COVERAGE),yes)
$1:: | §(coverage.build_dir)/.
endif
$1:: §($1.test-path)
	§(call shell.title,Running test $1: §(auto.prereq1))
	declare -i error_code=0
	set +e
ifeq (§(RUN_WITH_VALGRIND),yes)
	valgrind --leak-check=full --show-leak-kinds=all  --track-origins=yes --show-reachable=yes --error-exitcode=1 -- §(auto.prereq1)
else
	§(auto.prereq1)
endif
	error_code=§§?
	set -e
ifeq (§(ENABLE_COVERAGE),yes)
	printf -- '%s' §(auto.prereq1) > §(call shell.quote.var,§(coverage.build_dir)/§@.profraw.binary)
endif
	if (( error_code == 0 )); then
		§(call shell.trace,§(ansi.green)FINISH §(auto.prereq1)§(ansi.reset))
	else
		§(call shell.trace,§(ansi.red)FAILED (§§error_code) §(auto.prereq1)§(ansi.reset))
	fi

.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote.var,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote.var,§($1.build_dir)) == §(call shell.quote.var,§(build_dir)) ]] && exit
	§(call shell.title,Clean test $1)
	§(call shell.trace,rm -r §(call shell.quote.var,§($1.build_dir)))
	rm -r §(call shell.quote.var,§($1.build_dir))

test:: $1
# END template-test $1
endef # template-test


#▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀


define template-lib
# START template-lib
$1.build_dir ?= §(build_dir)/$1
$1.lib-name ?= $1
$1.lib-path ?= §($1.build_dir)/§($1.lib-name).a
$1.sources ?=
$1.objects ?=
$1.CXXFLAGS ?= §(CXXFLAGS)
$1.CPPFLAGS ?= §(CPPFLAGS)
$1.LDFLAGS ?= §(LDFLAGS)
$1.TARGET_ARCH ?= §(TARGET_ARCH)
$1.LDLIBS ?= §(LDLIBS)

$1.objects += §(foreach s,§(call mk.pack,§($1.sources)),§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(call mk.pack,§s))).o))

.PHONY: $1
$1:: §($1.lib-path)

.PRECIOUS: §($1.lib-path)
§($1.lib-path): §($1.objects) | §(call mk.unpack,§(dir §(call mk.pack,§($1.lib-path)))).
	§(call shell.title,AR rcs §($1.lib-path) §($1.objects))
	§(AR) rcs §($1.lib-path) §($1.objects)
	§(RANLIB) §($1.lib-path)


.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote.var,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote.var,§($1.build_dir)) == §(call shell.quote.var,§(build_dir)) ]] && exit
	§(call shell.title,Clean static lib $1)
	§(call shell.trace,rm -r §(call shell.quote.var,§($1.build_dir)))
	rm -r §(call shell.quote.var,§($1.build_dir))

.PHONY: clean
clean:: clean.$1

§(foreach s,$\
	§(call mk.pack,§($1.sources)),$\
	§(call template.eval,$\
		§(call template-compile.cc,$\
			$1,$\
			§(call mk.unpack,§s),$\
			§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(call mk.pack,§s))).o)$\
		)$\
	)$\
)

# END template-lib
endef # template-lib





define template-solib
# START template-lib
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
$1.TARGET_ARCH ?= §(TARGET_ARCH)
$1.LDLIBS ?= §(LDLIBS)

$1.objects += §(foreach s,§(call mk.pack,§($1.sources)),§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(call mk.pack,§s))).o))

.PHONY: $1
$1:: §($1.lib-path)

.PRECIOUS: §($1.lib-path)
§($1.lib-path): §($1.objects) | §(call mk.unpack,§(dir §(call mk.pack,§($1.lib-path)))).
	§(call shell.title,LINK -shared -o §(call shell.quote.var,§($1.lib-path)) §(call shell.quote.list,§($1.objects)) §($1.LDLIBS))
	§(CXX)\
		§($1.CXXFLAGS) -fPIC\
		§($1.CPPFLAGS)\
		§($1.LDFLAGS)\
		-shared -Wl,-soname,§($1.soname)\
		§($1.TARGET_ARCH)\
		-MMD -MP -MF §(call shell.quote.var,§($1.lib-path)).d\
		-o §(call shell.quote.var,§($1.lib-path))\
		§(call shell.quote.list,§($1.objects))\
		§($1.LDLIBS)\
		# end
	ln -sfr §($1.lib-path) §($1.build_dir)/§($1.lib-name).so
	ln -sfr §($1.lib-path) §($1.build_dir)/§($1.soname)



.PHONY: clean.$1
clean.$1::
	[[ ! -e §(call shell.quote.var,§($1.build_dir)) ]] && exit
	[[ §(call shell.quote.var,§($1.build_dir)) == §(call shell.quote.var,§(build_dir)) ]] && exit
	§(call shell.title,Clean shared lib $1)
	§(call shell.trace,rm -r §(call shell.quote.var,§($1.build_dir)))
	rm -r §(call shell.quote.var,§($1.build_dir))

.PHONY: clean
clean:: clean.$1

§(foreach s,$\
	§(call mk.pack,§($1.sources)),$\
	§(call template.eval,$\
		§(call template-compile.cc,$\
			$1,$\
			§(call mk.unpack,§s),$\
			§($1.build_dir)/§(call mk.unpack,§(basename §(notdir §(call mk.pack,§s))).o)$\
		)$\
	)$\
)

# END template-so
endef # template-so


else
$(error There are no more phases!)

endif











comment.do ?=
ifneq ($(comment.do),)
define comment
## Notes on makefile
## see -> 3.7 How make Reads a Makefile
```
immediate = deferred
immediate ?= deferred
immediate := immediate
immediate += deferred or immediate

immediate : immediate ; deferred
	deferred
```

## Loading phases

INIT phase
- make initial config
- load/create config.mk
- set deferred values
- set immediate values that do not depend on user choices

USER
- user sets his defaults:
	- he must set `build_dir`
	- he should set `source_dir`

SETUP phase
- immediate values that depend on user choices

USER
- user sets rules, generate templates, etc


## Automatic variables with support to spaces
$@  ->  $(auto.target)
$<  ->  $(auto.prereq1)
$^  ->  $(auto.prereqs-uniq)
$+  ->  $(auto.prereqs)
$?  ->  $(auto.prereqs-new)
$|  ->  $(auto.oo-prereqs)

endef
$(comment.do)
endif