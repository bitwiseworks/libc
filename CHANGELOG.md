# History of changes for LIBC Next

#### Version 0.1.4 (2020-03-27)

* Add more errors to strerror/perror [#8].
* sys/stat.h: Swap st_mtim and st_mtimespec and friends [#45].
* Add wide char output functions (wprintf, putwc and families) [#8].
* Fix WCHAR_MIN/MAX and WINT_MIN/MAX definitions [#8].
* b_timeHighResNano: Use integer arithmetics [#53].
* Add clock_gettime and friends [#53].
* Implement POSIX semaphores [#54].
* Make new POSIX 2008 string functions available by default [#57].
* Make isblank available by default [#61].
* _sockaddr_storage.h: Change alignment to 32 bits [#62].
* sys/socket.h: Add socklen_t, sa_family_t, sockaddr_storage [#62].
* select: Do a simple wait cycle when nfds is 0 [bitwiseworks/libevent-os2#1].
* Add wide char output functions (wscanf, getwc and families) [#8].
* Add fgetws and fputws wide char functions [#8].
* math.h: Align with latest FreeBSD [bitwiseworks/gcc-os2#15].
* Add wcsxfrm function [#68].
* sys/socket.h: Add SHUT_* constants for shutdown() [bitwiseworks/qtwebengine-chromium-os2#3].
* signal.h: Force si_code constants to be (signed) int [bitwiseworks/qtwebengine-chromium-os2#3].

#### Version 0.1.3 (2019-12-25)

* Restrict 2-nd __cxchg operand to A/B/C/D registers [#47].
* symlink: Return EEXIST if path name exists and is directory [#48].
* posix_spawn: Return errno on failure instead of -1 [#46].
* sys/stat.h: Add more BSD-like aliases like st_mtimensec and st_mtim [#45].
* Rebuild with newer libiberty (binutils 2.33.1) to fix EMXOMF crash [#49].

#### Version 0.1.2 (2019-07-15)

* Get rid of direct resolving of DosOpenL and friends. [#36].
* unlink() shall not delete directories, but return EISDIR [upstream].
* Make _fnlwr/_fnlwr2 work for path rewrites (/@unixroot and such) [#39].
* Provide pwd_mkdb tool that handles : to $ replacement in drive names in pw_dir [#1].
* Add getrusage() implementation [#40].

#### Version 0.1.1 (2019-02-24)

* Fix failures to fork children from drive root directory [#31].
* Silence emxomf warnings not fixable by programmer [#32].

#### Version 0.1.0 (2019-02-15)

* Initial release of LIBC Next based on kLIBC version 0.6.6 CSD6 (released on 27-10-2014). The below list of changes shows important changes, improvements and fixes since this version.
* Change main LIBC DLL name from LIBC066.DLL to LIBCN0.DLL [#17].
* Add bww copyright and git commit information to all binaries and DLLs [#17].
* Change LIBC global mutex and shared memory names from INNOTEKLIBC to BWWLIBC [#17].
* Add \__LIBCN\__, \__LIBCN_MINOR\__, __LIBCN_PREREQ defines to features.h [#29].
* Fix resetting file access mode to O_WRONLY if opened with O_NOINHERIT [#2].
* Define __LONG_LONG_SUPPORTED on modern C++ (C++11 and above) [#3].
* Make readdir return DT_LNK for symlinks [#9].
* Make sure SIGCHLD is raised after a zombie for wait[pid] is created [#10].
* Make reaplath fail on non-existing paths [#11].
* Make stat succeed on file names with trailing spaces [#12].
* Make [f]close return 0 regardless of DosClose result if LIBC handle is freed [bitwiseworks/libcx#60].
* Fix typo in dlclose backend that would result in random return values [#14].
* Increase dlerror buffer to 260+64 chars (to fit the full DLL path).
* Fix LIBC makefiles to support building with GCC 4.x.x [#4].
* Remove libiberty headers and libs (newer ones are part of our binutils port) [#4].
* Align emxexp and innidmdll to newer libiberty from binutils [#4].
* Backport extern inline handling from newer GLIBC (2.28) [#4].
* Remove GCC 3 C++ library from LIBC DLL (our GCC 4 port provides a separate C++ DLL) [#4].
* Remove deprecated __STDC_CONSTANT_MACROS and __STDC_LIMIT_MACROS [#3, http://trac.netlabs.org/libc/ticket/297].
* Define max_align_t in stddef.h as required by ISO C [#3, http://trac.netlabs.org/libc/ticket/297].
* Fix LIBC_LOGGING processing by LIBC itself [#3, http://trac.netlabs.org/libc/ticket/361].
* Fix incorrect __KLIBC_ARG_MASK usage in __spawne [#3, http://trac.netlabs.org/libc/ticket/362].
* Reset global thread data in child after fork [#3, http://trac.netlabs.org/libc/ticket/363].
* Support more OS/2 APIRET codes when converting to errno [#3, http://trac.netlabs.org/libc/ticket/364].
* Add new __LIBC_FORK_CTX_FLAGS_LAST fork callback flag [#3, http://trac.netlabs.org/libc/ticket/366].
* emxomfld: Fix Invalid WKEXT record errors [#3, http://trac.netlabs.org/libc/ticket/376].
* Remove BSD defines from sys/param.h and types.h [#3, http://trac.netlabs.org/libc/ticket/377].
* Fix rare DosAllocMemEx(OBJ_LOCATION) bug [#3, http://trac.netlabs.org/libc/ticket/384].
* Remove /@unixroot/bin and /@unixroot/sbin from default paths [#15].
* Add NO_STRIP makefile variable to disable stripping debug info [#22].
* Fix LIBC makefiles to better support building LIBC RPMs [#24].
* Fix infinite loop in atexit, a GCC 4 regression [#25, #4].
* Properly define kHeapDbgException as weak symbol [#26, #4].
* Fix all compiler warnings under GCC 4 [#21].
* Fix incorrect process termination reason report in some cases [#21].
* Properly return old timer values in getitimer and setitimer APIs [#21].
* Fix parameter validation in waitid APIs [#21].
* Make __libc_TLSAlloc return ENOMEM when no memory instead of random errno [#21].
* Add getdents and getdirentries APIs [#27].
* Export __libc_native2errno from main DLL [#16].
* Add CMSG_LEN and CMSG_SPACE to sys/socket.h [#7].
* Add modern ISO C definition for NULL to stddef.h [#20].
* Update Safe DOS wrappers with ones from kLIBC 0.7 trunk [#24].
* Import emxomfstrip from kLIBC 0.7 trunk [#24].
* Add SafeDosEnumAttribute to libos2 [#19].
* Implement sysconf(_SC_NPROCESSORS_ONLN) [#5].
