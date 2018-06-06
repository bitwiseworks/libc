/* emx/bigint.h (emx+gcc) */

/* Big integers, for internal use by the floating point conversion
   routines only.  Do not use in application programs. */

/* Number of bits in a _bi_word. */

#define _BI_WORDSIZE    32

/* Maximum value of a _bi_word. */

#define _BI_WORDMAX     0xffffffff

/* Bigints are made up of `digits' of this type. */

typedef unsigned long _bi_word;

/* This type must be exactly twice as big as a _bi_word. */

typedef unsigned long long _bi_dword;

/* Ditto, signed. */

typedef signed long long _bi_sdword;

/* A bigint. */

typedef struct
{
  /* Number of used elements of v; either n is 0 or v[n-1] is
     non-zero. */

  int n;

  /* The least significant word is stored in v[0] (unless n is 0). */

  _bi_word *v;
} _bi_bigint;

/* Compute the number of words required for a bigint of BITS bits. */

#define _BI_WORDS(bits) (((bits) + _BI_WORDSIZE - 1) / _BI_WORDSIZE)

/* Declare a bigint variable named VAR of WORDS words.  The _BI_INIT
   macro must be used (as statement) to initialize such a variable. */

#define _BI_DECLARE(var,words) \
  _bi_word var##__v[words]; \
  _bi_bigint var

/* Initialize a bigint variable defined with _BI_DECLARE.  This macro
   should be used as statement. */

#define _BI_INIT(var) (var.v = var##__v)

/* For the following functions, source and destination bigints must be
   distinct, unless documented otherwise . */

/* Assign the word SRC to the bigint pointed to by DST (having space
   for DST_WORDS words).  Return 0 on success, 1 on overflow. */

int _bi_set_w (_bi_bigint *dst, int dst_words, _bi_word src);

/* Assign the double word SRC to the bigint pointed to by DST (having
   space for DST_WORDS words).  Return 0 on success, 1 on overflow. */

int _bi_set_d (_bi_bigint *dst, int dst_words, _bi_dword src);

/* Assign the bigint pointed to by SRC to the bigint pointed to by DST
   (having space for DST_WORDS words).  Return 0 on success, 1 on
   overflow. */

int _bi_set_b (_bi_bigint *dst, int dst_words, const _bi_bigint *src);

/* Compare the bigint pointed to by SRC1 to the bigint pointed to by
   SRC2.  Return 1 if the first bigint is bigger than the second
   bigint, 0 if the bigints are equal, or -1 if the first bigint is
   smaller than the second one. */

int _bi_cmp_bb (const _bi_bigint *src1, const _bi_bigint *src2);

/* Compare the bigint pointed to by SRC1 to 2^SHIFT2.  SHIFT2 may be
   negative.  Return 1 if the bigint is bigger than the power of two,
   0 if the numbers are equal, or -1 if the bigint is smaller than the
   power of two. */

int _bi_cmp_pow2 (const _bi_bigint *src1, int shift2);

/* Add the bigints pointed to by SRC1 and SRC2, storing the result to
   the bigint pointed to by DST (having space for DST_WORDS words).
   DST may point to the same bigint as SRC1 or SRC2.  Return 0 on
   success, 1 on overflow. */

int _bi_add_bb (_bi_bigint *dst, int dst_words,
                const _bi_bigint *src1, const _bi_bigint *src2);

/* Subtract from the bigint pointed to by DST the product of FACTOR
   and the bigint pointed to by SRC.  Return 0 on success.  Return 1
   and leave the bigint pointed to by DST unchanged if the result
   would be negative. */
   
int _bi_sub_mul_bw (_bi_bigint *dst, const _bi_bigint *src, _bi_word factor);

/* Shift left the word SRC by SHIFT bits (SHIFT >= 0) and store the
   result to the bigint pointed to by DST (having space for DST_WORDS
   words).  Return 0 on success, 1 on overflow. */

int _bi_shl_w (_bi_bigint *dst, int dst_words, _bi_word src, int shift);

/* Shift left the bigint pointed to by SRC by SHIFT bits (SHIFT >= 0)
   and store the result to the bigint pointed to by DST (having space
   for DST_WORDS words).  DST may point to the same bigint as SRC.
   Return 0 on success, 1 on overflow. */

