/* omflibwr.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Write an OMFLIB. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omflib0.h"
#include <sys/omflib.h>


int omflib_write_record (struct omflib *p, byte rec_type, word rec_len,
                         const byte *buffer, int chksum, char *error)
{
  struct omf_rec rec;
  byte sum;
  int i, len;
  char name[256];

  rec.rec_type = rec_type;
  rec.rec_len = (chksum ? rec_len + 1 : rec_len);
  if (fwrite (&rec, sizeof (rec), 1, p->f) != 1
      || fwrite (buffer, rec_len, 1, p->f) != 1)
    return omflib_set_error (error);
  if (chksum)
    {
      sum = rec_type + (rec.rec_len & 0xff) + (rec.rec_len >> 8);
      for (i = 0; i < rec_len; ++i)
        sum += buffer[i];
      sum = (byte)(256 - sum);
      if (fputc (sum, p->f) == EOF)
        return omflib_set_error (error);
    }
  switch (rec_type)
    {
    case MODEND:
    case MODEND|REC32:
      if (omflib_pad (p->f, p->page_size, FALSE, error) != 0)
        return -1;
      if (p->state != OS_SIMPLE)
        {
          len = strlen (p->mod_name);
          if (len < 255)
            {
              memcpy (name, p->mod_name, len);
              name[len] = '!';
              name[len+1] = 0;
              if (omflib_add_pub (p, name, p->mod_page, error) != 0)
                return -1;
            }
        }
      break;

    case THEADR:
      p->state = OS_EMPTY;
      break;

    case ALIAS:
      p->state = (p->state == OS_EMPTY ? OS_SIMPLE : OS_OTHER);
      break;

    case COMENT:
      if (p->state == OS_EMPTY && rec_len >= 3
          && buffer[1] == IMPDEF_CLASS && buffer[2] == IMPDEF_SUBTYPE)
        p->state = OS_SIMPLE;
      else
        p->state = OS_OTHER;
      break;

    default:
      p->state = OS_OTHER;
      break;
    }
  return 0;
}


int omflib_write_module (struct omflib *p, const char *name, word *pagep,
                         char *error)
{
  byte buf[256];
  long long_page;
  int len;

  p->state = OS_EMPTY;
  if (omflib_module_name (p->mod_name, name) != 0)
    {
      strcpy (error, "Module name too long");
      return -1;
    }
  long_page = ftell (p->f) / p->page_size;
  if (long_page > 65535)
    {
      strcpy (error, "Library too big -- increase page size");
      return -1;
    }
  *pagep = p->mod_page = (word)long_page;
  len = strlen (name);
  memcpy (buf+1, name, len);
  buf[0] = (byte)len;
  return omflib_write_record (p, THEADR, len + 1, buf, TRUE, error);
}
