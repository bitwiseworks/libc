# $Id: Makefile 1208 2004-02-09 00:24:46Z bird $

SUBDIRS ?= weak optlink			
EXCLUDE ?= test "475.gmk" != "$$gmk" && test "throw.gmk" != "$$gmk"

default: all

clean all check distclean install:
	for gmk in $(wildcard *.gmk); \
	do \
		echo "doing: $$gmk"; \
		if $(EXCLUDE); then \
			if $(MAKE) $@ -f $$gmk $(OPT_MAKE);  \
			then true;	\
			else exit 8; \
			fi; \
		fi; \
	done
	for dir in $(SUBDIRS); \
	do \
		if $(MAKE) -C $$dir $@; \
		then true;	\
		else exit 8; \
		fi; \
	done
	
cleanfast:	
	rm *.o *.obj *.exe *.map
	
