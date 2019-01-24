MAKEFILE_DIR := $(patsubst %/, %, $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

OUT := $(MAKEFILE_DIR)-build/
INS := $(MAKEFILE_DIR)-install/

ASM := $(realpath $(MAKEFILE_DIR)/bin/ml.exe)

ifeq ($(realpath $(ASM)),)
$(error ASM points to non-existing file [$(ASM)])
endif

MAKE_DEFS := "NO_STRIP=1" "OUT=$(OUT)" "INS=$(INS)" "ASM=$(ASM) -c" "SHELL=/@unixroot/usr/bin/sh.exe"

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
