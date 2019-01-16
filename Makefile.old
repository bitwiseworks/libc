# $Id: Makefile 3935 2014-10-26 13:49:41Z bird $
#
# Top level makefile.
#
# Copyright (c) 2003-2014 Knut St. Osmundsen
# Copyright (c) 2003-2004 InnoTek Systemberatung GmbH
# Author: knut st. osmundsen <bird@anduin.net>
#
# All Rights Reserved
#


###############################################################################
##   Global Variables
##   (D = no drive)
###############################################################################
PWD            := $(shell pwd)
PWDD           := $(shell pwd|sed 's/^[a-zA-Z]://')
MAKEFILE		= $(PWD)/Makefile

ifndef BUILD_MODE
export BUILD_MODE=DEBUG
endif

ifndef PATH_TOP
export PATH_TOP  := $(PWD)
endif
ifndef PATH_TOPD
export PATH_TOPD := $(PWDD)
endif


export PATH_BIN        = $(PATH_TOP)/bin/$(BUILD_PLATFORM)/$(BUILD_MODE)
export PATH_BIND       = $(PATH_TOPD)/bin/$(BUILD_PLATFORM)/$(BUILD_MODE)
export PATH_OBJ        = $(PATH_TOP)/obj/$(BUILD_PLATFORM)/$(BUILD_MODE)
export PATH_OBJD       = $(PATH_TOPD)/obj/$(BUILD_PLATFORM)/$(BUILD_MODE)
export PATH_BUILTUNIX  = $(PATH_OBJ)/builtunix
export PATH_BUILTUNIXD = $(PATH_OBJD)/builtunix
export PATH_BUILTTOOLS = $(PATH_BUILTUNIX)/usr
export PATH_BUILTTOOLSD= $(PATH_BUILTUNIXD)/usr
PATH_BUILTUNIX_TMP     = $(PATH_BUILTUNIX).tmp
PATH_BUILTUNIX_TMPD    = $(PATH_BUILTUNIXD).tmp
PATH_BUILTTOOLS_TMP    = $(PATH_BUILTUNIX_TMP)/usr
PATH_BUILTTOOLS_TMPD   = $(PATH_BUILTUNIX_TMPD)/usr

# Debug info or not (when ever we feel like passing down such options).
# Several ways to do this it seems.
ifeq "$(BUILD_MODE)" "RELEASE"
BUILD_DEBUGINFO = -s
BUILD_ENABLE_SYMBOLS = --disable-symbols
BUILD_OPTIMIZE	= -O2 -mcpu=pentium -mpreferred-stack-boundary=2 -malign-strings=0 -falign-loops=2 -falign-jumps=2 -falign-functions=2
else
BUILD_OPTIMIZE	= -O0
BUILD_DEBUGINFO = -g
BUILD_ENABLE_SYMBOLS = --enable-symbols
endif


# Version and CVS defines - real stuff in env.cmd.
export GCC_VERSION              ?= 3.3.5
export GCC_VERSION_SHORT        ?= 335
ifdef OFFICIAL_BIRD_VERSION
export GCC_RELEASE_ID           ?= csd6
else
export GCC_RELEASE_ID           ?= x
endif
export GCC_CVS_VENDOR			?= GNU
export GCC_CVS_REL				?= GCC_3-3-5
export BINUTILS_VERSION 		?= 2.14
export BINUTILS_VERSION_SHORT	?= 214
export BINUTILS_CVS_VENDOR		?= GNU
export BINUTILS_CVS_REL			?= BINUTILS_2-14
export EMX_VERSION				?= 0.9d-fix04
export EMX_VERSION_SHORT        ?= 9d04
export EMX_CVS_VENDOR			?= EMX
export EMX_CVS_REL				?= EMX_0-9D-FIX04
export LIBC_VERSION             ?= 0.6.6
export LIBC_VERSION_SHORT       ?= 066

# innotek version and timestamp
ifndef BUILD_TS
export BUILD_TS					:= $(shell date '+%Y-%m-%d %H:%M')
else
export BUILD_TS
endif
ifdef OFFICIAL_BIRD_VERSION
export INNOTEK_VERSION			?= (Bird Build $(BUILD_TS) ($(GCC_RELEASE_ID)))
else
export INNOTEK_VERSION			?= (Non-Bird Build $(BUILD_TS) ($(GCC_RELEASE_ID)))
endif


#
# For builds on Linux Host builds we do the environment setup here.
# This is essentially a mimicking of what we do on OS/2.
#
ifndef BUILD_PLATFORM
ifeq "$(shell uname -s)" "Linux"
export BUILD_PLATFORM       ?= LINUX
export BUILD_PROJECT        ?= GCCOS2
export SH                   ?= /bin/sh
export ASH                  ?= /bin/ash
export BASH                 ?= /bin/bash
export AWK                  ?= /bin/gawk
export GAWK                 ?= /bin/gawk
export CONFIG_SHELL         ?= $(SH)
export MAKESHELL            ?= $(SH)
export PATH_EMX             ?= /usr
export PATH_IGCC            ?= /usr
export TMP                  ?= /tmp
export TMPDIR               ?= /tmp
endif
endif
export PATH_IGCC            ?= $(PATH_EMXPGCC)
export PATH_EMXPGCC         ?= $(PATH_IGCC)



# Misc Helpers
ALL_PREFIX                  = $(PATH_BIND)/$(GCC_VERSION)-$(GCC_RELEASE_ID)/usr
TOOL_CVS_DIFF_TREE          = -cvs diff -R -N -w -u -r
ifeq "$(BUILD_PLATFORM)" "OS2"
#NICE						= nice -i
NICE						= nice
else
NICE						=
endif




###############################################################################
###############################################################################
###############################################################################
###############################################################################
#
#    M a i n   R u l e z
#
###############################################################################
###############################################################################
###############################################################################
###############################################################################

# default is quick
all: all-quick
all-logged: all-quick-logged

# release builds are built 'double quick'.
all-double-quick:



# old bootstrap building.
bootstrap all-boostrap:
	mkdir -p $(PATH_OBJ)
	$(NICE) $(MAKE) $(MAKEOPT) -j 1 -C . all-bootstrap-logged 2>&1 | $(UNIXROOT)/usr/bin/tee $(PATH_OBJ)/build-`date +"%Y%m%d-%H%M%S"`.log

all-bootstrap-logged:	\
        all-banner-start \
		all-env \
		all-sanity \
		all-preload \
		all-versionstamps \
		all-symlinks \
		\
        all-stage1 \
        all-stage2 \
		\
		all-install \
		\
		all-symlinks-unlink \
		all-preload-unload
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Make Ended Successfully: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

#
# banners
#

all-banner-start:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Make started: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-stage1:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Stage 1 - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-stage2:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Stage 2 - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-builtunix-initial:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Initializing builtunix tree"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-builtunix-stage2:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Copying stage2 builtunix"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-install:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Install - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-install-done:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Install - done: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-symlinks-start:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ symlinks - staring: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-symlinks-done:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ symlinks - done: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-symlinks-unlink-start:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ symlinks unlinking - staring: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-banner-symlinks-unlink-done:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ symlinks unlinking - done: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"


#
# Sanity and environment dumps - to make it easier to figure out make bugs.
#

.PHONY: all-env
all-env:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "Variables:"
ifdef OFFICIAL_BIRD_VERSION
	@echo "    OFFICIAL_BIRD_VERSION= $(OFFICIAL_BIRD_VERSION)"
