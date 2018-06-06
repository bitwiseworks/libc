/* fnmatch.c (emx+gcc) -- Copyright (c) 1994-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <emx/fnmatch.h>
#include <InnoTekLIBC/locale.h>
#include <ctype.h>

/* In OS/2 and DOS styles, both / and \ separate components of a path.
   This macro returns true iff C is a separator. */

#define IS_OS2_COMP_SEP(C)  ((C) == '/' || (C) == '\\')


/* This macro returns true if C is at the end of a component of a
   path. */

#define IS_OS2_COMP_END(C)  ((C) == 0 || IS_OS2_COMP_SEP (C))


/* Return a pointer to the next component of the path SRC, for OS/2
   and DOS styles.  When the end of the string is reached, a pointer
   to the terminating null character is returned. */

static const unsigned char *skip_comp_os2 (const unsigned char *src)
{
  /* Correct reading: em-be-ce-el :-) */
  int mbcl;
  /* Skip characters until hitting a separator or the end of the
     string. */

  while (!IS_OS2_COMP_END (*src))
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *src, mbcl) && src[1] != 0)
      src += mbcl;
    else
      ++src;

  /* Skip the separator if we hit a separator. */

  if (*src != 0)
    ++src;
  return src;
}


/* Return true iff the path P contains a colon. */

static int has_colon (const unsigned char *p)
{
  int mbcl;

  while (*p != 0)
    if (*p == ':')
      return 1;
    else if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, p[0], mbcl) && p[1] != 0)
      p += mbcl;
    else
      ++p;
  return 0;
}


/* Compare a single component (directory name or file name) of the
   paths, for OS/2 and DOS styles.  MASK and NAME point into a
   component of the wildcard and the name to be checked, respectively.
   Comparing stops at the next separator.  The FLAGS argument is the
   same as that of fnmatch().  HAS_DOT is true if a dot is in the
   current component of NAME.  The number of dots is not restricted,
   even in DOS style.  Return _FNM_MATCH iff MASK and NAME match.
   Note that this function is recursive. */

static int match_comp_os2 (const unsigned char *mask,
                           const unsigned char *name,
                           unsigned flags, int has_dot)
{
  int rc, mbcl;

  for (;;)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *mask, mbcl) && mask[1] != 0)
      {
        if (memicmp (mask, name, mbcl))
          return FNM_NOMATCH;
        mask += mbcl; name += mbcl;
      }
    else
      switch (*mask)
        {
        case 0:

          /* There must be no extra characters at the end of NAME when
             reaching the end of MASK unless _FNM_PATHPREFIX is set:
             in that case, NAME may point to a separator. */

          if (*name == 0)
            return _FNM_MATCH;
          if ((flags & _FNM_PATHPREFIX) && IS_OS2_COMP_SEP (*name))
            return _FNM_MATCH;
          return FNM_NOMATCH;

        case '/':
        case '\\':

          /* Separators match separators. */

          if (IS_OS2_COMP_SEP (*name))
            return _FNM_MATCH;

          /* If _FNM_PATHPREFIX is set, a trailing separator in MASK
             is ignored at the end of NAME. */

          if ((flags & _FNM_PATHPREFIX) && mask[1] == 0 && *name == 0)
            return _FNM_MATCH;

          /* Stop comparing at the separator. */

          return FNM_NOMATCH;

        case '?':

          /* A question mark matches one character.  It does not match
             a dot.  At the end of the component (and before a dot),
             it also matches zero characters. */

          if (*name != '.' && !IS_OS2_COMP_END (*name))
            {
              if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, name[0], mbcl) && name[1] != 0)
                name += mbcl;
              else
                ++name;
            }
          ++mask;
          break;

        case '*':

          /* An asterisk matches zero or more characters.  In DOS
             mode, dots are not matched. */

          do
            {
              ++mask;
            } while (*mask == '*');
          for (;;)
            {
              rc = match_comp_os2 (mask, name, flags, has_dot);
              if (rc != FNM_NOMATCH)
                return rc;
              if (IS_OS2_COMP_END (*name))
                return FNM_NOMATCH;
              if (*name == '.' && (flags & _FNM_STYLE_MASK) == _FNM_DOS)
                return FNM_NOMATCH;
              if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, name[0], mbcl) && name[1] != 0)
                name += mbcl;
              else
                ++name;
            }

        case '.':

          /* A dot matches a dot.  It also matches the implicit dot at
             the end of a dot-less NAME. */

          ++mask;
          if (*name == '.')
            ++name;
          else if (has_dot || !IS_OS2_COMP_END (*name))
            return FNM_NOMATCH;
          break;

        default:

          /* All other characters match themselves. */

          if (flags & _FNM_IGNORECASE)
            {
              if (tolower (*mask) != tolower (*name))
                return FNM_NOMATCH;
            }
          else
            {
              if (*mask != *name)
                return FNM_NOMATCH;
            }
          ++mask; ++name;
          break;
        }
}


