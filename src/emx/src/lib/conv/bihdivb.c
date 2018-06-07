/* bihdivb.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <assert.h>
#include <sys/builtin.h>
#include <emx/bigint.h>

/* Return NUM/DEN, replace NUM with NUM%DEN.  The quotient must not
   exceed 1 << (_BI_WORDSIZE/2-1). */

_bi_word _bi_hdiv_rem_b (_bi_bigint *num, const _bi_bigint *den)
{
  _bi_word q, nhi, dhi;
  int shift, neg;

  assert (den->n != 0);         /* DEN must be non-zero */
  assert (den->v[den->n-1] != 0); /* DEN must be normalized */

  /* The quotient is zero if NUM is zero or if NUM < DEN. */

  if (num->n == 0 || num->n < den->n)
    return 0;

  assert (num->n <= den->n + 1); /* Overflow must not occur */
  assert (num->v[num->n-1] != 0); /* NUM must be normalized */

  /* Fetch the most significant words of both numbers. */

  nhi = num->v[num->n-1];
  dhi = den->v[den->n-1];

  /* Use single-precision division if the numbers are single-precision
     numbers. */

  if (num->n == 1)              /* This implies den->n == 1, see above */
    {
      q = nhi / dhi;
      neg = _bi_sub_mul_bw (num, den, q);
      assert (!neg);
      return q;
    }

  /* Now we know a bit more about the numbers; again, the quotient is
     zero if NUM < DEN. */

  if (num->n == den->n && nhi < dhi)
    return 0;

  /* No trivial cases left.  Compute an estimate for the quotient by
     looking at the most significant bits.  Theorem 1 gives
     relationships between the estimate and the quotient. */

  /* Obtain the most significant _BI_WORDSIZE bits of NUM and scale DEN
     appropriatly to keep the quotient unchanged.  This maximizes the
     maximum possible quotient and increases the likelyhook of the
     estimate being correct. */

  shift = _BI_WORDSIZE - __fls (nhi);
  if (shift == 0)
    {
      /* For num->n > den->n, the quotient would be too big. */
      assert (num->n == den->n);
    }
  else
    {
      nhi = (nhi << shift) | (num->v[num->n-2] >> (_BI_WORDSIZE - shift));
      if (den->n < num->n)
        dhi >>= (_BI_WORDSIZE - shift);
      else                      /* den->n >= num->n >= 2 */
        dhi = (dhi << shift) | (den->v[den->n-2] >> (_BI_WORDSIZE - shift));
    }

  /* This assertion guarantees that DHI/NHI is either the quotient or
     the quotient plus one.  The assertion can fail if the quotient is
     bigger than supported (see theorem 2). */

  assert (dhi >= 1 << (_BI_WORDSIZE/2-1));

  /* Estimate the quotient by looking at the most significant
     _BI_WORDSIZE bits of NHI. */

  q = nhi / dhi;

  /* If Q is zero, the quotient cannot be Q-1 and therefore we've got
     the correct quotient. */

  if (q == 0)
    {
      assert (_bi_cmp_bb (num, den) < 0);
      return 0;
    }

  /* Q is exact if NHI/(DHI+1) = NHI/DHI.  Q is also exact if DHI is
     the maximum value of a _bi_word and NHI is not the maximum value of
     a _bi_word. */

  if ((_bi_word)(dhi + 1) != 0 ? q == nhi / (dhi + 1) : (_bi_word)(nhi + 1) !=  0)
    {
      neg = _bi_sub_mul_bw (num, den, q);
      assert (!neg);
      assert (_bi_cmp_bb (num, den) < 0);
      return q;
    }

  /* Theorem 2 guarantees that only two cases are left.  First try Q,
     which is the correct quotient with extremely high probability. */

  neg = _bi_sub_mul_bw (num, den, q);
  if (neg)
    {
      /* The probability of hitting this case seems to be proportional
         to 2^-_BI_WORDSIZE.  For _BI_WORDSIZE of 32, it's hardly ever
         hit in practice if the quotient is <10.  This code has been
         tested by reducing SHIFT to 7, which is the minimum value (by
         theorem 2) for quotients up to 10. */

      q -= 1;
      neg = _bi_sub_mul_bw (num, den, q);
      assert (!neg);
    }
  assert (_bi_cmp_bb (num, den) < 0);
  return q;
}

/* Theorem 1: Given positive integers X, Y, and S.  Let x:=[X/S],
   y:=[Y/S], y non-zero.  Then, [x/(y+1)] <= [X/Y] <= [x/y].

   Proof: Suppose that X and Y are unknown.  For given x and y, a
   lower bound for [X/Y] can be obtained by choosing the smallest X
   and biggest Y for x and y, respectively:

     [X/Y] >= [Xmin/Ymax] =  [(x*S+0) / (y*S+S-1)]
                          =  [x*S / ((y+1)*S-1)]
                          >= [x*S / ((y+1)*S]       (note that y >= 2, S >= 1)
                          =  [x/(y+1)]

   For given x and y, an upper bound for [X/Y] can be obtained by
   choosing the biggest X and smallest Y for x and y, respectively:

     [X/Y] <= [Xmax/Ymin] =  [(x*S+S-1) / (y*S+0)]
                          =  [x/y + ((S-1)/S)/y]
                          =  [[x/y] + x/y-[x/y] + ((S-1)/S)/y]
                          =  [x/y] + [x/y-[x/y] + ((S-1)/S)/y]
                          <= [x/y] + [(y-1)/y + ((S-1)/S)/y]
                          =  [x/y] + [(y-1+(S-1)/S)/y]
                          =  [x/y]                      qed.


   Theorem 2: Let x and y be positive integers.  Then,

     [x/y] - [x/(y+1)] <= 1

   for x <= y^2+2y-1.

   Proof:

     [x/y] =  [x/y - x/(y+1) + x/(y+1)]
           =  [(x*(y+1) - x*y)/(y*(y+1)) + x/(y+1)]
           =  [x / (y*(y+1)) + x/(y+1)]
           <= [(y*y+2*y-1) / (y*(y+1)) + x/(y+1)]   (for x <= y^2+2y-1)
           =  [1 + (x+(1-1/y))/(y+1)]
           =  1 + [x/(y+1)]                             qed. */

