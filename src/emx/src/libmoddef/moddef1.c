/* moddef1.c (emx+gcc) -- Copyright (c) 1992-1998 by Eberhard Mattes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <share.h>
#include <errno.h>
#include <sys/moddef.h>

#define ERRNO (*_errno ())

#define MDF_OPEN 0x0001

struct _md
{
  _md_token token;
  FILE *file;
  long number;
  long linenumber;
  unsigned flags;
  const char *ptr;
  char string[2048];
  char buffer[2048];
};

/* An entry of the keywords table associates a keyword string with a
   token index. */

struct keyword
{
  char *name;
  _md_token token;
};

/* This table contains all the keywords.  Some of the keywords are not
   (officially) used. */

static struct keyword keywords[] =
{
  {"ALIAS",           _MD_ALIAS},
  {"BASE",            _MD_BASE},
  {"CLASS",           _MD_CLASS},
  {"CODE",            _MD_CODE},
  {"CONFORMING",      _MD_CONFORMING},
  {"CONTIGUOUS",      _MD_CONTIGUOUS},
  {"DATA",            _MD_DATA},
  {"DESCRIPTION",     _MD_DESCRIPTION},
  {"DEV386",          _MD_DEV386},
  {"DEVICE",          _MD_DEVICE},
  {"DISCARDABLE",     _MD_DISCARDABLE},
  {"DOS4",            _MD_DOS4},
  {"DYNAMIC",         _MD_DYNAMIC},
  {"EXECUTEONLY",     _MD_EXECUTEONLY},
  {"EXECUTE-ONLY",    _MD_EXECUTEONLY},
  {"EXECUTEREAD",     _MD_EXECUTEREAD},
  {"EXETYPE",         _MD_EXETYPE},
  {"EXPANDDOWN",      _MD_EXPANDDOWN},
  {"EXPORTS",         _MD_EXPORTS},
  {"FIXED",           _MD_FIXED},
  {"HEAPSIZE",        _MD_HEAPSIZE},
  {"HUGE",            _MD_HUGE},
  {"IMPORTS",         _MD_IMPORTS},
  {"IMPURE",          _MD_IMPURE},
  {"INCLUDE",         _MD_INCLUDE},
  {"INITGLOBAL",      _MD_INITGLOBAL},
  {"INITINSTANCE",    _MD_INITINSTANCE},
  {"INVALID",         _MD_INVALID},
  {"IOPL",            _MD_IOPL},
  {"LIBRARY",         _MD_LIBRARY},
  {"LOADONCALL",      _MD_LOADONCALL},
  {"LONGNAMES",       _MD_NEWFILES},
  {"MAXVAL",          _MD_MAXVAL},
  {"MIXED1632",       _MD_MIXED1632},
  {"MOVABLE",         _MD_MOVEABLE},
  {"MOVEABLE",        _MD_MOVEABLE},
  {"MULTIPLE",        _MD_MULTIPLE},
  {"NAME",            _MD_NAME},
  {"NEWFILES",        _MD_NEWFILES},
  {"NODATA",          _MD_NODATA},
  {"NOEXPANDDOWN",    _MD_NOEXPANDDOWN},
  {"NOIOPL",          _MD_NOIOPL},
  {"NONAME",          _MD_NONAME},
  {"NONCONFORMING",   _MD_NONCONFORMING},
  {"NONDISCARDABLE",  _MD_NONDISCARDABLE},
  {"NONE",            _MD_NONE},
  {"NONPERMANENT",    _MD_NONPERMANENT},
  {"NONSHARED",       _MD_NONSHARED},
  {"NOTWINDOWCOMPAT", _MD_NOTWINDOWCOMPAT},
  {"OBJECTS",         _MD_OBJECTS},
  {"OLD",             _MD_OLD},
  {"ORDER",           _MD_ORDER},
  {"OS2",             _MD_OS2},
  {"PERMANENT",       _MD_PERMANENT},
  {"PHYSICAL",        _MD_PHYSICAL},
  {"PRELOAD",         _MD_PRELOAD},
  {"PRIVATE",         _MD_PRIVATE},
  {"PRIVATELIB",      _MD_PRIVATELIB},
  {"PROTECT",         _MD_PROTECT},
  {"PROTMODE",        _MD_PROTMODE},
  {"PURE",            _MD_PURE},
  {"READONLY",        _MD_READONLY},
  {"READWRITE",       _MD_READWRITE},
  {"REALMODE",        _MD_REALMODE},
  {"RESIDENT",        _MD_RESIDENT},
  {"RESIDENTNAME",    _MD_RESIDENTNAME},
  {"SEGMENTS",        _MD_SEGMENTS},
  {"SHARED",          _MD_SHARED},
  {"SINGLE",          _MD_SINGLE},
  {"STACKSIZE",       _MD_STACKSIZE},
  {"STUB",            _MD_STUB},
  {"SWAPPABLE",       _MD_SWAPPABLE},
  {"TERMGLOBAL",      _MD_TERMGLOBAL},
  {"TERMINSTANCE",    _MD_TERMINSTANCE},
  {"UNKNOWN",         _MD_UNKNOWN},
  {"VIRTUAL",         _MD_VIRTUAL},
  {"WINDOWAPI",       _MD_WINDOWAPI},
  {"WINDOWCOMPAT",    _MD_WINDOWCOMPAT},
  {"WINDOWS",         _MD_WINDOWS},
  {NULL,              _MD_word}
};


