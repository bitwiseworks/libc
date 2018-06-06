/* os2.h,v 1.3 2004/09/14 22:27:35 bird Exp */
/** @file
 * EMX
 */

#ifndef NO_INCL_SAFE_HIMEM_WRAPPERS
# include <os2safe.h>
#endif

#ifndef _OS2_H
#define _OS2_H

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef _Cdecl
#define _Cdecl
#endif
#ifndef _Far16
#define _Far16
#endif
#ifndef _Optlink
#define _Optlink
#endif
#ifndef _Pascal
#define _Pascal
#endif
#ifndef _Seg16
#define _Seg16
#endif
#ifndef _System
#define _System
#endif

#if defined (USE_OS2_TOOLKIT_HEADERS)
#include <os2tk.h>
#else
#include <os2emx.h>          /* <-- change this line to use Toolkit headers */
#endif
#include <os2thunk.h>

#if defined (__cplusplus)
}
#endif

#endif /* not _OS2_H */
