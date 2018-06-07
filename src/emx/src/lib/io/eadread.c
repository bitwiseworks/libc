/* eadread.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_FSMACROS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <sys/ead.h>
#include <emx/umalloc.h>
#include "ea.h"

struct gea_data
{
  int gea_alloc;
  int gea_used;
  int total_name_len;
  int total_value_size;
  int fea_size;
  int count;
  ULONG *patch;
  PGEA2LIST gea_ptr;
};


static int _ead_make_gea (struct _ead_data *ead, PDENA2 pdena, void *arg)
{
  ULONG size;
  PGEA2 pgea;
  struct gea_data *p;

  p = arg;
  size = _EA_ALIGN (sizeof (GEA2) + pdena->cbName);
  if (p->gea_used + size > p->gea_alloc)
    {
      p->gea_alloc += 512;             /* increment must be > size */
      p->gea_ptr = realloc (p->gea_ptr, p->gea_alloc);
      if (p->gea_ptr == NULL)
        {
          errno = ENOMEM;
          return -1;
        }
    }
  pgea = (PGEA2)((char *)p->gea_ptr + p->gea_used);
  pgea->oNextEntryOffset = size;
  pgea->cbName = pdena->cbName;
  if (pdena->szName[pdena->cbName] != 0)
    abort ();
  memcpy (pgea->szName, pdena->szName, pdena->cbName + 1);
  p->patch = &pgea->oNextEntryOffset;
  p->gea_used += size;
  size = _EA_SIZE2 (pdena);
  p->fea_size += size;
  p->total_name_len += pdena->cbName;
  p->total_value_size += pdena->cbValue;
  ++p->count;
  return 0;
}


int _ead_read (_ead ead, const char *path, int handle, int flags)
{
  ULONG rc;
  struct gea_data gd;
  EAOP2 eaop;
  FS_VAR();

  _ead_clear (ead);
  gd.gea_alloc = sizeof (GEA2LIST);
  gd.gea_ptr = malloc (gd.gea_alloc);
  if (gd.gea_ptr == NULL)
    {
      errno = ENOMEM;
      return -1;
    }
  gd.gea_used = sizeof (eaop.fpGEA2List->cbList);
  gd.fea_size = sizeof (eaop.fpFEA2List->cbList);
  gd.total_name_len = 0;
  gd.total_value_size = 0;
  gd.count = 0;
  gd.patch = NULL;
  if (_ead_enum (ead, path, handle, _ead_make_gea, &gd) < 0)
    {
      if (gd.gea_ptr != NULL)
        free (gd.gea_ptr);
      return -1;
    }
  if (gd.count > 0)
    {
      *gd.patch = 0;
      eaop.fpGEA2List = gd.gea_ptr; gd.gea_ptr = NULL;
      eaop.fpGEA2List->cbList = gd.gea_used;
      if (_ead_size_buffer (ead, gd.fea_size) < 0)
        {
          free (eaop.fpGEA2List);
          return -1;
        }
      ead->buffer->cbList = gd.fea_size;
      eaop.fpFEA2List = ead->buffer;
      eaop.oError = 0;
      FS_SAVE_LOAD();
      if (path == NULL)
        rc = DosQueryFileInfo (handle, FIL_QUERYEASFROMLIST, &eaop,
                               sizeof (eaop));
      else
        rc = DosQueryPathInfo (path, FIL_QUERYEASFROMLIST, &eaop,
                               sizeof (eaop));
      FS_RESTORE();
      if (rc != 0)
        {
          free (eaop.fpGEA2List);
          _ea_set_errno (rc);
          return -1;
        }
      free (eaop.fpGEA2List); eaop.fpGEA2List = NULL;
    }
  else if (gd.gea_ptr != NULL)
    {
      free (gd.gea_ptr); gd.gea_ptr = NULL;
    }
  if (gd.count != 0 && _ead_make_index (ead, gd.count) < 0)
    {
      errno = ENOMEM;
      return -1;
    }
  ead->count = gd.count;
  ead->total_value_size = gd.total_value_size;
  ead->total_name_len = gd.total_name_len;
  return 0;
}


int _ead_enum (struct _ead_data *ead, const char *path, int handle,
               int (*function)(struct _ead_data *ead, PDENA2 pdena, void *arg),
               void *arg)
{
  void *dena_buf;
  const void *fileref;
  ULONG dena_buf_size, index, count, rc, reftype, hf, i;
  PDENA2 pdena;
  int expand_dena_buf;
  FS_VAR();

  if (path != NULL)
    {
      reftype = ENUMEA_REFTYPE_PATH;
      i = strlen(path) + 1;
      fileref = alloca(i);
      memcpy((void*)fileref, path, i);
    }
  else
    {
      hf = handle;
      reftype = ENUMEA_REFTYPE_FHANDLE;
      fileref = &hf;
    }
  dena_buf_size = 0; dena_buf = NULL;
  expand_dena_buf = 1; index = 1;
  for (;;)
    {
      if (expand_dena_buf)
        {
          dena_buf_size += 0x20000; /* DosEnumAttribute is broken */
          dena_buf = _lrealloc (dena_buf, dena_buf_size);
          if (dena_buf == NULL)
            {
              errno = ENOMEM;
              return -1;
            }
        }
      count = -1;
      FS_SAVE_LOAD();
      rc = DosEnumAttribute (reftype, fileref, index,
                             dena_buf, dena_buf_size, &count,
                             ENUMEA_LEVEL_NO_VALUE);
      FS_RESTORE();
      if (rc == ERROR_BUFFER_OVERFLOW)
        expand_dena_buf = 1;
      else if (rc != 0)
        {
          free (dena_buf);
          _ea_set_errno (rc);
          return -1;
        }
      else if (count == 0)
        break;
      else
        {
          expand_dena_buf = 0; pdena = dena_buf;
          for (i = 0; i < count; ++i)
            {
              if (function (ead, pdena, arg) < 0)
                {
                  free (dena_buf);
                  return -1;
                }
              pdena = (PDENA2)((char *)pdena + pdena->oNextEntryOffset);
            }
          index += count;
        }
    }
  free (dena_buf);
  return 0;
}
