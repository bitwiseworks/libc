# $Id: Makefile 2241 2005-07-10 09:53:08Z bird $
	
	
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
LDFLAGS=
CFLAGS = -O -D_GNU_SOURCE -std=gnu99 -DOBJPFX=\"$(OUTDIR)/\"
ifeq ($(shell uname -m),amd64)
CFLAGS+= -I sysdeps/x86_64 -I .
else
CFLAGS+= -I sysdeps/i386/i686 -I sysdeps/i386 -I .
endif


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
CFLAGS     += -D__GLIBC__ \
	-DHAVE___STRTOD_INTERNAL \
	-DHAVE_ASPRINTF \
	-DHAVE_GETLINE \
	-DHAVE_STRNLEN \
	-DHAVE_STPNCPY \
	-DHAVE_STRCHRNUL \
	-DHAVE_RAWMEMCHR \
	-DHAVE_MEMRCHR \
	-DHAVE_STRSEP \
	-DHAVE_STRNDUP \
	-DHAVE_MEMPCPY \
	-DHAVE_NEWLOCALE \
	-DHAVE_64BIT_FILEIO_TYPES \
	-DHAVE_SWPRINTF \
	-DHAVE_RPC_NETDB_H \
	-DHAVE_NL_LANGINFO_EXTENTIONS \
	-DHAVE_FPUTWS
endif

ifeq ($(TARGET),innoteklibc)
SKIPPED    := \
	assert/test-assert-perr.c \
	\
	inet/bug-if1.c \
	inet/test-ifaddrs.c \
	inet/test_ifindex.c \
	inet/tst-ether_aton.c \
	\
	io/test-stat.c \
	io/test-stat2.c \
	\
	libio/bug-ftell.c \
	libio/bug-mmap-fflush.c \
	libio/bug-rewind.c \
	libio/bug-rewind2.c \
	libio/bug-ungetwc1.c \
	libio/bug-ungetwc2.c \
	libio/bug-wfflush.c \
	libio/test-fmemopen.c \
	libio/tst-ext.c \
	libio/tst-fgetws.c \
	libio/tst-fopenloc.c \
	libio/tst-swscanf.c \
	libio/tst-ungetwc1.c \
	libio/tst-ungetwc2.c \
	libio/tst-widetext.c \
	libio/tst_getwc.c \
	libio/tst_putwc.c \
	libio/tst_swprintf.c \
	libio/tst_swscanf.c \
	libio/tst_wprintf.c \
	libio/tst_wprintf2.c \
	libio/tst_wscanf.c \
	\
	login/tst-grantpt.c \
	login/tst-utmpx.c \
	\
	malloc/tst-mallocstate.c \
	\
	math/test-matherr.c \
	math/bug-tgmath1.c \
	math/test-tgmath.c \
	math/test-tgmath-ret.c \
	\
	misc/tst-efgcvt.c \
	misc/tst-mntent.c \
	\
	nss/bug-erange.c \
	\
	posix/bug-ga1.c	\
	posix/bug-ga2.c \
	posix/test-vfork.c \
	posix/tst-boost.c \
	posix/tst-execl2.c \
	posix/tst-execle2.c \
	posix/tst-execlp2.c \
	posix/tst-execv2.c \
	posix/tst-execve2.c \
	posix/tst-execvp2.c \
	posix/tst-execvp3.c \
	posix/tst-getaddrinfo.c \
	posix/tst-getaddrinfo2.c \
	posix/tst-gnuglob.c \
	posix/tst-mmap.c \
	posix/tst-preadwrite64.c \
	posix/tst-truncate64.c \
	posix/tst-vfork1.c \
    posix/tst-vfork2.c \
	\
	pwd/tst-getpw.c \
	\
	stdio-common/tst-cookie.c \
	stdio-common/tst-fmemopen.c \
	stdio-common/tst-fmemopen2.c \
	stdio-common/tst-obprintf.c \
	stdio-common/tst-perror.c \
	stdio-common/tst-pop8en.c \
	stdio-common/tst-printfsz.c \
	stdio-common/tst-swprintf.c \
	\
    stdlib/test-a64l.c \
	stdlib/tst-setcontext.c \
	\
	string/test-ffs.c \
	string/test-stpncpy.c \
	string/tst-bswap.c \
	string/tst-strfry.c \
	\
	sunrpc/tst-getmyaddr.c \
	\
	time/tst_wcsftime.c \
	time/tst-ftime_l.c \
	time/tst-getdate.c \
	time/tst-strptime2.c \
	\
	timezone/test-tz.c \
	timezone/tst-timezone.c \
	\
	wcsmbs/tst-wcpncpy.c \
	wcsmbs/tst-wcsnlen.c \

