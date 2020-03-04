/* _input.c (emx+gcc) -- Copyright (c) 1990-2000 by Eberhard Mattes */
/* wide char changes by Dmitriy Kuminov */

#ifdef _WIDECHAR
#define _INPUT _winput
#define _GETC_INLINE _getwc_inline
#define _UNGETC_NOLOCK _ungetwc_nolock
#define _EOF WEOF
#define _CHAR wchar_t
#define _LIT(str) L##str
#define _TO_INT(c) ((int)(c))
#define _STDTOLD wcstold
#define _STDTOD wcstod
#define _STDTOF wcstof
#else
#define _INPUT _input
#define _GETC_INLINE _getc_inline
#define _UNGETC_NOLOCK _ungetc_nolock
#define _EOF EOF
#define _CHAR char
#define _LIT(str) str
#define _TO_INT(c) ((unsigned char)(c))
#define _STDTOLD strtold
#define _STDTOD strtod
#define _STDTOF strtof
#endif

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <locale.h>
#include <emx/io.h>
#include <InnoTekLIBC/locale.h>
#include <wchar.h>
#ifdef _WIDECHAR
#include <wctype.h>
#else
#include <ctype.h>
#endif
#include "getputc.h"

#define FALSE   0
#define TRUE    1

#define SIZE_HH         (('h' << 8) | 'h')
#define SIZE_LL         (('l' << 8) | 'l')


/* This structure holds the local variables of _input() which are
   passed to the functions called by _input(). */

typedef struct
{
  FILE *stream;                 /* Where input comes from */
  int width;                    /* Field width or what's left over of it */
  int chars;                    /* Number of characters read */
  int volatile count;           /* Number of fields assigned */
  _CHAR * volatile more;         /* Big buffer for %e format */
  int collected;                /* Number of characters collected */
  size_t more_size;             /* Size of the above */
  int size;                     /* Size (0, 'h', SIZE_HH, 'l', SIZE_LL, 'L', 'j', 'z', 'Z', 't' or 'q') */
  char width_given;             /* A field width has been given */
  unsigned char ungetc_count;   /* stream's _ungetc_count prior to get0() */
  enum
    {
      OK,                       /* Situation normal */
      END_OF_FILE,              /* End of file (stop at next directive) */
      MATCHING_FAILURE,         /* Matching failure (stop immediately) */
      INPUT_FAILURE,            /* Input failure (stop immediately) */
      OUT_OF_MEMORY             /* Out of memory (stop immediately) */
    } status;
} ilocal;


/* Return the next input character.  Return EOF on input failure.
   This is the only place where input is read from the stream or
   whatever.  This function must return a value representable by an
   `unsigned char', or EOF.  Otherwise, isdigit() etc. won't work. */

static int get0 (ilocal *v)
{
  int c;

  /* Save the stream's _ungetc_count; we need this to decide whether
     to push back or leave unread the character if we don't eat it. */

  v->ungetc_count = v->stream->_ungetc_count;

  /* Read a character from the stream. */

  c = _GETC_INLINE (v->stream);
  if (c == _EOF)
    {
      v->status = END_OF_FILE;
      return _EOF;
    }
  ++v->chars;
  return _TO_INT(c);
}


/* Return the next character of the current input field.  Return EOF
   if the field width is exhausted or on input failure. */

static int get (ilocal *v)
{
  if (v->width == 0)
    return _EOF;
  --v->width;
  return get0 (v);
}


/* Undo reading the previously read character.  Do not use ungetc()
   unless it has been put into the stream with ungetc(). */

static void make_unread (ilocal *v, int c)
{
  if (c != EOF)
    {
      if (v->ungetc_count != 0)
        {
          /* This case is simple: the character was put into the
             stream with ungetc(), so we must also push it back with
             ungetc(). */

          _UNGETC_NOLOCK (c, v->stream);
        }
      else
        {
          /* Pretend that the character was not read.  Don't forget to
             reset the EOF indicator! */

          if (v->stream->_ptr == v->stream->_buffer)
            abort ();
          --v->stream->_ptr;
          ++v->stream->_rcount;
          v->stream->_flags &= ~_IOEOF;
        }
      --v->chars;
    }
}


