/* ead.c (emx+gcc) -- Copyright (c) 1993-2000 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <sys/ead.h>
#include "ea.h"

_ead _ead_create (void)
{
  _ead p;

  p = malloc (sizeof (*p));
  if (p == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }
  p->count = 0;
  p->max_count = 0;
  p->total_value_size = 0;
  p->total_name_len = 0;
  p->buffer_size = 0;
  p->index = NULL;
  p->buffer = NULL;
  return p;
}


void _ead_destroy (_ead ead)
{
  if (ead->buffer != NULL) free (ead->buffer);
  if (ead->index != NULL) free (ead->index);
  free (ead);
}


void _ead_clear (_ead ead)
{
  ead->count = 0;
  ead->total_value_size = 0;
  ead->total_name_len = 0;
}


int _ead_count (_ead ead)
{
  return ead->count;
}


int _ead_value_size (_ead ead, int index)
{
  if (index == 0)
    return ead->total_value_size;
  else if (index < 1 || index > ead->count)
    {
      errno = EINVAL;
      return -1;
    }
  else
    return ead->index[index-1]->cbValue;
}


int _ead_name_len (_ead ead, int index)
{
  if (index == 0)
    return ead->total_name_len;
  else if (index < 1 || index > ead->count)
    {
      errno = EINVAL;
      return -1;
    }
  else
    return ead->index[index-1]->cbName;
}


const char *_ead_get_name (_ead ead, int index)
{
  if (index < 1 || index > ead->count)
    {
      errno = EINVAL;
      return NULL;
    }
  else
    return (const char *)ead->index[index-1]->szName;
}


const void *_ead_get_value (_ead ead, int index)
{
  if (index < 1 || index > ead->count)
    {
      errno = EINVAL;
      return NULL;
    }
  else
    return ead->index[index-1]->szName + ead->index[index-1]->cbName + 1;
}


int _ead_get_flags (_ead ead, int index)
{
  if (index < 1 || index > ead->count)
    {
      errno = EINVAL;
      return -1;
    }
  else
    return ead->index[index-1]->fEA;
}


int _ead_find (_ead ead, const char *name)
{
  int i;

  for (i = 0; i < ead->count; ++i)
    if (strcmp (name, ead->index[i]->szName) == 0)
      return i+1;
  errno = ENOENT;
  return -1;
}


int _ead_make_index (struct _ead_data *ead, int new_count)
{
  int i;
  PFEA2 pfea;

  if (new_count > ead->max_count)
    {
      ead->max_count = new_count;
      ead->index = realloc (ead->index, ead->max_count * sizeof (*ead->index));
      if (ead->index == NULL)
        {
          ead->max_count = 0;
          ead->count = 0;
          errno = ENOMEM;
          return -1;
        }
    }
  pfea = &ead->buffer->list[0];
  for (i = 0; i < new_count; ++i)
    {
      ead->index[i] = (PFEA2)pfea;
      pfea = (PFEA2)((char *)pfea + pfea->oNextEntryOffset);
    }
  return 0;
}


int _ead_size_buffer (struct _ead_data *ead, int new_size)
{
  if (new_size > ead->buffer_size)
    {
      ead->buffer_size = new_size;
      ead->buffer = realloc (ead->buffer, ead->buffer_size);
      if (ead->buffer == NULL)
        {
          ead->buffer_size = 0;
          ead->count = 0;
          errno = ENOMEM;
          return -1;
        }
    }
  return 0;
}
