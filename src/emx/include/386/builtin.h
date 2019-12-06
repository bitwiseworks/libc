/* 386/builtin.h (emx+gcc) */
/** @file
 * EMX 0.9d-fix04
 */

#ifndef _I386_BUILTIN_H
#define _I386_BUILTIN_H

#include <sys/cdefs.h>
#include <stdint.h>

__BEGIN_DECLS


static __inline__ signed char __cxchg (__volatile__ signed char *p,
                                       signed char v)
{
  __asm__ __volatile__ ("xchgb %0, %1" : "=m"(*p), "=q"(v) : "1"(v));
  return v;
}

static __inline__ short __sxchg (__volatile__ short *p, short v)
{
  __asm__ __volatile__ ("xchgw %0, %1" : "=m"(*p), "=r"(v) : "1"(v));
  return v;
}

static __inline__ int __lxchg (__volatile__ int *p, int v)
{
  __asm__ __volatile__ ("xchgl %0, %1" : "=m"(*p), "=r"(v) : "1"(v));
  return v;
}

static __inline__ void __enable (void)
{
  __asm__ __volatile__ ("sti");
}

static __inline__ void __disable (void)
{
  __asm__ __volatile__ ("cli");
}


/**
 * Performs an atomical xchg on an unsigned int.
 * @returns old value.
 * @param   pu      Pointer to the value to update.
 * @param   u       The new value.
 */
static __inline__ unsigned __atomic_xchg(__volatile__ unsigned *pu, unsigned u)
{
    __asm__ __volatile__ ("xchgl %0, %1" : "=m" (*pu), "=r" (u) : "1" (u));
    return u;
}

/**
 * Performs an atomical xchg on an 16-bit unsigned integer.
 *
 * @returns old value.
 * @param   pu16    Pointer to the value to update.
 * @param   u16     The new value.
 */
static inline uint16_t __atomic_xchg_word(volatile uint16_t *pu16, uint16_t u16)
{
    __asm__ __volatile__ ("xchgw %0, %1" : "=m" (*pu16), "=r" (u16) : "1" (u16));
    return u16;
}

/**
 * Atomically sets a bit and return the old one.
 *
 * @returns 1 if the bwit was set, 0 if it was clear.
 * @param   pv      Pointer to base of bitmap.
 * @param   uBit    Bit in question.
 */
static __inline__ int __atomic_set_bit(__volatile__ void *pv, unsigned uBit)
{
    __asm__ __volatile__("lock; btsl %2, %1\n\t"
                         "sbbl %0,%0"
                         : "=r" (uBit),
                           "=m" (*(__volatile__ unsigned *)pv)
                         : "0"  (uBit)
                         : "memory");
    return uBit;
}


/**
 * Atomically clears a bit.
 *
 * @param   pv      Pointer to base of bitmap.
 * @param   uBit    Bit in question.
 */
static __inline__ void __atomic_clear_bit(__volatile__ void *pv, unsigned uBit)
{
    __asm__ __volatile__("lock; btrl %1, %0"
                         : "=m" (*(__volatile__ unsigned *)pv)
                         : "r" (uBit));
}


/**
 * Atomically (er?) tests if a bit is set.
 *
 * @returns non zero if the bit was set.
 * @returns 0 if the bit was clear.
 * @param   pv      Pointer to base of bitmap.
 * @param   uBit    Bit in question.
 */
static __inline__ int __atomic_test_bit(const __volatile__ void *pv, unsigned uBit)
{
    __asm__ __volatile__("btl %0, %1\n\t"
                         "sbbl %0, %0\t\n"
                         : "=r" (uBit)
                         : "m"  (*(const __volatile__ unsigned *)pv),
                           "0"  (uBit));
    return uBit;
}


/**
 * Atomically add a 32-bit unsigned value to another.
 *
 * @param   pu      Pointer to the value to add to.
 * @param   uAdd    The value to add to *pu.
 */
static __inline__ void __atomic_add(__volatile__ unsigned *pu, const unsigned uAdd)
{
    __asm__ __volatile__("lock; addl %1, %0"
                         : "=m" (*pu)
                         : "nr" (uAdd),
                           "m"  (*pu));
}

