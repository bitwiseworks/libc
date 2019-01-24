MAKEFILE_DIR := $(patsubst %/, %, $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

LIBC_OUTPUT_DIR ?= $(MAKEFILE_DIR)-build/
LIBC_INSTALL_DIR ?= $(MAKEFILE_DIR)-install/

LIBC_ASM_EXE ?= $(realpath $(MAKEFILE_DIR)/bin/ml.exe)

ifeq ($(realpath $(LIBC_ASM_EXE)),)
$(error ASM points to non-existing file [$(LIBC_ASM_EXE)])
endif

MAKE_DEFS := \
  "NO_STRIP=1" \
  "OUT=$(LIBC_OUTPUT_DIR)" \
  "INS=$(LIBC_INSTALL_DIR)" \
  "ASM=$(LIBC_ASM_EXE) -c" \
  "SHELL=/@unixroot/usr/bin/sh.exe"

ifdef LIBC_OPTIMIZE_FLAGS
  MAKE_DEFS += "OPTIMIZE_FLAGS=$(LIBC_OPTIMIZE_FLAGS)"
endif

all: release

release: release-tools release-libs

release-tools:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) -j1 tools

# NOTE: There is a bug in kmk's .NOTPARALLEL processing triggered by $(_STD_WILDWILD) deps for
# $.stmp-libc-std that cause it to generate libc-std.h out of order so that files including it
# build earlier. A workaround is to generate this file up front in a separate invocation.
release-libs-common:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) -f libonly.gmk libc-std.h

release-libs: release-libs-common
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) libs

release-libs-core: release-libs-common
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) LIBC_CORE_ONLY=1 libs

release-install:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) install
