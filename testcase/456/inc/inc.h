/* base include template.
 * add #define INC_NO to the top of it. 
 */

#ifndef INC_NO
#define INC_NO
#endif 
#define _INC_CAT(a,b)   b##a
#define INC_CAT(a)      _INC_CAT(INC_NO, a)

extern int _INC_CAT( INC_NO , giInc);

#ifdef INC_CODE
int INC_CAT(inc) (int i)
{
    i += 1;
    return i;
}
#endif 

#undef INC_NO
#undef _INC_CAT
#undef INC_CAT
