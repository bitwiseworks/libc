# Makefile for syslog replacement
#
# @configure_input@
#
# Copyright (c) 1998-2000 Carnegie Mellon University.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer. 
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. The name "Carnegie Mellon University" must not be used to
#    endorse or promote products derived from this software without
#    prior written permission. For permission or any other legal
#    details, please contact  
#      Office of Technology Transfer
#      Carnegie Mellon University
#      5000 Forbes Avenue
#      Pittsburgh, PA  15213-3890
#      (412) 268-4387, fax: (412) 268-7395
#      tech-transfer@andrew.cmu.edu
#
# 4. Redistributions of any form whatsoever must retain the following
#    acknowledgment:
#    "This product includes software developed by Computing Services
#     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
#
# CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
# FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
# AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
# OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# 

srcdir = .
VPATH = @srcdir@

#CC = @CC@

INSTALL = @INSTALL@
RANLIB = echo

DEFS =
CPPFLAGS = 
LIBS = -lsyslog -lsocket

CFLAGS = -g -Zomf
LDFLAGS = -Zexe -g -Zomf

SHELL = /bin/sh
MAKEDEPEND = @MAKEDEPEND@

all: libsyslog.a logger

install:
	@echo "Install syslogd and syslog.conf as appropriate"

.c.o:
	$(CC) -c $(CPPFLAGS) $(DEFS) -I$(srcdir) $(CFLAGS) $<

libsyslog.a: syslog.o
	emxomfar cr libsyslog.a syslog.o
	$(RANLIB) libsyslog.a

syslogd: syslogd.o
	$(CC) $(LDFLAGS) -o syslogd syslogd.o $(LIBS)

logger: logger.o libsyslog.a
	$(CC) $(LDFLAGS) -o logger logger.o $(LIBS)

clean:
	rm -f *.o *.a Makefile.bak

distclean: clean
	rm -f Makefile

depend:
	${MAKEDEPEND} $(CPPFLAGS) $(DEFS) -I$(srcdir) $(CFLAGS) *.c $(srcdir)/*.c 1>makedepend.log 2>&1

# DO NOT DELETE THIS LINE -- make depend depends on it.


