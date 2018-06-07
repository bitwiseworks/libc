/*-
 * Copyright (c) 2001 The FreeBSD Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/include/complex.h,v 1.6 2004/08/14 18:03:21 stefanf Exp $
 */

/**
 * FreeBSD 5.3
 */

#ifndef _COMPLEX_H
#define	_COMPLEX_H

#ifdef __GNUC__
#if __STDC_VERSION__ < 199901
#define	_Complex	__complex__
#endif
#define	_Complex_I	1.0fi
#endif

#define	complex		_Complex
#define	I		_Complex_I

#include <sys/cdefs.h>

__BEGIN_DECLS

double		_STD(cabs)(double complex);
float		_STD(cabsf)(float complex);
double		_STD(cimag)(double complex);
float		_STD(cimagf)(float complex);
long double	_STD(cimagl)(long double complex);
double complex	_STD(conj)(double complex);
float complex	_STD(conjf)(float complex);
long double complex
		_STD(conjl)(long double complex);
double		_STD(creal)(double complex);
float		_STD(crealf)(float complex);
long double	_STD(creall)(long double complex);

/* bird: */
long double          _STD(cabsl)(long double complex);
double complex       _STD(cacos)(double complex);
float complex        _STD(cacosf)(float complex);
double complex       _STD(cacosh)(double complex);
float complex        _STD(cacoshf)(float complex);
long double complex  _STD(cacoshl)(long double complex);
long double complex  _STD(cacosl)(long double complex);
double               _STD(carg)(double complex);
float                _STD(cargf)(float complex);
long double          _STD(cargl)(long double complex);
double complex       _STD(casin)(double complex);
float complex        _STD(casinf)(float complex);
double complex       _STD(casinh)(double complex);
float complex        _STD(casinhf)(float complex);
long double complex  _STD(casinhl)(long double complex);
long double complex  _STD(casinl)(long double complex);
double complex       _STD(catan)(double complex);
float complex        _STD(catanf)(float complex);
double complex       _STD(catanh)(double complex);
float complex        _STD(catanhf)(float complex);
long double complex  _STD(catanhl)(long double complex);
long double complex  _STD(catanl)(long double complex);
double complex       _STD(ccos)(double complex);
float complex        _STD(ccosf)(float complex);
double complex       _STD(ccosh)(double complex);
float complex        _STD(ccoshf)(float complex);
long double complex  _STD(ccoshl)(long double complex);
long double complex  _STD(ccosl)(long double complex);
double complex       _STD(cexp)(double complex);
float complex        _STD(cexpf)(float complex);
long double complex  _STD(cexpl)(long double complex);
double complex       _STD(clog)(double complex);
float complex        _STD(clogf)(float complex);
long double complex  _STD(clogl)(long double complex);
double complex       _STD(cpow)(double complex, double complex);
float complex        _STD(cpowf)(float complex, float complex);
long double complex  _STD(cpowl)(long double complex, long double complex);
double complex       _STD(cproj)(double complex);
float complex        _STD(cprojf)(float complex);
long double complex  _STD(cprojl)(long double complex);
double complex       _STD(csin)(double complex);
float complex        _STD(csinf)(float complex);
double complex       _STD(csinh)(double complex);
float complex        _STD(csinhf)(float complex);
long double complex  _STD(csinhl)(long double complex);
long double complex  _STD(csinl)(long double complex);
double complex       _STD(csqrt)(double complex);
float complex        _STD(csqrtf)(float complex);
long double complex  _STD(csqrtl)(long double complex);
double complex       _STD(ctan)(double complex);
float complex        _STD(ctanf)(float complex);
double complex       _STD(ctanh)(double complex);
float complex        _STD(ctanhf)(float complex);
long double complex  _STD(ctanhl)(long double complex);
long double complex  _STD(ctanl)(long double complex);

double complex       _STD(clog10)(double complex);
float complex        _STD(clog10f)(float complex);
long double complex  _STD(clog10l)(long double complex);

__END_DECLS

#endif /* _COMPLEX_H */

