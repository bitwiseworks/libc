/* $Id: back-fsresolve-1.c 3913 2014-10-24 13:50:28Z bird $ */
/** @file
 * kLibC Testcase - Overflow handling in __libc_back_fsResolve on OS/2.
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/syslimits.h>
#include <InnoTekLIBC/backend.h>

int main()
{
    /*
     * Create a PATH_MAX sized buffer with an inaccessible page following it.
     */
    size_t const cbMem = ((PATH_MAX + PAGE_SIZE) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint8_t      *pbMem = (uint8_t *)memalign(cbMem, PAGE_SIZE);
    if (!pbMem)
    {
        fprintf(stderr, "memalign failed to allocate %#zx bytes of page aligned memory.\n", cbMem);
        return 1;
    }

    memset(pbMem + cbMem - PAGE_SIZE, 0xff, PAGE_SIZE);
    if (mprotect(pbMem + cbMem - PAGE_SIZE, PAGE_SIZE, PROT_NONE) != 0)
    {
        fprintf(stderr, "mprotect(, PAGE_SIZE, PROT_NONE) failed: %d\n", errno);
        return 1;
    }
    char * const pbPathMaxBuf = (char *)(pbMem + cbMem - PAGE_SIZE - PATH_MAX);

    /*
     * Generate some stupid input.
     */
    char szInput[PATH_MAX * 3];
    unsigned cchInput = 0;
    szInput[cchInput++] = 'C';
    szInput[cchInput++] = ':';
    szInput[cchInput++] = '\\';
    while (cchInput < sizeof(szInput) - 2)
    {
        szInput[cchInput++] = '0' + ((cchInput / 2) % 10);
        szInput[cchInput++] = '\\';
    }
    szInput[cchInput] = '\0';

    /*
     * Use the buffer as output, checking for overflows when presented with
     * various input lengths.
     */
    int      cErrors = 0;
    char     szPathMaxBuf2[PATH_MAX];
    unsigned off;
    for (off = 128; off < cchInput; off++)
    {
        char const chSaved = szInput[off];
        szInput[off] = '\0';

        /* Simple. */
        int rc1 = __libc_Back_fsPathResolve(szInput, pbPathMaxBuf, PATH_MAX,
                                            __LIBC_BACKFS_FLAGS_RESOLVE_DIRECT_BUF | __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);

        /* Lower case driver letter triggers different code path. */
        szInput[0] += 'a' - 'A';
        int rc2 = __libc_Back_fsPathResolve(szInput, pbPathMaxBuf, PATH_MAX,
                                            __LIBC_BACKFS_FLAGS_RESOLVE_DIRECT_BUF | __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);
        szInput[0] -= 'a' - 'A';

        /* Use the electric buffer for input, temporarily making it read-only. */
        char *pchElectricInput = (char *)(pbMem + cbMem - PAGE_SIZE - off - 1);
        memcpy(pchElectricInput, szInput, off + 1);
        mprotect(pchElectricInput, off + 1, PROT_READ);
        int rc3 = __libc_Back_fsPathResolve(pchElectricInput, szPathMaxBuf2, sizeof(szPathMaxBuf2),
                                            __LIBC_BACKFS_FLAGS_RESOLVE_DIRECT_BUF | __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);
        mprotect(pchElectricInput, off + 1, PROT_READ | PROT_WRITE);

        memcpy(pchElectricInput, szInput, off + 1);
        pchElectricInput[0] += 'a' - 'A';
        mprotect(pchElectricInput, off + 1, PROT_READ);
        int rc4 = __libc_Back_fsPathResolve(pchElectricInput, szPathMaxBuf2, sizeof(szPathMaxBuf2),
                                            __LIBC_BACKFS_FLAGS_RESOLVE_DIRECT_BUF | __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE);
        mprotect(pchElectricInput, off + 1, PROT_READ | PROT_WRITE);


        /* Check RCs. */
        if (   rc1 != rc3
            || rc2 != rc4)
        {
            fprintf(stderr, "off=%-3u rc1=%d rc2=%d rc3=%d rc4=%d\n", off, rc1, rc2, rc3, rc4);
            cErrors++;
        }

        /* Restore input. */
        szInput[off] = chSaved;
    }

    if (cErrors)
    {
        printf("failed - %u errors\n", cErrors);
        return 1;
    }
    printf("success\n");
    return 0;
}

