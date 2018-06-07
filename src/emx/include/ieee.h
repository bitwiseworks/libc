/* ieee.h,v 1.2 2004/09/14 22:27:33 bird Exp */
/** @file
 * EMX
 */

#define	_IEEE		1
#define _HIDDENBIT	1
#define _LENBASE	1
#define _EXPBASE	(1 << _LENBASE)

#define _DEXPLEN	11
#define DMAXEXP		((1 << _DEXPLEN - 1) - 1 + _IEEE)
#define DMINEXP		(-(DMAXEXP - 3))
#define DSIGNIF		(64 - _DEXPLEN + _HIDDENBIT - 1)
#define MAXDOUBLE	1.7976931348623157e+308
#define MINDOUBLE       2.2250738585072014e-308
#define LN_MAXDOUBLE	1418.87227860620804838
#define LN_MINDOUBLE	(-1417.48598424508815776)

#define _FEXPLEN	8
#define FMAXEXP		((1 << _FEXPLEN - 1) - 1 + _IEEE)
#define FMINEXP		(-(FMAXEXP - 3))
#define FSIGNIF		(32  - _FEXPLEN + _HIDDENBIT - 1)
#define MAXFLOAT	3.40282347e+38
#define MINFLOAT        1.17549435e-38
#define LN_MAXFLOAT	88.7228394
#define LN_MINFLOAT     (-87.336544750553102)
