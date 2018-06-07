/*
 * MODIFIED math.h adding _STD() to all the functions which requires this,
 * this saves a lot of annoying source changes!
 * This is not mentv for compiling!
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/*
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $FreeBSD: src/lib/msun/src/math.h,v 1.61 2005/04/16 21:12:47 das Exp $
 */

#ifndef _MATH_H_
#define	_MATH_H_

#include <sys/cdefs.h>
#include <sys/_types.h>
#include <machine/_limits.h>

/*
 * ANSI/POSIX
 */
extern const union __infinity_un {
	unsigned char	__uc[8];
	double		__ud;
} __infinity;

extern const union __nan_un {
	unsigned char	__uc[sizeof(float)];
	float		__uf;
} __nan;

#if __GNUC_PREREQ__(3, 3) || (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 800)
#define	__MATH_BUILTIN_CONSTANTS
#endif

#if __GNUC_PREREQ__(3, 0) && !defined(__INTEL_COMPILER)
#define	__MATH_BUILTIN_RELOPS
#endif

#ifdef __MATH_BUILTIN_CONSTANTS
#define	HUGE_VAL	__builtin_huge_val()
#else
#define	HUGE_VAL	(__infinity.__ud)
#endif

#if __ISO_C_VISIBLE >= 1999
#define	FP_ILOGB0	(-__INT_MAX)
#define	FP_ILOGBNAN	__INT_MAX

#ifdef __MATH_BUILTIN_CONSTANTS
#define	HUGE_VALF	__builtin_huge_valf()
#define	HUGE_VALL	__builtin_huge_vall()
#define	INFINITY	__builtin_inf()
#define	NAN		__builtin_nan("")
#else
#define	HUGE_VALF	(float)HUGE_VAL
#define	HUGE_VALL	(long double)HUGE_VAL
#define	INFINITY	HUGE_VALF
#define	NAN		(__nan.__uf)
#endif /* __MATH_BUILTIN_CONSTANTS */

#define	MATH_ERRNO	1
#define	MATH_ERREXCEPT	2
#define	math_errhandling	MATH_ERREXCEPT

/* XXX We need a <machine/math.h>. */
#if defined(__ia64__) || defined(__sparc64__)
#define	FP_FAST_FMA
#endif
#ifdef __ia64__
#define	FP_FAST_FMAL
#endif
#define	FP_FAST_FMAF

/* Symbolic constants to classify floating point numbers. */
#define	FP_INFINITE	0x01
#define	FP_NAN		0x02
#define	FP_NORMAL	0x04
#define	FP_SUBNORMAL	0x08
#define	FP_ZERO		0x10
#define	fpclassify(x) \
    ((sizeof (x) == sizeof (float)) ? __fpclassifyf(x) \
    : (sizeof (x) == sizeof (double)) ? __fpclassifyd(x) \
    : __fpclassifyl(x))

#define	isfinite(x)					\
    ((sizeof (x) == sizeof (float)) ? __isfinitef(x)	\
    : (sizeof (x) == sizeof (double)) ? __isfinite(x)	\
    : __isfinitel(x))
#define	isinf(x)					\
    ((sizeof (x) == sizeof (float)) ? __isinff(x)	\
    : (sizeof (x) == sizeof (double)) ? isinf(x)	\
    : __isinfl(x))
#define	isnan(x)					\
    ((sizeof (x) == sizeof (float)) ? isnanf(x)		\
    : (sizeof (x) == sizeof (double)) ? isnan(x)	\
    : __isnanl(x))
#define	isnormal(x)					\
    ((sizeof (x) == sizeof (float)) ? __isnormalf(x)	\
    : (sizeof (x) == sizeof (double)) ? __isnormal(x)	\
    : __isnormall(x))

#ifdef __MATH_BUILTIN_RELOPS
#define	isgreater(x, y)		__builtin_isgreater((x), (y))
#define	isgreaterequal(x, y)	__builtin_isgreaterequal((x), (y))
#define	isless(x, y)		__builtin_isless((x), (y))
#define	islessequal(x, y)	__builtin_islessequal((x), (y))
#define	islessgreater(x, y)	__builtin_islessgreater((x), (y))
#define	isunordered(x, y)	__builtin_isunordered((x), (y))
#else
#define	isgreater(x, y)		(!isunordered((x), (y)) && (x) > (y))
#define	isgreaterequal(x, y)	(!isunordered((x), (y)) && (x) >= (y))
#define	isless(x, y)		(!isunordered((x), (y)) && (x) < (y))
#define	islessequal(x, y)	(!isunordered((x), (y)) && (x) <= (y))
#define	islessgreater(x, y)	(!isunordered((x), (y)) && \
					((x) > (y) || (y) > (x)))