endif
	@echo "    PWD                  = $(PWD)"
	@echo "    PWDD                 = $(PWDD)"
	@echo "    PATH_TOP             = $(PATH_TOP)"
	@echo "    PATH_TOPD            = $(PATH_TOPD)"
	@echo "    PATH_OBJ             = $(PATH_OBJ)"
	@echo "    PATH_OBJD            = $(PATH_OBJD)"
	@echo "    PATH_BIN             = $(PATH_BIN)"
	@echo "    PATH_BIND            = $(PATH_BIND)"
	@echo "    ALL_PREFIX           = $(ALL_PREFIX)"
	@echo "    PATH_IGCC            = $(PATH_IGCC)"
	@echo "    PATH_EMX             = $(PATH_EMX)"
	@echo "    BUILD_MODE           = $(BUILD_MODE)"
	@echo "    BUILD_PLATFORM       = $(BUILD_PLATFORM)"
	@echo "    BUILD_PROJECT        = $(BUILD_PROJECT)"
	@echo "    GCC_VERSION          = $(GCC_VERSION)"
	@echo "    GCC_VERSION_SHORT    = $(GCC_VERSION_SHORT)"
	@echo "    GCC_RELEASE_ID       = $(GCC_RELEASE_ID)"
	@echo "    GCC_CVS_VENDOR       = $(GCC_CVS_VENDOR)"
	@echo "    GCC_CVS_REL          = $(GCC_CVS_REL)"
	@echo "    BINUTILS_VERSION     = $(BINUTILS_VERSION)"
	@echo "    BINUTILS_VERSION_SHORT = $(BINUTILS_VERSION_SHORT)"
	@echo "    BINUTILS_CVS_VENDOR  = $(BINUTILS_CVS_VENDOR)"
	@echo "    BINUTILS_CVS_REL     = $(BINUTILS_CVS_REL)"
	@echo "    EMX_VERSION          = $(EMX_VERSION)"
	@echo "    EMX_VERSION_SHORT    = $(EMX_VERSION_SHORT)"
	@echo "    EMX_CVS_VENDOR       = $(EMX_CVS_VENDOR)"
	@echo "    EMX_CVS_REL          = $(EMX_CVS_REL)"
	@echo "    MAKEFILE             = $(MAKEFILE)"
	@echo "    UNIXROOT             = $(UNIXROOT)"
	@echo "    AC_PREFIX            = $(AC_PREFIX)"
	@echo "    AC_MACRODIR          = $(AC_MACRODIR)"
	@echo "    HOSTNAME             = $(HOSTNAME)"
	@echo "    USER                 = $(USER)"
	@echo "    LOGNAME              = $(LOGNAME)"
	@echo "    TMP                  = $(TMP)"
	@echo "    TMPDIR               = $(TMPDIR)"
	@echo "    USER                 = $(USER)"
	@echo "    AWK                  = $(AWK)"
	@echo "    GAWK                 = $(GAWK)"
	@echo "    SH                   = $(SH)"
	@echo "    ASH                  = $(ASH)"
	@echo "    BASH                 = $(BASH)"
	@echo "    CONFIG_SHELL         = $(CONFIG_SHELL)"
	@echo "    MAKESHELL            = $(MAKESHELL)"
	@echo "    EMXSHELL             = $(EMXSHELL)"
	@echo "    SHELL                = $(SHELL)"
	@echo "    MAKE                 = $(MAKE)"
	@echo "    MAKEOPT              = $(MAKEOPT)"
	@echo "    PATH                 = $(PATH)"
	@echo "    C_INCLUDE_PATH       = $(C_INCLUDE_PATH)"
	@echo "    CPLUS_INCLUDE_PATH   = $(CPLUS_INCLUDE_PATH)"
	@echo "    OBJC_INCLUDE_PATH    = $(OBJC_INCLUDE_PATH)"
	@echo "    LIBRARY_PATH         = $(LIBRARY_PATH)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "Content of PATH_OBJ:"
	-ls -la $(PATH_OBJ)/
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "Content of PATH_BIN:"
	-ls -la $(PATH_BIN)/
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"


.PHONY: all-sanity
all-sanity:
	@echo -n "Checking build environment sanity... "
	@if [ "x$(BUILD_PROJECT)" != "xGCCOS2" ]; then \
		echo ""; echo "Error: BUILD_PROJECT is wrong or isn't defined!"; \
		exit 8; \
	fi
	@if [ "x$(BUILD_PLATFORM)" != "xOS2" -a "x$(BUILD_PLATFORM)" != "xLINUX" ]; then \
		echo ""; echo "Error: BUILD_PLATFORM is wrong or isn't defined!"; \
		exit 8; \
	fi
	@if [ "x$(BUILD_PLATFORM)" = "xOS2" -a "x$(UNIXROOT)" = "x" ]; then \
		echo ""; echo "Error: UNIXROOT isn't defined!"; \
		exit 8; \
	fi
	@if [ "x$(BUILD_PLATFORM)" != "xOS2" -a "x$(UNIXROOT)" != "x" ]; then \
		echo ""; echo "Error: UNIXROOT is defined!"; \
		exit 8; \
	fi
ifeq "$(BUILD_PLATFORM)" "OS2"
	@if gcc --version | grep -qe ".*[3]\.[2-9]\.[0-9]"; then \
		true; \
	else \
		echo ""; echo "Warning: GCC v3.2.x or higher is recommended!"; \
	fi
	@if ar --version | grep -qe ".*2\.11\.[2-9]" -e ".*[2]\.1[2-9][.0-9]*"; then \
		true; \
	else \
		echo ""; echo "Warning: AR v2.11.2 or higher is recommended!"; \
	fi
endif
	@echo "ok"


#
# Preload tools we commonly use this speeds up stuff.
#
PRELOADED_TOOLS = bin/sh.exe bin/echo.exe bin/true.exe usr/bin/test.exe usr/bin/expr.exe \
     usr/bin/gawk.exe bin/sed.exe bin/rm.exe bin/cat.exe bin/cp.exe bin/mkdir.exe bin/rm.exe

.PHONY: all-preload-unload all-preload
all-preload:
	@echo "Preloading tools:"
ifeq "$(BUILD_PLATFORM)" "OS2"
	@for tool in $(PRELOADED_TOOLS); do \
	    echo -n " $$tool";	\
		emxload -e $(UNIXROOT)/$$tool;	\
	done
	@echo ""
	emxload -e gcc.exe g++.exe ld.exe as.exe ar.exe ld.exe emxomfld.exe emxomf.exe emxomfar.exe \
		`gcc -print-prog-name=cc1` \
		`gcc -print-prog-name=as` \
		`gcc -print-prog-name=cc1plus`
endif
	@echo ""

all-preload-unload:
ifeq "$(BUILD_PLATFORM)" "OS2"
	emxload -qw
endif


#
# Version stamping/branding.
# 	Update various version strings which are printed from the tools to tell
#	the build date and that's our built. This helps us tell releases apart.
#
# IMPORTANT! Take care not to commit the changed files
# (TODO! Try make these changes to non-cvs backed files. (long-term-goal))
.PHONY: all-versionstamps
all-versionstamps: $(PATH_OBJ)/.ts.versionstamped
$(PATH_OBJ)/.ts.versionstamped:
#	echo '#define INNOTEK_VERSION  "$(INNOTEK_VERSION)"' > include/innotekversion.h
	$(MAKE) $(MAKEOPT) -j 1 -f $(MAKEFILE) "INNOTEK_VERSION=$(INNOTEK_VERSION)" gcc-versionstamps binutils-versionstamps emx-versionstamps
	mkdir -p $(@D)
	touch $@


#
# Stages
#	A rebuild with the new toolsuite is usually required when there is ABI
# 	changes and other vital changes done to the tools.
#

.PHONY: all-stage1 all-stage1-it
all-stage1: $(PATH_OBJ)/.all-stage1
$(PATH_OBJ)/.all-stage1:
	$(MAKE) $(MAKEOPT) -j 1 -f $(MAKEFILE) all-stage1-it
	echo "$(@F)" > $(PATH_OBJ)/.last-stage
	touch $@
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Stage 1 - done: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-stage1-it: \
		all-banner-stage1 \
		all-builtunix-initial \
        all-binutils \
		all-gcc	\
		all-emx

.PHONY: all-stage2 all-stage2-it
all-stage2: $(PATH_OBJ)/.all-stage2
$(PATH_OBJ)/.all-stage2: \
		$(PATH_OBJ)/.all-stage2.save-stage1
	$(MAKE) $(MAKEOPT) -j 1 -f $(MAKEFILE) all-stage2-it
	echo "$(@F)" > $(PATH_OBJ)/.last-stage
	touch $@
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Stage 2 - done: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

