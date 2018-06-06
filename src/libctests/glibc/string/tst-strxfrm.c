/* Based on a test case by Paul Eggert.  */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static int
test (const char *locale, const char *string)
{
  size_t bufsize;
  size_t r;
  size_t l;
  char *buf;
#ifdef HAVE_NEWLOCALE
  locale_t loc;
#endif
  int result = 0;

  if (setlocale (LC_COLLATE, locale) == NULL)
    {
      printf ("cannot set locale \"%s\"\n", locale);
      return 1;
    }
  bufsize = strxfrm (NULL, string, 0) + 1;
  buf = malloc (bufsize);
  if (buf == NULL)
    {
      printf ("cannot allocate %zd bytes\n", bufsize);
      return 1;
    }
  r = strxfrm (buf, string, bufsize);
  l = strlen (buf);
  if (r != l)
    {
       printf ("locale \"%s\": strxfrm returned %zu, strlen returned %zu\n",
	       locale, r, l);
       result = 1;
    }

#ifdef HAVE_NEWLOCALE
  loc = newlocale (1 << LC_ALL, locale, NULL);

  r = strxfrm_l (buf, string, bufsize, loc);
  l = strlen (buf);
  if (r != l)
    {
       printf ("locale \"%s\": strxfrm_l returned %zu, strlen returned %zu\n",
	       locale, r, l);
       result = 1;
    }

  freelocale (loc);
#endif

  free (buf);

  return result;
}


int
main (void)
{
  int result = 0;

  result |= test ("C", "");
  result |= test ("C", "abcABCxyzXYZ|+-/*(%$");
#ifdef __BSD__ /* BSD is missing the aliases. loosers. */
  result |= test ("en_US.ISO8859-1", "");
  result |= test ("en_US.ISO8859-1", "abcABCxyzXYZ|+-/*(%$");
#else
  result |= test ("en_US.ISO-8859-1", "");
  result |= test ("en_US.ISO-8859-1", "abcABCxyzXYZ|+-/*(%$");
#endif
  result |= test ("de_DE.UTF-8", "");
  result |= test ("de_DE.UTF-8", "abcABCxyzXYZ|+-/*(%$");

  return result;
}
