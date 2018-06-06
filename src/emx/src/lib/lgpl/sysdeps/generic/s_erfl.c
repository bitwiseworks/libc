#include "libc-alias-glibc.h"
#include <math.h>
#include <stdio.h>
#include <errno.h>

long double
_STD(erfl) (long double x)
{
  fputs ("__erfl not implemented\n", stderr);
  errno = ENOSYS;
  return 0.0;
}
/* w e a k _ a l i a s (__erfl, erfl) */

//stub_warning (erfl)

long double
_STD(erfcl) (long double x)
{
  fputs ("__erfcl not implemented\n", stderr);
  errno = ENOSYS;
  return 0.0;
}
/* w e a k _ a l i a s (__erfcl, erfcl) */

//stub_warning (erfcl)
//#include <stub-tag.h>
