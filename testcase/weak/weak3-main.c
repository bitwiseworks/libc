/* Example on weak aliasing as found in some GCC manual:
 * http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
 */
#include <stdio.h>

/* GCC Style */
int __f ()
{
    return 1;
}
int f () __attribute__ ((weak, alias ("__f")));


/* SUN style */
extern int WeakExternalWithLocalDefault_ResolveDefault(void);
int LocalDefault(void)
{
    return 2;
}
#pragma weak WeakExternalWithLocalDefault_ResolveDefault = LocalDefault

extern int ExternalDefault(void);
extern int   WeakExternalWithExternalDefault_ResolveDefault(void);
#pragma weak WeakExternalWithExternalDefault_ResolveDefault = ExternalDefault

/* resolve weak external (still SUN style) */
extern int   WeakExternalWithLocalDefault_ResolveWKEXT(void);
#pragma weak WeakExternalWithLocalDefault_ResolveWKEXT = LocalDefault

#if 0 /* BROKEN_GAS */
extern int   WeakExternalWithExternalDefault_ResolveWKEXT(void);
#pragma weak WeakExternalWithExternalDefault_ResolveWKEXT = ExternalDefault
#endif


int main()
{
    int rc = 0;
    int i;

    i = f();
    if (i != 1)
    {
        printf("weak3: f() returned %d expected %d\n", i, 1);
        rc++;
    }

    i = WeakExternalWithLocalDefault_ResolveDefault();
    if (i != 2)
    {
        printf("weak3: WeakExternalWithLocalDefault_ResolveDefault() returned %d expected %d\n", i, 2);
        rc++;
    }

    i = WeakExternalWithExternalDefault_ResolveDefault();
    if (i != -1)
    {
        printf("weak3: WeakExternalWithExternalDefault_ResolveDefault() returned %d expected %d\n", i, -1);
        rc++;
    }

    i = WeakExternalWithLocalDefault_ResolveWKEXT();
    if (i != -2)
    {
        printf("weak3: WeakExternalWithLocalDefault_ResolveWKEXT() returned %d expected %d\n", i, -2);
        rc++;
    }

#if 0 /* BROKEN_GAS */
    i = WeakExternalWithExternalDefault_ResolveWKEXT();
    if (i != -3)
    {
        printf("weak3: WeakExternalWithExternalDefault_ResolveWKEXT() returned %d expected %d\n", i, -3);
        rc++;
    }
#endif

    return rc;
}
