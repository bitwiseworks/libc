/* eadfea.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ead.h>
#include "ea.h"

int _ead_fea2list_size (_ead ead)
{
  if (ead->count == 0)
    return 0;
  else
    return (int)ead->buffer->cbList;
}


const void *_ead_get_fea2list (_ead ead)
{
  if (ead->count == 0)
    return NULL;
  else
    return (const void *)ead->buffer;
}


void *_ead_fea2list_to_fealist (const void *src)
{
  const FEA2LIST *s;
  const FEA2 *ps;
  PFEALIST d;
  PFEA pd;
  ULONG size, add;
  char *q;

  s = (const FEA2LIST *)src;
  size = sizeof (ULONG);
  if (s->cbList > sizeof (ULONG))
    {
      ps = &s->list[0];
      for (;;)
        {
          add = sizeof (FEA) + 1 + ps->cbName + ps->cbValue;
          size += add;
          if (ps->oNextEntryOffset == 0)
            break;
          ps = (const FEA2 *)((char *)ps + ps->oNextEntryOffset);
        }
    }
  d = malloc (size);
  if (d == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }
  d->cbList = size;
  pd = &d->list[0];
  if (s->cbList > sizeof (ULONG))
    {
      ps = &s->list[0];
      for (;;)
        {
          pd->fEA = ps->fEA;
          pd->cbName = ps->cbName;
          pd->cbValue = ps->cbValue;
          q = (char *)pd + sizeof (FEA);
          memcpy (q, ps->szName, ps->cbName + 1);
          q += ps->cbName + 1;
          memcpy (q, ps->szName + ps->cbName + 1, ps->cbValue);
          q += ps->cbValue;
          pd = (PFEA)q;
          if (ps->oNextEntryOffset == 0)
            break;
          ps = (const FEA2 *)((char *)ps + ps->oNextEntryOffset);
        }
    }
  return d;
}


void *_ead_fealist_to_fea2list (const void *src)
{
  const FEALIST *s;
  const FEA *ps;
  PFEA2LIST d;
  PFEA2 pd;
  ULONG size, add, offset, *patch;
  char *q;
  const char *t;

  s = (const FEALIST *)src;
  size = sizeof (ULONG);
  offset = sizeof (ULONG);
  while (offset < s->cbList)
    {
      ps = (const FEA *)((char *)src + offset);
      size += _EA_SIZE2 (ps);
      offset += sizeof (FEA) + 1 + ps->cbName + ps->cbValue;
    }
  d = malloc (size);
  if (d == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }
  d->cbList = size;
  pd = &d->list[0];
  offset = sizeof (ULONG);
  patch = NULL;
  while (offset < s->cbList)
    {
      ps = (const FEA *)((char *)src + offset);
      pd->fEA = ps->fEA;
      pd->cbName = ps->cbName;
      pd->cbValue = ps->cbValue;
      t = (char *)src + offset + sizeof (FEA);
      q = pd->szName;
      memcpy (q, t, ps->cbName + 1);
      t += ps->cbName + 1;
      q += ps->cbName + 1;
      memcpy (q, t, ps->cbValue);
      add = _EA_SIZE2 (ps);
      patch = &pd->oNextEntryOffset;
      pd->oNextEntryOffset = add;
      pd = (PFEA2)((char *)pd + add);
      offset += sizeof (FEA) + 1 + ps->cbName + ps->cbValue;
    }
  if (patch != NULL)
    *patch = 0;
  return d;
}


int _ead_use_fea2list (_ead ead, const void *src)
{
  const FEA2LIST *s;
  PFEA2 pfea;
  int count;

  s = src;
  ead->count = 0;
  if (_ead_size_buffer (ead, s->cbList) < 0)
    return -1;
  memcpy (ead->buffer, src, s->cbList);
  count = 0;
  if (s->cbList > sizeof (ULONG))
    {
      pfea = &ead->buffer->list[0];
      for (;;)
        {
          ++count;
          if (pfea->oNextEntryOffset == 0)
            break;
          pfea = (PFEA2)((char *)pfea + pfea->oNextEntryOffset);
        }
    }
  if (_ead_make_index (ead, count) < 0)
    return -1;
  ead->count = count;
  return 0;
}