#define COLLECT(X) do { collect (v, buf, sizeof (buf) / sizeof (buf[0]), X); \
                        if (v->status == OUT_OF_MEMORY) return; } while (0)

static void collect (ilocal *v, _CHAR *buf, size_t buf_size,
                     _CHAR c)
{
  if (v->more == NULL && v->collected < buf_size)
    buf[v->collected] = c;
  else
    {
      if (v->collected >= v->more_size)
        {
          v->more_size += 512;
          v->more = realloc (v->more, v->more_size * sizeof(_CHAR));
          if (v->more == NULL)
            {
              v->status = OUT_OF_MEMORY;
              return;
            }
          if (v->collected == buf_size)
            memcpy (v->more, buf, buf_size * sizeof(_CHAR));
        }
      v->more[v->collected] = c;
    }
  ++v->collected;
}


/* Skip leading whitespace of an input field and return the first
   non-whitespace character.  Hitting EOF is treated as input failure.
   Don't forget to check `v->status' after calling this function. */

static int skip (ilocal *v)
{
  int c;

  do
    {
      c = get0 (v);
      if (c == _EOF)
        {
          v->status = INPUT_FAILURE;
          return _EOF;
        }
    } while (isspace (c));

  /* We have read one character of the field, therefore we have to
     decrement `v->width'.  Note that `v->width' is never zero on
     entry to skip(). */

  --v->width;
  return c;
}


static void inp_char (ilocal *v, char *dst)
{
  int c, ok;
#ifdef _WIDECHAR
  char mb[MB_LEN_MAX];
  mbstate_t mbs = {{0}};
#endif

  if (v->status == END_OF_FILE)
    {
      v->status = INPUT_FAILURE;
      return;
    }
  if (!v->width_given)
    v->width = 1;
  ok = FALSE;
  while ((c = get (v)) != _EOF)
    {
#ifndef _WIDECHAR
      /* No conversion is needed. */
      ok = TRUE;
      if (dst != NULL)
        *dst++ = c;
#else
      /* Convert from wchar_t to multibyte. */
      size_t cb = wcrtomb(mb, c, &mbs);
      if (cb > 0)
        {
          ok = TRUE;
          if (dst != NULL)
            for (size_t i = 0; i < cb; ++i)
              *dst++ = mb[i];
        }
      else
        break;
#endif
    }
  if (!ok)
    v->status = INPUT_FAILURE;
  else if (dst != NULL)
    ++v->count;
}


static void inp_wchar (ilocal *v, wchar_t *dst)
{
  int c, ok;
#ifndef _WIDECHAR
  char mb[MB_LEN_MAX];
  mbstate_t mbs = {{0}};
  size_t len = 0;
#endif

  if (v->status == END_OF_FILE)
    {
      v->status = INPUT_FAILURE;
      return;
    }
  if (!v->width_given)
    v->width = 1;
  ok = FALSE;
  while ((c = get (v)) != _EOF)
    {
#ifdef _WIDECHAR
      /* No conversion is needed. */
      ok = TRUE;
      if (dst != NULL)
        *dst++ = c;
#else
      /* Convert from multibyte to wchar_t. */
      mb[len++] = c;
      wchar_t wc;
      size_t cb = mbrtowc(&wc, mb, len, &mbs);
      if (cb > 0)
        {
          ok = TRUE;
          if (dst != NULL)
            *dst++ = wc;
          len = 0;
        }
      else if (len == MB_LEN_MAX || cb != (size_t) -2)
        break;
      /* -2 is incomplete sequence, read further */
#endif
    }
  if (!ok)
    v->status = INPUT_FAILURE;
  else if (dst != NULL)
    ++v->count;
}


