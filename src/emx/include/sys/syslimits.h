/** @file sys/syslimits.h
 * BSD like sys/syslimits.h file.
 *
 * TCPV40HDRS does include this file, but as we don't need to be
 * 100% compatible we don't care.
 */

#ifndef _SYS_SYSLIMITS_H_
#define _SYS_SYSLIMITS_H_

/** @group syslimits parameters.
 * These are in sys/syslimits.h on BSD.
 * @{
 */
#ifndef ARG_MAX
/** Max argument size for an exec function.
 * OS2: DosExecPgm does not accept more than 32KB of command line arguments
 *      (ditto for environment). So, we keep the limit a few bytes short of
 *      this to avoid rounding errors on the user side. */
#define ARG_MAX         0x7fe0
#endif

#ifndef CHILD_MAX
/** Maximum simultaneous processes.
 * OS2: Max threads config.sys param is the (theoretical) limit. */
#define CHILD_MAX       4096
#endif

#ifndef LINK_MAX
/** Max file link count.
 * OS2: Doesn't mean anything on OS/2. */
#define LINK_MAX                0x7fff
#endif

#ifndef LOGIN_NAME_MAX
/** Max login name length including terminating NULL. */
#define LOGIN_NAME_MAX  17
#endif

#ifndef MAX_CANON
/** Max bytes in term canon input line.
 * OS2: what's this? */
#define MAX_CANON       255
#endif

#ifndef MAX_INPUT
/** Max bytes in term canon input line.
 * OS2: what's this? */
#define MAX_INPUT       255
#endif

#ifndef NAME_MAX
/** Max chars in a filename.
 * Filename no path. (POSIX) */
#define NAME_MAX        256
#endif

#ifndef NGROUPS_MAX
/** Max supplemental group id's.
 * OS2: doesn't make much sense yet. */
#define NGROUPS_MAX     16
#endif

#ifndef OPEN_MAX
/** Max number of open files per process.
 * OS2: Using DosSetMaxFH the theoretical maximum should be 0xfff0 I believe.
 */
#define OPEN_MAX        0xfff0
#endif

#ifndef PATH_MAX
/** Max number of bytes in a pathname. */
#define PATH_MAX        260
#endif

#ifndef PIPE_BUF
/** Max number of bytes for atomic pipe writes.
 * OS2: doesn't make sense. */
#define PIPE_BUF        0x200
#endif

#ifndef IOV_MAX
/** Max elements in i/o vector.
 * OS2: eeeh what's this? */
#define IOV_MAX         0x400
#endif
/** @} */

#endif
