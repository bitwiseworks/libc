/* ulsitem.h,v 1.3 2004/09/14 22:27:36 bird Exp */
/** @file
 * IGCC.
 */
/*
 * Based on header written by Andrew Zabolotny <bit@eltech.ru>
 * See ulsitem.h for further info.
 */

#ifndef _LANGINFO_H_
#define	_LANGINFO_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

#ifndef _NL_ITEM_DECLARED
typedef	__nl_item	nl_item;
#define	_NL_ITEM_DECLARED
#endif

/* The following constans are a replica of the ones in ulsitem.h
   They *must* be defined exactly the same way to keep the compiler quiet. */

#define D_T_FMT		1
#define D_FMT		2
#define T_FMT		3
#define AM_STR		4
#define PM_STR		5

#define ABDAY_1		6
#define ABDAY_2		7
#define ABDAY_3		8
#define ABDAY_4		9
#define ABDAY_5		10
#define ABDAY_6		11
#define ABDAY_7		12

#define DAY_1		13
#define DAY_2		14
#define DAY_3		15
#define DAY_4		16
#define DAY_5		17
#define DAY_6		18
#define DAY_7		19

#define ABMON_1		20
#define ABMON_2		21
#define ABMON_3		22
#define ABMON_4		23
#define ABMON_5		24
#define ABMON_6		25
#define ABMON_7		26
#define ABMON_8		27
#define ABMON_9		28
#define ABMON_10	29
#define ABMON_11	30
#define ABMON_12	31

#define MON_1		32
#define MON_2		33
#define MON_3		34
#define MON_4		35
#define MON_5		36
#define MON_6		37
#define MON_7		38
#define MON_8		39
#define MON_9		40
#define MON_10		41
#define MON_11		42
#define MON_12		43

#define RADIXCHAR	44
#define THOUSEP		45
#if __BSD_VISIBLE || __XSI_VISIBLE <= 500
#define YESSTR		46
#define NOSTR		47
#endif
#define CRNCYSTR	48
#define CODESET		49

/* Additional constants defined in XPG4 */

#define T_FMT_AMPM	55
#define ERA		56
#define ERA_D_FMT	57
#define ERA_D_T_FMT	58
#define ERA_T_FMT	59
#define ALT_DIGITS	60
#define YESEXPR		61
#define NOEXPR		62

/* LSA feature */
#define DATESEP		63
#define TIMESEP		64
#define LISTSEP		65

__BEGIN_DECLS
char *nl_langinfo(nl_item);
__END_DECLS

#endif /* !_LANGINFO_H_ */
