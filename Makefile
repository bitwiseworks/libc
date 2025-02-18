MAKEFILE_DIR := $(patsubst %/,%,$(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

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

LIBC_OPTIMIZE_FLAGS ?= -O3 -g -march=pentium4 \
  -ffile-prefix-map=$(MAKEFILE_DIR)=libc \
  -ffile-prefix-map=$(patsubst %/,%,$(LIBC_OUTPUT_DIR))=libc \
  -ffile-prefix-map=$(TEMP)=libc

ifdef LIBC_OPTIMIZE_FLAGS
  MAKE_DEFS += "OPTIMIZE_FLAGS=$(LIBC_OPTIMIZE_FLAGS)"
endif

all: release

release: release-tools release-libs

release-dep:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) -f Makefile.gmk dep

release-cleandep:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) -f Makefile.gmk cleandep

release-tools:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) tools

# NOTE: There is a bug in kmk's .NOTPARALLEL processing triggered by $(_STD_WILDWILD) deps for
# $.stmp-libc-std that causes it to generate libc-std.h out of order so that files including it
# build earlier. A workaround is to generate this file up front in a separate invocation.
release-libs-common:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) -f libonly.gmk libc-std.h

release-libs: release-libs-common
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) libs

release-libs-core: release-libs-common
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) LIBC_CORE_ONLY=1 libs

#release-libs-core-fast:
#	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) LIBC_CORE_ONLY=1 -f libonly.gmk libc-dll

release-install:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) install

release-clean:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) clean

release-distbuild: export PATH := $(subst /,\,$(LIBC_INSTALL_DIR)bin;$(PATH))
release-distbuild: export LIBRARY_PATH := $(LIBC_INSTALL_DIR)lib;$(LIBRARY_PATH)
release-distbuild: export C_INCLUDE_PATH := $(LIBC_INSTALL_DIR)include;$(C_INCLUDE_PATH)
release-distbuild:
	@test -d "$(LIBC_INSTALL_DIR)"
	$(MAKE) release-tools
	$(MAKE) release-libs

release-bootstrap:
	rm -rf "$(LIBC_OUTPUT_DIR)"
	rm -rf "$(LIBC_INSTALL_DIR)"
	$(MAKE) release
	$(MAKE) release-install
	$(MAKE) release-clean
	rm -rf "$(LIBC_OUTPUT_DIR)"
	$(MAKE) release-distbuild
	rm -rf "$(LIBC_INSTALL_DIR)"
	$(MAKE) release-install

rm-build-dirs:
	rm -rf "$(LIBC_OUTPUT_DIR)"
	rm -rf "$(LIBC_INSTALL_DIR)"

#help:
#	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) help

#.NOTPARALLEL: release-tools release-libs release-libs-common