/**
 * Atomically subtract a 32-bit unsigned value from another.
 *
 * @param   pu      Pointer to the value to subtract from.
 * @param   uAdd    The value to subtract from *pu.
 */
static __inline__ void __atomic_sub(__volatile__ unsigned *pu, const unsigned uSub)
{
    __asm__ __volatile__("lock; subl %1, %0"
                         : "=m" (*pu)
                         : "nr" (uSub),
                           "m"  (*pu));
}

/**
 * Atomically increments a 32-bit unsigned value.
 *
 * @param   pu      Pointer to the value to increment.
 */
static __inline__ void __atomic_increment(__volatile__ unsigned *pu)
{
    __asm__ __volatile__("lock; incl %0"
                         : "=m" (*pu)
                         : "m"  (*pu));
}

/**
 * Atomically increments a 32-bit unsigned value.
 *
 * @returns The new value.
 * @param   pu32    Pointer to the value to increment.
 */
static __inline__ uint32_t __atomic_increment_u32(uint32_t __volatile__ *pu32)
{
    uint32_t u32;
    __asm__ __volatile__("lock; xadd %0, %1\n\t"
                         "incl %0\n\t"
                         : "=r" (u32),
                           "=m" (*pu32)
                         : "0" (1)
                         : "memory");
    return u32;
}

/**
 * Atomically increments a 32-bit signed value.
 *
 * @returns The new value.
 * @param   pi32    Pointer to the value to increment.
 */
static __inline__ int32_t __atomic_increment_s32(int32_t __volatile__ *pi32)
{
    int32_t i32;
    __asm__ __volatile__("lock; xadd %0, %1\n\t"
                         "incl %0\n\t"
                         : "=r" (i32),
                           "=m" (*pi32)
                         : "0" (1)
                         : "memory");
    return i32;
}

/**
 * Atomically increments a 16-bit unsigned value.
 *
 * @param   pu16    Pointer to the value to increment.
 */
static __inline__ void __atomic_increment_u16(uint16_t __volatile__ *pu16)
{
    __asm__ __volatile__("lock; incw %0"
                         : "=m" (*pu16)
                         : "m"  (*pu16));
}

/**
 * Atomically decrements a 32-bit unsigned value.
 *
 * @param   pu      Pointer to the value to decrement.
 */
static __inline__ void __atomic_decrement(__volatile__ unsigned *pu)
{
    __asm__ __volatile__("lock; decl %0"
                         : "=m" (*pu)
                         : "m"  (*pu));
}

/**
 * Atomically decrements a 32-bit unsigned value.
 *
 * @returns The new value.
 * @param   pu32      Pointer to the value to decrement.
 */
static __inline__ uint32_t __atomic_decrement_u32(__volatile__ uint32_t *pu32)
{
    uint32_t u32;
    __asm__ __volatile__("lock; xadd %0, %1\n\t"
                         "decl %0\n\t"
                         : "=r" (u32),
                           "=m" (*pu32)
                         : "0" (-1)
                         : "memory");
    return u32;
}

/**
 * Atomically decrements a 32-bit signed value.
 *
 * @returns The new value.
 * @param   pi32    Pointer to the value to decrement.
 */
static __inline__ int32_t __atomic_decrement_s32(__volatile__ int32_t *pi32)
{
    int32_t i32;
    __asm__ __volatile__("lock; xadd %0, %1\n\t"
                         "decl %0\n\t"
                         : "=r" (i32),
                           "=m" (*pi32)
                         : "0" (-1)
                         : "memory");
    return i32;
}

/**
 * Atomically decrements a 16-bit unsigned value.
 *
 * @returns The new value.
 * @param   pu16    Pointer to the value to decrement.
 */
static __inline__ void __atomic_decrement_u16(uint16_t __volatile__ *pu16)
{
    __asm__ __volatile__("lock; decw %0"
                         : "=m" (*pu16)
                         : "m"  (*pu16));
}

