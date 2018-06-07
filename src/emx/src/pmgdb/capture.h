/* capture.h -*- C++ -*-
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


class capture_string
{
public:
  capture_string () { str = NULL; }
  ~capture_string () { delete[] str; }
  void set (const char *s, size_t len);
  void set (const char *s) { set (s, strlen (s)); }
  bool is_set () const { return str != NULL; }
  const char *get () const { return str; }
private:
  char *str;
};

class capture_number
{
public:
  capture_number () { flag = false; }
  ~capture_number () {}
  void set (long v) { value = v; flag = true; }
  bool is_set () const { return flag; }
  long get () const { return value; }
private:
  long value;
  bool flag;
};

class capture_bool
{
public:
  capture_bool () { flag = false; }
  ~capture_bool () {}
  void set (bool v) { value = v; flag = true; }
  bool is_set () const { return flag; }
  bool get () const { return value; }
private:
  // TODO: Use three-value enum?
  bool value;
  bool flag;
};

class capture
{
public:
  capture () { next = NULL; }
  ~capture () {}

  class capture *next;          // TODO: make private, add iterator
  capture_string source_file;
  capture_number source_lineno;
  capture_number source_addr;
  capture_number bpt_number;
  capture_string bpt_disposition;
  capture_bool   bpt_enable;
  capture_number bpt_address;
  capture_string bpt_what;
  capture_string bpt_condition;
  capture_number bpt_ignore;
  capture_number disp_number;
  capture_string disp_format;
  capture_string disp_expr;
  capture_string disp_value;
  capture_bool   disp_enable;
  capture_string srcs_file;
  capture_string source_location;
  capture_string show_value;
  capture_string error;

  friend void delete_capture (capture *capt);
};
