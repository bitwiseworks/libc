/* dtsort.c (emx+gcc) -- Copyright (c) 1987-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/dirent.h>
#include <sys/dirtree.h>


static struct
{
  struct _dt_node **buf;
  int buf_alloc;
  const char *spec;
} _dt_sort_data;


static int _dt_compare_size (const struct _dt_node *n1,
                             const struct _dt_node *n2)
{
  if (n1->size < n2->size)
    return -1;
  else if (n1->size > n2->size)
    return 1;
  else
    return 0;
}



static int _dt_compare_time (const struct _dt_node *n1,
                             const struct _dt_node *n2)
{
  if (n1->mtime < n2->mtime)
    return -1;
  else if (n1->mtime > n2->mtime)
    return 1;
  else
    return 0;
}


static int _dt_compare_file (const struct _dt_node *n1,
                             const struct _dt_node *n2)
{
  int a1, a2;

  a1 = n1->attr & A_DIR;
  a2 = n2->attr & A_DIR;
  if (a1 < a2)
    return 1;
  else if (a1 > a2)
    return -1;
  else
    return 0;
}


static int _dt_compare_ext (const struct _dt_node *n1,
                            const struct _dt_node *n2)
{
  const char *ext1, *ext2;
  int d;

  ext1 = _getext (n1->name);
  ext2 = _getext (n2->name);
  if (ext1 == NULL && ext2 == NULL)
    return 0;
  if (ext1 == NULL && ext2 != NULL)
    return -1;
  if (ext2 == NULL && ext1 != NULL)
    return 1;
  d = strcasecmp (ext1, ext2);
  if (d == 0)
    d = strcmp (ext1, ext2);
  return d;
}


static int _dt_compare_name (const struct _dt_node *n1,
                             const struct _dt_node *n2)
{
  int d;

  d = strcasecmp (n1->name, n2->name);
  if (d == 0)
    d = strcmp (n1->name, n2->name);
  return d;
}


static int _dt_sort_compare (const void *x1, const void *x2)
{
  const struct _dt_node *n1, *n2;
  const char *p;
  int result;

  n1 = *(const struct _dt_node **)x1;
  n2 = *(const struct _dt_node **)x2;
  for (p = _dt_sort_data.spec; *p != 0; ++p)
    {
      switch (*p)
        {
        case 'n':
          result = _dt_compare_name (n1, n2);
          break;
        case 'N':
          result = _dt_compare_name (n2, n1);
          break;
        case 'e':
          result = _dt_compare_ext (n1, n2);
          break;
        case 'E':
          result = _dt_compare_ext (n2, n1);
          break;
        case 't':
          result = _dt_compare_time (n1, n2);
          break;
        case 'T':
          result = _dt_compare_time (n2, n1);
          break;
        case 's':
          result = _dt_compare_size (n1, n2);
          break;
        case 'S':
          result = _dt_compare_size (n2, n1);
          break;
        case 'f':
          result = _dt_compare_file (n1, n2);
          break;
        case 'F':
          result = _dt_compare_file (n2, n1);
          break;
        default:
          result = 0;
          break;
        }
      if (result != 0)
        return result;
    }
  return 0;
}


static void _dt_sort_recurse (struct _dt_node **tree)
{
  int i, n;
  struct _dt_node *p, **add;

  n = 0;
  for (p = *tree; p != NULL; p = p->next)
    ++n;
  if (n > _dt_sort_data.buf_alloc)
    {
      _dt_sort_data.buf_alloc = n;
      _dt_sort_data.buf = realloc (_dt_sort_data.buf,
                                   n * sizeof (*_dt_sort_data.buf));
      if (_dt_sort_data.buf == NULL)
        return;
    }
  if (n >= 2)
    {
      n = 0;
      for (p = *tree; p != NULL; p = p->next)
        _dt_sort_data.buf[n++] = p;
      qsort (_dt_sort_data.buf, n, sizeof (*_dt_sort_data.buf),
             _dt_sort_compare);
      add = tree; *tree = NULL;
      for (i = 0; i < n; ++i)
        {
          p = _dt_sort_data.buf[i];
          p->next = NULL;
          *add = p;
          add = &p->next;
        }
    }
  for (p = *tree; p != NULL; p = p->next)
    if (p->sub != NULL)
      _dt_sort_recurse (&p->sub);
}


void _dt_sort (struct _dt_tree *dt, const char *spec)
{
  _dt_sort_data.buf = NULL;
  _dt_sort_data.buf_alloc = 0;
  _dt_sort_data.spec = spec;
  _dt_sort_recurse (&dt->tree);
  if (_dt_sort_data.buf != NULL)
    free (_dt_sort_data.buf);
}
