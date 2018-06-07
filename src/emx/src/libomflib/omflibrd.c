/* omflibrd.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Read an OMFLIB. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "omflib0.h"
#include <sys/omflib.h>


struct omflib *omflib_open (const char *fname, char *error)
{
  FILE *f;
  struct omflib *p;
  struct lib_header hdr;

  f = fopen (fname, "rb");
  if (f == NULL)
    goto failure;
  if (fread (&hdr, sizeof (hdr), 1, f) != 1)
    {
      if (ferror (f))
        goto failure;
      strcpy (error, "Invalid library header");
      fclose (f);
      return NULL;
    }
  if (hdr.rec_type != LIBHDR || hdr.rec_len < 7)
    {
      strcpy (error, "Invalid library header");
      fclose (f);
      return NULL;
    }
  p = malloc (sizeof (struct omflib));
  if (p == NULL)
    {
      errno = ENOMEM;
      goto failure;
    }
  p->f = f;
  p->page_size = hdr.rec_len + 3;
  p->dict_offset = hdr.dict_offset;
  p->dict_blocks = hdr.dict_blocks;
  p->flags = hdr.flags;
  p->mod_tab = NULL;
  p->mod_alloc = 0;
  p->mod_count = -1;
  p->dict = NULL;
  p->pub_tab = NULL;
  p->pub_alloc = 0;
  p->pub_count = 0;
  p->output = FALSE;
  p->state = OS_EMPTY;
  p->mod_page = 0;
  p->mod_name[0] = 0;
  return p;

failure:
  strcpy (error, strerror (errno));
  if (f != NULL)
    fclose (f);
  return NULL;
}


int omflib_read_dictionary (struct omflib *p, char *error)
{
  if (p->output)
    {
      strcpy (error, "Not implemented for output library");
      return -1;
    }
  p->dict = malloc (512 * p->dict_blocks);
  if (p->dict == NULL)
    {
      errno = ENOMEM;
      return omflib_set_error (error);
    }
  fseek (p->f, p->dict_offset, SEEK_SET);
  if (fread (p->dict, 512, p->dict_blocks, p->f) != p->dict_blocks)
    {
      if (ferror (p->f))
        strcpy (error, strerror (errno));
      else
        strcpy (error, "Dictionary truncated");
      free (p->dict);
      p->dict = NULL;
      return -1;
    }
  return 0;
}


static int mod_compare (const void *x1, const void *x2)
{
  word page1, page2;

  page1 = ((const struct omfmod *)x1)->page;
  page2 = ((const struct omfmod *)x2)->page;
  if (page1 < page2)
    return -1;
  else if (page1 > page2)
    return 1;
  else
    return 0;
}


int omflib_make_mod_tab (struct omflib *p, char *error)
{
  int block, bucket, bv, len;
  char string[256];
  struct omfmod *mod;
  byte *d, *s;

  if (p->dict == NULL && omflib_read_dictionary (p, error) != 0)
    return -1;
  p->mod_count = 0;
  d = p->dict;
  for (block = 0; block < p->dict_blocks; ++block, d += 512)
    for (bucket = 0; bucket < 37; ++bucket)
      {
        bv = d[bucket];
        if (bv != 0)
          {
            s = d + bv * 2;
            len = *s;
            if (s[len] == '!')
              {
                memcpy (string, s+1, len-1);
                string[len-1] = 0;
                if (p->mod_count >= p->mod_alloc)
                  {
                    p->mod_alloc += 16;
                    p->mod_tab = realloc (p->mod_tab, p->mod_alloc
                                          * sizeof (struct omfmod));
                    if (p->mod_tab == NULL)
                      {
                        p->mod_count = -1;
                        p->mod_alloc = 0;
                        errno = ENOMEM;
                        return omflib_set_error (error);
                      }
                  }
                mod = &p->mod_tab[p->mod_count];
                mod->page = s[len+1] | (s[len+2] << 8);
                mod->name = strdup (string);
                mod->flags = 0;
                if (mod->name == NULL)
                  {
                    errno = ENOMEM;
                    return omflib_set_error (error);
                  }
                ++p->mod_count;
              }
          }
      }
  qsort (p->mod_tab, p->mod_count, sizeof (p->mod_tab[0]), mod_compare);
  return 0;
}


int omflib_module_count (struct omflib *p, char *error)
{
  if (p->mod_count == -1 && omflib_make_mod_tab (p, error) != 0)
    return -1;
  return p->mod_count;
}


int omflib_module_info (struct omflib *p, int n, char *name, int *page,
                        char *error)
{
  if (p->mod_count == -1 && omflib_make_mod_tab (p, error) != 0)
      return -1;
  if (n < 0 || n >= p->mod_count)
    {
      strcpy (error, "Module number out of range");
      return -1;
    }
  strcpy (name, p->mod_tab[n].name);
  *page = p->mod_tab[n].page;
  return 0;
}


int omflib_find_symbol (struct omflib *p, const char *name, char *error)
{
  int block_index, bucket_index;
  int bv, len, bucket_count;
  byte *ptr, *block, buf[257];
  int (*compare)(const void *s1, const void *s2, size_t n);

  if (p->dict == NULL && omflib_read_dictionary (p, error) != 0)
    return -1;
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
  bucket_count = 37;
  compare = (p->flags & 1 ? memcmp : memicmp);
  for (;;)
    {
      block = p->dict + 512 * block_index;
      bv = block[bucket_index];
      if (bv == 0)
        {
          if (block[37] != 0xff)
            return 0;
          bucket_count = 0;     /* Keep bucket_index! */
        }
      else
        {
          ptr = block + 2 * bv;
          if (*ptr == len && compare (ptr+1, buf+1, len) == 0)
            return ptr[len+1] + (ptr[len+2] << 8);
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
            return 0;
          bucket_count = 37;
        }
    }
}


int omflib_find_module (struct omflib *p, const char *name, char *error)
{
  char buf[256+1];

  if (omflib_module_name (buf, name) != 0)
    {
      strcpy (error, "Module name too long");
      return -1;
    }
  strcat (buf, "!");
  return omflib_find_symbol (p, buf, error);
}


long omflib_page_pos (struct omflib *p, int page)
{
  return page * p->page_size;
}
