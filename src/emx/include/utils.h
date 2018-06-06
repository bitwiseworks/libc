/* utils.h,v 1.6 2004/09/14 22:27:36 bird Exp */
/** @file
 * OS/2 TCPIP
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <sys/cdefs.h>

#ifdef TCPV40HDRS

unsigned long   TCPCALL lswap(unsigned long);
unsigned short  TCPCALL bswap(unsigned short);
int             TCPCALL rexec(char **, int, char *, char *, char *, int *);

/* Definition for bswap */
#ifndef htonl
#include <machine/endian.h>
#endif

#define ovbcopy(x,y,z)  bcopy((x),(y),(z))
#define copyout(x,y,z)  memcpy((y),(x),(z))
#ifndef XP_OS2_VACPP /* mozilla */
/* We've got the real thing now. (aliased at least)
#define strcasecmp(x,y)     strcmpi((x),(y))
#define strncasecmp(x,y,z)  strnicmp(x,y,z)
*/
#endif

__BEGIN_DECLS

#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T) /* bird: emx */
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#define _SIZE_T                         /* bird: emx */
#endif

int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);

__END_DECLS

/* MIN/MAX */
#include <sys/param.h>
/* timercmp */
#include <sys/time.h>

#else
int     TCPCALL rexec(char **, int, char *, char *, char *, int *);
#endif /*TCPV40HDRS*/

#endif
