/* moddef2.c (emx+gcc) -- Copyright (c) 1992-1995 by Eberhard Mattes */

#include <stdio.h>
#include <string.h>
#include <sys/moddef.h>

#define FALSE 0
#define TRUE  1

typedef int _md_callback (struct _md *md, const _md_stmt *stmt,
                          _md_token token, void *arg);

static int error (struct _md *md, _md_callback *callback, _md_stmt *stmt,
                  _md_token stmt_token, _md_error code, void *arg)
{
  stmt->error.code = code;
  stmt->error.stmt = stmt_token;
  return callback (md, stmt, _MD_parseerror, arg);
}

static _md_token sync (struct _md *md)
{
  _md_token token;

  token = _md_get_token (md);
  for (;;)
    switch (token)
      {
      case _MD_BASE:
      case _MD_CODE:
      case _MD_DATA:
      case _MD_DESCRIPTION:
      case _MD_EXETYPE:
      case _MD_EXPORTS:
      case _MD_HEAPSIZE:
      case _MD_IMPORTS:
      case _MD_OLD:
      case _MD_PROTMODE:
      case _MD_REALMODE:
      case _MD_SEGMENTS:
      case _MD_STACKSIZE:
      case _MD_STUB:
      case _MD_eof:
      case _MD_ioerror:
      case _MD_missingquote:
        return token;
      default:
        token = _md_next_token (md);
        break;
      }
}


#define ERROR(CODE) \
  do { result = error (md, callback, &stmt, stmt_token, (CODE), arg); \
       if (result != 0) return result; \
       token = sync (md); goto next_stmt;} while (FALSE)

#define CALLBACK \
  do { result = callback (md, &stmt, stmt_token, arg); \
       if (result != 0) return result; } while (FALSE)

