/*
    Locale support implementation through OS/2 Unicode API.
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    Transform a string so that the resulting string is suitable for
    comparison with strcmp() and the comparison results will use the
    alphabetic order in conformance to currently selected locale.
*/

#define INCL_FSMACROS
#include <os2emx.h>
#include "libc-alias.h"
#include <InnoTekLIBC/locale.h>
#include <stddef.h>
#include <string.h>

struct __strxfrm_arg
{
    char *out;
    size_t size;
};

static int __uni_strxfrm(UniChar *ucs, void *arg)
{
    struct __strxfrm_arg *x = (struct __strxfrm_arg *)arg;

    /* BUG WARNING!
     * As far as I've observed Unicode DLL has a bug that UniStrxfrm returns
     * one character less than it really fills in the buffer. I haven't
     * implemented any workaround for that first because it can be fixed
     * in the future and second because the information at the end of the
     * buffer seems very seldom really needed. UniStrxfrm generates a lot
     * of output, and every character in the input buffer generates three
     * characters in the output buffer: one wchar_t and two bytes that
     * seems to be related to character type (e.g. similar to character
     * flags isXXX() works with).
     */

    size_t rs = UniStrxfrm(__libc_gLocaleCollate.lobj, (UniChar *)x->out, ucs, x->size / sizeof(UniChar));
    if (!x->size)
        x->size = rs * sizeof(UniChar) + 3; /* We add three byte so we'll get enough space for a UniChar null terminator
                                               and any incorrect returns from UniStrxfrm. The caller will add 1 more byte for us.
                                               This means that we'll return less on the actual job, but that's hopefully acceptable. */
    else
    {
        /* rs is in UniChar's without trailing zero. */
        rs *= sizeof(UniChar);
        size_t size = x->size;
        x->size = rs;
        if (rs >= size)
            rs = size - 1;

        /* The string returned by Unicode API often contain zero characters
           (in the top or bottom 8 bits of a Unicode character).
           This is inappropiate for MBCS strings, so we increment all
           character codes by one except code 0xff (which is very seldom
           encountered). There is no other way to represent a Unicode
           xfrm'ed string as a MBCS string, alas. */

        for (int i = 0; i < rs; i++)
            if (x->out[i] != -1)
                x->out[i]++;
        x->out[rs] = '\0';
    }
    return 0;
}

/* Copy s2 to s1, applying the collate transform. */
size_t _STD(strxfrm) (char *s1, const char *s2, size_t size)
{
    if (__libc_gLocaleCollate.mbcs)
    {
        /* When using MBCS codepaes, we will convert the entire string to
           Unicode and then apply the UniStrxfrm() function. The output strings
           can be much longer than the original in this case, but if user program
           is correctly written, it will work since strxfrm will return the
           required output string length. */
        struct __strxfrm_arg x;
        FS_VAR();
        FS_SAVE_LOAD();
        x.out = s1;
        x.size = size;
        __libc_ucs2Do (__libc_gLocaleCollate.uobj, (char *)s2, &x, __uni_strxfrm);
        FS_RESTORE();
        return x.size;
    }

    /* buffer size query */
    if (!size)
        return strlen(s2);

    /* fill buffer as far as there is room */
    register const char *psz = s2;
    register unsigned char c;
    while ((c = *psz) && size)
    {
        size--;
        *s1++ = __libc_gLocaleCollate.auchWeight[c];
        psz++;
    }

    /* if more input then skip to the end so we get the length right. */
    if (c)
        psz = strchr(psz + 1, '\0');

    /* Append trailing zero, if there is space. */
    if (size)
        *s1 = '\0';
    return psz - s2;
}