EXPECTED   := \
	wcsmbs/tst-mbrtowc.c \
	wcsmbs/tst-wcrtomb.c \
	time/tst-posixtz.c \
	time/tst-strftime.c \
	stdlib/tst-putenv.c \
	stdio-common/test-popen.c \
	stdio-common/test-vfprintf.c \
	stdio-common/tst-fphex.c \
    stdio-common/tstscanf.c \
	resolv/tst-aton.c \
	posix/bug-glob1.c \
	posix/bug-glob2.c \
	posix/bug-regex18.c \
	posix/bug-regex22.c \
	posix/testfnm.c \
	posix/tst-chmod.c \
	posix/tst-dir.c \
	posix/tst-fnmatch.c	\
	posix/tst-pcre.c \
	posix/tst-rxspencer.c \
	misc/tst-tsearch.c  \
	misc/tst-dirname.c \
	io/bug-ftw4.c \
	iconv/tst-iconv1.c \
	iconv/tst-iconv2.c \
	iconv/tst-iconv3.c \
	iconv/tst-iconv5.c \
	grp/testgrp.c \
	dirent/opendir-tst1.c \
	catgets/test-gencat.c \
	math/test-fenv.c \
	libio/bug-fopena+.c	\
	libio/tst-sscanf.c \
	libio/tst-atime.c \
	posix/tst-preadwrite.c
	
# NOTES:	
# stdio-common/test-vfprintf.c: 	File duplication and truncation needs exploring with regards to file position!
# stdio-common/tst-fphex.c:			We don't implement %a.
# stdio-common/tst-sscanf.c: 		Is doing some thousand separator and internationalization stuff which we don't implement.
# stdio-common/tstscanf.c: 			two cases of sscan working in three different ways depending on glibc, bsd or libc. cool!
# posix/runtests.c:					This really needs looking at on BSD.
# posix/testfnm.c:					This looks bad!
# posix/tst-fnmatch.c:				This too!
# misc/tst-tsearch.c:				This really needs looking at on BSD.
# io/bug-ftw4.c:					open("dir")
# iconv/*.c:						Are all about UCS4 which we don't support.
# dirent/opendir-tst1.c:			Uses a FIFO.
	
CFLAGS     += -Zomf -g -I sysdeps/os2 \
	-Dprogram_invocation_short_name=getprogname\(\) \
	-DHAVE_STRNLEN \
	-DHAVE_MEMPCPY \
	-DHAVE_ASPRINTF \
	-DHAVE_GETLINE \
	-DHAVE_STRCHRNUL \
	-DHAVE_RAWMEMCHR \
	-DHAVE_MEMRCHR
LDFLAGS    += -Zstack 512
_TARGET_OK := ok

posix/runptests.c_ARGS = 0  		# expect 0 errors ?
math/test-float.c_ARGS = -e 273		# expect 273 errors
math/test-ifloat.c_ARGS = -e 217	# expect 217 errors
math/test-double.c_ARGS = -e 174	# expect 174 errors
math/test-idouble.c_ARGS = -e 142	# expect 142 errors
math/test-ldouble.c_ARGS = -e 212	# expect 212 errors
math/test-ildoubl.c_ARGS = -e 153	# expect 153 errors
endif

