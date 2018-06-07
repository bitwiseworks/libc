# $Id: Makefile 1456 2004-09-05 10:17:06Z bird $

SUBDIRS ?=
#EXCLUDE ?= true

default: all

clean all check distclean install:
	for gmk in $(wildcard *.gmk); \
	do \
		echo "doing: $$gmk"; \
		if [ "$$gmk" != "namelessunions.gmk" ] then \
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
	
