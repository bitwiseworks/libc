/* bipow5.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <alloca.h>
#include <emx/bigint.h>
#include "bipow5.tab"


int _bi_pow5 (_bi_bigint *dst, int dst_words,
              int exp, int shift, const _bi_bigint *factor)
{
#ifdef TABLE1_SIZE
  int f1;
#endif
  int f2, f3, ov;
  _bi_bigint b2, b3, bt;
  const _bi_bigint *p;

  if (factor != NULL && factor->n == 0)
    {
      dst->n = 0;
      return 0;
    }

#ifdef TABLE1_SIZE
  f1 = exp % (TABLE1_SIZE + 1);
  exp = exp / (TABLE1_SIZE + 1);
#endif
  f2 = exp % (TABLE2_SIZE + 1);
  f3 = exp / (TABLE2_SIZE + 1);

  p = NULL;
  if (f3 != 0)
    {
      if (f3 > TABLE3_SIZE)
        return 1;
      b3.n = table3[f3] - table3[f3-1];
      b3.v = (_bi_word *)table4 + table3[f3-1]; /* cast away const */
      p = &b3;
    }

  if (f2 != 0)
    {
      b2.n = table2[f2] - table2[f2-1];
      b2.v = (_bi_word *)table4 + table2[f2-1]; /* cast away const */
      if (p == NULL)
        p = &b2;
      else
        {
          if (_bi_mul_bb (dst, dst_words, p, &b2) != 0)
            return 1;
          p = dst;
        }
    }

  if (factor != NULL && (factor->n != 1 || factor->v[0] != 1))
    {
      if (p == NULL)
        p = factor;
      else
        {
          if (factor->n == 1)
            ov = _bi_mul_bw (dst, dst_words, p, factor->v[0]);
          else
            {
              if (p == dst)
                {
                  bt.v = alloca (dst->n * sizeof (_bi_word));
                  _bi_set_b (&bt, dst->n, dst);
                  p = &bt;
                }
              ov = _bi_mul_bb (dst, dst_words, p, factor);
            }
          if (ov != 0)
            return ov;
          p = dst;
        }
    }

#ifdef TABLE1_SIZE
  if (f1 != 0)
    {
      if (p == NULL)
        {
          if (shift != 0)
            {
              ov = _bi_shl_w (dst, dst_words, table1[f1-1], shift);
              shift = 0;
            }
          else
            ov = _bi_set_w (dst, dst_words, table1[f1-1]);
        }
      else
        ov = _bi_mul_bw (dst, dst_words, p, table1[f1-1]);
      if (ov != 0)
        return 1;
      p = dst;
    }
#endif

  if (shift != 0)
    {
      if (p == NULL)
        ov = _bi_shl_w (dst, dst_words, 1, shift);
      else
        ov = _bi_shl_b (dst, dst_words, p, shift);
      if (ov != 0)
        return 1;
      p = dst;
    }

  if (p == NULL)
    return _bi_set_w (dst, dst_words, 1);
  else if (p != dst)
    return _bi_set_b (dst, dst_words, p);
  else
    return 0;
}