ifeq ($(TARGET),bsd)
SKIPPED    := \
	argp/tst-argp1.c \
	argp/argp-test.c \
	\
	assert/test-assert-perr.c \
	\
	crypt/cert.c \
	\
	iconv/tst-iconv1.c \
	iconv/tst-iconv2.c \
	iconv/tst-iconv3.c \
	iconv/tst-iconv5.c \
	\
	inet/tst-ether_aton.c \
	\
	intl/tst-ngettext.c \
	\
	string/test-mempcpy.c \
	string/test-stpncpy.c \
	string/tst-bswap.c \
	string/tst-strfry.c
EXCEPTIONS := \
	catgets/test-gencat.c
_TARGET_OK := ok
CFLAGS     += -I sysdeps/unix/bsd -D__BSD__ \
	-DHAVE_ASPRINTF \
	-DHAVE_SWPRINTF \
	-DHAVE_GETLINE
endif

ifndef _TARGET_OK
$(error Invalid TARGET definition '$(TARGET)'. valid: glibc, innoteklibc, bsd)
endif


nothing:
	@echo TS=$(TS)


#
# The test.
#

ARGP := \
	argp/tst-argp1.c \
	argp/argp-test.c
	
ASSERT := \
	assert/test-assert-perr.c \
	assert/test-assert.c
	
CATGETS := \
	catgets/test-gencat.c
	
#CONFORM - separate.

CRYPT := \
	crypt/md5c-test.c \
	crypt/cert.c
	
crypt/md5c-test.c_LDFLAGS = -lcrypt
crypt/cert.c_LDFLAGS = -lcrypt
crypt/cert.c_INPUT = crypt/cert.input
	
CSU := \
	csu/tst-empty.c
#	csu/tst-atomic.c \
#	csu/tst-atomic-long.c \
	
CTYPE := \
	ctype/test_ctype.c
	
DEBUG := # skipped

DIRENT := \
	dirent/list.c \
	dirent/bug-readdir1.c \
	dirent/opendir-tst1.c \
	dirent/tst-seekdir.c

DLFCN := # postponed
ELF := # postponed
GMON := # postponed

GNULIB := \
	gnulib/tst-gcc.c

GRP := \
	grp/testgrp.c
#	grp/tst_fgetgrent.c
	
ICONV := \
	iconv/tst-iconv1.c \
	iconv/tst-iconv2.c \
	iconv/tst-iconv3.c \
	iconv/tst-iconv5.c

ICONVDATA := # skipped

INET := \
	inet/bug-if1.c \
	inet/htontest.c \
	inet/test-ifaddrs.c \
	inet/test_ifindex.c \
	inet/tst-ether_aton.c \
	inet/tst-gethnm.c \
	inet/tst-network.c \
	inet/tst-ntoa.c

INTL := \
	intl/tst-ngettext.c
# more intl todo	
IO := \
        io/bug-ftw1.c \
        io/bug-ftw2.c \
        io/bug-ftw3.c \
        io/bug-ftw4.c \
        io/ftwtest.c \
        io/test-lfs.c \
        io/test-stat.c \
        io/test-stat2.c \
        io/test-utime.c \
        io/tst-fcntl.c \
        io/tst-getcwd.c \
        io/tst-statvfs.c
# todo io/ftwtest.sh
io/bug-ftw2.c_DIR = ./io

LIBIO := \
        libio/bug-fopena+.c \
        libio/bug-fseek.c \
        libio/bug-ftell.c \
        libio/bug-mmap-fflush.c \
        libio/bug-rewind.c \
        libio/bug-rewind2.c \
        libio/bug-ungetc.c \
        libio/bug-ungetc2.c \
        libio/bug-ungetc3.c \
        libio/bug-ungetc4.c \
        libio/bug-ungetwc1.c \
        libio/bug-ungetwc2.c \
        libio/bug-wfflush.c \
        libio/test-fmemopen.c \
        libio/test-freopen.c \
        libio/tst-atime.c \
        libio/tst-eof.c \
        libio/tst-ext.c \
        libio/tst-fgetws.c \
        libio/tst-fopenloc.c \
        libio/tst-freopen.c \
        libio/tst-mmap-eofsync.c \
        libio/tst-mmap-fflushsync.c \
        libio/tst-mmap-offend.c \
        libio/tst-mmap-setvbuf.c \
        libio/tst-mmap2-eofsync.c \
        libio/tst-sscanf.c \
        libio/tst-swscanf.c \
        libio/tst-ungetwc1.c \
        libio/tst-ungetwc2.c \
        libio/tst-widetext.c \
        libio/tst_getwc.c \
        libio/tst_putwc.c \
        libio/tst_swprintf.c \
        libio/tst_swscanf.c \
        libio/tst_wprintf.c \
        libio/tst_wprintf2.c \
        libio/tst_wscanf.c
	
