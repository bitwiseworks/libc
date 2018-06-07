/* eadadd.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ead.h>
#include "ea.h"

int _ead_add (_ead ead, const char *name, int flags, const void *value,
              int size)
{
  int i, len, new_size, offset;
  PFEA2 dst, last;

  i = _ead_find (ead, name);
  if (i >= 1)
    {
      if (_ead_replace (ead, i, flags, value, size) < 0)
        return -1;
      return i;
    }
  len = strlen (name);
  new_size = _EA_SIZE1 (len, size);
  if (ead->count == 0)
    offset = sizeof (ULONG);
  else
    {
      offset = ead->buffer->cbList;
      last = ead->index[ead->count - 1];
      last->oNextEntryOffset = _EA_SIZE2 (last);
    }
  if (_ead_size_buffer (ead, offset + new_size) < 0)
    return -1;
  ead->buffer->cbList = offset + new_size;
  dst = (PFEA2)((char *)ead->buffer + offset);
  dst->oNextEntryOffset = 0;
  dst->fEA = flags;
  dst->cbName = len;
  dst->cbValue = size;
  memcpy (dst->szName, name, len + 1);
  memcpy (dst->szName + len + 1, value, size);
  if (_ead_make_index (ead, ead->count + 1) < 0)
    return -1;
  ++ead->count;
  return ead->count;
}


int _ead_replace (_ead ead, int index, int flags, const void *value, int size)
{
  PFEA2 dst;
  int old_size, new_size, offset;

  if (index < 1 || index > ead->count)
    {
      errno = EINVAL;
      return -1;
    }
  dst = ead->index[index-1];
  new_size = _EA_SIZE1 (dst->cbName, size);
  old_size = _EA_SIZE2 (dst);
  offset = (char *)dst - (char *)ead->buffer;
  dst = NULL;
  if (new_size > old_size)
    {
      if (_ead_size_buffer (ead, ead->buffer_size - old_size + new_size) < 0)
        return -1;
    }
  dst = (PFEA2)((char *)ead->buffer + offset);
  if (new_size != old_size && index != ead->count)
    memmove ((char *)dst + new_size, (char *)dst + old_size,
             ead->buffer->cbList - (offset + old_size));
  dst->fEA = flags;
  dst->cbValue = size;
  dst->oNextEntryOffset = (index == ead->count ? 0 : new_size);
  ead->buffer->cbList += new_size - old_size;
  memcpy (dst->szName + dst->cbName + 1, value, size);
  return _ead_make_index (ead, ead->count);
}


int _ead_delete (_ead ead, int index)
{
  PFEA2 ptr;
  int old_size;

  if (index < 1 || index > ead->count)
    {
      errno = EINVAL;
      return -1;
    }
  ptr = ead->index[index-1];
  old_size = _EA_SIZE2 (ptr);
  memmove (ptr, (char *)ptr + old_size,
           ead->buffer->cbList - (((char *)ptr - (char *)ead->buffer)
                                  + old_size));
  ead->buffer->cbList -= old_size;
  if (_ead_make_index (ead, ead->count - 1) < 0)
    return -1;
  --ead->count;
  return 0;
}