#define	isunordered(x, y)	(isnan(x) || isnan(y))
#endif /* __MATH_BUILTIN_RELOPS */

#define	signbit(x)					\
    ((sizeof (x) == sizeof (float)) ? __signbitf(x)	\
    : (sizeof (x) == sizeof (double)) ? __signbit(x)	\
    : __signbitl(x))

typedef	__double_t	double_t;
typedef	__float_t	float_t;
#endif /* __ISO_C_VISIBLE >= 1999 */

/*
 * XOPEN/SVID
 */
#if __BSD_VISIBLE || __XSI_VISIBLE
#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

#define	MAXFLOAT	((float)3.40282346638528860e+38)
extern int _STD(signgam);
#endif /* __BSD_VISIBLE || __XSI_VISIBLE */

#if __BSD_VISIBLE
#if 0
/* Old value from 4.4BSD-Lite math.h; this is probably better. */
#define	HUGE		HUGE_VAL
#else
#define	HUGE		MAXFLOAT
#endif
#endif /* __BSD_VISIBLE */

/*
 * Most of these functions depend on the rounding mode and have the side
 * effect of raising floating-point exceptions, so they are not declared
 * as __pure2.  In C99, FENV_ACCESS affects the purity of these functions.
 */
__BEGIN_DECLS
/*
 * ANSI/POSIX
 */
int	__fpclassifyd(double) __pure2;
int	__fpclassifyf(float) __pure2;
int	__fpclassifyl(long double) __pure2;
int	__isfinitef(float) __pure2;
int	__isfinite(double) __pure2;
int	__isfinitel(long double) __pure2;
int	__isinff(float) __pure2;
int	__isinfl(long double) __pure2;
int	__isnanl(long double) __pure2;
int	__isnormalf(float) __pure2;
int	__isnormal(double) __pure2;
int	__isnormall(long double) __pure2;
int	__signbit(double) __pure2;
int	__signbitf(float) __pure2;
int	__signbitl(long double) __pure2;

double	_STD(acos)(double);
double	_STD(asin)(double);
double	_STD(atan)(double);
double	_STD(atan2)(double, double);
double	_STD(cos)(double);
double	_STD(sin)(double);
double	_STD(tan)(double);

double	_STD(cosh)(double);
double	_STD(sinh)(double);
double	_STD(tanh)(double);

double	_STD(exp)(double);
double	_STD(frexp)(double, int *);	/* fundamentally !__pure2 */
double	_STD(ldexp)(double, int);
double	_STD(log)(double);
double	_STD(log10)(double);
double	_STD(modf)(double, double *);	/* fundamentally !__pure2 */

double	_STD(pow)(double, double);
double	_STD(sqrt)(double);

double	_STD(ceil)(double);
double	_STD(fabs)(double) __pure2;
double	_STD(floor)(double);
double	_STD(fmod)(double, double);

/*
 * These functions are not in C90.
 */
#if __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 || __XSI_VISIBLE
double	_STD(acosh)(double);
double	_STD(asinh)(double);
double	_STD(atanh)(double);
double	_STD(cbrt)(double);
double	_STD(erf)(double);
double	_STD(erfc)(double);
double	_STD(exp2)(double);
double	_STD(expm1)(double);
double	_STD(fma)(double, double, double);
double	_STD(hypot)(double, double);
int	_STD(ilogb)(double) __pure2;
int	_STD(isinf)(double) __pure2;
int	_STD(isnan)(double) __pure2;
double	_STD(lgamma)(double);
long long _STD(llrint)(double);
long long _STD(llround)(double);
double	_STD(log1p)(double);
double	_STD(logb)(double);
long	_STD(lrint)(double);
long	_STD(lround)(double);
double	_STD(nextafter)(double, double);
double	_STD(remainder)(double, double);
double	_STD(remquo)(double, double, int *);
double	_STD(rint)(double);
#endif /* __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 || __XSI_VISIBLE */