$(PATH_OBJ)/.all-stage2.save-stage1:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Saving Stage 1 - Starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	$(MAKE) $(MAKEOPT) -j 1 -f $(MAKEFILE) all-preload-unload
	$(MAKE) $(MAKEOPT) -j 1 -f $(MAKEFILE) all-builtunix-stage2
	rm -Rf $(PATH_OBJ)/stage1
	mkdir -p $(PATH_OBJ)/stage1
	if [ -d $(PATH_OBJ)/gcc ] ; then mv $(PATH_OBJ)/gcc $(PATH_OBJ)/stage1/gcc; fi
	if [ -d $(PATH_OBJ)/emx ] ; then mv $(PATH_OBJ)/emx $(PATH_OBJ)/stage1/emx; fi
	$(MAKE) $(MAKEOPT) -j 1 -f $(MAKEFILE) all-preload
	touch $@
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Saving Stage 1 - done: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

all-stage2-it: \
		all-banner-stage2 \
        all-binutils \
		all-gcc \
        all-emx


#
# Install to bin tree makeing it ready for packing.
#
.PHONY: all-install
all-install: \
	all-banner-install \
	gcc-install	\
	emx-install \
	misc-install \
	all-strip-install \
	all-banner-install-done \

# strips release build installations.
.PHONY: all-strip-install
all-strip-install:
ifeq ($(BUILD_MODE), RELEASE)
	echo "Stripping installed binaries..."
	exes="`find $(PATH_BIN) -name '*.exe' | sed -e 's,/,\\\\,g'`"; \
	dlls="`find $(PATH_BIN) -name '*.dll' | sed -e 's,/,\\\\,g'`"; \
	for i in $$exes $$dlls; \
	do \
		if lxlite /F+ /AP:4096 /MRN /MLN /MF1 $$i; then true; \
		else exit 1; \
		fi; \
	done
endif


#
# Generate all the diffs we have to supply.
#
.PHONY: all-diff
all-diff: \
	gcc-diff \
	binutils-diff \
	emx-diff \





###############################################################################
###############################################################################
###############################################################################
#
#    Q u i c k   b o o t s t r a p p i n g
#
###############################################################################
###############################################################################
###############################################################################

# This is the default buildtype now.
# It requires a very up-to-date gcc build, not good for bootstrapping from old GCCs.
.PHONY: quick all-quick
quick all-quick:
	mkdir -p $(PATH_OBJ)
	$(NICE) $(MAKE) $(MAKEOPT) -j 1 -C . all-quick-logged 2>&1 | $(UNIXROOT)/usr/bin/tee $(PATH_OBJ)/build-`date +"%Y%m%d-%H%M%S"`.log

all-quick-banner-start:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap started:            $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
.PHONY: all-quick-logged
all-quick-logged: \
        all-quick-banner-start \
		all-env \
		all-sanity \
		all-quick-builtunix-initial \
		all-versionstamps \
		all-symlinks-unlink \
		\
		all-quick-step1 \
		all-quick-step2 \
		all-quick-step3 \
		all-quick-step4 \
		\
		all-quick-install \
		all-preload-unload
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Ended Successfully: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

.PHONY: all-quick-builtunix-initial
all-quick-builtunix-initial: $(PATH_OBJ)/.quick-builtunix-initial
all-quick-builtunix-initial-it: \
		all-preload-unload \
		all-builtunix-initial \
		all-preload
$(PATH_OBJ)/.quick-builtunix-initial:
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-quick-builtunix-initial-it
	touch $@


# This build type is used for release builds.
# We're doing two quick builds here to ensure everything is alright.
.PHONY: double-quick all-double-quick
double-quick all-double-quick:
	mkdir -p $(PATH_OBJ)
	$(NICE) $(MAKE) $(MAKEOPT) -j 1 -C . all-double-quick-logged 2>&1 | $(UNIXROOT)/usr/bin/tee $(PATH_OBJ)/build-`date +"%Y%m%d-%H%M%S"`.log

all-double-quick-banner-start:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Double Quick Bootstrap started:            $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
PHONY: all-double-quick-logged
all-double-quick-logged: \
        all-double-quick-banner-start \
		all-env \
		all-sanity \
		all-preload-unload \
		all-double-quick-builtunix-initial \
		all-preload \
		all-versionstamps \
		all-symlinks-unlink \
		\
		all-double-quick-stage1 \
		all-preload-unload \
		all-double-quick-save-stage1 \
		all-preload \
		all-double-quick-stage2 \
		\
		all-quick-install \
		all-preload-unload
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Double Quick Bootstrap Ended Successfully: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

.PHONY: all-double-quick-builtunix-initial
all-double-quick-builtunix-initial: $(PATH_OBJ)/.doublequick-builtunix-initial
all-double-quick-builtunix-initial-it: \
		all-preload-unload \
		all-builtunix-initial \
		all-preload
$(PATH_OBJ)/.doublequick-builtunix-initial:
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-double-quick-builtunix-initial-it
	touch $@

# stage1
.PHONY: all-double-quick-stage1 all-double-quick-stage1-it
all-double-quick-stage1: $(PATH_OBJ)/.doublequick-stage1
all-double-quick-stage1-it: \
		all-quick-step1 \
		all-quick-step2 \
		all-quick-step3 \
		all-quick-step4
$(PATH_OBJ)/.doublequick-stage1:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Double Quick Bootstrap Stage 1 - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-double-quick-stage1-it
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Double Quick Bootstrap Stage 1 - done:     $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	touch $@

# save stage1
.PHONY: all-double-quick-save-stage1 all-double-quick-save-stage1-it
all-double-quick-save-stage1: $(PATH_OBJ)/.doublequick-stage1-saved
all-double-quick-save-stage1-it: all-preload-unload
	mkdir -p $(PATH_OBJ)/stage1
	if [ -d $(PATH_OBJ)/emx      ]; then rm -Rf $(PATH_OBJ)/stage1/emx      && mv $(PATH_OBJ)/emx      $(PATH_OBJ)/stage1/emx;      fi
	if [ -d $(PATH_OBJ)/binutils ]; then rm -Rf $(PATH_OBJ)/stage1/binutils && mv $(PATH_OBJ)/binutils $(PATH_OBJ)/stage1/binutils; fi
	if [ -d $(PATH_OBJ)/gcc      ]; then rm -Rf $(PATH_OBJ)/stage1/gcc      && mv $(PATH_OBJ)/gcc      $(PATH_OBJ)/stage1/gcc;      fi
	if [ -f $(PATH_OBJ)/.quick-last-step ]; then mv -f $(PATH_OBJ)/.quick-last-step  $(PATH_OBJ)/stage1; fi
	rm -f $(PATH_OBJ)/.quick*
$(PATH_OBJ)/.doublequick-stage1-saved:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Double Quick Bootstrap Saving Stage 1 - starting:  $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-double-quick-save-stage1-it
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Double Quick Bootstrap Saving Stage 1 - done:      $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	touch $@

# stage2
.PHONY: all-double-quick-stage2 all-double-quick-stage2-it
all-double-quick-stage2: $(PATH_OBJ)/.doublequick-stage2
all-double-quick-stage2-it: \
		all-quick-step1 \
		all-quick-step2 \
		all-quick-step3 \
		all-quick-step4
$(PATH_OBJ)/.doublequick-stage2:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Double Quick Bootstrap Stage 2 - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-double-quick-stage2-it
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Double Quick Bootstrap Stage 2 - done:     $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	touch $@


# step 1 builds the base libraries.
.PHONY: all-quick-step1 all-quick-step1-it
all-quick-step1: $(PATH_OBJ)/.quick-step1
all-quick-step1-it: \
		all-preload \
        emx-quick-libs \
		all-preload-unload \
		emx-quick-libs-install \
		all-preload
$(PATH_OBJ)/.quick-step1:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Step 1 - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-quick-step1-it
	echo "$(@F)" > $(PATH_OBJ)/.quick-last-step
	touch $@
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Step 1 - done:     $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

