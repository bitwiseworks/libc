/* bidivbb.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <alloca.h>
#include <assert.h>
#include <sys/builtin.h>
#include <emx/bigint.h>


/* Helper functions defined in bidivhlp.c and bidivhlp.s. */

_bi_word _bi_div_estimate (_bi_word *p, _bi_word v1, _bi_word v2);
int _bi_div_subtract (_bi_word *u, const _bi_word *v, int n, _bi_word q);
void _bi_div_add_back (_bi_word *u, const _bi_word *v, int n);


/* Note: Up to one additional word may be needed for NUM_REM. */

int _bi_div_rem_bb (_bi_bigint *num_rem, int num_rem_words,
                    _bi_bigint *quot, int quot_words,
                    const _bi_bigint *den)
{
  int nn, dn, shift, i, j, m, ov;
  _bi_word q, v1, v2;
  _bi_bigint tmp;

  nn = num_rem->n;
  dn = den->n;

  if (dn == 0)
    return 1;

  if (nn + 1 > num_rem_words)
    return 1;

  if (nn < dn || (nn == dn && num_rem->v[nn-1] < den->v[nn-1]))
    return _bi_set_w (quot, quot_words, 0);

  if (dn == 1)
    {
      _bi_word rem;
      if (_bi_div_rem_bw (quot, quot_words, &rem, num_rem, den->v[0]) != 0)
        return 1;
      return _bi_set_w (num_rem, num_rem_words, rem);
    }

/* See Donald Ervin Knuth, The Art of Computer Programming, Volume 2,
   4.3.1, Algorithm D. */

  m = nn - dn;
  if (m + 1 > quot_words)
    return 1;
  quot->n = m + 1;

  /* D1.  Normalize: shift left the numerator and the denominator to
     make the most significant word of denominator >= _BI_WORDSIZE/2.
     This won't change the quotient. */

  shift = _BI_WORDSIZE - __fls (den->v[dn-1]);
  if (shift != 0)
    {

      ov = _bi_shl_b (num_rem, num_rem_words, num_rem, shift);
      assert (!ov);
      tmp.n = dn;
      tmp.v = alloca (dn * sizeof (_bi_word));
      ov = _bi_shl_b (&tmp, dn, den, shift);
      assert (!ov);
      den = &tmp;
    }

  if (nn == num_rem->n)
    num_rem->v[nn] = 0;

  /* D2 and D7.  Loop over all words of NUM_DEN, starting at u(0), the
     most significant word. */

#define u(i)    num_rem->v[nn-(i)]
#define v(i)    den->v[dn-(i)]

  v1 = den->v[dn-1];
  v2 = den->v[dn-2];
  for (j = 0; j <= m; ++j)
    {
      /* D3.  Estimate the next quotient word. */

      q = _bi_div_estimate (&u(j), v1, v2);

      /* D4.  Multiply and subtract. */
      /* D5.  Test remainder. */

      if (_bi_div_subtract (&u(j+dn), &v(dn), dn, q))
        {
          /* D6.  Add back. */
          q -= 1;
          _bi_div_add_back (&u(j+dn), &v(dn), dn);
        }
      quot->v[m-j] = q;
    }

  /* D8.  Unnormalize. */

  i = dn;
  while (i > 0 && num_rem->v[i-1] == 0)
    --i;
  num_rem->n = i;
  if (shift != 0)
    _bi_shr_b (num_rem, num_rem_words, num_rem, shift);

  /* Normalize QUOT. */

  if (quot->v[quot->n-1] == 0)
    quot->n -= 1;

  return 0;
}