/* Compare a single component (directory name or file name) of the
   paths, for all styles which need component-by-component matching.
   MASK and NAME point to the start of a component of the wildcard and
   the name to be checked, respectively.  Comparing stops at the next
   separator.  The FLAGS argument is the same as that of fnmatch().
   Return _FNM_MATCH iff MASK and NAME match. */

static int match_comp (const unsigned char *mask, const unsigned char *name,
                       unsigned flags)
{
  const unsigned char *s;
  int mbcl;

  switch (flags & _FNM_STYLE_MASK)
    {
    case _FNM_OS2:
    case _FNM_DOS:

      /* For OS/2 and DOS styles, we add an implicit dot at the end of
         the component if the component doesn't include a dot. */

      s = name;
      while (!IS_OS2_COMP_END (*s) && *s != '.')
        if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, s[0], mbcl) && s[1] != 0)
          s += mbcl;
        else
          ++s;
      return match_comp_os2 (mask, name, flags, *s == '.');

    default:
      return _FNM_ERR;
    }
}



/* In Unix styles, / separates components of a path.  This macro
   returns true iff C is a separator. */

#define IS_UNIX_COMP_SEP(C)  ((C) == '/')


/* This macro returns true if C is at the end of a component of a
   path. */

#define IS_UNIX_COMP_END(C)  ((C) == 0 || IS_UNIX_COMP_SEP (C))


/* Match complete paths for Unix styles.  The FLAGS argument is the
   same as that of fnmatch().  COMP points to the start of the current
   component in NAME.  Return _FNM_MATCH iff MASK and NAME match.  The
   backslash character is used for escaping ? and * unless
   FNM_NOESCAPE is set. */