# step 2 builds the emx base utilities.
.PHONY: all-quick-step2 all-quick-step2-it
all-quick-step2: $(PATH_OBJ)/.quick-step2
all-quick-step2-it: \
        emx-quick-rest \
		all-preload-unload \
		emx-quick-rest-install \
		all-preload
$(PATH_OBJ)/.quick-step2:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Step 2 - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-quick-step2-it
	echo "$(@F)" > $(PATH_OBJ)/.quick-last-step
	touch $@
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Step 2 - done:     $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

# step 3 builds all the binutil utilities and libraries.
.PHONY: all-quick-step3 all-quick-step3-it
all-quick-step3: $(PATH_OBJ)/.quick-step3
all-quick-step3-it: \
        binutils-quick \
		all-preload-unload \
		binutils-quick-installstage \
		all-preload
$(PATH_OBJ)/.quick-step3:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Step 3 - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-quick-step3-it
	echo "$(@F)" > $(PATH_OBJ)/.quick-last-step
	touch $@
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Step 3 - done:     $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"

# step 4 builds gcc.
.PHONY: all-quick-step4 all-quick-step4-it
all-quick-step4: $(PATH_OBJ)/.quick-step4
all-quick-step4-it: \
        gcc-quick \
		all-preload-unload \
		gcc-quick-installstage \
		all-preload
$(PATH_OBJ)/.quick-step4:
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Step 4 - starting: $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	$(MAKE) $(MAKEOPT) -f $(MAKEFILE) all-quick-step4-it
	echo "$(@F)" > $(PATH_OBJ)/.quick-last-step
	touch $@
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
	@echo "+ Quick Bootstrap Step 4 - done:     $(shell date)"
	@echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"


# Installs the stuff we've built.
.PHONY: all-quick-install
all-quick-install: \
		all-banner-install \
		gcc-install	\
		binutils-install \
		emx-install \
		misc-install \
		all-strip-install \
		all-banner-install-done


###############################################################################
###############################################################################
###############################################################################
#
#    B u i l t   U n i x
#
###############################################################################
###############################################################################
###############################################################################

all-builtunix-initial: \
		all-banner-builtunix-initial \
		all-builtunix-clean \
        all-builtunix-dirs \
		$(PATH_BUILTTOOLS)/bin/dllar.cmd \
		$(PATH_BUILTTOOLS)/omfhack/ar.exe \
		$(PATH_BUILTTOOLS)/omfhack/realar.exe \
		$(PATH_BUILTTOOLS)/omfhack/ranlib.exe

all-builtunix-clean: \
		all-preload-unload
	rm -Rf $(PATH_BUILTUNIX)

all-builtunix-dirs:
	mkdir -p $(PATH_BUILTTOOLS)/bin $(PATH_BUILTTOOLS)/lib $(PATH_BUILTTOOLS)/include $(PATH_BUILTTOOLS)/omfhack

$(PATH_BUILTTOOLS)/bin/dllar.cmd $(PATH_BUILTTOOLS_TMP)/bin/dllar.cmd: $(PATH_TOP)/src/misc/dllar.cmd
	cp $< $@
$(PATH_BUILTTOOLS)/omfhack/ranlib.exe $(PATH_BUILTTOOLS_TMP)/omfhack/ranlib.exe: $(PATH_TOP)/tools/bin/true.exe
	cp $< $@
$(PATH_BUILTTOOLS)/omfhack/ar.exe: $(PATH_IGCC)/bin/emxomfar.exe
	cp $< $@
$(PATH_BUILTTOOLS_TMP)/omfhack/ar.exe: $(PATH_BUILTTOOLS_TMP)/bin/emxomfar.exe
	cp $< $@
$(PATH_BUILTTOOLS)/omfhack/realar.exe $(PATH_BUILTTOOLS_TMP)/omfhack/realar.exe: $(PATH_IGCC)/bin/ar.exe
	cp $< $@
$(PATH_BUILTTOOLS_TMP)/omfhack:
	mkdir -p $@

all-builtunix-stage2: \
		all-banner-builtunix-stage2 \
		gcc-builtunix-stage2 \
		emx-builtunix-stage2 \
		all-builtunix-stage2-libs \
		$(PATH_BUILTTOOLS_TMP)/bin/dllar.cmd \
		$(PATH_BUILTTOOLS_TMP)/omfhack \
		$(PATH_BUILTTOOLS_TMP)/omfhack/ar.exe \
		$(PATH_BUILTTOOLS_TMP)/omfhack/realar.exe \
		$(PATH_BUILTTOOLS_TMP)/omfhack/ranlib.exe \
		all-builtunix-clean
	mv -f $(PATH_BUILTUNIX_TMP) $(PATH_BUILTUNIX)

all-builtunix-stage2-libs:
	-for aoutlib in `find $(PATH_BUILTUNIX_TMP) -name "*.a" | sed -e 's/\.a$$//' `; \
	do \
		if [ ! -f $(aoutlib).lib ]; then \
			echo "  $${aoutlib}"; \
			$(PATH_BUILTTOOLS_TMP)/bin/emxomf.exe $${aoutlib}.a; \
		fi;\
	done


###############################################################################
###############################################################################
###############################################################################
#
#    S y m l i n k s
#
#
#		We use symlinking of binutils stuff into the gcc tree so we can
#		build everything in one go and take advantage of the gcc
#		makesystem which will use the binutils we built when doing the
#		gcc libraries and such.
#
#		On OS/2 symlinks doesn't exist so, we'll copy the source trees.
#		Copy because this will for directories work ok with cvs, moving
#		directories would cause 'cvs update -d' to refetch stuff.
#
#		IMPORTANT! Take care, the 'symlined' stuff will be removed at the
#		end of the build. Remeber to do all-symlinks-unlink before a fresh
#		build is started as the duplicated binutils things doesn't get
#		updated by cvs.
#
#
#w##############################################################################
###############################################################################
###############################################################################
all-symlinks: \
		all-banner-symlinks-start \
		all-symlinks-binutils \
		all-banner-symlinks-done

all-symlinks-unlink: \
		all-banner-symlinks-unlink-start \
		all-symlinks-unlink-binutils \
		all-banner-symlinks-unlink-done


ifeq "$(BUILD_PLATFORM)" "OS2"
TOOL_SYMLINK_FILE   = cp -p
TOOL_SYMLINK_DIR    = cp -pRf
TOOL_SYMLINK_MKDIR  = mkdir
TOOL_UNSYMLINK_DIR  = rm -Rf
else
TOOL_SYMLINK_FILE   = ln -fs
TOOL_SYMLINK_DIR    = ln -fs
TOOL_SYMLINK_MKDIR  = true
TOOL_UNSYMLINK_DIR  = rm
endif

# ld is broken, so don't use it!
# ld \

SYMLINKS_BINUTILS_TO_GCC_DIRS = \
bfd \
binutils \
etc \
gas \
gprof \
intl \
opcodes \
texinfo \
libiberty \
include/aout \
include/coff \
include/elf	\
include/gdb	\
include/mpw	\
include/nlm	\
include/opcode \
include/regs \

SYMLINKS_BINUTILS_TO_GCC_FILES = \
include/alloca-conf.h \
include/ansidecl.h \
include/bfdlink.h \
include/bin-bugs.h \
include/bout.h \
include/ChangeLog \
include/COPYING \
include/demangle.h \
include/dis-asm.h \
include/dyn-string.h \
include/fibheap.h \
include/filenames.h \
include/floatformat.h \
include/fnmatch.h \
include/fopen-bin.h \
include/fopen-same.h \
include/fopen-vms.h \
include/gdbm.h \
include/getopt.h \
include/hashtab.h \
include/hp-symtab.h \
include/ieee.h \
include/libiberty.h \
include/MAINTAINERS \
include/md5.h \
include/oasys.h \
include/objalloc.h \
include/obstack.h \
include/os9k.h \
include/partition.h \
include/progress.h \
include/safe-ctype.h \
include/sort.h \
include/splay-tree.h \
include/symcat.h \
include/ternary.h \
include/xregex.h \
include/xregex2.h \
include/xtensa-config.h \
include/xtensa-isa-internal.h \
include/xtensa-isa.h \


