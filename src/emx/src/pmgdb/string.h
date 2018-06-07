/* string.h -*- C++ -*-
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


class bstring
{
public:
  bstring () { str = NULL; }
  virtual ~bstring () { delete[] str; }
  char * modify () { return str; }
  virtual void set (const char *s, int len) = 0;
  virtual void set (int len) = 0;
  virtual void set_null () = 0;
  bool is_null () const { return str == NULL; }
  int length () const { return strlen (str); }
  void set (const char *s) { set (s, strlen (s)); }
  operator const char * () const { return str; }
  operator const unsigned char * () const { return (unsigned char *)str; }
  const char operator [] (int index) const { return str[index]; }
  bool operator == (const bstring &b) const;
  bool operator != (const bstring &b) const { return !operator== (b); }

protected:
  char *str;
};

// A vstring is expected to be modified quite often

class vstring : public bstring
{
public:
  vstring () { alloc = 0; }
  ~vstring () {}
  void set (const char *s) { bstring::set (s); }
  void set (const char *s, int len);
  void set (int len);
  void set_null ();
  vstring &operator = (const char *s) { set (s); return *this; }
  vstring &operator = (const vstring &src);

protected:
  int alloc;
};

// An fstring is expected to be set only once

class fstring : public bstring
{
public:
  fstring () {}
  ~fstring () {}
  void set (const char *s) { bstring::set (s); }
  void set (const char *s, int len);
  void set (int len);
  void set_null ();
  fstring &operator = (const char *s) { set (s); return *this; }
  fstring &operator = (const fstring &src);
};


// An astring is expected to be appended to

class astring : public vstring
{
private:
  typedef vstring parent;
public:
  astring () { cur_len = 0; }
  ~astring () {}
  void set (const char *s) { bstring::set (s); }
  void set (const char *s, int len) { parent::set (s, len); cur_len = len; }
  void set (int len) { parent::set (len); if (len < cur_len) cur_len = len; }
  void set_null () { parent::set_null (); cur_len = 0; }
  void append (const char *s, int len);
  void append (const char *s) { append (s, strlen (s)); }
  vstring &operator = (const char *s) { set (s); return *this; }
  astring &operator = (const astring &src);
  int length () const { return cur_len; }
  void remove_trailing (char c);

private:
  int cur_len;
};


// String consisting of lines

class lstring
{
public:
  lstring () { str = NULL; vec = NULL; n = 0; }
  lstring (const char *s);
  lstring (const bstring &s);
  lstring (const lstring &s);
  ~lstring ();
  lstring &operator = (const lstring &s);
  void set (const char *s, size_t len);
  void set (const char *s) { set (s, strlen (s)); }
  int get_lines () const { return n; }
  const char *operator [] (int index) const
    { return (index >= 0 && index < n) ? vec[index] : NULL; }
  void sort (int (*compare) (const char *, const char *));
  void sort ();

private:
  void assign (const lstring &s);
  char *str;
  const char **vec;
  int n;
  size_t str_len;
};