/**
 * Atomically increments a 32-bit unsigned value if less than max.
 *
 * @returns 0 if incremented.
 * @returns uMax when not updated.
 * @param   pu      Pointer to the value to increment.
 * @param   uMax    *pu must not be above this value.
 */
static __inline__ int __atomic_increment_max(__volatile__ unsigned *pu, const unsigned uMax)
{
    unsigned rc = 0;
    __asm__ __volatile__("movl  %2, %%eax\n\t"
                         "1:\n\t"
                         "movl  %%eax, %0\n\t"
                         "cmpl  %3, %0\n\t"
                         "jb    2f\n\t"
                         "jmp   4f\n"
                         "2:\n\t"
                         "incl  %0\n\t"
                         "lock; cmpxchgl %0, %1\n\t"
                         "jz    3f\n\t"
                         "jmp   1b\n"
                         "3:"
                         "xorl  %0, %0\n\t"
                         "4:"
                         : "=b" (rc),
                           "=m" (*pu)
                         : "m"  (*pu),
                           "nd" (uMax),
                           "0"  (rc)
                         : "%eax");
    return rc;
}


/**
 * Atomically increments a 16-bit unsigned value if less than max.
 *
 * @returns New value.
 * @returns Current value | 0xffff0000 if current value is less or equal to u16Min.
 * @param   pu16    Pointer to the value to increment.
 * @param   u16Max  *pu16 must not be above this value after the incrementation.
 */
static inline unsigned __atomic_increment_word_max(volatile uint16_t *pu16, const uint16_t u16Max)
{
    unsigned rc = 0;
    __asm__ __volatile__("movw  %2, %%ax\n\t"
                         "1:\n\t"
                         "movw  %%ax, %w0\n\t"
                         "cmpw  %w3, %%ax\n\t"
                         "jb    2f\n\t"
                         "orl   $0xffff0000, %0\n\t"
                         "jmp   3f\n"
                         "2:\n\t"
                         "incw  %w0\n\t"
                         "lock; cmpxchgw %w0, %2\n\t"
                         "jz    3f\n\t"
                         "jmp   1b\n\t"
                         "3:"
                         : "=r" (rc),
                           "=m" (*pu16)
                         : "m"  (*pu16),
                           "nr"  (u16Max),
                           "0"  (rc)
                         : "%eax");
    return rc;
}


/**
 * Atomically decrements a 32-bit unsigned value if greater than a min.
 *
 * @returns 0 if decremented.
 * @returns uMin when not updated.
 * @param   pu      Pointer to the   value to decrement.
 * @param   uMin    *pu must not be below this value.
 */
static __inline__ int __atomic_decrement_min(__volatile__ unsigned *pu, const unsigned uMin)
{
    unsigned rc = 0;
    __asm__ __volatile__("movl  %2, %%eax\n"
                         "1:\n\t"
                         "movl  %%eax, %0\n\t"
                         "cmpl  %3, %0\n\t"
                         "ja    2f\n\t"
                         "jmp   4f\n"
                         "2:\n\t"
                         "decl  %0\n\t"
                         "lock; cmpxchgl %0, %1\n\t"
                         "jz    3f\n\t"
                         "jmp   1b\n"
                         "3:"
                         "xorl  %0, %0\n\t"
                         "4:"
                         : "=b" (rc),
                           "=m" (*pu)
                         : "m"  (*pu),
                           "nr" (uMin),
                           "0"  (rc)
                         : "%eax");
    return rc;
}


/**
 * Atomically decrements a 16-bit unsigned value if greater than a min.
 *
 * @returns New value.
 * @returns Current value | 0xffff0000 if current value is less or equal to u16Min.
 * @param   pu16    Pointer to the  value to decrement.
 * @param   u16Min  *pu16 must not be below this value after the decrementation.
 */