# Symlink binutils stuff to gcc.
# 	Note the test should've been "! -e", but that doesn' work in ash... :/
all-symlinks-binutils:
	for file in $(SYMLINKS_BINUTILS_TO_GCC_FILES); do \
		if [ ! -f "$(PATH_TOP)/src/gcc/.symlinked.`echo $${file} | sed -e 's@/@_@g'`" ]; then  \
			echo symlinking file src/gcc/$${file} to src/binutils/$${file} ; \
			if 	rm -f $(PATH_TOP)/src/gcc/$$file && \
				$(TOOL_SYMLINK_FILE)  $(PATH_TOP)/src/binutils/$$file $(PATH_TOP)/src/gcc/$$file && \
				touch                 $(PATH_TOP)/src/gcc/.symlinked.`echo $${file} | sed -e 's@/@_@g'`; \
				then true ; \
			else exit 1; \
			fi ; \
		fi ; \
	done
	for dir in $(SYMLINKS_BINUTILS_TO_GCC_DIRS) ; do \
		if [ ! -f "$(PATH_TOP)/src/gcc/.symlinked.`echo $${dir} | sed -e 's@/@_@g'`" ] ; then  \
			echo symlinking directory src/gcc/$${dir} to src/binutils/$${dir} ; \
			if [ -d "$(PATH_TOP)/src/gcc/$$dir" ] ; then \
			    echo removing existing directory: $${dir} ; \
				rm -Rf $(PATH_TOP)/src/gcc/$$dir ; \
			fi ; \
			if 	$(TOOL_SYMLINK_MKDIR) $(PATH_TOP)/src/gcc/$$dir && \
				$(TOOL_SYMLINK_DIR)   $(PATH_TOP)/src/binutils/$$dir $(PATH_TOP)/src/gcc/`echo $${dir}| sed -e '/\//!d' -e 's@\([a-zA-z0-9]*\)/.*@\1/@'` ; \
				touch                 $(PATH_TOP)/src/gcc/.symlinked.`echo $${dir} | sed -e 's@/@_@g'` ; \
				then true ; \
			else exit 1; \
			fi ; \
		fi ; \
	done
	-ls -a1 src/gcc/.sym*

all-symlinks-unlink-binutils:
	for name in `ls src/gcc/.symlinked* | sed -e "s/.*\.symlinked.//" -e "s/_/\//g"`; do \
		echo unlinking $${name} ; \
		if [ -d "$(PATH_TOP)/src/gcc/$${name}" ] ; then  \
			rm -Rf $(PATH_TOP)/src/gcc/$${name} ; \
		else \
			rm -f $(PATH_TOP)/src/gcc/$${name} ; \
		fi ; \
		if [ ! -f "$(PATH_TOP)/src/gcc/$${name}" -a ! -d "$(PATH_TOP)/src/gcc/$${name}" ] ; then \
			rm  $(PATH_TOP)/src/gcc/.symlinked.`echo $${name} | sed -e 's@/@_@g'` ; \
		else \
			echo "unlink error: $(PATH_TOP)/src/gcc/$${name} exist" ; \
			ls -l "$(PATH_TOP)/src/gcc/$${name}" ; \
			exit 1; \
		fi ; \
	done
	-ls -a1 src/gcc/.sym*


###############################################################################
###############################################################################
###############################################################################
#
#    G C C
#
###############################################################################
###############################################################################
###############################################################################
all-gcc       gcc:	\
        gcc-autoconf-refresh \
        gcc-bootstrap
	echo "Successfully build GCC."



GCC_DIRS = \
gcc/libiberty \
gcc/zlib \
gcc/gcc \
gcc/libstdc++-v3 \
gcc/boehm-gc \
gcc/fastjar \
gcc/libf2c \
gcc/libf2c/libF77 \
gcc/libf2c/libI77 \
gcc/libf2c/libU77 \
gcc/libffi \
gcc/libjava \
gcc/libjava/libltdl \
gcc/libobjc \

# Here is a problem, we can't regenerate binutils makefiles when symlinked into
# gcc. At least not yet. The result is messed up severely.
not_yet =\
gcc/bfd \
gcc/binutils \
gcc/gas \
gcc/gprof \
gcc/ld \
gcc/libiberty \
gcc/opcodes
#gcc/intl

GCC_CONFIGURE_DIRS = $(GCC_DIRS)


# configure.in/configure
.PHONY: gcc-autoconf-refresh gcc-autoconf-rerun gcc-autoconf-clean gcc-autoconf-remove
gcc-autoconf-refresh gcc-autoconf-rerun gcc-autoconf-clean gcc-autoconf-remove:
ifeq "$(BUILD_PLATFORM)" "LINUX"
	-$(SH) $(PATH_TOP)/xfix.sh
endif
	for dir in $(GCC_CONFIGURE_DIRS); do \
		echo $$dir; \
		if $(MAKE) $(MAKEOPT) -j 1 $(@:gcc-autoconf-%=%) -f $(PWD)/config.gmk -C src/$$dir ; then \
			true; \
		else \
			exit 8; \
		fi \
	done


# build the components.
.PHONY: gcc-bootstrap gcc-build gcc-install gcc-configure
gcc-bootstrap gcc-build gcc-install gcc-configure:
	mkdir -p $(PATH_OBJ)/gcc
	$(MAKE) $(MAKEOPT) -j 1 -C $(PATH_OBJ)/gcc -f $(MAKEFILE) $@-it

# let the build/bootstrap create the gcc import .def file before kicking off a build.
gcc-bootstrap gcc-build: src/gcc/gcc/config/i386/emx-libgcc_so_d.def
src/gcc/gcc/config/i386/emx-libgcc_so_d.def: src/emx/src/lib/libgcc_d.awk $(PATH_OBJD)/emx/omf/libc.def src/emx/src/lib/libc.def
	$(GAWK) -f src/emx/src/lib/libgcc_d.awk $(filter %.def,$^) > $@


# When changed directory
#   We pass down BOOT_ flags for stage2+.
#   Because strip is broken on OS/2 we pass -s for release build and -g for
#   non release builds - This overrides some stuff in src/gcc/gcc/config/i386/t-emx!
# For the 2nd stage we need a hack for using the right specs when linking...
#SPEC_HACK = $(if $(wildchar $(PATH_OBJ)/.all-stage1),\
#              -specs $(PATH_TOP)/src/emx/src/lib/libc.specs, \
#              -specs $(PATH_TOP)/tools/x86.os2/gcc/staged/lib/gcc-lib/i386-pc-os2-emx/$(GCC_VERSION)/specs)
SPECS_HACK =
gcc-bootstrap-it gcc-build-it: \
		$(PATH_OBJ)/gcc/.ts.configured
ifeq "$(BUILD_PLATFORM)" "OS2"
	unset GCCLOAD ; export LT_OS2_LDFLAGS="-Zomf -g" ; \
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/gcc \
	    LIBGCC2_DEBUG_CFLAGS="$(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem " \
		      CFLAGS="$(SPEC_HACK) $(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem" \
		    CXXFLAGS="$(SPEC_HACK) $(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem" \
		 BOOT_CFLAGS="$(SPEC_HACK) $(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem" \
		     LDFLAGS="$(SPEC_HACK) $(BUILD_DEBUGINFO) -Zhigh-mem -Zcrtdll -Zstack 1024 -Zomf" \
		BOOT_LDFLAGS="$(SPEC_HACK) $(BUILD_DEBUGINFO) -Zhigh-mem -Zcrtdll -Zstack 1024 -Zomf" \
		$(subst build, all, $(patsubst gcc-%-it, %, $@))
else
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/gcc $(subst build, all, $(patsubst gcc-%-it, %, $@))
endif