static int match_unix (const unsigned char *mask, const unsigned char *name,
                       unsigned flags, const unsigned char *comp)
{
  unsigned char c1, c2;
  char invert, matched;
  const unsigned char *start;
  int rc, mbcl;

  for (;;)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *mask, mbcl))
      {
        if (memicmp (mask, name, mbcl))
          return FNM_NOMATCH;
        mask += mbcl; name += mbcl;
      }
    else
      switch (*mask)
        {
        case 0:

          /* There must be no extra characters at the end of NAME when
             reaching the end of MASK unless _FNM_PATHPREFIX is set:
             in that case, NAME may point to a separator. */

          if (*name == 0)
            return _FNM_MATCH;
          if ((flags & _FNM_PATHPREFIX) && IS_UNIX_COMP_SEP (*name))
            return _FNM_MATCH;
          return FNM_NOMATCH;

        case '?':

          /* A question mark matches one character.  It does not match
             the component separator if FNM_PATHNAME is set.  It does
             not match a dot at the start of a component if FNM_PERIOD
             is set. */

          if (*name == 0)
            return FNM_NOMATCH;
          if ((flags & FNM_PATHNAME) && IS_UNIX_COMP_SEP (*name))
            return FNM_NOMATCH;
          if (*name == '.' && (flags & FNM_PERIOD) && name == comp)
            return FNM_NOMATCH;
          ++mask;
          if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, name[0], mbcl) && name[1] != 0)
            name += mbcl;
          else
            ++name;
          break;

        case '*':

          /* An asterisk matches zero or more characters.  It does not
             match the component separator if FNM_PATHNAME is set.  It
             does not match a dot at the start of a component if
             FNM_PERIOD is set. */

          if (*name == '.' && (flags & FNM_PERIOD) && name == comp)
            return FNM_NOMATCH;
          do
            {
              ++mask;
            } while (*mask == '*');
          for (;;)
            {
              rc = match_unix (mask, name, flags, comp);
              if (rc != FNM_NOMATCH)
                return rc;
              if (*name == 0)
                return FNM_NOMATCH;
              if ((flags & FNM_PATHNAME) && IS_UNIX_COMP_SEP (*name))
                return FNM_NOMATCH;
              if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, name[0], mbcl) && name[1] != 0)
                name += mbcl;
              else
                ++name;
            }

        case '/':

          /* Separators match only separators.  If _FNM_PATHPREFIX is
             set, a trailing separator in MASK is ignored at the end
             of NAME. */

          if (!(IS_UNIX_COMP_SEP (*name)
                || ((flags & _FNM_PATHPREFIX) && *name == 0
                    && (mask[1] == 0
                        || (!(flags & FNM_NOESCAPE) && mask[1] == '\\'
                            && mask[2] == 0)))))
            return FNM_NOMATCH;

          ++mask;
          if (*name != 0) ++name;

          /* This is the beginning of a new component if FNM_PATHNAME
             is set. */

          if (flags & FNM_PATHNAME)
            comp = name;
          break;

        case '[':

          /* A set of characters.  Always case-sensitive. */

          if (*name == 0)
            return FNM_NOMATCH;
          if ((flags & FNM_PATHNAME) && IS_UNIX_COMP_SEP (*name))
            return FNM_NOMATCH;
          if (*name == '.' && (flags & FNM_PERIOD) && name == comp)
            return FNM_NOMATCH;

          invert = 0; matched = 0;
          ++mask;

          /* If the first character is a ! or ^, the set matches all
             characters not listed in the set. */

          if (*mask == '!' || *mask == '^')
            {
              ++mask; invert = 1;
            }
          else if (IS_MBCS_PREFIX (&__libc_GLocaleCtype, *name))
            {
              /* DBCS characters can only match inverted sets. */

              return FNM_NOMATCH;
            }

          /* Loop over all the characters of the set.  The loop ends
             if the end of the string is reached or if a ] is
             encountered unless it directly follows the initial [ or
             [-. */

          start = mask;
          while (!(*mask == 0 || (*mask == ']' && mask != start)))
            {
              /* Get the next character which is optionally preceded
                 by a backslash. */

              c1 = *mask++;
              if (IS_MBCS_PREFIX (&__libc_GLocaleCtype, c1))
                return _FNM_ERR; /* DBCS characters not supported in sets */
              if (!(flags & FNM_NOESCAPE) && c1 == '\\')
                {
                  if (*mask == 0)
                    break;
                  c1 = *mask++;
                }

              /* Ranges of characters are written as a-z.  Don't
                 forget to check for the end of the string and to
                 handle the backslash.  If the character after - is a
                 ], it isn't a range. */

              if (*mask == '-' && mask[1] != ']')
                {
                  ++mask;       /* Skip the - character */
                  if (!(flags & FNM_NOESCAPE) && *mask == '\\')
                    ++mask;
                  if (*mask == 0)
                    break;
                  c2 = *mask++;
                  if (IS_MBCS_PREFIX (&__libc_GLocaleCtype, c2))
                    return _FNM_ERR; /* DBCS chars not supported in sets */
                }
              else
                c2 = c1;

              /* Now check whether this character or range matches NAME. */

              if (c1 <= *name && *name <= c2)
                matched = 1;
            }

          /* If the end of the string is reached before a ] is found,
             back up to the [ and compare it to NAME. */

          if (*mask == 0)
            {
              if (*name != '[')
                return FNM_NOMATCH;
              ++name;
              mask = start;
              if (invert)
                --mask;
            }
          else
            {
              if (invert)
                matched = !matched;
              if (!matched)
                return FNM_NOMATCH;
              ++mask;           /* Skip the ] character */
              if (*name != 0)
                {
                  if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, name[0], mbcl) && name[1] != 0)
                    name += mbcl;
                  else
                    ++name;
                }
            }
          break;

        case '\\':
          ++mask;
          if (flags & FNM_NOESCAPE)
            {
              if (*name != '\\')
                return FNM_NOMATCH;
              ++name;
            }
          else if (*mask == '*' || *mask == '?')
            {
              if (*mask != *name)
                return FNM_NOMATCH;
              ++mask; ++name;
            }
          break;

        default:

          /* All other characters match themselves. */

          if (flags & _FNM_IGNORECASE)
            {
              if (tolower (*mask) != tolower (*name))
                return FNM_NOMATCH;
            }
          else
            {
              if (*mask != *name)
                return FNM_NOMATCH;
            }
          ++mask; ++name;
          break;
        }
}