libio/tst-widetext.c_INPUT = libio/tst-widetext.input
libio/tst_getwc.c_INPUT = libio/tst_getwc.input
libio/tst_wscanf.c_INPUT = libio/tst_wscanf.input
	
libio/test-freopen.c_ARGS = $(ABSOUTDIR)/libio/test-freopen.out
libio/test-freopen.c_EXPECT      = libio/test-freopen.expect
libio/test-freopen.c_EXPECT_FILE = $(OUTDIR)/libio/test-freopen.out
libio/test-freopen.c_EXPECT_TEXT = yes

LINUXTHREADS := # skipped

LOCALE := \
	locale/tst-C-locale.c

LOCALEDATA := # todo later

LOGIN := \
        login/tst-grantpt.c \
        login/tst-utmp.c \
        login/tst-utmpx.c
	
MALLOC := \
        malloc/tst-calloc.c \
        malloc/tst-malloc.c \
        malloc/tst-mallocstate.c \
        malloc/tst-mcheck.c \
        malloc/tst-obstack.c \
        malloc/tst-valloc.c
#        malloc/tst-mtrace.c
#        malloc/tst-mtrace.sh

MATH := \
        math/basic-test.c \
        math/bug-nextafter.c \
        math/bug-nexttoward.c \
        math/bug-tgmath1.c \
        math/test-fenv.c \
        math/test-float.c \
        math/test-fpucw.c \
        math/test-matherr.c \
        math/test-misc.c \
        math/test-tgmath-ret.c \
        math/test-tgmath.c \
        math/tst-definitions.c \
        math/test-idouble.c \
        math/test-ifloat.c \
        math/test-double.c \
        math/test-ldouble.c \
        math/test-ildoubl.c
#        math/atest-exp2.c
#        math/atest-sincos.c
#         math/atest-exp.c

$(foreach t,$(MATH),$(eval $(t)_LDFLAGS := -lm))
$(foreach t,$(filter math/test-%,$(MATH)), \
$(eval $(t)_DEPS := $(OUTDIR)/math/libm-test.stamp) \
$(eval $(t)_CFLAGS := -I $(OUTDIR)/math/) \
)

$(OUTDIR)/math/libm-test.stamp: \
		sysdeps/i386/fpu/libm-test-ulps \
		math/libm-test.inc \
		math/gen-libm-test.pl Makefile
	cd math && perl gen-libm-test.pl -u ../$< -o ../$(OUTDIR)/math/
	@echo > $@


MISC :=\
        misc/tst-dirname.c \
        misc/tst-efgcvt.c \
        misc/tst-fdset.c \
        misc/tst-hsearch.c \
        misc/tst-mntent.c \
        misc/tst-tsearch.c
misc/tst-tsearch.c_LDFLAGS = -lm

NPTL := # skipped

NSS := \
        nss/bug-erange.c \
        nss/test-netdb.c