int _bi_shl_b (_bi_bigint *dst, int dst_words,
               const _bi_bigint *src, int shift);

/* Shift right the bigint pointed to by SRC by SHIFT bits (SHIFT >= 0)
   and store the result to the bigint pointed to by DST (having space
   for DST_WORDS words).  DST may point to the same bigint as SRC.
   Return 0 on success, 1 on overflow. */

int _bi_shr_b (_bi_bigint *dst, int dst_words, const _bi_bigint *src,
               int shift);

/* Store the product of the word FACTOR and the bigint pointed to by
   SRC to the bigint pointed to by DST (having space for DST_WORDS
   words).  DST may point to the same bigint as SRC.  Return 0 on
   success, 1 on overflow. */

int _bi_mul_bw (_bi_bigint *dst, int dst_words,
                const _bi_bigint *src, _bi_word factor);

/* Store the product of the bigints pointed to by SRC1 and SRC2 to the
   bigint pointed to by DST (having space for DST_WORDS words).
   Return 0 on success, 1 on overflow.  The check for overflow is not
   based on the actual values of the bigints, only on their size. */

int _bi_mul_bb (_bi_bigint *dst, int dst_words,
                const _bi_bigint *src1, const _bi_bigint *src2);

/* Divide the bigint pointed to by NUM by the bigint pointed to by
   DEN.  Replace NUM with the remainder of the division, return the
   quotient.  The quotient must fit in _BI_WORDSIZE-1 bits. */

_bi_word _bi_hdiv_rem_b (_bi_bigint *num, const _bi_bigint *den);

/* Divide the bigint pointed to by NUM by 2^SHIFT (SHIFT >= 0).
   Replace NUM with the remainder of the division, return the
   quotient.  The quotient must fit in a _bi_word. */

_bi_word _bi_wdiv_rem_pow2 (_bi_bigint *num, int shift);

/* Compute the product of 5^EXP, 2^SHIFT, and the bigint pointed to by
   FACTOR.  0 <= EXP <= 4951 (5039, actually), 0 <= SHIFT.  If FACTOR
   is NULL, the factor is taken to be one.  Store the result to the
   bigint pointed to by DST (having space for DST_WORDS words).
   Return 0 on success, 1 on overflow. */

int _bi_pow5 (_bi_bigint *dst, int dst_words,
              int exp, int shift, const _bi_bigint *factor);

/* Return the number of the most significant set bit of the bigint
   pointed to by SRC.  The least significant bit is numbered 1.
   Return 0 if the bigint is zero. */

int _bi_fls (const _bi_bigint *src);

/* Divide the bigint pointed to by NUM by the word DEN.  Store the
   quotient to the bigint pointed to by QUOT (having space for
   QUOT_WORDS words) and store the remainder to the word pointed by
   REM.  Return 0 on success, 1 on overflow or division by zero. */

int _bi_div_rem_bw (_bi_bigint *quot, int quot_words, _bi_word *rem,
                    const _bi_bigint *num, _bi_word den);

/* Divide the bigint pointed to by NUM_REM by the bigint pointed to by
   DEN.  Store the quotient to the bigint pointed to by QUOT (having
   space for QUOT_WORDS words) and store the remainder to the bigint
   pointed by NUM_REM (having space for NUM_REM_WORDS words, replacing
   the numerator).  There must be at least one extra word available in
   the bigint pointed to by NUM_REM.  Return 0 on success, 1 on
   overflow or division by zero. */

int _bi_div_rem_bb (_bi_bigint *num_rem, int num_rem_words,
                    _bi_bigint *quot, int quot_words,
                    const _bi_bigint *den);

/* Divide the bigint pointed to by NUM_REM by 2^SHIFT (SHIFT >= 0).
   Store the quotient to the bigint pointed to by QUOT (having space
   for QUOT_WORDS words) and store the remainder to the bigint pointed
   by NUM_REM (replacing the numerator).  Return 0 on success, 1 on
   overflow. */

int _bi_div_rem_pow2 (_bi_bigint *num_rem, _bi_bigint *quot, int quot_words,
                      int shift);