#if __BSD_VISIBLE || __XSI_VISIBLE
double	_STD(j0)(double);
double	_STD(j1)(double);
double	_STD(jn)(int, double);
double	_STD(scalb)(double, double);
double	_STD(y0)(double);
double	_STD(y1)(double);
double	_STD(yn)(int, double);

#if __XSI_VISIBLE <= 500 || __BSD_VISIBLE
double	_STD(gamma)(double);
#endif
#endif /* __BSD_VISIBLE || __XSI_VISIBLE */

#if __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999
double	_STD(copysign)(double, double) __pure2;
double	_STD(fdim)(double, double);
double	_STD(fmax)(double, double) __pure2;
double	_STD(fmin)(double, double) __pure2;
double	_STD(nearbyint)(double);
double	_STD(round)(double);
double	_STD(scalbln)(double, long);
double	_STD(scalbn)(double, int);
double	_STD(tgamma)(double);
double	_STD(trunc)(double);
#endif

/*
 * BSD math library entry points
 */
#if __BSD_VISIBLE
double _STD(drem)(double, double);
int	_STD(finite)(double) __pure2;
int	_STD(isnanf)(float) __pure2;

/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
double	_STD(gamma_r)(double, int *);
double	_STD(lgamma_r)(double, int *);

/*
 * IEEE Test Vector
 */
double	_STD(significand)(double);
#endif /* __BSD_VISIBLE */

/* float versions of ANSI/POSIX functions */
#if __ISO_C_VISIBLE >= 1999
float	_STD(acosf)(float);
float	_STD(asinf)(float);
float	_STD(atanf)(float);
float	_STD(atan2f)(float, float);
float	_STD(cosf)(float);
float	_STD(sinf)(float);
float	_STD(tanf)(float);

float	_STD(coshf)(float);
float	_STD(sinhf)(float);
float	_STD(tanhf)(float);

float	_STD(exp2f)(float);
float	_STD(expf)(float);
float	_STD(expm1f)(float);
float	_STD(frexpf)(float, int *);	/* fundamentally !__pure2 */
int	_STD(ilogbf)(float) __pure2;
float	_STD(ldexpf)(float, int);
float	_STD(log10f)(float);
float	_STD(log1pf)(float);
float	_STD(logf)(float);
float	_STD(modff)(float, float *);	/* fundamentally !__pure2 */

float	_STD(powf)(float, float);
float	_STD(sqrtf)(float);

float	_STD(ceilf)(float);
float	_STD(fabsf)(float) __pure2;
float	_STD(floorf)(float);
float	_STD(fmodf)(float, float);
float	_STD(roundf)(float);

float	_STD(erff)(float);
float	_STD(erfcf)(float);
float	_STD(hypotf)(float, float);
float	_STD(lgammaf)(float);

float	_STD(acoshf)(float);
float	_STD(asinhf)(float);
float	_STD(atanhf)(float);
float	_STD(cbrtf)(float);
float	_STD(logbf)(float);
float	_STD(copysignf)(float, float) __pure2;
long long _STD(llrintf)(float);
long long _STD(llroundf)(float);
long	_STD(lrintf)(float);
long	_STD(lroundf)(float);
float	_STD(nearbyintf)(float);
float	_STD(nextafterf)(float, float);
float	_STD(remainderf)(float, float);
float	_STD(remquof)(float, float, int *);
float	_STD(rintf)(float);
float	_STD(scalblnf)(float, long);
float	_STD(scalbnf)(float, int);
float	_STD(truncf)(float);

float	_STD(fdimf)(float, float);
float	_STD(fmaf)(float, float, float);
float	_STD(fmaxf)(float, float) __pure2;
float	_STD(fminf)(float, float) __pure2;
#endif

/*
 * float versions of BSD math library entry points
 */
#if __BSD_VISIBLE
float	_STD(dremf)(float, float);
int	_STD(finitef)(float) __pure2;
float	_STD(gammaf)(float);
float	_STD(j0f)(float);
float	_STD(j1f)(float);
float	_STD(jnf)(int, float);
float	_STD(scalbf)(float, float);
float	_STD(y0f)(float);
float	_STD(y1f)(float);
float	_STD(ynf)(int, float);

/*
 * Float versions of reentrant version of gamma & lgamma; passes
 * signgam back by reference as the second argument; user must
 * allocate space for signgam.
 */
