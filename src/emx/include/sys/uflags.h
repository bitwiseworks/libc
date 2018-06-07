/* sys/uflags.h (emx+gcc) */

#ifndef _SYS_UFLAGS_H
#define _SYS_UFLAGS_H

/* Constants for _uflags() */

#define _UF_SIG_MODEL           0x0003  /* Mask */
#define _UF_SIG_EMX             0x0000
#define _UF_SIG_SYSV            0x0001
#define _UF_SIG_BSD             0x0002
#define _UF_SIG_RESERVED        0x0003

#define _UF_SBRK_MODEL          0x000c  /* Mask */
#define _UF_SBRK_CONTIGUOUS     0x0000
#define _UF_SBRK_MONOTONOUS     0x0004
#define _UF_SBRK_ARBITRARY      0x0008
#define _UF_SBRK_RESERVED1      0x000c

#define _UF_PTRACE_MODEL        0x0030  /* Mask */
#define _UF_PTRACE_STANDARD     0x0000
#define _UF_PTRACE_NOTIFY       0x0010
#define _UF_PTRACE_MULTITHREAD  0x0020

#if defined (__cplusplus)
extern "C" {
#endif

int _uflags (int, int);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_UFLAGS_H */
