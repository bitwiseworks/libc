#include "libc-alias.h"

/* This is the location of "environ" variable.
   Since it is put into c_app library, statically linked applications
   get it from here, and applications built with libc.dll get it from
   the C library DLL. Dynamic libraries don't have environ at all,
   unless they're using libc.dll in which case they get application's
   environment.  */
char **_STD(environ) = 0;
char **_org_environ = 0;
