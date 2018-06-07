/* _seekhdr.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <memory.h>
#include <io.h>
#include <errno.h>

int _seek_hdr (int handle)
{
  struct
    {
      char magic[2];
      char fill1[6];
      unsigned short hdr_size;
    } exe_hdr;
  struct
    {
      char sig[16];
      char bound;
      char fill1;
      unsigned short hdr_loc_lo;      /* cannot use long, alignment! */
      unsigned short hdr_loc_hi;
    } patch;
  off_t original_pos;
  int saved_errno;

  original_pos = tell (handle);
  if (read (handle, &exe_hdr, sizeof (exe_hdr)) != sizeof (exe_hdr))
    goto failure;
  if (memcmp (exe_hdr.magic, "MZ", 2) != 0)
    return (lseek (handle, original_pos, SEEK_SET) == -1 ? -1 : 0);
  if (lseek (handle, original_pos + 16 * exe_hdr.hdr_size, SEEK_SET) == -1)
    goto failure;
  if (read (handle, &patch, sizeof (patch)) != sizeof (patch))
    goto failure;
  if (memcmp (patch.sig, "emx", 3) != 0)
    goto failure;
  if (lseek (handle, original_pos + patch.hdr_loc_lo
              + 65536 * patch.hdr_loc_hi, SEEK_SET) == -1)
    goto failure;
  return 0;

failure:
  saved_errno = errno;
  lseek (handle, original_pos, SEEK_SET);
  errno = saved_errno;
  return -1;
}
