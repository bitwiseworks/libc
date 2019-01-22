/* types.h,v 1.4 2004/09/14 22:27:36 bird Exp */
/** @file
 * IGCC, OS/2 Tcpip compataility.
 */
#ifndef __TYPES_H_
#define __TYPES_H_

#include <sys/cdefs.h>

/* Machine type dependent parameters. */
#include <machine/ansi.h>
/*#include <machine/types.h> sys/types.h does this job. */

#include <sys/types.h>

#ifdef TCPV40HDRS

#include <errno.h>
#include <nerrno.h>
#include <sys/time.h>
#include <utils.h>
#include <strings.h> /* index() */

#define NIL ((char *) 0)
#define PZERO 0
#if 0 /* Don't pretend we're BSD (see sys/param.h) */
#define BSD 43
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define MAXHOSTNAMELEN 120
#ifndef MAXPATHLEN
#define MAXPATHLEN 260
#endif
#ifndef MAXSOCKETS
#define MAXSOCKETS 2048
#endif

#else  /* TCPV40HDRS */

/* OS/2 additions */
#ifndef MAXSOCKETS
#define MAXSOCKETS 32768
#endif
#define BSDMAXSOCKETS 2048

#endif /* TCPV40HDRS */

#endif

