/* ioctl.h (emx+gcc) */

#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#if defined (__cplusplus)
extern "C" {
#endif

#include <sys/ioccom.h>
#include <sys/sockio.h>
#include <sys/filio.h>

#if !defined (TCGETA)
#define TCGETA      1
#define TCSETA      2
#define TCSETAW     3
#define TCSETAF     4
#define TCFLSH      5
#define TCSBRK      6
#define TCXONC      7
#endif

#define _TCGA       8           /* Used internally for tcgetattr() */
#define _TCSANOW    9           /* Used internally for tcsetattr() */
#define _TCSADRAIN  10          /* Used internally for tcsetattr() */
#define _TCSAFLUSH  11          /* Used internally for tcsetattr() */

#if !defined (FGETHTYPE)
#define FGETHTYPE  32
#endif

#if !defined (HT_FILE)
#define HT_FILE         0
#define HT_UPIPE        1
#define HT_NPIPE        2
#define HT_DEV_OTHER    3
#define HT_DEV_NUL      4
#define HT_DEV_CON      5
#define HT_DEV_CLK      7
#define HT_SOCKET       8
#define HT_ISDEV(n)     ((n) >= HT_DEV_OTHER && (n) <= HT_DEV_CLK)
#endif

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_IOCTL_H */