static inline unsigned __atomic_decrement_word_min(volatile uint16_t *pu16, const uint16_t u16Min)
{
    unsigned rc = 0;
    __asm__ __volatile__("movw  %2, %%ax\n\t"
                         "1:\n\t"
                         "movw  %%ax, %w0\n\t"
                         "cmpw  %w3, %%ax\n\t"
                         "ja    2f\n\t"
                         "orl   $0xffff0000, %0\n\t"
                         "jmp   3f\n"
                         "2:\n\t"
                         "decw  %%bx\n\t"
                         "lock; cmpxchgw %w0, %1\n\t"
                         "jz    3f\n\t"
                         "jmp   1b\n"
                         "3:"
                         : "=b" (rc),
                           "=m" (*pu16)
                         : "m"  (*pu16),
                           "nr" (u16Min),
                           "0"  (rc)
                         : "%eax");
    return rc;
}


/**
 * Atomically compare and exchange a 32-bit word.
 *
 * @returns 1 if changed, 0 if unchanged (i.e. boolean).
 * @param   pu32    Pointer to the value to compare & exchange.
 * @param   u32New  The new value.
 * @param   u32Cur  The current value. Only update if *pu32 equals this one.
 */
static inline unsigned __atomic_cmpxchg32(volatile uint32_t *pu32, uint32_t u32New, uint32_t u32Old)
{
    __asm__ __volatile__("lock; cmpxchgl %2, %1\n\t"
                         "setz  %%al\n\t"
                         "movzx %%al, %%eax\n\t"
                         : "=a" (u32Old),
                           "=m" (*pu32)
                         : "r"  (u32New),
                           "0" (u32Old));
    return (unsigned)u32Old;
}


#define __ROTATE_FUN(F,I,T) \
  static __inline__ T F (T value, int shift) \
  { \
    __asm__ (I " %b2, %0" : "=g"(value) : "0"(value), "c"(shift) : "cc"); \
    return value; \
  } \
  static __inline__ T F##1 (T value) \
  { \
    __asm__ (I " $1, %0" : "=g"(value) : "0"(value) : "cc"); \
    return value; \
  }

#define __ROTATE(V,S,F) ((__builtin_constant_p (S) && (int)(S) == 1) \
                         ? F##1 (V) : F (V, S))

__ROTATE_FUN (__crotr, "rorb", unsigned char)
__ROTATE_FUN (__srotr, "rorw", unsigned short)
__ROTATE_FUN (__lrotr, "rorl", unsigned long)

__ROTATE_FUN (__crotl, "rolb", unsigned char)
__ROTATE_FUN (__srotl, "rolw", unsigned short)
__ROTATE_FUN (__lrotl, "roll", unsigned long)

#define _crotr(V,S) __ROTATE (V, S, __crotr)
#define _srotr(V,S) __ROTATE (V, S, __srotr)
#define _lrotr(V,S) __ROTATE (V, S, __lrotr)
#define _crotl(V,S) __ROTATE (V, S, __crotl)
#define _srotl(V,S) __ROTATE (V, S, __srotl)
#define _lrotl(V,S) __ROTATE (V, S, __lrotl)

#define _rotr(V,S) _lrotr (V, S)
#define _rotl(V,S) _lrotl (V, S)


static __inline__ int __fls (int v)
{
  int r;

  __asm__ __volatile__ ("bsrl %1, %0;"
                        "jnz 1f;"
                        "movl $-1, %0;"
                        ".align 2, 0x90;"
                        "1:"
                        : "=r"(r) : "r"(v) : "cc");
  return r + 1;
}

/* Quick routines similar to div() and friends, but inline */

static __inline__ long __ldivmod (long num, long den, long *rem)
{
  long q, r;
  __asm__ ("cltd; idivl %2"
           : "=a" (q), "=&d" (r)
           : "r?m" (den), "a" (num));
  *rem = r;
  return q;
}

static __inline__ unsigned long __uldivmod (unsigned long num,
  unsigned long den, unsigned long *rem)
{
  unsigned long q, r;
  __asm__ ("xorl %%edx,%%edx; divl %2"
           : "=a" (q), "=&d" (r)
           : "r?m" (den), "a" (num));
  *rem = r;
  return q;
}