static void inp_str (ilocal *v, char *dst)
{
  int c;
#ifdef _WIDECHAR
  char mb[MB_LEN_MAX];
  mbstate_t mbs = {{0}};
#endif

  c = skip (v);
  if (v->status != OK)
    return;
  while (c != _EOF && !isspace (c))
    {
#ifndef _WIDECHAR
      /* No conversion is needed. */
      if (dst != NULL)
        *dst++ = c;
#else
      /* Convert from wchar_t to multibyte. */
      size_t cb = wcrtomb(mb, c, &mbs);
      if (cb > 0)
        {
          if (dst != NULL)
            for (size_t i = 0; i < cb; ++i)
              *dst++ = mb[i];
        }
      else
        break;
#endif
      c = get (v);
    }
  if (dst != NULL)
    {
      *dst = 0;
      ++v->count;
    }
  make_unread (v, c);
}


static void inp_wstr (ilocal *v, wchar_t *dst)
{
  int c;
#ifndef _WIDECHAR
  char mb[MB_LEN_MAX];
  mbstate_t mbs = {{0}};
  size_t len = 0;
#endif

  c = skip (v);
  if (v->status != OK)
    return;

  while (c != _EOF && !isspace (c))
    {
#ifdef _WIDECHAR
      /* No conversion is needed. */
      if (dst != NULL)
        *dst++ = c;
#else
      /* Convert from multibyte to wchar_t. */
      mb[len++] = c;
      wchar_t wc;
      size_t cb = mbrtowc(&wc, mb, len, &mbs);
      if (cb > 0)
        {
          if (dst != NULL)
            *dst++ = wc;
          len = 0;
        }
      else if (len == MB_LEN_MAX || cb != (size_t) -2)
        break;
      /* -2 is incomplete sequence, read further */
#endif
      c = get (v);
    }
  if (dst != NULL)
    {
      *dst = 0;
      ++v->count;
    }
#ifndef _WIDECHAR
  /* unread all failed bytes (in reverse order) */
  if (len)
    while (len)
      make_unread (v, mb[--len]);
  else
#endif
  make_unread (v, c);
}


/* TODO a simple array-based char map is not suitable for wchar_t since it
   would need 65536 values, not 256, which is too much. We need a more complex
   structure to track the scanset. */
#ifndef _WIDECHAR
static void inp_set (ilocal *v, const _CHAR **pfmt, unsigned char *dst)
{
  char map[256], end, done;
  unsigned char f;
  const _CHAR *format;
  int c, i, ok;

  if (v->status == END_OF_FILE)
    {
      v->status = INPUT_FAILURE;
      return;
    }
  format = *pfmt;
  bzero (map, 256);
  end = 0;
  ++format;
  if (*format == '^')
    {
      ++format; end = 1;
    }
  i = 0; done = 0;
  do
    {
      f = (unsigned char)*format;
      switch (f)
        {
        case 0:
          *pfmt = format - 1;   /* Avoid skipping past 0 */
          done = 1;
          break;
        case ']':
          if (i > 0)
            {
              *pfmt = format;
              done = 1;
              break;
            }
          /* no break */
        default:
          if (format[1] == '-' && format[2] != 0 && format[2] != ']'
              && f < (unsigned char)format[2])
            {
              memset (map+f, 1, (unsigned char)format[2] - f + 1);
              format += 2;
            }
          else
            map[f] = 1;
          break;
        }
      ++format; ++i;
    } while (!done);
  ok = FALSE;
  c = get (v);
  while (c != _EOF && map[c] != end)
    {
      ok = TRUE;
      if (dst != NULL)
        *dst++ = (unsigned char)c;
      c = get (v);
    }
  if (!ok)
    {
      if (c != _EOF)
        {
          make_unread (v, c);
          v->status = MATCHING_FAILURE;
        }
      else
          v->status = INPUT_FAILURE;
      return;
    }
  if (dst != NULL)
    {
      *dst = 0;
      ++v->count;
    }
  make_unread (v, c);
}
#endif