int _md_parse (struct _md *md, _md_callback *callback, void *arg)
{
  _md_token token, stmt_token;
  _md_stmt stmt;
  const char *s;
  long n;
  int ok, result;

  token = _md_get_token (md);
  stmt_token = token;
  stmt.line_number = _md_get_linenumber (md);
  switch (stmt_token)
    {
    case _MD_eof:
      ERROR (_MDE_EMPTY);
      break;

    case _MD_NAME:
      stmt.line_number = _md_get_linenumber (md);
      token = _md_next_token (md);
      stmt.name.name[0] = 0;
      if (token == _MD_quote || token == _MD_word)
        {
          _strncpy (stmt.name.name, _md_get_string (md),
                    sizeof (stmt.name.name));
          token = _md_next_token (md);
        }
      switch (token)
        {
        case _MD_WINDOWAPI:
          stmt.name.pmtype = _MDT_WINDOWAPI;
          token = _md_next_token (md);
          break;
        case _MD_WINDOWCOMPAT:
          stmt.name.pmtype = _MDT_WINDOWCOMPAT;
          token = _md_next_token (md);
          break;
        case _MD_NOTWINDOWCOMPAT:
          stmt.name.pmtype = _MDT_NOTWINDOWCOMPAT;
          token = _md_next_token (md);
          break;
        default:
          stmt.name.pmtype = _MDT_DEFAULT;
          break;
        }
      stmt.name.newfiles = FALSE;
      if (token == _MD_NEWFILES)
        {
          stmt.name.newfiles = TRUE;
          token = _md_next_token (md);
        }
      CALLBACK;
      break;

    case _MD_LIBRARY:
      stmt.line_number = _md_get_linenumber (md);
      token = _md_next_token (md);
      if (token == _MD_quote || token == _MD_word)
        {
          _strncpy (stmt.library.name, _md_get_string (md),
                    sizeof (stmt.library.name));
          token = _md_next_token (md);
        }
      switch (token)
        {
        case _MD_INITGLOBAL:
          stmt.library.init = _MDIT_GLOBAL;
          token = _md_next_token (md);
          break;
        case _MD_INITINSTANCE:
          stmt.library.init = _MDIT_INSTANCE;
          token = _md_next_token (md);
          break;
        default:
          stmt.library.init = _MDIT_DEFAULT;
          break;
        }
      switch (token)
        {
        case _MD_TERMGLOBAL:
          stmt.library.term = _MDIT_GLOBAL;
          token = _md_next_token (md);
          break;
        case _MD_TERMINSTANCE:
          stmt.library.term = _MDIT_INSTANCE;
          token = _md_next_token (md);
          break;
        default:
          stmt.library.term = _MDIT_DEFAULT;
          break;
        }
      if (token == _MD_PRIVATELIB)
        token = _md_next_token (md);
      CALLBACK;
      break;

    case _MD_VIRTUAL:
    case _MD_PHYSICAL:
      stmt.line_number = _md_get_linenumber (md);
      token = _md_next_token (md);
      if (token != _MD_DEVICE)
        ERROR (_MDE_DEVICE_EXPECTED);
      token = _md_next_token (md);
      stmt.device.name[0] = 0;
      if (token == _MD_quote || token == _MD_word)
        {
          _strncpy (stmt.device.name, _md_get_string (md),
                    sizeof (stmt.device.name));
          token = _md_next_token (md);
        }
      CALLBACK;
      break;

    default:
      break;
    }

next_stmt:
  while (token != _MD_eof)
    {
      stmt_token = token;
      stmt.line_number = _md_get_linenumber (md);
      switch (stmt_token)
        {
        case _MD_BASE:
          token = _md_next_token (md);
          if (token != _MD_equal)
            ERROR (_MDE_EQUAL_EXPECTED);
          token = _md_next_token (md);
          if (token != _MD_number)
            ERROR (_MDE_NUMBER_EXPECTED);
          token = _md_next_token (md);
          CALLBACK;
          break;

        case _MD_CODE:
          token = _md_next_token (md);
          stmt.segment.segname[0] = 0;
          stmt.segment.classname[0] = 0;
          stmt.segment.attr = 0;
          ok = TRUE;
          while (ok)
            {
              switch (token)
                {
                case _MD_CONFORMING:
                  stmt.segment.attr |= _MDS_CONFORMING;
                  break;
                case _MD_DISCARDABLE:
                  stmt.segment.attr |= _MDS_DISCARDABLE;
                  break;
                case _MD_EXECUTEONLY:
                  stmt.segment.attr |= _MDS_EXECUTEONLY;
                  break;
                case _MD_EXECUTEREAD:
                  stmt.segment.attr |= _MDS_EXECUTEREAD;
                  break;
                case _MD_FIXED:
                  stmt.segment.attr |= _MDS_FIXED;
                  break;
                case _MD_IOPL:
                  stmt.segment.attr |= _MDS_IOPL;
                  break;
                case _MD_LOADONCALL:
                  stmt.segment.attr |= _MDS_LOADONCALL;
                  break;
                case _MD_MOVEABLE:
                  stmt.segment.attr |= _MDS_MOVEABLE;
                  break;
                case _MD_NOIOPL:
                  stmt.segment.attr |= _MDS_NOIOPL;
                  break;
                case _MD_NONCONFORMING:
                  stmt.segment.attr |= _MDS_NONCONFORMING;
                  break;
                case _MD_NONDISCARDABLE:
                  stmt.segment.attr |= _MDS_NONDISCARDABLE;
                  break;
                case _MD_NONSHARED:
                case _MD_IMPURE:
                  stmt.segment.attr |= _MDS_NONSHARED;
                  break;
                case _MD_PRELOAD:
                  stmt.segment.attr |= _MDS_PRELOAD;
                  break;
                case _MD_SHARED:
                case _MD_PURE:
                  stmt.segment.attr |= _MDS_SHARED;
                  break;
                default:
                  ok = FALSE;
                  break;
                }
              if (ok)
                token = _md_next_token (md);
            }
          CALLBACK;
          break;

        case _MD_DATA:
          token = _md_next_token (md);
          stmt.segment.segname[0] = 0;
          stmt.segment.classname[0] = 0;
          stmt.segment.attr = 0;
          ok = TRUE;
          while (ok)
            {
              switch (token)
                {
                case _MD_FIXED:
                  stmt.segment.attr |= _MDS_FIXED;
                  break;
                case _MD_IOPL:
                  stmt.segment.attr |= _MDS_IOPL;
                  break;
                case _MD_LOADONCALL:
                  stmt.segment.attr |= _MDS_LOADONCALL;
                  break;
                case _MD_MOVEABLE:
                  stmt.segment.attr |= _MDS_MOVEABLE;
                  break;
                case _MD_MULTIPLE:
                  stmt.segment.attr |= _MDS_MULTIPLE;
                  break;
                case _MD_NOIOPL:
                  stmt.segment.attr |= _MDS_NOIOPL;
                  break;
                case _MD_NONE:
                  stmt.segment.attr |= _MDS_NONE;
                  break;
                case _MD_NONSHARED:
                case _MD_IMPURE:
                  stmt.segment.attr |= _MDS_NONSHARED;
                  break;
                case _MD_PRELOAD:
                  stmt.segment.attr |= _MDS_PRELOAD;
                  break;
                case _MD_READONLY:
                  stmt.segment.attr |= _MDS_READONLY;
                  break;
                case _MD_READWRITE:
                  stmt.segment.attr |= _MDS_READWRITE;
                  break;
                case _MD_SHARED:
                case _MD_PURE:
                  stmt.segment.attr |= _MDS_SHARED;
                  break;
                case _MD_SINGLE:
                  stmt.segment.attr |= _MDS_SINGLE;
                  break;
                default:
                  ok = FALSE;
                  break;
                }
              if (ok)
                token = _md_next_token (md);
            }
          CALLBACK;
          break;

        case _MD_DESCRIPTION:
          token = _md_next_token (md);
          if (token != _MD_quote)
            ERROR (_MDE_STRING_EXPECTED);
          s = _md_get_string (md);
          if (strlen (s) > 255)
            ERROR (_MDE_STRING_TOO_LONG);
          _strncpy (stmt.descr.string, s, sizeof (stmt.descr.string));
          token = _md_next_token (md);
          CALLBACK;
          break;

        case _MD_EXETYPE:
          token = _md_next_token (md);
          switch (token)
            {
            case _MD_OS2:
              stmt.exetype.type = _MDX_OS2;
              token = _md_next_token (md);
              break;
            case _MD_UNKNOWN:
              stmt.exetype.type = _MDX_UNKNOWN;
              token = _md_next_token (md);
              break;
            case _MD_WINDOWS:
              stmt.exetype.type = _MDX_WINDOWS;
              stmt.exetype.major_version = 0;
              stmt.exetype.minor_version = 0;
              token = _md_next_token (md);
              if (token == _MD_number)
                {
                  stmt.exetype.major_version = _md_get_number (md);
                  token = _md_next_token (md);
                  if (token == _MD_dot)
                    {
                      token = _md_next_token (md);
                      if (token == _MD_number)
                        {
                          stmt.exetype.minor_version = _md_get_number (md);
                          token = _md_next_token (md);
                        }
                    }
                }
              break;
            default:
              stmt.exetype.type = _MDX_DEFAULT;
              break;
            }
          CALLBACK;
          break;

        case _MD_EXPORTS:
          token = _md_next_token (md);
          while (token == _MD_quote || token == _MD_word)
            {
              stmt.line_number = _md_get_linenumber (md);
              stmt.export.ordinal = 0;
              stmt.export.pwords = 0;
              stmt.export.flags = 0;
              stmt.export.internalname[0] = 0;
              _strncpy (stmt.export.entryname, _md_get_string (md),
                        sizeof (stmt.export.entryname));
              token = _md_next_token (md);
              if (token == _MD_equal)
                {
                  token = _md_next_token (md);
                  if (token != _MD_quote && token != _MD_word)
                    ERROR (_MDE_NAME_EXPECTED);
                  _strncpy (stmt.export.internalname, _md_get_string (md),
                            sizeof (stmt.export.internalname));
                  token = _md_next_token (md);
                }
              if (token == _MD_at)
                {
                  stmt.export.flags |= _MDEP_ORDINAL;
                  token = _md_next_token (md);
                  if (token != _MD_number)
                    ERROR (_MDE_NUMBER_EXPECTED);
                  n = _md_get_number (md);
                  if (n < 1 || n > 65535)
                    ERROR (_MDE_INVALID_ORDINAL);
                  stmt.export.ordinal = n;
                  token = _md_next_token (md);
                  if (token == _MD_NONAME)
                    {
                      stmt.export.flags |= _MDEP_NONAME;
                      token = _md_next_token (md);
                    }
                  else if (token == _MD_RESIDENTNAME)
                    {
                      stmt.export.flags |= _MDEP_RESIDENTNAME;
                      token = _md_next_token (md);
                    }
                }
              if (token == _MD_NODATA)
                {
                  stmt.export.flags |= _MDEP_NODATA;
                  token = _md_next_token (md);
                }
              if (token == _MD_number)
                {
                  stmt.export.flags |= _MDEP_PWORDS;
                  stmt.export.pwords = _md_get_number (md);
                  token = _md_next_token (md);
                }
              CALLBACK;
            }
          break;

        case _MD_HEAPSIZE:
          token = _md_next_token (md);
          if (token == _MD_number)
            {
              stmt.heapsize.maxval = FALSE;
              stmt.heapsize.size = _md_get_number (md);
            }
          else if (token == _MD_MAXVAL)
            {
              stmt.heapsize.size = 0;
              stmt.heapsize.maxval = TRUE;
            }
          else
            ERROR (_MDE_NUMBER_EXPECTED);
          token = _md_next_token (md);
          CALLBACK;
          break;

        case _MD_IMPORTS:
          token = _md_next_token (md);
          while (token == _MD_quote || token == _MD_word)
            {
              stmt.line_number = _md_get_linenumber (md);
              _strncpy (stmt.import.modulename, _md_get_string (md),
                        sizeof (stmt.import.modulename));
              stmt.import.entryname[0] = 0;
              stmt.import.internalname[0] = 0;
              stmt.import.ordinal = 0;
              stmt.import.flags = 0;
              token = _md_next_token (md);
              if (token == _MD_equal)
                {
                  _strncpy (stmt.import.internalname, stmt.import.modulename,
                            sizeof (stmt.import.internalname));
                  token = _md_next_token (md);
                  if (token != _MD_quote && token != _MD_word)
                    ERROR (_MDE_NAME_EXPECTED);
                  _strncpy (stmt.import.modulename, _md_get_string (md),
                            sizeof (stmt.import.modulename));
                  token = _md_next_token (md);
                }
              if (token != _MD_dot)
                ERROR (_MDE_DOT_EXPECTED);
              token = _md_next_token (md);
              if (token == _MD_word)
                _strncpy (stmt.import.entryname, _md_get_string (md),
                          sizeof (stmt.import.entryname));
              else if (token == _MD_number)
                {
                  stmt.import.flags |= _MDIP_ORDINAL;
                  n = _md_get_number (md);
                  if (n < 1 || n > 65535)
                    ERROR (_MDE_INVALID_ORDINAL);
                  stmt.import.ordinal = n;
                }
              else
                ERROR (_MDE_NUMBER_EXPECTED);
              token = _md_next_token (md);
              CALLBACK;
            }
          break;

        case _MD_OLD:
          token = _md_next_token (md);
          if (token != _MD_quote)
            ERROR (_MDE_STRING_EXPECTED);
          _strncpy (stmt.old.name, _md_get_string (md),
                    sizeof (stmt.old.name));
          token = _md_next_token (md);
          CALLBACK;
          break;

        case _MD_PROTMODE:
        case _MD_REALMODE:
          token = _md_next_token (md);
          CALLBACK;
          break;

        case _MD_SEGMENTS:
          token = _md_next_token (md);
          while (token == _MD_quote || token == _MD_word)
            {
              stmt.line_number = _md_get_linenumber (md);
              _strncpy (stmt.segment.segname, _md_get_string (md),
                        sizeof (stmt.segment.segname));
              token = _md_next_token (md);
              if (token == _MD_CLASS)
                {
                  token = _md_next_token (md);
                  if (token != _MD_quote && token != _MD_word)
                    ERROR (_MDE_NAME_EXPECTED);
                  _strncpy (stmt.segment.classname, _md_get_string (md),
                            sizeof (stmt.segment.classname));
                  token = _md_next_token (md);
                }
              else
                stmt.segment.classname[0] = 0;
              stmt.segment.attr = 0;
              ok = TRUE;
              while (ok)
                {
                  switch (token)
                    {
                    case _MD_ALIAS:
                      stmt.segment.attr |= _MDS_ALIAS;
                      break;
                    case _MD_CONFORMING:
                      stmt.segment.attr |= _MDS_CONFORMING;
                      break;
                    case _MD_DISCARDABLE:
                      stmt.segment.attr |= _MDS_DISCARDABLE;
                      break;
                    case _MD_EXECUTEONLY:
                      stmt.segment.attr |= _MDS_EXECUTEONLY;
                      break;
                    case _MD_EXECUTEREAD:
                      stmt.segment.attr |= _MDS_EXECUTEREAD;
                      break;
                    case _MD_FIXED:
                      stmt.segment.attr |= _MDS_FIXED;
                      break;
                    case _MD_INVALID:
                      stmt.segment.attr |= _MDS_INVALID;
                      break;
                    case _MD_IOPL:
                      stmt.segment.attr |= _MDS_IOPL;
                      break;
                    case _MD_LOADONCALL:
                      stmt.segment.attr |= _MDS_LOADONCALL;
                      break;
                    case _MD_MIXED1632:
                      stmt.segment.attr |= _MDS_MIXED1632;
                      break;
                    case _MD_MOVEABLE:
                      stmt.segment.attr |= _MDS_MOVEABLE;
                      break;
                    case _MD_NOIOPL:
                      stmt.segment.attr |= _MDS_NOIOPL;
                      break;
                    case _MD_NONCONFORMING:
                      stmt.segment.attr |= _MDS_NONCONFORMING;
                      break;
                    case _MD_NONDISCARDABLE:
                      stmt.segment.attr |= _MDS_NONDISCARDABLE;
                      break;
                    case _MD_NONSHARED:
                    case _MD_IMPURE:
                      stmt.segment.attr |= _MDS_NONSHARED;
                      break;
                    case _MD_PRELOAD:
                      stmt.segment.attr |= _MDS_PRELOAD;
                      break;
                    case _MD_READONLY:
                      stmt.segment.attr |= _MDS_READONLY;
                      break;
                    case _MD_READWRITE:
                      stmt.segment.attr |= _MDS_READWRITE;
                      break;
                    case _MD_SHARED:
                    case _MD_PURE:
                      stmt.segment.attr |= _MDS_SHARED;
                      break;
                    default:
                      ok = FALSE;
                      break;
                    }
                  if (ok)
                    token = _md_next_token (md);
                }
              CALLBACK;
            }
          break;

        case _MD_STACKSIZE:
          token = _md_next_token (md);
          if (token != _MD_number)
            ERROR (_MDE_NUMBER_EXPECTED);
          stmt.stacksize.size = _md_get_number (md);
          token = _md_next_token (md);
          CALLBACK;
          break;

        case _MD_STUB:
          token = _md_next_token (md);
          if (token == _MD_quote)
            {
              _strncpy (stmt.stub.name, _md_get_string (md),
                        sizeof (stmt.stub.name));
              stmt.stub.none = FALSE;
            }
          else if (token == _MD_NONE)
            {
              stmt.stub.name[0] = 0;
              stmt.stub.none = TRUE;
            }
          else
            ERROR (_MDE_STRING_EXPECTED);
          token = _md_next_token (md);
          CALLBACK;
          break;

        case _MD_ioerror:
          ERROR (_MDE_IO_ERROR);
          break;

        case _MD_missingquote:
          ERROR (_MDE_MISSING_QUOTE);
          break;

        default:
          ERROR (_MDE_INVALID_STMT);
        }
    }
  return 0;
}
