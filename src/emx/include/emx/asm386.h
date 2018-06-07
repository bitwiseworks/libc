/* emx/asm386.h (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include <sys/errno.h>

/* Macros for defining standard libc functions */

#define _STD(x) __std_ ## x

#define _xam fxam; fstsw %ax; andb $0x45, %ah

#define j_nan cmpb $0x01, %ah; je
#define j_inf cmpb $0x05, %ah; je

/* MATHSUFFIX1 is for sin() vs. sinf() vs. sinl() */
/* MATHSUFFIX2 is for _sin() vs. _sinf() vs. _sinl() */
/* MATHSUFFIX3 is for __sin() vs. __sinf() vs. __sinl() + defined sin... */

#if defined (LONG_DOUBLE)
#define FLD fldt
#define MATHSUFFIX1(X)  _STD(X##l)
#define MATHSUFFIX2(X)  __##X##l
#define MATHSUFFIX3(X)  ___##X##l
#define CONV(X)
#elif defined (FLOAT)
#define FLD flds
#define MATHSUFFIX1(X)  _STD(X##f)
#define MATHSUFFIX2(X)  __##X##f
#define MATHSUFFIX3(X)  ___##X##f
#define CONV(X) fstps X; flds X
#else
#define FLD fldl
#define MATHSUFFIX1(X)  _STD(X)
#define MATHSUFFIX2(X)  __##X
#define MATHSUFFIX3(X)  ___##X
#define CONV(X) fstpl X; fldl X
#endif

#define LABEL0(name)    _##name
#define LABEL(name)     LABEL0(name)

#define ALIGN   .align  4, 0x90
#define ALIGNP2(a) .align  a, 0x90

#define SET_ERRNO_CONST(x) \
        call    __errno ;\
        movl    x, (%eax)

#if defined (__GPROF__)
#define PROFILE_FRAME	call __mcount
#define PROFILE_NOFRAME	pushl %ebp; movl %esp, %ebp; call __mcount; popl %ebp
#else
#define PROFILE_FRAME
#define PROFILE_NOFRAME
#endif

#if defined (__EPILOGUE__)
#define EPILOGUE_NO_RET(name) LABEL(__POST$##name):
#else
#define EPILOGUE_NO_RET(name)
#endif

#define EPILOGUE(name) EPILOGUE_NO_RET (name) ret

/* Cf. <math.h> */

#define	FP_INFINITE	0x01
#define	FP_NAN		0x02
#define	FP_NORMAL	0x04
#define	FP_SUBNORMAL	0x08
#define	FP_ZERO		0x10