static void inp_int_base (ilocal *v, void *dst, int base)
{
  char neg, ok;
  unsigned long long n;
  int c, digit;

  c = skip (v);
  if (v->status != OK) return;
  neg = FALSE;
  if (c == '+')
    c = get (v);
  else if (c == '-')
    {
      neg = TRUE;
      c = get (v);
    }

  n = 0; ok = FALSE;

  if (base == 0)
    {
      base = 10;
      if (c == '0')
        {
          c = get (v);
          if (c == 'x' || c == 'X')
            {
              base = 16;
              c = get (v);
            }
          else
            {
              base = 8;
              ok = TRUE; /* We've seen a digit! */
            }
        }
    }
  else if (base == 16 && c == '0')
    {
      c = get (v);
      if (c == 'x' || c == 'X')
        c = get (v);
      else
        ok = TRUE;          /* We've seen a digit! */
    }

  while (c != _EOF)
    {
      if (isdigit (c))
        digit = c - '0';
      else if (isupper (c))
        digit = c - 'A' + 10;
      else if (islower (c))
        digit = c - 'a' + 10;
      else
        break;
      if (digit < 0 || digit >= base)
        break;
      ok = TRUE;
      n = n * base + digit;
      c = get (v);
    }
  if (!ok)
    {
      if (c != _EOF)
        {
          make_unread (v, c);
          v->status = MATCHING_FAILURE;
        }
      else
          v->status = INPUT_FAILURE;
      return;
    }
  if (neg)
    n = -n;
  if (dst != NULL)
    {
      switch (v->size)
        {
        case 'h':
          *(short *)dst = n;
          break;
        case SIZE_HH:
          *(signed char *)dst = n;
          break;
        case 'l':
          *(long *)dst = n;
          break;
        case SIZE_LL:
        case 'L':                       /* GNU */
        case 'q':                       /* bsd */
          *(long long *)dst = n;
          break;
        case 'j':
          *(intmax_t *)dst = n;
          break;
        case 'z':
        case 'Z':
          *(size_t *)dst = n;
          break;
        case 't':
          *(ptrdiff_t *)dst = n;
          break;
        default:
          *(int *)dst = n;
          break;
        }
      ++v->count;
    }
  make_unread (v, c);
}


static void inp_int (ilocal *v, _CHAR f, void *dst)
{
  switch (f)
    {
    case 'i':
      inp_int_base (v, dst, 0);
      break;

    case 'd':
      inp_int_base (v, dst, 10);
      break;

    case 'u':
      inp_int_base (v, dst, 10);
      break;

    case 'o':
      inp_int_base (v, dst, 8);
      break;

    case 'x':
    case 'X':
    case 'p':
      inp_int_base (v, dst, 16);
      break;

    default:
      abort ();
    }
}


static void inp_float (ilocal *v, void *dst)
{
  int c;
  const _CHAR *text;
  _CHAR *p, *q;
  char ok;
  _CHAR buf[128];
  long double lx;
  double dx;
  float fx;

  v->collected = 0;
  c = skip (v);
  if (v->status != OK) return;
  ok = FALSE;
  if (c == '+' || c == '-')
    {
      COLLECT (c);
      c = get (v);
    }
  if (c == 'i' || c == 'I')
    text = _LIT("infinity");
  else if (c == 'n' || c == 'N')
    text = _LIT("nan");
  else
    text = NULL;

  if (text != NULL)
    {
      int n = 0;
      do
        {
          ++n;
          COLLECT (c);
          c = get (v);
        } while (text[n] != 0 && tolower (c) == text[n]);
      if (text[n] == 0 && text[0] == 'n' && c == '(')
        {
          do
            {
              COLLECT (c);
              c = get (v);
            } while (c == '_' || isalnum (c));
          if (c == ')')
            {
              COLLECT (c);
              c = get (v);
            }
          else
            n = 0;              /* Matching failure */
        }
      if (!(text[n] == 0 || (text[0] == 'i' && n == 3)))
        {
          make_unread (v, c);
          v->status = MATCHING_FAILURE;
          return;
        }
    }
  else
    {
      while (isdigit (c))
        {
          COLLECT (c);
          ok = TRUE;
          c = get (v);
        }
      if (c == _TO_INT(__libc_gLocaleLconv.s.decimal_point[0]))
        {
          COLLECT (c);
          c = get (v);
          while (isdigit (c))
            {
              COLLECT (c);
              ok = TRUE;
              c = get (v);
            }
        }
      if (!ok)
        {
          if (c != _EOF)
            {
              make_unread (v, c);
              v->status = MATCHING_FAILURE;
            }
          else
              v->status = INPUT_FAILURE;
          return;
        }

      if (c == 'e' || c == 'E')
        {
          COLLECT (c);
          c = get (v);
          if (c == '+' || c == '-')
            {
              COLLECT (c);
              c = get (v);
            }
          if (!isdigit (c))
            {
              if (c != _EOF)
                {
                  make_unread (v, c);
                  v->status = MATCHING_FAILURE;
                }
              else
                  v->status = INPUT_FAILURE;
              return;
            }
          while (isdigit (c))
            {
              COLLECT (c);
              c = get (v);
            }
        }
    }

  make_unread (v, c);
  COLLECT (0);

  p = ((v->more != NULL) ? v->more : buf);
  lx = 0.0; dx = 0.0; fx = 0.0;
  switch (v->size)
    {
    case 'L':
      lx = _STDTOLD (p, &q);
      break;
    case 'l':
      dx = _STDTOD (p, &q);
      break;
    default:
      fx = _STDTOF (p, &q);
      break;
    }
  if (q == buf || *q != 0) /* Ignore overflow! */
    {
      v->status = MATCHING_FAILURE;
      return;
    }
  if (dst != NULL)
    {
      switch (v->size)
        {
        case 'L':
          *(long double *)dst = lx;
          break;
        case 'l':
          *(double *)dst = dx;
          break;
        default:
          *(float *)dst = fx;
          break;
        }
      ++v->count;
    }
}


