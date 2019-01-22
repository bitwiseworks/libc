/** @file sys/param.h
 * BSD like sys\param.h file.
 *
 * TCPV40HDRS does include this file, but as we don't need to be
 * 100% compatible we don't care.
 */

#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H
#define _SYS_PARAM_H_ /* toolkit */

#if defined (__cplusplus)
extern "C" {
#endif


#if !defined (NULL)
#if defined (__cplusplus)
#define NULL    0
#else
#define NULL    ((void*)0)
#endif
#endif


#if 0 /* Don't pretend we're BSD (see below) */
/** @group BSD version defines.
 * OS2: The toolkit headers define these. Resent FreeBSD release does too.
 * Warning! Be aware that config scripts and programs may check for these and
 *          assume they're running on BSD. We might have to take out these
 *          defines.
 * @{
 */
/** System version - year and month */
#ifdef TCPV40HDRS
#define BSD     43
#else
#define BSD     199506
#endif
#ifndef TCPV40HDRS
/** Indicate BSD4.3 features present. */
#define BSD4_3  1
/** Indicate BSD4.4 features present. */
#define BSD4_4  1
#endif
/** @} */
#endif


#ifndef LOCORE
#include "../types.h" /* #include <types.h> frequently includes the wrong header. */
#endif
#include <sys/syslimits.h>
#include <sys/signal.h>
#include <machine/param.h>
#include <machine/limits.h>


/** @group  System Parameters (BSD flavored)
 *
 * @{ */
#ifndef MAXCOMLEN
/** Max command name remembered. */
#define MAXCOMLEN       19
#endif

#ifndef MAXINTERP
/** Max interpreter file name length (ELF?).
 * OS2: Have no meaning. */
#define MAXINTERP       12
#endif

#ifndef MAXLOGNAME
/** Max login name length including terminating NULL. */
#define MAXLOGNAME      LOGIN_NAME_MAX
#endif

#ifndef MAXUPRC
/** Maximum simultaneous processes. */
#define MAXUPRC         CHILD_MAX
#endif

#ifndef NCARGS
/** Max argument size for an exec function.
 * OS2: DosExecPgm does not accept more than 32KB of command line arguments
 *      (ditto for environment). So, we keep the limit a few bytes short of
 *      this to avoid rounding errors on the user side. */
#define NCARGS          ARG_MAX
#endif
#ifndef NGROUPS
/** Max supplemental group id's.
 * OS2: doesn't make much sens just set it high. */
#define NGROUPS         NGROUPS_MAX
#endif

#ifndef NOFILE
/** Max number of open files per process.
 * OS2: Using DosSetMaxFH the theoretical maximum should be 0xfff0 I believe.
 */
#define NOFILE          OPEN_MAX
#endif

#ifndef NOGROUP
/** Marker for empty group set member.
 * OS2: no meaning currently. */
#define NOGROUP         0xffff
#endif

#ifndef MAXHOSTNAMELEN
/** Max number of bytes in a hostname. */
#define MAXHOSTNAMELEN  256
#endif

#ifndef SPECNAMELEN
/** Max number of bytes in a devicename.
 * OS2: no real meaning. */
#define SPECNAMELEN     16
#endif

#ifndef MAXNAMLEN
/** Max number of chars in a file name.
 * BSD name for posix NAME_MAX. */
#define MAXNAMLEN       NAME_MAX
#endif

#ifndef MAXPATHLEN
/** Max number of chars in a path name.
 * BSD name for posix PATH_MAX.
 * @remark Very strict BSD kernel/user code may expect it to be a number which
 *         is the power of two an. The OS/2 number is not a power of 2. */
#define MAXPATHLEN      PATH_MAX
#endif

#ifndef MAXBSIZE
/** Max filesystem block size.
 * Pretty internal FreeBSD define which may be used to optimize file access.
 * It must be a power of 2. */
#define MAXBSIZE        65536
#endif

#ifndef BKVASIZE
/** Nominal buffer space per buffer, in bytes.
 * Pretty internal FreeBSD define which may be used to optimize file access.
 * It must be a power of 2. */
#define BKVASIZE        16384
#endif

#ifndef BKVAMASK
/** Block mask of some kind... Very internal BSD stuff which noone really should use. */
#define BKVAMASK        (BKVASIZE - 1)
#endif

/** @} */


/** @group EMX defines
 * @{
 */
#ifndef HZ
/** Frequencey of something but have no clue of what...
 * Freebsd isn't defining this, linux is but only for the kernel sources.
 * Considered EMX specific. */
#define HZ        100
#endif
/** @} */


/** @group Bitmap Access
 * The bitmaps in question is arrays if integer.
 * @{ */
/** number of bytes in an int */
#define NBPW sizeof(int)
/** Set a bit in the bitmap */
#define setbit(Bitmap,iBit)   ( (Bitmap)[(i)/NBBY] |=   1 << ((i) % NBBY)  )
/** Clear a bit in the bitmap */
#define clrbit(Bitmap,iBit)   ( (Bitmap)[(i)/NBBY] &= ~(1 << ((i) % NBBY)) )
/** Test if a bit in a bitmap is set. */
#define isset(Bitmap,iBit)    ( (Bitmap)[(i)/NBBY]  &  (1 << ((i) % NBBY)) )
/** Test if a bit in a bitmap is clear. */
#define isclr(Bitmap,iBit)    (((Bitmap)[(i)/NBBY]  & 1 << ((i) % NBBY)) == 0 )
/** @} */


/* Handle flags of some kind... Toolkit defines it here.
 * Freebsd headers indicates that these are KERNEL flags, but
 * there is a chance one or two tcpip ioctls are using them. */
#ifndef _POSIX_SOURCE
#ifndef FREAD
#define FREAD   0x0001
#define FWRITE  0x0002
#endif
#endif

#if 0 /* we don't need it, endian takes care of this */
/* Basic byte order conversion (non-inline).
 * Note that freebsd only does this when KERNEL is defined. */
#if !defined (htonl)
#define htonl(X) _swapl(X)
#define ntohl(X) _swapl(X)
#define htons(X) _swaps(X)
#define ntohs(X) _swaps(X)
#endif
unsigned short _swaps (unsigned short _x);
unsigned long _swapl (unsigned long _x);
#endif

/*
 * minimum and maximum macros. Use available g++ tricks.
 */
#if defined(__cplusplus) && (__GNUC__ >= 2) && (__GNUC__ <= 3)
# define MIN(a, b)      ( (a) <? (b) )
# define MAX(a, b)      ( (a) >? (b) )
#else
# define MIN(a, b)      ( (a) < (b) ? (a) : (b) )
# define MAX(a, b)      ( (a) > (b) ? (a) : (b) )
#endif

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_PARAM_H */