# configure it (invoked after directory change).
# 	We set CC to help configure finding it.
# 	We also set some LDFLAGS to get omf linking.
gcc-configure-it $(PATH_OBJ)/gcc/.ts.configured: $(PATH_TOP)/src/gcc/configure
ifeq "$(BUILD_PLATFORM)" "OS2"
	$(ASH) -c " \
		export CC=\"gcc.exe\" ; \
		export LDFLAGS=\"$(SPEC_HACK) $(BUILD_DEBUGINFO) -Zhigh-mem -Zcrtdll -Zstack 1024 -Zomf\" ; \
		export BOOT_LDFLAGS=\"$(SPEC_HACK) $(BUILD_DEBUGINFO) -Zhigh-mem -Zcrtdll -Zstack 1024 -Zomf\" ; \
		$< \
		--enable-clh \
		--enable-threads=os2 \
		--enable-shared=libgcc,bfd,opcodes \
		--enable-nls \
		--without-included-gettext \
		--with-local-prefix=$(subst \,/,$(PATH_IGCC)) \
		--prefix=/gcc \
		--with-gnu-as \
		--disable-libgcj \
		--enable-languages=c,c++ "
else
	$(ASH) -c "$< \
		--disable-clh \
		--enable-shared=libgcc,bfd,opcodes \
		--enable-nls \
		--without-included-gettext \
		--with-local-prefix=$(subst \,/,$(PATH_IGCC)) \
		--prefix=/gcc \
		--with-gnu-as \
		--disable-libgcj \
		--enable-languages=c,c++ "
endif
	touch $(PATH_OBJ)/gcc/.ts.configured
# TODO: Andy, on Linux --enable-clh result in linking errors.

# Install the GCC build
#	Repeating the prefix doesn't hurt anybody.
gcc-install-it:
	$(MAKE) $(MAKEOPT) prefix=$(ALL_PREFIX) install


# easy, update src/gcc/gcc/version.c
gcc-versionstamps:
	@echo "Version stamping gcc..."
	mv -f $(PATH_TOP)/src/gcc/gcc/version.c $(PATH_TOP)/src/gcc/gcc/version.tmp.c
	sed -e '/version_string/s/\([0-9]\.[0-9]*\.[0-9]*\).*/\1 $(INNOTEK_VERSION)";/' \
		$(PATH_TOP)/src/gcc/gcc/version.tmp.c > $(PATH_TOP)/src/gcc/gcc/version.c
	rm -f $(PATH_TOP)/src/gcc/gcc/version.tmp.c


# Install to builtunix directory.
##     WARNING! Another spec HACK!
gcc-builtunix-stage2:
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/gcc prefix=$(PATH_BUILTTOOLS_TMPD) install
##	cp $(PATH_TOP)/src/emx/src/lib/libc.specs $(PATH_OBJ)/gcc/gcc/specs


# Quick bootstrap workers.
gcc-quick:  \
	gcc-autoconf-refresh \
	gcc-build