#        posix/bug-regex5.c  - internal
#        posix/bug-regex20.c - internal
#        posix/bug-regex24.c - goes on forever it seems
POSIX :=\
        posix/bug-ga1.c \
        posix/bug-ga2.c \
        posix/bug-glob1.c \
        posix/bug-glob2.c \
        posix/bug-regex1.c \
        posix/bug-regex10.c \
        posix/bug-regex11.c \
        posix/bug-regex12.c \
        posix/bug-regex13.c \
        posix/bug-regex14.c \
        posix/bug-regex15.c \
        posix/bug-regex16.c \
        posix/bug-regex17.c \
        posix/bug-regex18.c \
        posix/bug-regex19.c \
        posix/bug-regex2.c \
        posix/bug-regex21.c \
        posix/bug-regex22.c \
        posix/bug-regex23.c \
        posix/bug-regex3.c \
        posix/bug-regex4.c \
        posix/bug-regex6.c \
        posix/bug-regex7.c \
        posix/bug-regex8.c \
        posix/bug-regex9.c \
        posix/runptests.c \
        posix/runtests.c \
        posix/test-vfork.c \
        posix/testfnm.c \
        posix/tst-boost.c \
        posix/tst-chmod.c \
        posix/tst-dir.c \
        posix/tst-exec.c \
        posix/tst-execl1.c \
        posix/tst-execl2.c \
        posix/tst-execle1.c \
        posix/tst-execle2.c \
        posix/tst-execlp1.c \
        posix/tst-execlp2.c \
        posix/tst-execv1.c \
        posix/tst-execv2.c \
        posix/tst-execve1.c \
        posix/tst-execve2.c \
        posix/tst-execvp1.c \
        posix/tst-execvp2.c \
        posix/tst-execvp3.c \
        posix/tst-fnmatch.c \
        posix/tst-fork.c \
        posix/tst-getaddrinfo.c \
        posix/tst-getaddrinfo2.c \
        posix/tst-getlogin.c \
        posix/tst-gnuglob.c \
        posix/tst-mmap.c \
        posix/tst-nanosleep.c \
        posix/tst-nice.c \
        posix/tst-pcre.c \
        posix/tst-preadwrite.c \
        posix/tst-preadwrite64.c \
        posix/tst-rxspencer.c \
        posix/tst-spawn.c \
        posix/tst-truncate.c \
        posix/tst-truncate64.c \
        posix/tst-vfork1.c \
        posix/tst-vfork2.c \
        posix/tst-waitid.c \
        posix/tstgetopt.c
#        posix/tst-regex.c - internal?
#        posix/tst-regex2.c - too new?
#        posix/tst-regexloc.c - internal?
#        posix/tst-sysconf.c - too new?
# todo:
#        posix/wordexp-test.c
#        posix/wordexp-tst.sh
#        posix/globtest.c
#        posix/globtest.sh
#        posix/tst-getconf.sh

posix/tst-fnmatch.c_INPUT = posix/tst-fnmatch.input
	
posix/tstgetopt.c_ARGS = -a -b -cfoobar --required foobar --optional=bazbug --none random --col --color --colour
posix/tst-exec.c_ARGS = -- ./tst-exec.exe --direct --direct --direct
posix/tst-spawn.c_ARGS = -- ./tst-spawn.exe --direct --direct --direct
posix/tst-dir.c_ARGS = `pwd` `pwd`/$(OUTDIR)/posix `pwd`/$(OUTDIR) `pwd`/$(OUTDIR)/tst-dir.exe
posix/tst-chmod.c_ARGS = `pwd`/posix
posix/tst-rxspencer.c_ARGS = --utf8 $(CURDIR)/posix/rxspencer/tests
posix/tst-pcre.c_ARGS = $(CURDIR)/posix/PCRE.tests
posix/tst-boost.c_ARGS = $(CURDIR)/posix/BOOST.tests
posix/bug-glob1.c_ARGS = "."
posix/tst-execvp3.c_ARGS = --test-dir="."
	

PWD := \
	pwd/tst-getpw.c
	
RESOLV := \
	resolv/tst-aton.c \
	resolv/tst-leaks.c \
	resolv/tst-leaks2.c
ifeq ($(TARGET),glibc)
resolv/tst-leaks.c_LDFLAGS = -lresolv
resolv/tst-leaks2.c_LDFLAGS = -lresolv
endif
	
RT :=  # todo
SETJMP := \
	setjmp/tst-setjmp.c
	