_md_token _md_get_token (const struct _md *md)
{
  return md->token;
}


long _md_get_number (const struct _md *md)
{
  return md->number;
}


const char *_md_get_string (const struct _md *md)
{
  return md->string;
}


long _md_get_linenumber (const struct _md *md)
{
  return md->linenumber;
}


/* Read a line from the module definition file to md->buffer.  Set
   md->token and return -1 on error or if the end of the file is
   reached.  Otherwise, return 0, increment the line number and move
   md->ptr to the start of the line. */

static int get_line (struct _md *md)
{
  char *p;

  if (fgets (md->buffer, sizeof (md->buffer), md->file) == NULL)
    {
      md->token = (ferror (md->file) ? _MD_ioerror : _MD_eof);
      return -1;
    }
  p = strchr (md->buffer, '\n');
  if (p != NULL) *p = 0;
  md->ptr = md->buffer;
  ++md->linenumber;
  return 0;
}


_md_token _md_next_token (struct _md *md)
{
  _md_token prev_token = md->token;
  const char *start, *end;
  char *p, quote_char;
  long n;
  int i;

  md->number = 0;
  md->string[0] = 0;
  for (;;)
    {
      if (*md->ptr == ' ' || *md->ptr == '\t')
        ++md->ptr;
      else if (*md->ptr == 0 || *md->ptr == ';')
        {
          if (get_line (md) != 0)
            return md->token;
        }
      else
        break;
    }
  if (*md->ptr == '\'' || *md->ptr == '\"')
    {
      quote_char = *md->ptr;
      md->token = _MD_quote;
      start = ++md->ptr;
      while (*md->ptr != 0 &&
             (*md->ptr != quote_char || md->ptr[1] == quote_char))
        ++md->ptr;
      if (*md->ptr != quote_char)
        {
          md->token = _MD_missingquote;
          return md->token;
        }
      end = md->ptr++;
    }
  else if (*md->ptr == '=')
    {
      md->token = _MD_equal;
      start = md->ptr++; end = md->ptr;
    }
  else if (*md->ptr == '@')
    {
      md->token = _MD_at;
      start = md->ptr++; end = md->ptr;
    }
  else if (*md->ptr == '.')
    {
      md->token = _MD_dot;
      start = md->ptr++; end = md->ptr;
    }
  else
    {
      md->token = _MD_word;
      start = md->ptr++;
      if (prev_token == _MD_LIBRARY || prev_token == _MD_NAME)
        md->ptr += strcspn (md->ptr, " \t=");
      else
        md->ptr += strcspn (md->ptr, " \t=@.");
      end = md->ptr;
    }
  _strncpy (md->string, start, end+1-start);
  if (md->token == _MD_word)
    for (i = 0; keywords[i].name != NULL; ++i)
      if (stricmp (md->string, keywords[i].name) == 0)
        {
          md->token = keywords[i].token;
          break;
        }
  if (md->token == _MD_word)
    {
      if (isdigit ((unsigned char)md->string[0]))
        {
          ERRNO = 0;
          n = strtol (md->string, &p, 0);
          if (p != md->string && *p == 0 && ERRNO == 0)
            {
              md->token = _MD_number;
              md->number = n;
            }
        }
    }
  return md->token;
}


struct _md *_md_use_file (FILE *f)
{
  struct _md *p;

  p = malloc (sizeof (*p));
  if (p == NULL)
    {
      ERRNO = ENOMEM;
      return NULL;
    }
  p->token = _MD_eof;
  p->string[0] = 0;
  p->buffer[0] = 0;
  p->ptr = p->buffer;
  p->number = 0;
  p->linenumber = 0;
  p->file = f;
  p->flags = 0;
  return p;
}


struct _md *_md_open (const char *fname)
{
  FILE *f;
  struct _md *p;

  f = _fsopen (fname, "rt", SH_DENYWR);
  if (f == NULL)
    return NULL;
  p = _md_use_file (f);
  if (p == NULL)
    fclose (f);
  else
    p->flags |= MDF_OPEN;
  return p;
}


int _md_close (struct _md *md)
{
  int result = 0;

  if (md->flags & MDF_OPEN)
    result = fclose (md->file);
  free (md);
  return result;
}
