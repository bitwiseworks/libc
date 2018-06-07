/* omflibut.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Utility functions for dealing with OMFLIBs. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omflib0.h"
#include <sys/omflib.h>

int omflib_set_error (char *error)
{
  strcpy (error, strerror (errno));
  return -1;
}


int omflib_close (struct omflib *p, char *error)
{
  int i;

  fclose (p->f);
  if (p->dict != NULL)
    free (p->dict);
  if (p->mod_tab != NULL)
    {
      for (i = 0; i < p->mod_count; ++i)
        free (p->mod_tab[i].name);
      free (p->mod_tab);
    }
  if (p->pub_tab != NULL)
    {
      for (i = 0; i < p->pub_count; ++i)
        free (p->pub_tab[i].name);
      free (p->pub_tab);
    }
  free (p);
  return 0;
}


#define ROL2(x) (((unsigned)(x) << 2) | ((unsigned)(x) >> 14))
#define ROR2(x) (((unsigned)(x) >> 2) | ((unsigned)(x) << 14))

void omflib_hash (struct omflib *p, const byte *name)
{
  int i, len;
  word block_index, bucket_index, block_index_delta, bucket_index_delta;
  byte c;

  len = name[0];
  block_index = 0;
  block_index_delta = 0;
  bucket_index = 0;
  bucket_index_delta = 0;
  for (i = 0; i < len; ++i)
    {
      c = name[i] | 0x20;
      block_index = ROL2 (block_index) ^ c;
      bucket_index_delta = ROR2 (bucket_index_delta) ^ c;
      c = name[len-i] | 0x20;
      bucket_index = ROR2 (bucket_index) ^ c;
      block_index_delta = ROL2 (block_index_delta) ^ c;
    }
  p->block_index = block_index % p->dict_blocks;
  p->block_index_delta = block_index_delta % p->dict_blocks;
  if (p->block_index_delta == 0)
    p->block_index_delta = 1;
  p->bucket_index = bucket_index % 37;
  p->bucket_index_delta = bucket_index_delta % 37;
  if (p->bucket_index_delta == 0)
    p->bucket_index_delta = 1;
}


int omflib_module_name (char *dst, const char *src)
{
  const char *base;
  char *s;
  int len;

  base = _getname (src);
  s = strchr (base, '.');
  if (s != NULL)
    len = s - base;
  else
    len = strlen (base);
  if (len > 255)
    {
      memcpy (dst, base, 255);
      dst[255] = 0;
      return -1;
    }
  else
    {
      memcpy (dst, base, len);
      dst[len] = 0;
      return 0;
    }
}