float	_STD(gammaf_r)(float, int *);
float	_STD(lgammaf_r)(float, int *);

/*
 * float version of IEEE Test Vector
 */
float	_STD(significandf)(float);
#endif	/* __BSD_VISIBLE */

/*
 * long double versions of ISO/POSIX math functions
 */
#if __ISO_C_VISIBLE >= 1999
#if 0
long double	_STD(acoshl)(long double);
long double	_STD(acosl)(long double);
long double	_STD(asinhl)(long double);
long double	_STD(asinl)(long double);
long double	_STD(atan2l)(long double, long double);
long double	_STD(atanhl)(long double);
long double	_STD(atanl)(long double);
long double	_STD(cbrtl)(long double);
#endif
long double	_STD(ceill)(long double);
long double	_STD(copysignl)(long double, long double) __pure2;
#if 0
long double	_STD(coshl)(long double);
long double	_STD(cosl)(long double);
long double	_STD(erfcl)(long double);
long double	_STD(erfl)(long double);
long double	_STD(exp2l)(long double);
long double	_STD(expl)(long double);
long double	_STD(expm1l)(long double);
#endif
long double	_STD(fabsl)(long double) __pure2;
long double	_STD(fdiml)(long double, long double);
long double	_STD(floorl)(long double);
long double	_STD(fmal)(long double, long double, long double);
long double	_STD(fmaxl)(long double, long double) __pure2;
long double	_STD(fminl)(long double, long double) __pure2;
#if 0
long double	_STD(fmodl)(long double, long double);
#endif
long double	_STD(frexpl)(long double value, int *); /* fundamentally !__pure2 */
#if 0
long double	_STD(hypotl)(long double, long double);
#endif
int		_STD(ilogbl)(long double) __pure2;
long double	_STD(ldexpl)(long double, int);
#if 0
long double	_STD(lgammal)(long double);
long long	_STD(llrintl)(long double);
#endif
long long	_STD(llroundl)(long double);
#if 0
long double	_STD(log10l)(long double);
long double	_STD(log1pl)(long double);
long double	_STD(log2l)(long double);
long double	_STD(logbl)(long double);
long double	_STD(logl)(long double);
long		_STD(lrintl)(long double);
#endif
long		_STD(lroundl)(long double);
#if 0
long double	_STD(modfl)(long double, long double *); /* fundamentally !__pure2 */
long double	nanl(const char *) __pure2; /* gcc intrinsic */
long double	_STD(nearbyintl)(long double);
#endif
long double	_STD(nextafterl)(long double, long double);
double		_STD(nexttoward)(double, long double);
float		_STD(nexttowardf)(float, long double);
long double	_STD(nexttowardl)(long double, long double);
#if 0
long double	_STD(powl)(long double, long double);
long double	_STD(remainderl)(long double, long double);
long double	_STD(remquol)(long double, long double, int *);
long double	_STD(rintl)(long double);
#endif
long double	_STD(roundl)(long double);
long double	_STD(scalblnl)(long double, long);
long double	_STD(scalbnl)(long double, int);
#if 0
long double	_STD(sinhl)(long double);
long double	_STD(sinl)(long double);
long double	_STD(sqrtl)(long double);
long double	_STD(tanhl)(long double);
long double	_STD(tanl)(long double);
long double	_STD(tgammal)(long double);
#endif
long double	_STD(truncl)(long double);

#endif /* __ISO_C_VISIBLE >= 1999 */

#ifdef __USE_GNU
void _STD(sincos)(double, double *, double *);
void _STD(sincosf)(float, float *, float *);
void _STD(sincosl)(long double, long double *, long double *);
float _STD(exp10f)(float);
double _STD(exp10)(double);
long double _STD(exp10l)(long double);
float _STD(log2f)(float);
double _STD(log2)(double);
long double _STD(log2l)(long double);
float _STD(tgammaf)(float);
long double _STD(significandl)(long double);
long double _STD(j0l)(long double);
long double _STD(j1l)(long double);
long double _STD(jnl)(int, long double);
long double _STD(scalbl)(long double, long double);
long double _STD(y0l)(long double);
long double _STD(y1l)(long double);
long double _STD(ynl)(int, long double);
long double _STD(gammal)(long double);
long double _STD(gammal_r)(long double,int *);
long double _STD(lgammal_r)(long double,int *);

#endif
__END_DECLS

#endif /* !_MATH_H_ */
