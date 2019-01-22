MAKEFILE_DIR := $(patsubst %/, %, $(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

OUT := $(MAKEFILE_DIR)-build/
INS := $(MAKEFILE_DIR)-install/

ASM := $(realpath $(MAKEFILE_DIR)/bin/ml.exe)

ifeq ($(realpath $(ASM)),)
$(error ASM points to non-existing file [$(ASM)])
endif

MAKE_DEFS := "OUT=$(OUT)" "INS=$(INS)" "ASM=$(ASM) -c" "SHELL=/@unixroot/usr/bin/sh.exe"

all: release

release: release-tools release-libs

release-tools:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) -j1 tools

release-libs:
	$(MAKE) -C src/emx MODE=opt $(MAKE_DEFS) libs