gcc-quick-installstage:
	rm -Rf $(PATH_BUILTUNIX_TMP)
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/gcc prefix=$(PATH_BUILTTOOLS_TMPD) install
	cp -Rf $(PATH_BUILTUNIX_TMP)/* $(PATH_BUILTUNIX)/
	rm -Rf $(PATH_BUILTUNIX_TMP)


# Generate diffs for GCC - part of packing a release.
.PHONY: gcc-diff $(ALL_PREFIX)/src/diffs/gcc-$(GCC_VERSION).diff
gcc-diff: $(ALL_PREFIX)/src/diffs/gcc-$(GCC_VERSION).diff
$(ALL_PREFIX)/src/diffs/gcc-$(GCC_VERSION).diff:
	mkdir -p $(@D)
	$(TOOL_CVS_DIFF_TREE) $(GCC_CVS_REL) src/gcc > $@





###############################################################################
###############################################################################
###############################################################################
#
#    B i n U t i l s
#
###############################################################################
###############################################################################
###############################################################################

BINUTILS_DIRS = \
binutils/bfd \
binutils/opcodes \
binutils/gas \
binutils/gprof \
binutils/binutils \
binutils/ld \

BINUTILS_NOT_DIRS =\
binutils/etc \
binutils/libiberty
#binutils/intl


BINUTILS_CONFIGURE_DIRS = $(BINUTILS_DIRS) $(BINUTILS_NOT_DIRS)

all-binutils  binutils:
	echo "Binutils are build together with the other GNU Tools in GCC."
	echo "Separate building of binutils is not configured."

binutils214:	\
        binutils-autoconf-refresh \
        binutils-build
	echo "Successfully build Binutils v2.14."



# configure.in/configure
.PHONY: binutils-autoconf-refresh binutils-autoconf-rerun binutils-autoconf-clean binutils-autoconf-remove
binutils-autoconf-refresh binutils-autoconf-rerun binutils-autoconf-clean binutils-autoconf-remove:
ifeq "$(BUILD_PLATFORM)" "LINUX"
	-$(SH) $(PATH_TOP)/xfix.sh
endif
	for dir in $(BINUTILS_CONFIGURE_DIRS); do \
		if $(MAKE) $(MAKEOPT) -j 1 $(@:binutils-autoconf-%=%) -f $(PWD)/config.gmk -C src/$$dir ; then \
			true; \
		else \
			exit 8; \
		fi \
	done


# build the components.
.PHONY: binutils-build binutils-install binutils-configure binutils-configure-elf
binutils-build binutils-install binutils-configure binutils-configure-elf:
	mkdir -p $(PATH_OBJ)/binutils
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/binutils -f $(MAKEFILE) $@-it


# When changed directory
binutils-build-it: \
		$(PATH_OBJ)/binutils/.ts.configured
	unset GCCLOAD ; export LT_OS2_LDFLAGS="-Zomf -g -Zmap" ; \
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/binutils \
		CFLAGS="$(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem" \
		LDFLAGS="$(BUILD_DEBUGINFO) -Zhigh-mem -Zstack 1024 -Zomf"
	unset GCCLOAD ; export LT_OS2_LDFLAGS="-Zomf -g -Zmap" ; \
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/binutils/gas-elf \
		CFLAGS="$(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem" \
		LDFLAGS="$(BUILD_DEBUGINFO) -Zhigh-mem -Zstack 1024 -Zomf"
	unset GCCLOAD ; export LT_OS2_LDFLAGS="-Zomf -g -Zmap" ; \
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/binutils/ld-elf \
		CFLAGS="$(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem" \
		LDFLAGS="$(BUILD_DEBUGINFO) -Zhigh-mem -Zstack 1024 -Zomf"

# configure it (invoked after directory change).
# 	We set CC to help configure finding it.
#	And we reconfigure libiberty to the gcc one.
binutils-configure-it $(PATH_OBJ)/binutils/.ts.configured: $(PATH_TOP)/src/binutils/configure
ifeq "$(BUILD_PLATFORM)" "OS2"
	$(ASH) -c " \
		CC=\"gcc.exe\" \
		CFLAGS=\"$(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem\"  \
		LDFLAGS=\"$(BUILD_DEBUGINFO) -Zhigh-mem -Zstack 1024 -Zomf\" \
		$< \
		--enable-clh \
		--enable-threads=os2 \
		--enable-shared=libgcc,bfd,opcodes \
		--enable-nls \
		--without-included-gettext \
		--with-local-prefix=$(subst \,/,$(PATH_IGCC)) \
		--prefix=/gcc \
		--with-gnu-as \
		--disable-libgcj \
		--enable-languages=c,c++ "
else
	$(ASH) -c "$< \
		--disable-clh \
		--enable-shared=libgcc,bfd,opcodes \
		--enable-nls \
		--without-included-gettext \
		--with-local-prefix=$(subst \,/,$(PATH_IGCC)) \
		--prefix=/gcc \
		--with-gnu-as \
		--disable-libgcj \
		--enable-languages=c,c++ "
endif
	mkdir -p gas-elf
	$(ASH) -c "cd gas-elf && \
		CC=\"gcc.exe\" \
		CFLAGS=\"$(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem\" \
		LDFLAGS=\"$(BUILD_DEBUGINFO) -Zhigh-mem -Zstack 1024 -Zomf\" \
		$(PATH_TOP)/src/binutils/gas/configure \
		--target=i386-pc-os2-elf \
		--enable-shared=libgcc,bfd,opcodes \
		--enable-nls \
		--without-included-gettext \
		--with-local-prefix=$(subst \,/,$(PATH_IGCC)) \
		--prefix=/gcc \
		--with-gnu-as \
		--program-suffix=-elf "
	mkdir -p ld-elf
	$(ASH) -c "cd ld-elf && \
		CC=\"gcc.exe\" \
		CFLAGS=\"$(BUILD_DEBUGINFO) $(BUILD_OPTIMIZE) -Zhigh-mem\"  \
		LDFLAGS=\"$(BUILD_DEBUGINFO) -Zhigh-mem -Zstack 1024 -Zomf\" \
		$(PATH_TOP)/src/binutils/ld/configure \
		--target=i386-pc-os2-elf \
		--enable-shared=libgcc,bfd,opcodes \
		--enable-nls \
		--without-included-gettext \
		--with-local-prefix=$(subst \,/,$(PATH_IGCC)) \
		--prefix=/gcc \
		--with-gnu-as \
		--program-suffix=-elf "
	touch $(PATH_OBJ)/binutils/.ts.configured



# Install the Binutils build
#	Repeating the prefix doesn't hurt anybody.
binutils-install-it:
	cd gas-elf && $(MAKE) $(MAKEOPT) prefix=$(ALL_PREFIX) install
	cd ld-elf  && $(MAKE) $(MAKEOPT) prefix=$(ALL_PREFIX) install
	$(MAKE) $(MAKEOPT) prefix=$(ALL_PREFIX) install
	mv -f $(ALL_PREFIX)/bin/ld.exe  $(ALL_PREFIX)/bin/ld-bad.exe
	mv -f $(ALL_PREFIX)/i386-pc-os2-emx/bin/ld.exe  $(ALL_PREFIX)/i386-pc-os2-emx/bin/ld-bad.exe


# Not so easy, need to patch a few files.
binutils-versionstamps:
	@echo "Version stamping binutils..."
	cp -f $(PATH_TOP)/src/binutils/binutils/version.c  $(PATH_TOP)/src/binutils/binutils/version.tmp.c
	sed -e '/printf.*program_version/s/%s %s.*\\n/%s %s $(INNOTEK_VERSION)\\n/' \
		$(PATH_TOP)/src/binutils/binutils/version.tmp.c > $(PATH_TOP)/src/binutils/binutils/version.c
	rm -f $(PATH_TOP)/src/binutils/binutils/version.tmp.c
	if [ -f $(PATH_TOP)/src/gcc/binutils/version.c ]; then \
		cp -f $(PATH_TOP)/src/gcc/binutils/version.c  $(PATH_TOP)/src/gcc/binutils/version.tmp.c && \
		sed -e '/printf.*program_version/s/%s %s.*\\n/%s %s $(INNOTEK_VERSION)\\n/' \
			$(PATH_TOP)/src/gcc/binutils/version.tmp.c > $(PATH_TOP)/src/gcc/binutils/version.c && \
		rm -f $(PATH_TOP)/src/gcc/binutils/version.tmp.c ; \
	fi
	@echo "Version stamping gas..."
	cp -f $(PATH_TOP)/src/binutils/gas/as.c $(PATH_TOP)/src/binutils/gas/as.tmp.c
	sed -e '/printf.*GNU.*assembler/s/%s.*\\n/%s $(INNOTEK_VERSION)\\n/' \
	    -e '/fprintf.*GNU.*assembler.*version.*BFD.*/s/using BFD version %s.*/using BFD version %s $(INNOTEK_VERSION)"),/ ' \
		$(PATH_TOP)/src/binutils/gas/as.tmp.c > $(PATH_TOP)/src/binutils/gas/as.c
	rm -f $(PATH_TOP)/src/binutils/gas/as.tmp.c
	if [ -f $(PATH_TOP)/src/gcc/gas/as.c ]; then \
		cp -f $(PATH_TOP)/src/gcc/gas/as.c  $(PATH_TOP)/src/gcc/gas/as.tmp.c && \
		sed -e '/printf.*GNU.*assembler/s/%s.*\\n/%s $(INNOTEK_VERSION)\\n/' \
			-e '/fprintf.*GNU.*assembler.*version.*BFD.*/s/using BFD version %s.*/using BFD version %s $(INNOTEK_VERSION)"),/ ' \
			$(PATH_TOP)/src/gcc/gas/as.tmp.c > $(PATH_TOP)/src/gcc/gas/as.c && \
		rm -f $(PATH_TOP)/src/gcc/gas/as.tmp.c ; \
	fi
	@echo "Version stamping ld..."
	cp -f $(PATH_TOP)/src/binutils/ld/ldver.c $(PATH_TOP)/src/binutils/ld/ldver.tmp.c
	sed -e '/fprintf.*GNU.*ld.*version/s/(with BFD %s).*/(with BFD %s) $(INNOTEK_VERSION)\\n"),/' \
		$(PATH_TOP)/src/binutils/ld/ldver.tmp.c > $(PATH_TOP)/src/binutils/ld/ldver.c
	rm -f $(PATH_TOP)/src/binutils/ld/ldver.tmp.c
	if [ -f $(PATH_TOP)/src/gcc/ld/ldver.c ]; then \
		cp -f $(PATH_TOP)/src/gcc/ld/ldver.c $(PATH_TOP)/src/gcc/ld/ldver.tmp.c && \
		sed -e '/fprintf.*GNU.*ld.*version/s/(with BFD %s).*/(with BFD %s) $(INNOTEK_VERSION)\\n"),/' \
			$(PATH_TOP)/src/gcc/ld/ldver.tmp.c > $(PATH_TOP)/src/gcc/ld/ldver.c && \
		rm -f $(PATH_TOP)/src/gcc/ld/ldver.tmp.c ; \
	fi


# Quick bootstrap workers.
binutils-quick:
	GCCLOAD=5 $(MAKE) $(MAKEOPT) -f $(MAKEFILE) binutils214

