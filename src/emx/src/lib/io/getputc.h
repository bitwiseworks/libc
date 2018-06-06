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
