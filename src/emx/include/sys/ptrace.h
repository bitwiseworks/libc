/* sys/ptrace.h (emx+gcc) */

#ifndef _SYS_PTRACE_H
#define _SYS_PTRACE_H

#if defined (__cplusplus)
extern "C" {
#endif

#define PTRACE_TRACEME          0 /* not used by emx */
#define PTRACE_PEEKTEXT         1
#define PTRACE_PEEKDATA         2
#define PTRACE_PEEKUSER         3
#define PTRACE_POKETEXT         4
#define PTRACE_POKEDATA         5
#define PTRACE_POKEUSER         6
#define PTRACE_RESUME           7
#define PTRACE_EXIT             8
#define PTRACE_STEP             9
#define PTRACE_SESSION          10
#define PTRACE_NOTIFICATION     11
#define PTRACE_CONT             12
#define PTRACE_THAW             13
#define PTRACE_FREEZE           14
#define PTRACE_ATTACH           15
#define PTRACE_DETACH           16

#define PTN_NONE                0
#define PTN_THREAD_NEW          1
#define PTN_THREAD_END          2
#define PTN_MODULE_LOAD         3
#define PTN_MODULE_FREE         4
#define PTN_PROC_NEW            5

#define PTNMOD_DLL              0x0001
#define PTNMOD_AOUT             0x0002

#define PTNPROC_FORK            0x0001
#define PTNPROC_AOUT            0x0002

#define PTRACE_GETPID(x)        ((unsigned)(x) & 0xffff)
#define PTRACE_GETTID(x)        (((unsigned)(x) >> 16) & 0xffff)
#define PTRACE_PIDTID(pid,tid)  (((unsigned)(tid) << 16) | (unsigned)(pid))

struct ptn_thread
{
  unsigned tid;
  unsigned long reserved[31];
};

struct ptn_module
{
  unsigned long hmte;
  unsigned long text_start;
  unsigned long text_size;
  unsigned long data_start;
  unsigned long data_size;
  unsigned long bss_start;
  unsigned long bss_size;
  unsigned long flags;
  unsigned long reserved[24];
  char name[260];
};

struct ptn_proc
{
  unsigned pid;
  unsigned long flags;
  unsigned long fork_addr;
  unsigned long reserved[29];
  char name[260];
};

union ptn_data
{
  struct ptn_thread thread;
  struct ptn_module module;
  struct ptn_proc proc;
};

int ptrace (int, int, int, int);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_PTRACE_H */
