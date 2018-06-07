/* omflibpb.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

/* Call a function for all the PUBDEF, IMPDEF, and ALIAS records of an
   OMFLIB. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omflib0.h"
#include <sys/omflib.h>


static int omflib_get_index (struct ptr *p, int *dst, char *error);


int omflib_pubdef_walk (struct omflib *p, word page,
                        int (*walker)(const char *name, char *error),
                        char *error)
{
  struct omf_rec rec;
  byte buf[1024];
  int ret;

  fseek (p->f, page * p->page_size, SEEK_SET);
  do
    {
      if (fread (&rec, sizeof (rec), 1, p->f) != 1)
        goto failure;
      if (rec.rec_len > sizeof (buf))
        {
          strcpy (error, "Record too long");
          return -1;
        }
      if (fread (buf, rec.rec_len, 1, p->f) != 1)
        goto failure;
      if (rec.rec_type == PUBDEF || rec.rec_type == (PUBDEF|REC32))
        {
          ret = omflib_pubdef (&rec, buf, page, walker, error);
          if (ret != 0) return ret;
        }
      else if (rec.rec_type == ALIAS)
        {
          ret = omflib_alias (&rec, buf, page, walker, error);
          if (ret != 0) return ret;
        }
      else if (rec.rec_type == COMENT && rec.rec_len >= 2 &&
               buf[1] == IMPDEF_CLASS && buf[2] == IMPDEF_SUBTYPE)
        {
          ret = omflib_impdef (&rec, buf, page, walker, error);
          if (ret != 0) return ret;
        }
    } while (rec.rec_type != MODEND && rec.rec_type != (MODEND|REC32));
  return 0;

failure:
  if (ferror (p->f))
    return omflib_set_error (error);
  strcpy (error, "Unexpected end of file");
  return -1;
}


int omflib_pubdef (struct omf_rec *rec, byte *buf, word page,
                   int (*walker)(const char *name, char *error),
                   char *error)
{
  int group_index, segment_index, type_index;
  struct ptr ptr;
  int len, i, ret;
  char name[256];

  ptr.ptr = buf;
  ptr.len = rec->rec_len - 1;
  if (omflib_get_index (&ptr, &group_index, error) != 0)
    return -1;
  if (omflib_get_index (&ptr, &segment_index, error) != 0)
    return -1;
  if (segment_index == 0)
    {
      if (ptr.len < 2)
        goto too_short;
      ptr.ptr += 2; ptr.len -= 2;
    }

  while (ptr.len > 0)
    {
      if (ptr.len < 1)
        goto too_short;
      len = ptr.ptr[0];
      ++ptr.ptr; --ptr.len;
      if (ptr.len < len)
        goto too_short;
      memcpy (name, ptr.ptr, len);
      name[len] = 0;
      ptr.ptr += len; ptr.len -= len;
      i = (rec->rec_type == (PUBDEF|REC32) ? 4 : 2);
      if (ptr.len < i) goto too_short;
      ptr.ptr += i; ptr.len -= i;
      if (omflib_get_index (&ptr, &type_index, error) != 0)
        return -1;
      ret = walker (name, error);
      if (ret != 0) return ret;
    }
  if (ptr.len != 0)
    {
      strcpy (error, "Invalid PUBDEF record");
      return -1;
    }
  return 0;

too_short:
  strcpy (error, "PUBDEF record too short");
  return -1;
}


int omflib_impdef (struct omf_rec *rec, byte *buf, word page,
                   int (*walker)(const char *name, char *error),
                   char *error)
{
  int len;
  char name[256];

  if (rec->rec_len < 5) goto too_short;
  len = buf[4];
  if (len + 5 > rec->rec_len) goto too_short;
  memcpy (name, buf+5, len);
  name[len] = 0;
  return walker (name, error);

too_short:
  strcpy (error, "IMPDEF record too short");
  return -1;
}


int omflib_alias (struct omf_rec *rec, byte *buf, word page,
                  int (*walker)(const char *name, char *error),
                  char *error)
{
  int len;
  char name[256];

  if (rec->rec_len < 1) goto too_short;
  len = buf[0];
  if (len + 2 > rec->rec_len) goto too_short;
  memcpy (name, buf+1, len);
  name[len] = 0;
  return walker (name, error);

too_short:
  strcpy (error, "ALIAS record too short");
  return -1;
}


static int omflib_get_index (struct ptr *p, int *dst, char *error)
{
  if (p->len < 1)
    goto too_short;
  if (p->ptr[0] <= 0x7f)
    {
      *dst = p->ptr[0];
      ++p->ptr;
      --p->len;
      return 0;
    }
  else if (p->len < 2)
    goto too_short;
  else
    {
      *dst = ((p->ptr[0] & 0x7f) << 8) | p->ptr[1];
      p->ptr += 2;
      p->len -= 2;
      return 0;
    }

too_short:
  strcpy (error, "Record too short");
  return -1;
}