/*
    Divide a 64-bit integer by a 32-bit one:

    A*2^32 + B    A            B + (A mod 32)
    ---------- = --- * 2^32 + ----------------
        C         C                  C
*/
static __inline__ long long __lldivmod (long long num, long den, long *rem)
{
  long long q;
  long r;
  __asm__ ("	movl	%%eax,%%esi;"
           "	movl	%%edx,%%eax;"
           "	pushl	%%edx;"
           "	cltd;"
           "	idivl	%2;"
           "	;"
/* Now ensure remainder is smallest of possible two values (negative and
   positive). For this we compare the remainder with positive and negative
   denominator/2; if it is smaller than one and bigger than another we
   consider it optimal, otherwise it can be made smaller by adding or
   subtracting denominator to it. This is done to ensure no overflow
   will occur at next division. */
           "	movl	%2,%%ecx;"
           "	sarl	$1,%%ecx;"	/* ecx = den/2 */
           "	cmpl	%%ecx,%%edx;"
           "	setl	%%bl;"
           "	negl	%%ecx;"
           "	cmpl	%%ecx,%%edx;"
           "	setl	%%bh;"
           "	xorb	%%bh,%%bl;"
           "	jnz	1f;"		/* Remainder is between -den/2...den/2 */
           "	;"
/* If remainder has same sign as denominator, we have to do r -= den; q++;
   otherwise we have to do r += den; q--; */
           "	movl	%2,%%ebx;"	/* ebx = den */
           "	xorl	%%edx,%%ebx;"	/* r ^ den */
           "	js	0f;"		/* Different signs */
           "	subl	%2,%%edx;"	/* r -= den */
           "	addl	$1,%%eax;"	/* q++ */
           "	adcl	$0,%%edx;"
           "	jmp	1f;"
           "	;"
           "0:	addl	%2,%%edx;"	/* r += den */
           "	subl	$1,%%eax;"	/* q-- */
           "	sbbl	$0,%%edx;"
           "	;"
           "1:	xchgl	%%eax,%%esi;"
           "	idivl	%2;"
           "	;"
           "	movl	%%edx,%1;"
           "	cltd;"
           "	addl	%%esi,%%edx;"
           "	;"
/* Check if numerator has the same sign as remainder; if they have different
   sign we should make the remainder have same sign as numerator to comply
   with ANSI standard, which says we always should truncate the quotient
   towards zero. */
           "	popl	%%ebx;"		/* ebx = num >> 32 */
           "	xorl	%1,%%ebx;"	/* sign(r) ^ sign(num) */
           "	jns	3f;"		/* jump if same sign */
           "	;"
/* If remainder has same sign as denominator, we have to do r -= den; q++;
   otherwise we have to do r += den; q--; */
           "	movl	%2,%%ebx;"
           "	xorl	%1,%%ebx;"	/* r ^ den */
           "	js	2f;"		/* Different signs */
           "	subl	%2,%1;"		/* r -= den */
           "	addl	$1,%%eax;"	/* q++ */
           "	adcl	$0,%%edx;"
           "	jmp	3f;"
           "	;"
           "2:	addl	%2,%1;"		/* r += den */
           "	subl	$1,%%eax;"	/* q-- */
           "	sbbl	$0,%%edx;"
           "	;"
           "3:	;"
           : "=A" (q), "=&c" (r)
           : "r" (den), "A" (num)
           : "ebx", "esi");
  *rem = r;
  return q;
}

/*
    Same as __lldivmod except that if A < C, we can do just one division
    instead of two because the result is always a 32-bit integer.
*/
static __inline__ unsigned long long __ulldivmod (unsigned long long num,
  unsigned long den, unsigned long *rem)
{
  unsigned long long q;
  unsigned long r;
  __asm__ ("	movl	%%eax,%1;"
           "	movl	%%edx,%%eax;"
           "	xorl	%%edx,%%edx;"
           "	divl	%2;"
           "	xchgl	%%eax,%%ecx;"
           "	divl	%2;"
           "	xchgl	%%edx,%1;"
           : "=A" (q), "=c" (r)
           : "r?m" (den), "A" (num));
  *rem = r;
  return q;
}

__END_DECLS
#endif /* not _I386_BUILTIN_H */