binutils-quick-installstage:
	rm -Rf $(PATH_BUILTUNIX_TMP)
	if test -f $(PATH_BUILTTOOLS_TMP)/bin/ld.exe; then cp -f $(PATH_BUILTTOOLS_TMP)/bin/ld.exe                       $(PATH_BUILTTOOLS_TMP)/bin/ld-saved.exe; fi
	if test -f $(PATH_BUILTTOOLS_TMP)/i386-pc-os2-emx/bin/ld.exe; then cp -f $(PATH_BUILTTOOLS_TMP)/i386-pc-os2-emx/bin/ld.exe       $(PATH_BUILTTOOLS_TMP)/i386-pc-os2-emx/bin/ld-saved.exe; fi
	$(MAKE) $(MAKEOPT) -C $(PATH_OBJ)/binutils prefix=$(PATH_BUILTTOOLS_TMPD) install
	mv -f $(PATH_BUILTTOOLS_TMP)/bin/ld.exe                  $(PATH_BUILTTOOLS_TMP)/bin/ld-bad.exe
	mv -f $(PATH_BUILTTOOLS_TMP)/i386-pc-os2-emx/bin/ld.exe  $(PATH_BUILTTOOLS_TMP)/i386-pc-os2-emx/bin/ld-bad.exe
	if test -f $(PATH_BUILTTOOLS_TMP)/bin/ld-saved.exe; then mv -f $(PATH_BUILTTOOLS_TMP)/bin/ld-saved.exe                 $(PATH_BUILTTOOLS_TMP)/bin/ld.exe; fi
	if test -f $(PATH_BUILTTOOLS_TMP)/i386-pc-os2-emx/bin/ld-saved.exe; then mv -f $(PATH_BUILTTOOLS_TMP)/i386-pc-os2-emx/bin/ld-saved.exe $(PATH_BUILTTOOLS_TMP)/i386-pc-os2-emx/bin/ld.exe; fi
	cp -Rf $(PATH_BUILTUNIX_TMP)/* $(PATH_BUILTUNIX)/
	rm -Rf $(PATH_BUILTUNIX_TMP)


# Generate diffs for Binutils (part of packing).
.PHONY: binutils-diff $(ALL_PREFIX)/src/diffs/binutils-$(BINUTILS_VERSION).diff
binutils-diff: $(ALL_PREFIX)/src/diffs/binutils-$(BINUTILS_VERSION).diff
$(ALL_PREFIX)/src/diffs/binutils-$(BINUTILS_VERSION).diff:
	mkdir -p $(@D)
	$(TOOL_CVS_DIFF_TREE) $(BINUTILS_CVS_REL) src/binutils > $@








###############################################################################
###############################################################################
###############################################################################
#
#    E M X
#
###############################################################################
###############################################################################
###############################################################################
all-emx  emx: \
		emx-build
	@echo "Successfully build EMX."

# Some helpers.
# TODO: Change OUT and INS to the right ones. Currently not possible as
#       doing such breaks the rules generating.
EMX_MODE = dbg
ifeq "$(BUILD_MODE)" "RELEASE"
EMX_MODE = opt
endif
#EMX_OUT  = out/
#EMX_INS  = out/install/
EMX_OUT  = $(PATH_OBJD)/emx/
EMX_INS  = $(ALL_PREFIX)/
EMX_MASM = $(PATH_TOP)/tools/x86.os2/masm/v6.0/binp/ml.exe
EMX_DEFINES = "OUT=$(EMX_OUT)" "INS=$(EMX_INS)" "MODE=$(EMX_MODE)" "ASM=$(EMX_MASM) -c"

# build the components. (directory changer rules)
.PHONY: emx-build emx-install emx-configure
emx-build emx-install:
	mkdir -p $(PATH_OBJ)/emx
	$(MAKE) $(MAKEOPT) -C $(PATH_TOP)/src/emx -f $(MAKEFILE) $@-it

emx-build-it:
	$(MAKE) $(MAKEOPT) -j 1 -C $(PATH_TOP)/src/emx $(EMX_DEFINES) tools
	$(MAKE) $(MAKEOPT) -C $(PATH_TOP)/src/emx $(EMX_DEFINES) all

emx-install-it:
	$(MAKE) $(MAKEOPT) -C $(PATH_TOP)/src/emx $(EMX_DEFINES) INS=$(ALL_PREFIX)/ install


# We pass down the INNOTEK_VERSION define when building EMX.
emx-versionstamps:
	@echo "Version stamping EMX... nothing to do"

# Install the compiled emx stuff to builtunix.
emx-builtunix-stage2:
	$(MAKE) $(MAKEOPT) -C $(PATH_TOP)/src/emx $(EMX_DEFINES) INS=$(PATH_BUILTTOOLS_TMP)/  install

# Quick bootstrap workers.
emx-quick-libs:
	GCCLOAD=3 $(MAKE) $(MAKEOPT) -f $(PATH_TOP)/src/emx/libonly.gmk -C $(PATH_TOP)/src/emx $(EMX_DEFINES) all

emx-quick-libs-install:
	$(MAKE) $(MAKEOPT) -f $(PATH_TOP)/src/emx/libonly.gmk -C $(PATH_TOP)/src/emx $(EMX_DEFINES) INS=$(PATH_BUILTTOOLS)/ install

emx-quick-rest:
	GCCLOAD=3 $(MAKE) $(MAKEOPT) -C $(PATH_TOP)/src/emx $(EMX_DEFINES) all

emx-quick-rest-install:
	$(MAKE) $(MAKEOPT) -C $(PATH_TOP)/src/emx $(EMX_DEFINES) INS=$(PATH_BUILTTOOLS)/ install


# Generate diffs for EMX (part of packing).
.PHONY: emx-diff $(ALL_PREFIX)/src/diffs/emx-$(EMX_VERSION).diff
emx-diff: $(ALL_PREFIX)/src/diffs/emx-$(EMX_VERSION).diff
$(ALL_PREFIX)/src/diffs/emx-$(EMX_VERSION).diff:
	mkdir -p $(@D)
	$(TOOL_CVS_DIFF_TREE) $(EMX_CVS_REL) src/emx > $@





###############################################################################
###############################################################################
###############################################################################
#
#    P A C K I N G
#
###############################################################################
###############################################################################
###############################################################################
packing: \
		packing-all \
		packing-doc \
		packing-libc \

ZIPFLAGS=-rX9
ZIPBASE=$(PATH_BIN)/GCC-$(GCC_VERSION)-$(GCC_RELEASE_ID)

packing-all:
	rm -f $(ZIPBASE).zip
	cd $(ALL_PREFIX)/.. && zip $(ZIPFLAGS) $(ZIPBASE).zip *

packing-doc:
	rm -f $(ZIPBASE)-doc.zip
	cd $(ALL_PREFIX)/.. && zip $(ZIPFLAGS) $(ZIPBASE)-doc.zip usr/doc/GCC-$(GCC_VERSION)/*

packing-libc:
	rm -f $(PATH_BIN)/libc-$(LIBC_VERSION)-$(GCC_RELEASE_ID).zip
	cd $(ALL_PREFIX)/lib && zip $(ZIPFLAGS) $(PATH_BIN)/libc-$(LIBC_VERSION)-$(GCC_RELEASE_ID).zip libc*.dll gcc*.dll
	cd $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION) && zip $(ZIPFLAGS) $(PATH_BIN)/libc-$(LIBC_VERSION)-$(GCC_RELEASE_ID).zip COPYING.LIB

packing-src: \
		packing-src-gcc \
		packing-src-binutils \
		packing-src-emx \

packing-src-gcc:
	rm -f $(ZIPBASE)-src-GCC.zip
	zip $(ZIPFLAGS) $(ZIPBASE)-src-GCC.zip src/gcc -x \*CVS\* -x \*.svn\*

packing-src-binutils:
	rm -f $(ZIPBASE)-src-binutils.zip
	zip $(ZIPFLAGS) $(ZIPBASE)-src-binutils.zip src/binutils -x \*CVS\* -x \*.svn\*

packing-src-emx:
	rm -f $(ZIPBASE)-src-emx.zip
	zip $(subst r,,$(ZIPFLAGS)) $(ZIPBASE)-src-emx.zip src/emx/* -x \*CVS\* -x \*.svn\* -x \*testcase\* -x \*out\*
	zip $(ZIPFLAGS) $(ZIPBASE)-src-emx.zip src/emx/src/* src/emx/include/* src/emx/src/bsd/* -x \*CVS\* -x \*.svn\*


###############################################################################
###############################################################################
###############################################################################
#
#    M I S C
#
###############################################################################
###############################################################################
###############################################################################

misc-install:
	mkdir -p $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
	mkdir -p $(ALL_PREFIX)/lib
	mkdir -p $(ALL_PREFIX)/bin
	cp $(PATH_TOP)/doc/ReleaseNotes.os2         $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
	cp $(PATH_TOP)/doc/MailingLists.os2         $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
	cp $(PATH_TOP)/doc/Install.os2              $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
	cp $(PATH_TOP)/doc/Licenses.os2             $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
	cp $(PATH_TOP)/doc/KnownProblems.os2        $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
	cp $(PATH_TOP)/doc/COPYING.LIB              $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
	cp $(PATH_TOP)/doc/COPYING                  $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
#	cp $(PATH_TOP)/ChangeLog                    $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)/ChangeLog.os2
	cp $(PATH_TOP)/src/emx/ChangeLog.LIBC       $(ALL_PREFIX)/doc/GCC-$(GCC_VERSION)
	cp $(PATH_TOP)/src/misc/MakeOmfLibs.cmd     $(ALL_PREFIX)/lib
	cp $(PATH_TOP)/src/misc/dllar.cmd           $(ALL_PREFIX)/bin
	cp $(PATH_TOP)/src/misc/gccenv.cmd          $(ALL_PREFIX)/bin


#
# Checkout rule
#
checkout update up:
	cvs -q update -d -P 2>&1 | tee up.log

ifdef OFFICIAL_BIRD_VERSION
ifneq ($(filter-out eniac delirium os2bld,$(HOSTNAME)),)
$(error yea, nice try! Now go be ashamed of yourself!)
endif
endif

# DO NOT DELETE
