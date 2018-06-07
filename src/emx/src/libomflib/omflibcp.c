/* omflibcp.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Copy a module from one OMFLIB to another OMFLIB. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "omflib0.h"
#include <sys/omflib.h>


static struct omflib *pubdef_dst_lib;
static word pubdef_page;


static int add_pubdef (const char *name, char *error);


int omflib_copy_module (struct omflib *dst_lib, FILE *dst_file,
                        struct omflib *src_lib, FILE *src_file,
                        const char *mod_name, char *error)
{
  struct omf_rec rec;
  word page;
  long long_page, pos;
  char theadr_name[256];
  char libmod_name[256];
  char caller_name[256];
  byte buf[1024], buf2[256+6];
  enum omf_state state;
  enum {RT_THEADR, RT_OTHER} cur_rt, prev_rt;
  int copy;

  theadr_name[0] = 0; libmod_name[0] = 0;
  if (mod_name == NULL)
    caller_name[0] = 0;
  else
    omflib_module_name (caller_name, mod_name);
  if (dst_lib != NULL)
    {
      long_page = ftell (dst_file) / dst_lib->page_size;
      if (long_page > 65535)
        {
          strcpy (error, "Library too big -- increase page size");
          return -1;
        }
      page = (word)long_page;
    }
  else
    page = 0;
  state = OS_EMPTY; prev_rt = RT_OTHER;
  do
    {
      if (fread (&rec, sizeof (rec), 1, src_file) != 1)
        goto failure_src;
      if (rec.rec_len > sizeof (buf))
        {
          strcpy (error, "Record too long");
          return -1;
        }
      if (fread (buf, rec.rec_len, 1, src_file) != 1)
        goto failure_src;
      copy = TRUE; cur_rt = RT_OTHER;
      switch (rec.rec_type)
        {
        case THEADR:
          if (rec.rec_len == 0 || buf[0] + 1 > rec.rec_len)
            {
              strcpy (error, "Truncated THEADR record");
              return -1;
            }
          buf[1+buf[0]] = 0;
          omflib_module_name (theadr_name, buf + 1);
          state = OS_EMPTY; cur_rt = RT_THEADR;
          break;

        case PUBDEF:
        case PUBDEF|REC32:
          if (dst_lib != NULL)
            {
              pubdef_dst_lib = dst_lib;
              pubdef_page = page;
              if (omflib_pubdef (&rec, buf, page, add_pubdef, error) != 0)
                return -1;
            }
          state = OS_OTHER;
          break;

        case ALIAS:
          if (dst_lib != NULL)
            {
              pubdef_dst_lib = dst_lib;
              pubdef_page = page;
              if (omflib_alias (&rec, buf, page, add_pubdef, error) != 0)
                return -1;
            }
          state = (state == OS_EMPTY ? OS_SIMPLE : OS_OTHER);
          break;

        case COMENT:
          if (rec.rec_len >= 3 && buf[1] == IMPDEF_CLASS
              && buf[2] == IMPDEF_SUBTYPE)
            {
              if (dst_lib != NULL)
                {
                  pubdef_dst_lib = dst_lib;
                  pubdef_page = page;
                  if (omflib_impdef (&rec, buf, page, add_pubdef, error) != 0)
                    return -1;
                }
              state = (state == OS_EMPTY ? OS_SIMPLE : OS_OTHER);
            }
          else if (rec.rec_len >= 2 && buf[1] == LIBMOD_CLASS)
            {
              if (dst_lib == NULL || mod_name != NULL)
                copy = FALSE;
              if (rec.rec_len < 3 || buf[2] + 3 > rec.rec_len)
                {
                  strcpy (error, "Truncated LIBMOD record");
                  return -1;
                }
              buf[3+buf[2]] = 0;
              omflib_module_name (libmod_name, buf + 3);
              state = OS_OTHER;
            }
          else
            state = OS_OTHER;
          break;

        case MODEND:
        case MODEND|REC32:
          /* Don't change STATE. */
          break;

        default:
          state = OS_OTHER;
        }

      if (prev_rt == RT_THEADR && dst_lib != NULL && mod_name != NULL)
        {
          if (state == OS_SIMPLE)
            cur_rt = RT_THEADR; /* Check again after next record */
          else if (strcmp (caller_name, theadr_name) != 0)
            {
              buf2[0] = 0;
              buf2[1] = LIBMOD_CLASS;
              buf2[2] = strlen (caller_name);
              memcpy (buf2 + 3, caller_name, buf2[2]);
              if (omflib_write_record (dst_lib, COMENT, buf2[2] + 3, buf2,
                                       TRUE, error) != 0)
                return -1;
            }
        }
      if (copy && dst_file != NULL)
        {
          if (fwrite (&rec, sizeof (rec), 1, dst_file) != 1
              || fwrite (buf, rec.rec_len, 1, dst_file) != 1)
            return omflib_set_error (error);
        }
      prev_rt = cur_rt;
    } while (rec.rec_type != MODEND && rec.rec_type != (MODEND|REC32));
  if (src_lib != NULL)
    {
      pos = ftell (src_file);
      if (pos % src_lib->page_size != 0)
        fseek (src_file, src_lib->page_size - pos % src_lib->page_size,
               SEEK_CUR);
    }
  if (dst_lib != NULL)
    {
      /* Don't use PUBDEF for the module name for modules that contain
         only an alias or an import definition. */

      if (state != OS_SIMPLE)
        {
          if (caller_name[0] != 0)
            strcpy (buf2, caller_name);
          else if (libmod_name[0] != 0)
            strcpy (buf2, libmod_name);
          else
            strcpy (buf2, theadr_name);
          strcat (buf2, "!");
          if (omflib_add_pub (dst_lib, buf2, page, error) != 0)
            return -1;
        }
      if (dst_file != NULL
          && omflib_pad (dst_file, dst_lib->page_size, FALSE, error) != 0)
        return -1;
    }
  return 0;
  
failure_src:
  if (ferror (src_file))
    return omflib_set_error (error);
  strcpy (error, "Unexpected end of file");
  return -1;
}


static int add_pubdef (const char *name, char *error)
{
  return omflib_add_pub (pubdef_dst_lib, name, pubdef_page, error);
}
