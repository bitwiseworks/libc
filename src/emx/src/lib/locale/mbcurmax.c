/*
    Locale support implementation through OS/2 Unicode API.
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    This file contains the _mb_cur_max variable which is the maximal
    length of a multibyte character (depends on LC_CTYPE locale setting).
    Accessed through MB_CUR_MAX macro defined in <stdlib.h>.
*/

#include <stdlib.h>

int __mb_cur_max = 1;
