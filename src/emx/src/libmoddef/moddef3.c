/* moddef3.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include <stdio.h>
#include <sys/moddef.h>

const char *_md_errmsg (_md_error code)
{
  switch (code)
    {
    case _MDE_IO_ERROR:
      return "Cannot read";
    case _MDE_MISSING_QUOTE:
      return "Missing quote";
    case _MDE_EMPTY:
      return "Empty file";
    case _MDE_NAME_EXPECTED:
      return "Name expected";
    case _MDE_STRING_EXPECTED:
      return "String expected";
    case _MDE_NUMBER_EXPECTED:
      return "Number expected";
    case _MDE_EQUAL_EXPECTED:
      return "`=' expected";
    case _MDE_DOT_EXPECTED:
      return "`.' expected";
    case _MDE_STRING_TOO_LONG:
      return "String too long";
    case _MDE_INVALID_ORDINAL:
      return "Invalid ordinal number";
    case _MDE_INVALID_STMT:
      return "Invalid statement";
    default:
      return "Unknown error";
    }
}
