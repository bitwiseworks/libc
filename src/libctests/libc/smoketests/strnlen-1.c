/* confdefs.h.  */
#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>

int main ()
{

#define S "foobar"
#define S_LEN (sizeof S - 1)

  /* At least one implementation is buggy: that of AIX 4.3 would
     give strnlen (S, 1) == 3.  */

  int i;
  for (i = 0; i < S_LEN + 1; ++i)
    {
      int expected = i <= S_LEN ? i : S_LEN;
      if (strnlen (S, i) != expected)
	exit (1);
    }
  exit (0);

  return 0;
}

