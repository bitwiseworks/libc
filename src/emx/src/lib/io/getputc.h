/* Internal inline routines used in some i/o functions */

static inline int _putc_inline (int _c, FILE *_s)
{
  return (--_s->_wcount >= 0 && (_c != '\n' || !(_s->_flags & _IOLBF))
          ? (unsigned char)(*_s->_ptr++ = (char)_c)
          : _flush (_c, _s));
}

static inline int _getc_inline (FILE *_s)
{
  return (--_s->_rcount >= 0
          ? (unsigned char)*_s->_ptr++
          : _fill (_s));
}

#ifdef _WIDECHAR

#define _IS_WSPRINTF_STREAM(s) \
  ((s->_flags & (_IOSPECIAL | _IOWIDE)) == (_IOSPECIAL | _IOWIDE) && \
   s->_flush == NULL)

wint_t __libc_putwc_convert (wchar_t _c, FILE *_s);
wint_t __libc_getwc_convert (FILE *_s);

static inline wint_t _putwc_inline (wchar_t _c, FILE *_s)
{
  if (_IS_WSPRINTF_STREAM (_s))
  {
    /* It's a special swprintf buffer of wchar_t, put _c directly. Note that we
       don't flag an error when attempting to write beyond the end of the
       string as we need the total number of characters printed. See also
       _flush(). */

    if (--_s->_wcount >= 0)
      *((*(wchar_t **)&_s->_ptr)++) = _c;
    else
      _s->_wcount = 0;
    return _c;
  }
  else
    return __libc_putwc_convert (_c, _s);
}

static inline wint_t _getwc_inline (FILE *_s)
{
  if (_IS_WSPRINTF_STREAM (_s))
  {
    /* It's a special swscanf buffer of wchar_t, get _c directly. Note that we
       don't set errno when attempting to read beyond the end of the
       string. See also _fill(). */

    return (--_s->_rcount >= 0
            ? *((*(wchar_t **)&_s->_ptr)++)
            : WEOF);
  }
  else
    return __libc_getwc_convert (_s);
}

#endif