SIGNAL := \
        signal/tst-raise.c \
        signal/tst-signal.c \
        signal/tst-sigset.c \
        signal/tst-sigsimple.c
	
SOFT-FP := # skipping

STDIO-COMMON := \
        stdio-common/temptest.c \
        stdio-common/test-fseek.c \
        stdio-common/test-fwrite.c \
        stdio-common/test-popen.c \
        stdio-common/test-vfprintf.c \
        stdio-common/test_rdwr.c \
        stdio-common/tst-cookie.c \
        stdio-common/tst-fdopen.c \
        stdio-common/tst-ferror.c \
        stdio-common/tst-fileno.c \
        stdio-common/tst-fmemopen.c \
        stdio-common/tst-fmemopen2.c \
        stdio-common/tst-fphex.c \
        stdio-common/tst-fseek.c \
        stdio-common/tst-gets.c \
        stdio-common/tst-obprintf.c \
        stdio-common/tst-perror.c \
        stdio-common/tst-popen.c \
        stdio-common/tst-printfsz.c \
        stdio-common/tst-rndseek.c \
        stdio-common/tst-sprintf.c \
        stdio-common/tst-sscanf.c \
        stdio-common/tst-swprintf.c \
        stdio-common/tst-tmpnam.c \
        stdio-common/tst-ungetc.c \
        stdio-common/tst-unlockedio.c \
        stdio-common/tst-wc-printf.c \
        stdio-common/tstdiomisc.c \
        stdio-common/tstgetln.c \
        stdio-common/tstscanf.c
stdio-common/tst-ferror.c_INPUT = stdio-common/tst-ferror.input
stdio-common/tst-gets.c_INPUT = stdio-common/tst-gets.input
stdio-common/tstgetln.c_INPUT = stdio-common/tstgetln.input
stdio-common/tstscanf.c_INPUT = stdio-common/tstscanf.input
#        stdio-common/tst-printf.c
#        stdio-common/tst-printf.sh
#        stdio-common/tst-unbputc.c
#        stdio-common/tst-unbputc.sh

STDLIB := \
	stdlib/bug-strtod.c \
	stdlib/test-a64l.c \
	stdlib/test-canon.c \
	stdlib/test-canon2.c \
	stdlib/testmb.c \
	stdlib/testmb2.c \
	stdlib/testrand.c \
	stdlib/testsort.c \
	stdlib/tst-bsearch.c \
	stdlib/tst-environ.c \
	stdlib/tst-limits.c \
	stdlib/tst-putenv.c \
	stdlib/tst-qsort.c \
	stdlib/tst-rand48.c \
	stdlib/tst-random.c \
	stdlib/tst-random2.c \
	stdlib/tst-setcontext.c \
	stdlib/tst-strtod.c \
	stdlib/tst-strtol.c \
	stdlib/tst-strtoll.c \
	stdlib/tst-system.c \
	stdlib/tst-xpg-basename.c \
	stdlib/testdiv.c
# todo:	
#	stdlib/tst-fmtmsg.c
#	stdlib/tst-fmtmsg.sh
# 	stdlib/tst-putenvmod.c
stdlib/testdiv.c_INPUT = stdlib/testdiv.input

STRING := \
	string/bug-strcoll1.c \
	string/bug-strncat1.c \
	string/bug-strpbrk1.c \
	string/bug-strspn1.c  \
	string/inl-tester.c   \
	string/noinl-tester.c \
	string/test-ffs.c     \
	string/test-memccpy.c \
	string/test-memchr.c  \
	string/test-memcmp.c  \
	string/test-memcpy.c  \
	string/test-memmove.c \
	string/test-mempcpy.c \
	string/test-memset.c  \
	string/test-stpcpy.c  \
	string/test-stpncpy.c \
	string/test-strcat.c  \
	string/test-strchr.c  \
	string/test-strcmp.c  \
	string/test-strcpy.c  \
	string/test-strcspn.c \
	string/test-strlen.c  \
	string/test-strncmp.c \
	string/test-strncpy.c \
	string/test-strpbrk.c \
	string/test-strrchr.c \
	string/test-strspn.c  \
	string/testcopy.c     \
	string/tester.c       \
	string/tst-bswap.c    \
	string/tst-inlcall.c  \
	string/tst-strfry.c   \
	string/tst-strlen.c   \
	string/tst-strtok.c   \
	string/tst-strxfrm.c  \