static int inp_main (ilocal *v, const _CHAR *format, char *arg_ptr)
{
  void *dst;
  _CHAR f;
  int c;
  char assign;
  int mbi, mbn;

  /* ISO 9899-1990, 7.9.6.2 "The format shall be a multibyte
     character sequence, beginning and ending in its initial shift
     state." */

  c = 0;
  while ((f = *format) != 0)
    {
      if (isspace (f))
        {
          do
            {
              ++format; f = *format;
            } while (isspace (f));
          do
            {
              c = get0 (v);
            } while (isspace (c));
          make_unread (v, c);
        }
      else if (f != '%')
        {
          /* ISO 9899-1990, 7.9.6.2: "A directive that is an
             ordinary multibyte character is executed by reading the
             next characters of the stream.  If one of the characters
             differs from one comprising the directive, the directive
             fails, and the differing and subsequent characters remain
             unread." */

          if (!CHK_MBCS_PREFIX (&__libc_GLocaleCtype, f, mbn))
            mbn = 1;
          for (mbi = 0; mbi < mbn; ++mbi)
            {
              c = get0 (v);
              if (c == _EOF) return (v->count == 0 ? _EOF : v->count);
              if (c != _TO_INT(*format))
                {
                  /* ISO 9899 does not allow us to leave the entire
                     multibyte character unread; only the non-matching
                     byte will be left in the stream. */

                  make_unread (v, c);
                  return v->count;
                }
              ++format;
            }
        }
      else
        {
          v->size = 0; v->width = INT_MAX; assign = TRUE; dst = NULL;
          v->width_given = FALSE;
          ++format;
          if (*format == '*')
            {
              assign = FALSE;
              ++format;
            }
          if (isdigit (_TO_INT(*format)))
            {
              v->width = 0;
              while (isdigit (_TO_INT(*format)))
                v->width = v->width * 10 + (*format++ - '0');

              /* The behavior for a zero width is declared undefined
                 by ISO 9899-1990.  In this implementation, a zero
                 width is equivalent to no width being specified. */

              if (v->width == 0)
                v->width = INT_MAX;
              else
                v->width_given = TRUE;
            }
          if (   *format == 'h' || *format == 'l' || *format == 'L'
              || *format == 'j' || *format == 'z' || *format == 't'
              || *format == 'q' || *format == 'Z' )
            {
              v->size = *format++;
              if (v->size == 'l' && *format == 'l')
                {
                  v->size = SIZE_LL; ++format;
                }
              else if (v->size == 'h' && *format == 'h')
                {
                  v->size = SIZE_HH; ++format;
                }
            }
          f = *format;
          switch (f)
            {
            /* TODO inp_set is not ready for wide char input */
#ifndef _WIDECHAR
            case '[':
              if (assign)
                dst = va_arg (arg_ptr, char *);
              inp_set (v, &format, dst);
              break;
            /* TODO l modifier support (scanf("%l[abc]")) */
#endif

            case 'c':
              if (v->size != 'l' && v->size != 'L')
                {
                  if (assign)
                    dst = va_arg (arg_ptr, char *);
                  inp_char (v, dst);
                  break;
                }
              /* fall thru */
            case 'C':
              if (assign)
                dst = va_arg (arg_ptr, wchar_t *);
              inp_wchar (v, dst);
              break;

            case 's':
              if (v->size != 'l' && v->size != 'L')
                {
                  if (assign)
                    dst = va_arg (arg_ptr, char *);
                  inp_str (v, dst);
                  break;
                }
              /* fall thru */
            case 'S':
                if (assign)
                  dst = va_arg (arg_ptr, wchar_t *);
                inp_wstr (v, dst);
                break;

            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
              if (assign)
                switch (v->size)
                  {
                  case 'L':
                    dst = va_arg (arg_ptr, long double *);
                    break;
                  case 'l':
                      dst = va_arg (arg_ptr, double *);
                    break;
                  default:
                    dst = va_arg (arg_ptr, float *);
                    break;
                  }
              inp_float (v, dst);
              break;

            case 'i':
            case 'd':
            case 'u':
            case 'o':
            case 'x':
            case 'X':
            case 'p':
              if (assign)
                switch (v->size)
                  {
                  case SIZE_HH:
                    dst = va_arg (arg_ptr, char *);
                    break;
                  case 'h':
                    dst = va_arg (arg_ptr, short *);
                    break;
                  case 'l':
                    dst = va_arg (arg_ptr, long *);
                    break;
                  case SIZE_LL:
                  case 'L':
                  case 'q':
                    dst = va_arg (arg_ptr, long long *);
                    break;
                  case 'j':
                    dst = va_arg (arg_ptr, uintmax_t *);
                    break;
                  case 'z':
                  case 'Z':
                    dst = va_arg (arg_ptr, size_t *);
                    break;
                  case 't':
                    dst = va_arg (arg_ptr, ptrdiff_t *);
                    break;
                  default:
                    dst = va_arg (arg_ptr, int *);
                    break;
                  }
              inp_int (v, f, dst);
              break;

            case 'n':
              if (assign)
                switch (v->size)
                  {
                  case SIZE_HH:
                    *(va_arg (arg_ptr, signed char *)) = v->chars;
                    break;
                  case 'h':
                    *(va_arg (arg_ptr, short *)) = v->chars;
                    break;
                  case 'l':
                    *(va_arg (arg_ptr, long *)) = v->chars;
                    break;
                  case SIZE_LL:
                  case 'L':
                  case 'q':
                    *(va_arg (arg_ptr, long long *)) = v->chars;
                    break;
                  case 'z':
                  case 'Z':
                    *(va_arg (arg_ptr, size_t *)) = v->chars;
                    break;
                  case 't':
                    *(va_arg (arg_ptr, ptrdiff_t *)) = v->chars;
                    break;
                  default:
                    *(va_arg (arg_ptr, int *)) = v->chars;
                    break;
                  }
              break;

            default:
              if (f == 0)                 /* % at end of string */
                return v->count;
              c = skip (v);
              if (c != f)
                return v->count;
              break;
            }
          switch (v->status)
            {
            case INPUT_FAILURE:
              return v->count == 0 ? _EOF : v->count;
            case MATCHING_FAILURE:
            case OUT_OF_MEMORY:
              return v->count;
            default:
              break;
            }
          ++format;
        }
    }
  return v->count;
}


int _INPUT (FILE *stream, const _CHAR *format, char *arg_ptr)
{
  ilocal v;
  int rc;

  v.stream = stream;
  v.more = NULL; v.more_size = 0;
  v.count = 0; v.chars = 0; v.status = OK;
  rc = inp_main (&v, format, arg_ptr);
  if (v.more != NULL)
    free (v.more);
  return rc;
}
