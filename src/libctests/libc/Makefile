# $Id: Makefile 3351 2007-05-07 02:20:15Z bird $


# default rule
all: tests-logged

#
# Basic setup.
#
OUTDIR = ./out
ABSOUTDIR = $(CURDIR)/out
TS     = $(shell date '+%Y-%m-%d-%H-%M-%S')
LOG    = $(OUTDIR)/$(1)-$(TS).log
CC     = gcc
CFLAGS = -g -Zomf -O -D_GNU_SOURCE -std=gnu99 -DOBJPFX=\"$(OUTDIR)/\" -I include


#
# Skipped tests, Expected failures.
#
ifeq ($(TARGET),)
TARGET     := innoteklibc
endif

ifeq ($(TARGET),glibc)
SKIPPED    :=
EXPECTED   :=
_TARGET_OK := ok
CFLAGS     += -D__GLIBC__
endif

ifeq ($(TARGET),innoteklibc)
SKIPPED    :=
EXPECTED   := smoketests/usleep-1.c
_TARGET_OK := ok
endif

ifeq ($(TARGET),bsd)
SKIPPED    :=
EXPECTED   :=
_TARGET_OK := ok
CFLAGS     += -D__BSD__
endif

ifndef _TARGET_OK
$(error Invalid TARGET definition '$(TARGET)'. valid: glibc, innoteklibc, bsd)
endif


nothing:
	@echo TS=$(TS)


#
# The smoke tests (various bugs and simple tests)
#

SMOKETESTS := \
	smoketests/64bitio-1.c \
	smoketests/access-1.c \
	smoketests/alloca-1.c \
	smoketests/atexit-1.c \
	smoketests/builtin-1.c \
	smoketests/execve-1.c \
	smoketests/fork-1.c \
	smoketests/fork-2.c \
	smoketests/fork-3.c \
	smoketests/fsinternals-1.c \
	smoketests/fsinternals-2.c \
	smoketests/isinfnan-1.c \
	smoketests/lseek-1.c \
	smoketests/highmalloc-1.c \
	smoketests/mbstuff-1.c \
	smoketests/nan-1.c \
	smoketests/nl_langinfo-1.c \
	smoketests/open-dev-null-1.c \
	smoketests/opendir-1.c \
	smoketests/pathrewrite-1.c \
	smoketests/paths-1.c \
	smoketests/setlocale-1.c \
	smoketests/spm-1.c \
	smoketests/stat-1.c \
	smoketests/strcoll-1.c \
	smoketests/strnlen-1.c \
	smoketests/strtof-1.c \
	smoketests/symlink-1.c \
	smoketests/sysctl-1.c \
	smoketests/socket-1.c \
	smoketests/tls-1.c \
	smoketests/tempnam-1.c \
	smoketests/tmpfile-1.c \
	smoketests/toupperlower-1.c \
	smoketests/toupperlower-2.c \
	smoketests/usleep-1.c \
	smoketests/waitpid-1.c \
	smoketests/wcscoll-1.c \
	smoketests/wctype-1.c \
	smoketests/embryo-1.c \
	smoketests/fork-exec-1.c \
	smoketests/kill-children-1.c \
	smoketests/kill-children-2.c \
	smoketests/memcpy-amd-1.c \
	smoketests/fstat-1.c \
	smoketests/fchmod-1.c \
	smoketests/nonblock-1.c \
	smoketests/fclose-2.c \
	smoketests/basename-dirname-1.c \
	smoketests/fcloseall-1.c \
	smoketests/rmdir-1.c

#	smoketests/weak-export-1.c - dll

smoketests/64bitio-1.c_ARGS = $(OUTDIR)/smoketests/2gb.file
smoketests/alloca-1.c_LDFLAGS = -Zstack 1024
smoketests/atexit-1.c_DEPS = $(OUTDIR)/smoketests/atexit-1.dll
smoketests/fork-3.c_DEPS = $(OUTDIR)/smoketests/fork-3.dll
smoketests/fsinternals-1.c_CFLAGS = -I ../../emx/src/lib
smoketests/fsinternals-1.c_LDFLAGS = -Zomf -static -lc_omf386
smoketests/fsinternals-2.c_CFLAGS = -I ../../emx/src/lib
smoketests/fsinternals-2.c_LDFLAGS = -Zomf -static -lc_omf386
smoketests/nl_langinfo-1.c_ENVS = LC_ALL=en_US.UTF-8
smoketests/spm-1.c_LDFLAGS = -Zomf -static -lc_omf386
#smoketests/embryo-1.c_DEPS = $(OUTDIR)/smoketests/asmstub.exe
#smoketests/embryo-1.c_ARGS = $(ABSOUTDIR)/smoketests/asmstub.exe
smoketests/embryo-1.c_DEPS = $(OUTDIR)/smoketests/asmstub-omf.exe
smoketests/embryo-1.c_ARGS = $(ABSOUTDIR)/smoketests/asmstub-omf.exe

$(OUTDIR)/smoketests/asmstub.exe: smoketests/asmstub.S
	mkdir -p $(dir $@)
	gcc -s -nostdlib -o $@ $^

$(OUTDIR)/smoketests/asmstub-omf.exe: smoketests/asmstub.S
	mkdir -p $(dir $@)
	gcc -c -Zaout -o $(OUTDIR)/smoketests/asmstub-omf.o smoketests/asmstub.S
	emxomf -m__text -o $(OUTDIR)/smoketests/asmstub-omf.obj $(OUTDIR)/smoketests/asmstub-omf.o
	gcc -Zomf -lend -s -nostdlib -o $@ $(OUTDIR)/smoketests/asmstub-omf.obj

