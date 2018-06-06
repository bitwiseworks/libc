/* bimulbb.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include <assert.h>
#include <emx/bigint.h>

int _bi_mul_bb (_bi_bigint *dst, int dst_words,
                const _bi_bigint *src1, const _bi_bigint *src2)
{
  int i, j, dn;
  _bi_dword tmp;
  _bi_word carry, factor;

  if (src1->n == 0 || src2->n == 0)
    {
      dst->n = 0;
      return 0;
    }
  if (src1->n + src2->n > dst_words)
    return 1;
  dn = src1->n + src2->n;
  for (i = 0; i < dn; ++i)
    dst->v[i] = 0;
  for (i = 0; i < src2->n; ++i)
    {
      factor = src2->v[i];
      carry = 0;
      for (j = 0; j < src1->n; ++j)
        {
          tmp = (_bi_dword)src1->v[j] * factor + dst->v[i+j] + carry;
          dst->v[i+j] = (_bi_word)tmp;
          carry = tmp >> _BI_WORDSIZE;
        }
      while (carry)
        {
          assert (i + j < dn);
          tmp = (_bi_dword)dst->v[i+j] + carry;
          dst->v[i+j] = (_bi_word)tmp;
          carry = tmp >> _BI_WORDSIZE;
          ++j;
        }
    }

  if (dn > 0 && dst->v[dn-1] == 0)
    --dn;
  assert (dn != 0 && dst->v[dn-1] != 0);
  dst->n = dn;
  return 0;
}
