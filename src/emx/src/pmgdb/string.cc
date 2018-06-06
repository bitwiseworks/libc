/* string.cc
   Copyright (c) 1996 Eberhard Mattes

This file is part of pmgdb.

pmgdb is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

pmgdb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pmgdb; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdlib.h>
#include <string.h>
#include "string.h"


bool bstring::operator == (const bstring &b) const
{
  if (str == NULL && b.str == NULL)
    return true;
  if (str == NULL || b.str == NULL)
    return false;
  return strcmp (str, b.str) == 0;
}


void vstring::set (const char *s, int len)
{
  if (len >= alloc)
    set (len);
  memcpy (str, s, len);
  str[len] = 0;
}


void vstring::set (int len)
{
  if (len >= alloc)
    {
      alloc = len + 1;
      alloc = ((alloc + 31) / 32) * 32;
      char *temp = new char[alloc];
      if (str != NULL)
        {
          strcpy (temp, str);
          delete[] str;
        }
      else
        *temp = 0;
      str = temp;
    }
  str[len] = 0;
}


void vstring::set_null ()
{
  delete[] str;
  str = NULL;
  alloc = 0;
}


vstring &vstring::operator = (const vstring &src)
{
  if (src.str == NULL)
    set_null ();
  else
    set (src.str);
  return *this;
}


void fstring::set (const char *s, int len)
{
  delete[] str;
  str = new char[len + 1];
  memcpy (str, s, len);
  str[len] = 0;
}


// Note: Does not keep the contents of the string

void fstring::set (int len)
{
  delete[] str;
  str = new char[len + 1];
  *str = 0;
}


void fstring::set_null ()
{
  delete[] str;
  str = NULL;
}


fstring &fstring::operator = (const fstring &src)
{
  if (src.str == NULL)
    set_null ();
  else
    set (src.str);
  return *this;
}


void astring::append (const char *s, int len)
{
  if (cur_len + len  >= alloc)
    set (cur_len + len);
  memcpy (str + cur_len, s, len);
  cur_len += len;
  str[cur_len] = 0;
}


astring &astring::operator = (const astring &src)
{
  if (src.str == NULL)
    set_null ();
  else
    {
      // TODO: no need to copy old string
      parent::set (src.str);
      cur_len = src.cur_len;
    }
  return *this;
}


void astring::remove_trailing (char c)
{
  while (cur_len > 0 && str[cur_len-1] == c)
    --cur_len;
  str[cur_len] = 0;
}


lstring::lstring (const char *s)
{
  str = NULL; vec = NULL;
  set (s);
}


lstring::lstring (const bstring &s)
{
  str = NULL; vec = NULL;
  set ((const char *)s);
}


lstring::lstring (const lstring &s)
{
  str = NULL; vec = NULL;
  assign (s);
}


lstring::~lstring ()
{
  delete[] str;
  delete[] vec;
}


lstring &lstring::operator = (const lstring &s)
{
  assign (s);
  return *this;
}


void lstring::assign (const lstring &s)
{
  delete[] str;
  delete[] vec;
  str_len = s.str_len;
  str = new char[str_len+1];
  memcpy (str, s.str, str_len+1);
  n = s.n;
  vec = new const char *[n];
  const char *p = str;
  for (int i = 0; i < n; ++i)
    {
      vec[i] = p;
      p += strlen (p) + 1;
    }
}


void lstring::set (const char *s, size_t len)
{
  delete[] str;
  delete[] vec;
  str_len = len;
  str = new char[len+1];
  memcpy (str, s, len+1);
  n = _memcount (str, '\n', len);
  if (len != 0 && str[len-1] != '\n')
    ++n;
  vec = new const char *[n];
  char *p = str;
  int i = 0;
  while (len != 0)
    {
      vec[i++] = p;
      char *end = (char *)memchr (p, '\n', len);
      if (end == NULL)
        break;
      *end = 0;
      len -= (end + 1 - p);
      p = end + 1;
    }
  if (i != n)
    abort ();
}


static int (*lstring_compare_strings)(const char *, const char *);


static int default_compare (const char *s1, const char *s2)
{
  return strcmp (s1, s2);
}


extern "C"
{
  static int lstring_compare (const void *p1, const void *p2)
    {
      return lstring_compare_strings (*(const char **)p1, *(const char **)p2);
    }
}

void lstring::sort ()
{
  sort (default_compare);
}


// This is not thread-safe!

void lstring::sort (int (*compare)(const char *, const char *))
{
  lstring_compare_strings = compare;
  qsort (vec, n, sizeof (*vec), lstring_compare);
}
