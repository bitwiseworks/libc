/* omflibcr.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Create a new OMFLIB. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "omflib0.h"
#include <sys/omflib.h>


struct omflib *omflib_create (const char *fname, int page_size, char *error)
{
  struct omflib *p;
  FILE *f;

  if (page_size < 16 || page_size > 32768
      || (page_size & (page_size - 1)) != 0)
    {
      strcpy (error, "Invalid page size");
      return NULL;
    }
  f = fopen (fname, "wb");
  if (f == NULL)
    {
      strcpy (error, strerror (errno));
      return NULL;
    }
  p = malloc (sizeof (struct omflib));
  if (p == NULL)
    {
      errno = ENOMEM;
      strcpy (error, strerror (errno));
      fclose (f);
      remove (fname);
      return NULL;
    }
  p->f = f;
  p->page_size = page_size;
  p->dict_offset = 0;
  p->dict_blocks = 0;
  p->flags = 1;
  p->mod_tab = NULL;
  p->mod_alloc = 0;
  p->mod_count = -1;
  p->dict = NULL;
  p->pub_tab = NULL;
  p->pub_alloc = 0;
  p->pub_count = 0;
  p->output = TRUE;
  p->state = OS_EMPTY;
  p->mod_page = 0;
  p->mod_name[0] = 0;
  return p;
}


int omflib_header (struct omflib *p, char *error)
{
  fseek (p->f, 0, SEEK_SET);
  return omflib_pad (p->f, p->page_size, TRUE, error);
}


static int omflib_add_dict (struct omflib *p, const char *name, int page,
                            char *error)
{
  int block_index, bucket_index;
  int bv, len, bucket_count;
  byte *block, *ptr;
  byte buf[257];
  int (*compare)(const void *s1, const void *s2, size_t n);

  len = strlen (name);
  if (len > 255)
    {
      strcpy (error, "Symbol name too long");
      return -1;
    }
  buf[0] = (byte)len;
  memcpy (buf+1, name, len);
  omflib_hash (p, buf);
  block_index = p->block_index;
  bucket_index = p->bucket_index;
  compare = (p->flags & 1 ? memcmp : memicmp);
  bucket_count = 37;
  block = p->dict + 512 * block_index;
  for (;;)
    {
      bv = block[bucket_index];
      if (bv == 0)
        {
          if (block[37] == 0xff)
            bucket_count = 0;
          else if (2 * block[37] + len + 4 > 512)
            {
              block[37] = 0xff;
              bucket_count = 0;
            }
          else
            {
              block[bucket_index] = block[37];
              ptr = block + 2 * block[37];
              memcpy (ptr, buf, len+1);
              ptr[len+1] = (byte)page;
              ptr[len+2] = (byte)(page >> 8);
              block[37] = (2 * block[37] + len + 3 + 1) / 2;
              if (block[37] == 0) block[37] = 0xff;
              return 0;
            }
        }
      else
        {
          ptr = block + 2 * bv;
          if (*ptr == len && compare (ptr+1, buf+1, len) == 0)
            {
              if (buf[len] != '!')
                {
                  strcpy (error, "Symbol multiply defined: ");
                  strcat (error, name);
                  return -1;
                }
              else
                {
                  /* Ignore multiply defined modules. This isn't correct behaviour,
                     but it helps emxomfar being more compatible with ar. */
                  return 0;
                }
            }
        }
      if (bucket_count != 0)
        {
          bucket_index += p->bucket_index_delta;
          if (bucket_index >= 37)
            bucket_index -= 37;
          --bucket_count;
        }
      if (bucket_count == 0)
        {
          block_index += p->block_index_delta;
          if (block_index >= p->dict_blocks)
            block_index -= p->dict_blocks;
          if (block_index == p->block_index)
            return 1;
          bucket_count = 37;
          block = p->dict + 512 * block_index;
        }
    }
}


static int omflib_build_dict (struct omflib *p, char *error)
{
  int i, ret;

  p->dict = realloc (p->dict, p->dict_blocks * 512);
  if (p->dict == NULL)
    {
      errno = ENOMEM;
      return omflib_set_error (error);
    }
  memset (p->dict, 0, p->dict_blocks * 512);
  for (i = 0; i < p->dict_blocks; ++i)
    p->dict[i * 512 + 37] = 38 / 2;
  for (i = 0; i < p->pub_count; ++i)
    {
      ret = omflib_add_dict (p, p->pub_tab[i].name, p->pub_tab[i].page, error);
      if (ret != 0)
        return ret;
    }
  return 0;
}


static unsigned isqrt (unsigned x)
{
  unsigned a, r, e, i;

  a = r = e = 0;
  for (i = 0; i < 32 / 2; ++i)
    {
      r = (r << 2) | (x >> (32-2));
      x <<= 2; a <<= 1;
      e = (a << 1) | 1;
      if (r >= e)
        {
          r -= e;
          a |= 1;
        }
    }
  return a;
}


/* Speed doesn't matter here. */

static int is_prime (unsigned n)
{
  unsigned i, q;

  q = isqrt (n);
  for (i = 3; i <= q; i += 2)
    if (n % i == 0)
      return 0;
  return 1;
}


static unsigned next_prime (unsigned n)
{
  if (n <= 1)
    return 2;
  if (n % 2 == 0)
    --n;
  do
    {
      n += 2;
    } while (!is_prime (n));
  return n;
}


int omflib_finish (struct omflib *p, char *error)
{
  struct lib_header hdr;
  int len, i, blocks;
  unsigned prime;
  long pos;
  struct omf_rec rec;

  if (!p->output)
    return 0;
  len = 0;
  for (i = 0; i < p->pub_count; ++i)
    len += strlen (p->pub_tab[i].name) + 3;
  blocks = (len + 511) / 512;
  blocks += (blocks * 128) / 512;
  ++blocks;
  prime = blocks;
  for (;;)
    {
      prime = next_prime (prime);
      if (prime > 65535)
        {
          strcpy (error, "Too many dictionary blocks");
          return -1;
        }
      p->dict_blocks = prime;
      i = omflib_build_dict (p, error);
      if (i < 0)
        return i;
      if (i == 0)
        break;
    }
  pos = ftell (p->f) + 3;
  rec.rec_type = LIBEND;
  if ((pos & 511) == 0)
    rec.rec_len = 0;
  else
    rec.rec_len = (word)(((pos | 511) + 1) - pos);
  if (fwrite (&rec, sizeof (rec), 1, p->f) != 1)
    return omflib_set_error (error);
  if (omflib_pad (p->f, 512, FALSE, error) != 0)
    return -1;
  hdr.rec_type = LIBHDR;
  hdr.rec_len = p->page_size - 3;
  hdr.dict_offset = ftell (p->f);
  hdr.dict_blocks = p->dict_blocks;
  hdr.flags = (byte)p->flags;
  fseek (p->f, 0, SEEK_SET);
  if (fwrite (&hdr, sizeof (hdr), 1, p->f) != 1)
    return omflib_set_error (error);
  fseek (p->f, hdr.dict_offset, SEEK_SET);
  if (fwrite (p->dict, 512, p->dict_blocks, p->f) != p->dict_blocks)
    return omflib_set_error (error);
  return 0;
}


int omflib_pad (FILE *f, int size, int force, char *error)
{
  long pos;

  pos = ftell (f);
  while ((pos & (size-1)) != 0 || force)
    {
      force = FALSE;
      if (fputc (0, f) != 0)
        return omflib_set_error (error);
      ++pos;
    }
  return 0;
}