/* Check whether the path name NAME matches the wildcard MASK.  Return
   0 (_FNM_MATCH) if it matches, _FNM_NOMATCH if it doesn't.  Return
   _FNM_ERR on error. The operation of this function is controlled by
   FLAGS.  This is an internal function, with unsigned arguments. */


static int _fnmatch_unsigned (const unsigned char *mask,
                              const unsigned char *name,
                              unsigned flags)
{
  int m_drive, n_drive, rc;

  /* Match and skip the drive name if present. */

  m_drive = ((isalpha (mask[0]) && mask[1] == ':') ? mask[0] : -1);
  n_drive = ((isalpha (name[0]) && name[1] == ':') ? name[0] : -1);

  if (m_drive != n_drive)
    {
      if (m_drive == -1 || n_drive == -1)
        return FNM_NOMATCH;
      if (!(flags & _FNM_IGNORECASE))
        return FNM_NOMATCH;
      if (tolower (m_drive) != tolower (n_drive))
        return FNM_NOMATCH;
    }

  if (m_drive != -1) mask += 2;
  if (n_drive != -1) name += 2;

  /* Colons are not allowed in path names, except for the drive name,
     which was skipped above. */

  if (has_colon (mask) || has_colon (name))
    return _FNM_ERR;

  /* The name "\\server\path" should not be matched by mask
     "\*\server\path".  Ditto for /. */

  switch (flags & _FNM_STYLE_MASK)
    {
    case _FNM_OS2:
    case _FNM_DOS:

      if (IS_OS2_COMP_SEP (name[0]) && IS_OS2_COMP_SEP (name[1]))
        {
          if (!(IS_OS2_COMP_SEP (mask[0]) && IS_OS2_COMP_SEP (mask[1])))
            return FNM_NOMATCH;
          name += 2;
          mask += 2;
        }
      break;

    case _FNM_POSIX:

      if (name[0] == '/' && name[1] == '/')
        {
          int i;

          name += 2;
          for (i = 0; i < 2; ++i)
            if (mask[0] == '/')
              ++mask;
            else if (mask[0] == '\\' && mask[1] == '/')
              mask += 2;
            else
              return FNM_NOMATCH;
        }

      /* In Unix styles, treating ? and * w.r.t. components is simple.
         No need to do matching component by component. */

      return match_unix (mask, name, flags, name);
    }

  /* Now compare all the components of the path name, one by one.
     Note that the path separator must not be enclosed in brackets. */

  while (*mask != 0 || *name != 0)
    {

      /* If _FNM_PATHPREFIX is set, the names match if the end of MASK
         is reached even if there are components left in NAME. */

      if (*mask == 0 && (flags & _FNM_PATHPREFIX))
        return _FNM_MATCH;

      /* Compare a single component of the path name. */

      rc = match_comp (mask, name, flags);
      if (rc != _FNM_MATCH)
        return rc;

      /* Skip to the next component or to the end of the path name. */

      mask = skip_comp_os2 (mask);
      name = skip_comp_os2 (name);
    }

  /* If we reached the ends of both strings, the names match. */

  if (*mask == 0 && *name == 0)
    return _FNM_MATCH;

  /* The names do not match. */

  return FNM_NOMATCH;
}


/* This is the public entrypoint, with signed arguments. */

int _STD(fnmatch) (const char *mask, const char *name, int flags)
{
  return _fnmatch_unsigned ((const unsigned char *)mask, (const unsigned char *)name, flags);
}
