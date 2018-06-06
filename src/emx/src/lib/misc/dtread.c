/* dtread.c (emx+gcc) -- Copyright (c) 1987-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/dirent.h>
#include <sys/dirtree.h>
#include <emx/io.h>
#include <emx/time.h>
#include <emx/syscalls.h>


static void _dt_free_recurse (struct _dt_node *dn)
{
  struct _dt_node *next;

  while (dn != NULL)
    {
      next = dn->next;
      if (dn->name != NULL)
        free (dn->name);
      if (dn->sub != NULL)
        _dt_free_recurse (dn->sub);
      free (dn);
      dn = next;
    }
}


void _dt_free (struct _dt_tree *dt)
{
  if (dt->strings != NULL)
    {
      free (dt->strings);
      dt->strings = NULL;
    }
  _dt_free_recurse (dt->tree);
  free (dt);
}


struct _dt_rdata
{
  struct _find find;
  char path[MAXPATHLEN+1];
  const char *dir;
  const char *mask;
  unsigned flags;
  int attr, dir_pass;
};


static int _dt_match (struct _dt_rdata *dp)
{
  if ((dp->flags & _DT_NOCPDIR) && (strcmp (dp->find.szName, ".") == 0 ||
                                    strcmp (dp->find.szName, "..") == 0))
    return 0;
  return 1;
}


static struct _dt_node *_dt_add (struct _dt_rdata *dp)
{
  struct _dt_node *pnew;
  struct tm tm;

  pnew = malloc (sizeof (*pnew));
  if (pnew == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }
  pnew->next = NULL;
  pnew->sub = NULL;
  pnew->name = strdup (dp->find.szName);
  if (pnew->name == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }
  _fnlwr2 (pnew->name, dp->dir);
  pnew->size = dp->find.cbFile;
  pnew->user = 0;
  pnew->attr = dp->find.attr;
  tm.tm_sec = (dp->find.time & 0x1f) * 2;
  tm.tm_min = (dp->find.time >> 5) & 0x3f;
  tm.tm_hour = (dp->find.time >> 11) & 0x1f;
  tm.tm_mday = dp->find.date & 0x1f;
  tm.tm_mon = ((dp->find.date >> 5) & 0x0f) - 1;
  tm.tm_year = ((dp->find.date >> 9) & 0x7f) + 1980 - 1900;
  tm.tm_isdst = -1;             /* unknown */
  pnew->mtime = _mktime (&tm);
  _loc2gmt (&pnew->mtime, -1);
  return pnew;
}


static int _dt_read_recurse (struct _dt_rdata *dp, struct _dt_node **dst,
                             int path_len)
{
  struct _dt_node *pnew, **add;
  int r, len2;

  *dst = NULL;
  add = dst;
  strcpy (dp->path + path_len, dp->mask);
  r = __findfirst (dp->path, dp->attr, &dp->find);
  while (r == 0)
    {
      if (!(dp->dir_pass && (dp->find.attr & A_DIR)) && _dt_match (dp))
        {
          pnew = _dt_add (dp);
          if (pnew == NULL)
            return -1;
          *add = pnew;
          add = &pnew->next;
        }
      r = __findnext (&dp->find);
    }
  if (errno != ENOENT)
    return -1;
  if (dp->dir_pass)
    {
      strcpy (dp->path + path_len, "*.*");
      r = __findfirst (dp->path, A_DIR|A_HIDDEN|A_SYSTEM, &dp->find);
      while (r == 0)
        {
          if ((dp->find.attr & A_DIR) && _dt_match (dp))
            {
              pnew = _dt_add (dp);
              if (pnew == NULL)
                return -1;
              *add = pnew;
              add = &pnew->next;
            }
          r = __findnext (&dp->find);
        }
      if (errno != ENOENT)
        return -1;
    }
  if (dp->flags & _DT_TREE)
    for (pnew = *dst; pnew != NULL; pnew = pnew->next)
      if ((pnew->attr & A_DIR) && strcmp (pnew->name, ".") != 0
          && strcmp (pnew->name, "..") != 0)
        {
          len2 = strlen (pnew->name);
          strcpy (dp->path + path_len, pnew->name);
          dp->path[path_len+len2] = '/';
          if (_dt_read_recurse (dp, &pnew->sub, path_len+len2+1) < 0)
            return -1;
        }
  return 0;
}


struct _dt_tree *_dt_read (const char *dir, const char *mask,
                           unsigned flags)
{
  struct _dt_tree *p;
  struct _dt_rdata data;
  int len, saved_errno;

  if (!_tzset_flag) tzset ();
  data.dir = dir;
  data.mask = mask;
  data.flags = flags;
  data.dir_pass = 1;
  if (strcmp (mask, "*.*") == 0
      || (strcmp (mask, "*") == 0))
    data.dir_pass = 0;
  data.attr = A_HIDDEN | A_SYSTEM;
  if (!data.dir_pass)
    data.attr |= A_DIR;
  len = strlen (dir);
  if (len >= sizeof (data.path))
    {
      errno = ENAMETOOLONG;
      return NULL;
    }
  memcpy (data.path, dir, len);
  if (len > 0 && !_trslash (data.path, len, 0))
    data.path[len++] = '/';
  p = malloc (sizeof (*p));
  if (p == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }
  p->strings = NULL;
  saved_errno = errno;
  if (_dt_read_recurse (&data, &p->tree, len) != 0)
    {
      _dt_free (p);
      return NULL;
    }
  errno = saved_errno;
  return p;
}
