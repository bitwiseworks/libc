#include "../../../msun/src/math_private.h"
#ifdef __INNOTEK_LIBC__
# define strong_alias(name, aliasname) _strong_alias(name, aliasname)
# define _strong_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((alias (#name)));
#endif

