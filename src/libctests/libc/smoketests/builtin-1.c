/* $Id: builtin-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * Testcase for 386/builtin.h (parts of it anyway).
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <386/builtin.h>
#include <stdio.h>

int main()
{
    unsigned rc;
#define CHECKRET(val, rc)  do { if ((rc) != (val)) { printf("tstAtomic: FAILURE rc=%d (%#x) expected %d (%#x), line %d\n", rc, rc, (val), (val), __LINE__); return 1; } } while (0)
#define CHECKRC(val)  CHECKRET(val,rc)
#define CHECKU32(val) do { if (u32 != (val)) { printf("tstAtomic: FAILURE u32=%d (%#x) expected %d (%#x), line %d\n", u32, u32, (val), (val), __LINE__); return 1; } } while (0)
#define CHECKS32(val) do { if (i32 != (val)) { printf("tstAtomic: FAILURE i32=%d (%#x) expected %d (%#x), line %d\n", i32, i32, (val), (val), __LINE__); return 1; } } while (0)
    static uint32_t u32;
    static int32_t  i32;
    rc = __atomic_cmpxchg32(&u32, 0, 0);
    CHECKRC(1); CHECKU32(0);
    rc = __atomic_cmpxchg32(&u32, 1, 0);
    CHECKRC(1); CHECKU32(1);
    rc = __atomic_cmpxchg32(&u32, 2, 0);
    CHECKRC(0); CHECKU32(1);
    rc = __atomic_cmpxchg32(&u32, 32, 3);
    CHECKRC(0); CHECKU32(1);
    rc = __atomic_cmpxchg32(&u32, 32, 1);
    CHECKRC(1); CHECKU32(32);

    u32 = 1;
    rc = __atomic_decrement_min(&u32, 1);
    CHECKRC(1); CHECKU32(1);
    rc = __atomic_increment_max(&u32, 16);
    CHECKRC(0); CHECKU32(2);
    rc = __atomic_increment_max(&u32, 16);
    CHECKRC(0); CHECKU32(3);
    rc = __atomic_increment_max(&u32, 16);
    CHECKRC(0); CHECKU32(4);
    rc = __atomic_decrement_min(&u32, 1);
    CHECKRC(0); CHECKU32(3);
    rc = __atomic_decrement_min(&u32, 1);
    CHECKRC(0); CHECKU32(2);
    rc = __atomic_decrement_min(&u32, 1);
    CHECKRC(0); CHECKU32(1);
    rc = __atomic_decrement_min(&u32, 1);
    CHECKRC(1); CHECKU32(1);
    rc = __atomic_decrement_min(&u32, 0xcddf);
    CHECKRC(1); CHECKU32(1);
    rc = __atomic_decrement_min(&u32, 0);
    CHECKRC(0); CHECKU32(0);
    rc = __atomic_decrement_min(&u32, 0);
    CHECKRC(0); CHECKU32(0);

    u32 = 15;
    rc = __atomic_increment_max(&u32, 16);
    CHECKRC(0); CHECKU32(16);
    rc = __atomic_increment_max(&u32, 16);
    CHECKRC(16); CHECKU32(16);
    u32 = ~0 - 1;
    rc = __atomic_increment_max(&u32, ~0);
    CHECKRC(0); CHECKU32(~0);
    rc = __atomic_increment_max(&u32, ~0);
    CHECKRC(~0); CHECKU32(~0);



#define CHECKU16(val) do { if (u16 != (val)) { printf("tstAtomic: FAILURE u16=%d (%#x) expected %d (%#x), line %d\n", u16, u16, (val), (val), __LINE__); return 1; } } while (0)
    static uint16_t u16;
    CHECKU16(0);
    rc = __atomic_increment_word_max(&u16, 0x777f);
    CHECKRC(1);
    rc = __atomic_increment_word_max(&u16, 0x777f);
    CHECKRC(2);
    rc = __atomic_increment_word_max(&u16, 0x777f);
    CHECKRC(3);
    rc = __atomic_decrement_word_min(&u16, 0);
    CHECKRC(2);
    rc = __atomic_decrement_word_min(&u16, 0);
    CHECKRC(1);
    rc = __atomic_decrement_word_min(&u16, 0);
    CHECKRC(0);
    rc = __atomic_decrement_word_min(&u16, 0);
    CHECKRC(0xffff0000); CHECKU16(0);
    rc = __atomic_decrement_word_min(&u16, 0);
    CHECKRC(0xffff0000); CHECKU16(0);
    rc = __atomic_decrement_word_min(&u16, 0);
    CHECKRC(0xffff0000); CHECKU16(0);
    rc = __atomic_decrement_word_min(&u16, 0);
    CHECKRC(0xffff0000); CHECKU16(0);

    u16 = 0xfffd;
    rc = __atomic_increment_word_max(&u16, 0xffff);
    CHECKRC(0xfffe); CHECKU16(0xfffe);
    rc = __atomic_increment_word_max(&u16, 0xffff);
    CHECKRC(0xffff); CHECKU16(0xffff);
    rc = __atomic_increment_word_max(&u16, 0xffff);
    CHECKRC(0xffffffff); CHECKU16(0xffff);

    uint32_t u32Ret;
    u32 = 1;
    u32Ret = __atomic_decrement_u32(&u32);
    CHECKRET(0,u32Ret); CHECKU32(0);
    u32Ret = __atomic_increment_u32(&u32);
    CHECKRET(1,u32Ret); CHECKU32(1);
    u32Ret = __atomic_increment_u32(&u32);
    CHECKRET(2,u32Ret); CHECKU32(2);
    u32Ret = __atomic_increment_u32(&u32);
    CHECKRET(3,u32Ret); CHECKU32(3);
    u32Ret = __atomic_increment_u32(&u32);
    CHECKRET(4,u32Ret); CHECKU32(4);
    u32Ret = __atomic_decrement_u32(&u32);
    CHECKRET(3,u32Ret); CHECKU32(3);
    u32Ret = __atomic_decrement_u32(&u32);
    CHECKRET(2,u32Ret); CHECKU32(2);

    int32_t i32Ret;
    i32 = 1;
    i32Ret = __atomic_decrement_s32(&i32);
    CHECKRET(0,i32Ret); CHECKS32(0);
    i32Ret = __atomic_increment_s32(&i32);
    CHECKRET(1,i32Ret); CHECKS32(1);
    i32Ret = __atomic_increment_s32(&i32);
    CHECKRET(2,i32Ret); CHECKS32(2);
    i32Ret = __atomic_increment_s32(&i32);
    CHECKRET(3,i32Ret); CHECKS32(3);
    i32Ret = __atomic_increment_s32(&i32);
    CHECKRET(4,i32Ret); CHECKS32(4);
    i32Ret = __atomic_decrement_s32(&i32);
    CHECKRET(3,i32Ret); CHECKS32(3);
    i32Ret = __atomic_decrement_s32(&i32);
    CHECKRET(2,i32Ret); CHECKS32(2);

    return 0;
}