$(OUTDIR)/smoketests/atexit-1.dll: smoketests/atexit-1-dll.c Makefile
	mkdir -p $(dir $@)
	gcc -shared $(CFLAGS) -o $@ $<

$(OUTDIR)/smoketests/fork-3.dll: smoketests/fork-3-dll.c Makefile
	mkdir -p $(dir $@)
	gcc -shared $(CFLAGS) -o $@ $<


#
# I/O tests
#

IO := \
	io/open-1.c \
	io/open-2.c \
	io/sprintf-1.c \
	io/sscanf-1.c


SUBTESTS := \
	SMOKETESTS \
	IO

.SUFFIXES:
.SUFFIXES: .c .exe .result


#
# Generate test rules
#
_TESTS :=

# process one test
define def_simple

$(eval _EXEC := $(strip $(if $($1_DIR),\
	cd $($(1)_DIR) && LC_ALL=C $($1_ENVS) $(ABSOUTDIR)/$(2).exe $($1_ARGS),\
	cd $(dir $(OUTDIR)/$2) && LC_ALL=C $($1_ENVS) ./$(notdir $2).exe $($1_ARGS))))

$(OUTDIR)/$2.result: $1 $(OUTDIR)/$(dir $1).dir_created $($(1)_DEPS)
	@echo "########## $1"; \
	echo  "##  BUILDING: $$(CC) $$(CFLAGS) $$(LDFLAGS) -o $(OUTDIR)/$2.exe $($(1)_CFLAGS) $($(1)_LDFLAGS) $$<"; \
	if $$(CC) $$(CFLAGS) $$(LDFLAGS) -o $(OUTDIR)/$2.exe $($(1)_CFLAGS) $($(1)_LDFLAGS) $$<; then \
		echo "##  RUNNING : ( $(_EXEC) )$(if $($(1)_INPUT), < $($(1)_INPUT))"; \
		if ( $(_EXEC) )$(if $($(1)_INPUT), < $($(1)_INPUT)); then \
			echo OK > $$@; \
			printf "########## %-26s - SUCCESS\n" "$1"; \
		else \
			echo "##  rc=$$$$?"; \
			printf "########## %-26s - $(if $(filter $1,$(EXPECTED)),EXPECTED FAILURE,FAILURE)\n" "$1"; \
			echo "$(if $(filter $1,$(EXPECTED)),EXPECTED,FAILURE)" > $$@; \
		fi \
	else \
		echo "##  rc=$$$$? "; \
		echo "##  $$(CC) $$(CFLAGS) -o $(OUTDIR)/$2.exe $($(1)_CFLAGS) $($(1)_LDFLAGS) $$<"; \
		printf "########## %-26s - BUILD FAILURE\n" "$1";  \
		echo "FAILURE - $$(CC) $$(CFLAGS) -o $(OUTDIR)/$2.exe $($(1)_CFLAGS) $($(1)_LDFLAGS) $$<" > $$@; \
	fi

$2: $(OUTDIR)/$2.result
	@

_TESTS += $(OUTDIR)/$2.result
$(eval _$(TEST) += $(OUTDIR)/$2.result)
endef

# skip on test
define def_skipped
$(OUTDIR)/$2.result: $(OUTDIR)/$(dir $1).dir_created
	@printf "########## %-26s - SKIPPED\n" "$1"; \
	echo "SKIPPED" > $$@

$2: $(OUTDIR)/$2.result
	@

_TESTS += $(OUTDIR)/$2.result
$(eval _$(TEST) += $(OUTDIR)/$2.result)
endef

# process a subtest
define def_subtest
$(eval test := $(shell echo $(TEST) | tr 'A-Z' 'a-z'))
$(eval _$(TEST) :=)
$(foreach src, $($(TEST)), \
	$(if $(filter $(src),$(SKIPPED)),\
		$(eval $(call def_skipped,$(src),$(basename $(src)),)),\
		$(eval $(call def_simple,$(src),$(basename $(src)),)) \
	) \
)

.PHONY: $(test)
$(test): $(_$(TEST))
	@echo "**** $$@ ****"
	@echo " SUCCESS : `cat /dev/null $$^ | grep OK | wc -l`"
	@echo " SKIPPED : `cat /dev/null $$^ | grep SKIPPED | wc -l`"
	@echo " EXPECTED: `cat /dev/null $$^ | grep EXPECTED | wc -l`"
	@echo " FAILURES: `cat /dev/null $$^ | grep FAILURE | wc -l`"

endef

# process all subtests
$(foreach TEST, $(SUBTESTS), $(eval $(def_subtest)))


#
# generate directory rules
#
define def_dir
$1.dir_created:
	@mkdir -p $1
	@echo > $$@
endef

_DIRS := $(sort $(dir $(_TESTS)))
$(foreach d,$(_DIRS), $(eval $(call def_dir,$(d))))


#
# All tests (expect the ones in the next sections).
#
tests: $(_TESTS)
	@echo "***************"
	@echo " SUCCESS : `cat /dev/null $^ | grep OK | wc -l`"
	@echo " SKIPPED : `cat /dev/null $^ | grep SKIPPED | wc -l`"
	@echo " EXPECTED: `cat /dev/null $^ | grep EXPECTED | wc -l`"
	@echo " FAILURES: `cat /dev/null $^ | grep FAILURE | wc -l`"

tests-logged:
	@echo "**** logging to $(call LOG,test) ****"
	@mkdir -p $(OUTDIR)
	-$(MAKE) tests 2>&1 | tee $(call LOG,test)

