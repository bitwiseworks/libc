/* _fseekhd.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <memory.h>
#include <errno.h>

int _fseek_hdr (FILE *stream)
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
  long original_pos;
  int saved_errno;

  original_pos = ftell (stream);
  if (fread (&exe_hdr, sizeof (exe_hdr), 1, stream) != 1)
    goto failure;
  if (memcmp (exe_hdr.magic, "MZ", 2) != 0)
    return (fseek (stream, original_pos, SEEK_SET) == -1 ? -1 : 0);
  if (fseek (stream, original_pos + 16 * exe_hdr.hdr_size, SEEK_SET) == -1)
    goto failure;
  if (fread (&patch, sizeof (patch), 1, stream) != 1)
    goto failure;
  if (memcmp (patch.sig, "emx", 3) != 0)
    goto failure;
  if (fseek (stream, original_pos + patch.hdr_loc_lo
             + 65536L * patch.hdr_loc_hi, SEEK_SET) == -1)
    goto failure;
  return 0;

failure:
  saved_errno = errno;
  fseek (stream, original_pos, SEEK_SET);
  errno = saved_errno;
  return -1;
}
