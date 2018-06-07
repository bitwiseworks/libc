# $Id: Makefile 3101 2007-04-14 23:46:49Z bird $
## @file
#
# Bootstrap makefile.
#
# Copyright (c) 2004 knut st. osmundsen <bird@anduin.net>
#
#
# This file is part of InnoTek LIBC.
#
# InnoTek LIBC is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# InnoTek LIBC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with InnoTek LIBC; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#

include version.smak

help:
	@echo "Welcome to $(PACKAGE) version $(VERSION) build system!"
	@echo "$(COPYRIGHT)"
	@echo ""
	@echo "There are four makefile you can invoke using the -f option of make:"
	@echo "    Makefile      - Bootstrap libc and tools (this makefile)."
	@echo "                    Does 1) toolonly.smak and 2) libonly.smak."
	@echo "    libonly.gmk   - Build libc using installed tools."
	@echo "    toolonly.gmk  - Build all the tools using installed libc & tools."
	@echo "    Makefile.gmk  - Build everything, building and using the tools."
	@echo "                    This normally never works unless the difference"
	@echo "                    between the libc versions you're building and using"
	@echo "                    are microscopical."
	@echo ""
	@echo "This makefile is used to bootstrap libc in is usually what you want to"
	@echo "use when building libc. It have the following goals:"
	@echo "    all           - Build tools and libc. (in that order)"
	@echo "    tools         - make -f toolonly.gmk all"
	@echo "    libs          - make -f libonly.gmk all"
	@echo "    clean         - Wipes all the output."
	@echo "    install       - installs everything."
	@echo "    bootstrap     - all + install + clean + all + install. This assumes"
	@echo "                    that your install directory is setup to override any"
	@echo "                    installed libc and tools when somethings installed."
	@echo "    core          - Don't build (some of) the alternative library versions."
	@echo "    core-install  - Install the stuff build with the core goal."
	@echo "    help          - This message."
	@echo ""
	@echo "Enjoy :-)"

all:
	+$(MAKE) -f toolonly.gmk $@
	+$(MAKE) -f libonly.gmk $@

libs lib:
	+$(MAKE) -f libonly.gmk all

tools tool:
	+$(MAKE) -f toolonly.gmk all

clean:
	+$(MAKE) -f Makefile.gmk $@

install:
	+$(MAKE) -f toolonly.gmk $@
	+$(MAKE) -f libonly.gmk $@

bootstrap:
	@echo "Warning, this goal hasn't been fully tested yet!"
	+$(MAKE) -f toolonly.gmk all
	+$(MAKE) -f libonly.gmk all
	+$(MAKE) -f toolonly.gmk install
	+$(MAKE) -f libonly.gmk install
	+$(MAKE) -f Makefile clean
	+$(MAKE) -f toolonly.gmk all
	+$(MAKE) -f libonly.gmk all
	+$(MAKE) -f toolonly.gmk install
	+$(MAKE) -f libonly.gmk install

core:
	+$(MAKE) -f libonly.gmk LIBC_CORE_ONLY=1 all
	
core-install:
	+$(MAKE) -f libonly.gmk LIBC_CORE_ONLY=1 install

