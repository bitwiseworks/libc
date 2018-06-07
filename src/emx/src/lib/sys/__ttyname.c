#include "libc-alias.h"
#include <stdlib.h>
#include <emx/syscalls.h>

int __ttyname (int handle, char *buf, size_t buf_size)
{
  (void)handle; (void)buf; (void)buf_size;
  perror ("ttyname() not implemented");
  return 0;
}