STRING_IN_EXP := \
	string/tst-svc.c

SUNRPC := \
	sunrpc/tst-getmyaddr.c
	
SYSDEPS := # todo

TIME := \
	time/clocktest.c \
	time/test_time.c \
	time/tst-ftime_l.c \
	time/tst-getdate.c \
	time/tst-mktime.c \
	time/tst-mktime2.c \
	time/tst-mktime3.c \
	time/tst-posixtz.c \
	time/tst-strftime.c \
	time/tst-strptime.c \
	time/tst-strptime2.c \
	time/tst_wcsftime.c

TIMEZONE := \
	timezone/test-tz.c \
	timezone/tst-timezone.c

WCSMBS := \
	wcsmbs/tst-btowc.c \
	wcsmbs/tst-mbrtowc.c \
	wcsmbs/tst-mbsrtowcs.c \
	wcsmbs/tst-wcpncpy.c \
	wcsmbs/tst-wcrtomb.c \
	wcsmbs/tst-wcsnlen.c \
	wcsmbs/tst-wcstof.c \
	wcsmbs/wcsmbs-tst1.c

WCTYPE := \
	wctype/test_wcfuncs.c \
	wctype/test_wctype.c	


SUBTESTS := \
	ARGP \
	ASSERT \
	CATGETS \
	CRYPT \
	CSU \
	CTYPE \
	DEBUG \
	DIRENT \
	DLFCN \
	ELF \
	GMON \
	GNULIB \
	GRP \
	ICONV \
	ICONVDATA \
	INET \
	INTL \
	IO \
	LIBIO \
	LINUXTHREADS \
	LOCALE \
	LOCALEDATA \
	LOGIN \
	MALLOC \
	MANUAL \
	MATH \
	MISC \
	NPTL \
	NSS \
	POSIX \
	PWD \
	RESOLV \
	RT \
	SCRIPTS \
	SETJMP \
	SIGNAL \
	SOFT-FP \
	STDIO-COMMON \
	STDLIB \
	STRING \
	SUNRPC \
	SYSDEPS \
	TIME \
	TIMEZONE \
	WCSMBS \
	WCTYPE

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
	cd $(dir $(OUTDIR)/$2) && LC_ALL=C $($1_ENVS) $(notdir $2).exe $($1_ARGS))))

$(OUTDIR)/$2.result: $1 $(OUTDIR)/$(dir $1).dir_created $($(1)_DEPS)
	@echo "########## $1"; \
	echo  "##  BUILDING: $$(CC) $$(CFLAGS) $$(LDFLAGS) -o $(OUTDIR)/$2.exe $($(1)_CFLAGS) $($(1)_LDFLAGS) $$<"; \
	if $$(CC) $$(CFLAGS) $$(LDFLAGS) -o $(OUTDIR)/$2.exe $($(1)_CFLAGS) $($(1)_LDFLAGS) $$<; then \
		echo "##  RUNNING : ( $(_EXEC) )$(if $($(1)_INPUT), < $($(1)_INPUT))"; \
		if ( $(_EXEC) )$(if $($(1)_INPUT), < $($(1)_INPUT)); then \
			echo OK > $$@; \
			printf "########## %-26s - SUCCESS $(if $(filter $1,$(EXPECTED)),!Used to fail!)\n" "$1"; \
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
	$(MAKE) tests 2>&1 | tee $(call LOG,test)

	
#
# Special and separate tests.
#
.PHONY: conform
conform:
	cd conform/ && perl ./conformtest.pl

conform-logged:
	@echo "**** logging to $(call LOG,conform) ****"
	@mkdir -p $(OUTDIR)
	$(MAKE) conform 2>&1 | tee $(call LOG,conform)

