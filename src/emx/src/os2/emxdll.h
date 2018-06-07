/* emxdll.h -- Global header file for emx.dll
   Copyright (c) 1992-2000 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


#define STR(X)  #X
#define XSTR(X) STR(X)

/* Round up to the next integral multiple of the page size. */

#define ROUND_PAGE(x)  ((((x) - 1) & ~0xfff) + 0x1000)

/* IBM TCP/IP's socket handles are < 2048.  See also emxdll.asm. */

#define MAX_SOCKETS     2048

/* Maximum number of file handles created by sock_init().  As of
   TCP/IP 2.0, sock_init() creates two handles. */

#define MAXFH_SOCK_INIT 2

/* Maximum number of heap objects (16 = 512 / 32). */

#define MAX_HEAP_OBJS   16

/* This is the default heap object size. */

#define HEAP_OBJ_SIZE   (32 * 1024 * 1024)

#define ENTRY_POINT     0x10000
#define U_OFFSET        0x0400
#define KERNEL_U_ADDR   0x0e0000000

/* Bits in `handle_flags[]' and `files[].flags' */

#define HF_FILE         0x0001
#define HF_DEV          0x0002
#define HF_UPIPE        0x0004
#define HF_NPIPE        0x0008
#define HF_CON          0x0010
#define HF_NUL          0x0040
#define HF_CLK          0x0080
#define HF_ASYNC        0x0100
#define HF_SOCKET       0x0200
#define HF_XF86SUP      0x0400
#define HF_OPEN         0x1000
#define HF_APPEND       0x2000
#define HF_NDELAY       0x4000
#define HF_NOINHERIT    0x80000000 /* Temporarily used while forking */

#define L_FLAG_DLL              0x0001
#define L_FLAG_LINK386          0x0002

#define FORK_REQ_DONE           0
#define FORK_REQ_INIT           1
#define FORK_REQ_DLL            2
#define FORK_REQ_MEM            3

#define FORK_OBJ_SIZE           0x20000 /* 128KB */

#define DEBUG_STDERR    0x0001  /* Open CON device, don't use handle 2 */
#define DEBUG_STACK     0x0002  /* Unused */
#define DEBUG_SETMEM    0x0004  /* Show setmem() arguments */
#define DEBUG_SYSCALL   0x0008  /* Show syscalls */
#define DEBUG_TERMIO    0x0010  /* Unused */
#define DEBUG_FIND1     0x0020  /* Fetch only 1 entry with DosFindFirst/Next */
#define DEBUG_QPINFO2   0x0040  /* Call DosQueryPathInfo twice */
#define DEBUG_NOXF86SUP 0x0080  /* Don't check for xf86sup devices */
#define DEBUG_NOINHSIG  0x0100  /* Don't inherit signal actions */
#define DEBUG_NOFDZERO  0x0200  /* Don't fix select()'s missing FD_ZERO bug */
#define DEBUG_NOFORKDLL 0x0400  /* Don't load DLLs in forked process */
#define DEBUG_SOCLOSE   0x0800  /* Don't mind socket closed behind our back */

#define NSIG            29

#define FIND_COUNT      4

#define FLAG_C          0x0001
#define FLAG_Z          0x0040

/* TRUNC_DRV() is defined for 'A' through 'Z', 'a' through 'z', and 1
   through 26. */

#define TRUNC_UNC       (1 << 0)
#define TRUNC_DRV(c)    (1 << ((c) & 0x1f))
#define TRUNC_ALL       (~0)

typedef void (sigfun)(int signo);

struct select_data;
struct stat;
struct termio;
struct termios;
struct timeb;
struct timeval;
struct _find;
struct _new_proc;
struct _profil;
struct _select;

struct signal_entry
{
  sigfun *handler;              /* Signal handler, SIG_DFL, or SIG_IGN */
  ULONG sa_mask;                /* Signals to be blocked */
  ULONG sa_flags;               /* Flags */
};

typedef struct
{
  ULONG text_base;
  ULONG text_end;
  ULONG data_base;
  ULONG data_end;
  ULONG bss_base;
  ULONG bss_end;
  ULONG heap_base;
  ULONG heap_end;
  ULONG heap_brk;
  ULONG heap_off;
  ULONG os2_dll;
  ULONG stack_base;
  ULONG stack_end;
  ULONG flags;
  ULONG reserved[2];
  char options[64];
} layout_table;

typedef struct
{
  ULONG last_sys_errno;         /* Error code for last syscall */
  ULONG prev_sys_errno;         /* Previous value of the above */
  ULONG find_handle;
  ULONG find_count;
  ULONG sig_blocked;            /* Blocked signals */
  ULONG sig_pending;            /* Pending signals */
  ULONG sig_prev_blocked;       /* Previous signal mask */
  FILEFINDBUF3 find_buf[FIND_COUNT];
  const FILEFINDBUF3 *find_next;
  struct signal_entry sig_table[NSIG];
} thread_data;

typedef union
{
  struct
    {
      ULONG edi;
      ULONG esi;
      ULONG ebp;
      ULONG esp;
      ULONG ebx;
      ULONG edx;
      ULONG ecx;
      ULONG eax;
      ULONG eflags;
      ULONG eip;
    } e;
  struct
    {
      USHORT di;
      USHORT edi_hi;
      USHORT si;
      USHORT esi_hi;
      USHORT bp;
      USHORT ebp_hi;
      USHORT sp;
      USHORT esp_hi;
      USHORT bx;
      USHORT ebx_hi;
      USHORT dx;
      USHORT edx_hi;
      USHORT cx;
      USHORT ecx_hi;
      USHORT ax;
      USHORT eax_hi;
      USHORT flags;
      USHORT eflags_hi;
      USHORT ip;
      USHORT eip_hi;
    } x;
  struct
    {
      ULONG edi;
      ULONG esi;
      ULONG ebp;
      ULONG esp;
      UCHAR bl;
      UCHAR bh;
      USHORT ebx_hi;
      UCHAR dl;
      UCHAR dh;
      USHORT edx_hi;
      UCHAR cl;
      UCHAR ch;
      USHORT ecx_hi;
      UCHAR al;
      UCHAR ah;
      USHORT eax_hi;
      ULONG eflags;
      ULONG eip;
    } b;
} syscall_frame;

struct fork_data_dll
{
  ULONG req_code;               /* Request code */
  HMODULE hmod;                 /* Module handle */
  char path[CCHMAXPATH+1];      /* File name */
};

struct fork_data_mem
{
  ULONG req_code;               /* Request code */
  ULONG address;
  ULONG count;
  HMODULE hmod;
  void *shared;
  char buf[1];
};

struct fork_data_init
{
  ULONG req_code;               /* Request code */
  ULONG msize;                  /* Size of shared memory object */
  ULONG brk;
  ULONG stack_base;
  ULONG stack_page;
  ULONG reg_ebp;
  ULONG umask;
  ULONG umask1;
  ULONG uflags;                 /* User flags */
  HEV req_sem;
  HEV ack_sem;
};

struct fork_sock
{
  int f;                    /* File handle */
  int s;                    /* Socket handle */
};

struct fork_data_done
{
  ULONG req_code;               /* Request code */
  ULONG size;                   /* Number of bytes used for this structure */
  struct signal_entry sig_actions[NSIG];
  ULONG handle_count;           /* Number of file handles */
  ULONG *fd_flags_array;        /* FD_CLOEXEC */
  ULONG sock_init_count;        /* Number of handles created by sock_init() */
  ULONG sock_init_array[MAXFH_SOCK_INIT];
  ULONG sock_count;             /* Number of sockets */
  struct fork_sock *sock_array; /* File and socket handles */
};

typedef union
{
  ULONG req_code;
  struct fork_data_dll dll;
  struct fork_data_mem mem;
  struct fork_data_init init;
  struct fork_data_done done;
} fork_data;


struct my_datetime
{
  ULONG seconds;
  ULONG minutes;
  ULONG hours;
  ULONG day;
  ULONG month;
  ULONG year;
};

/* This structure describes one heap object. */

struct heap_obj
{
  ULONG base;                   /* Base address */
  ULONG end;                    /* End address */
  ULONG brk;                    /* Address of first unused byte */
};

extern BYTE opt_drive;
extern ULONG opt_trunc;
extern BYTE opt_expand;
extern BYTE opt_quote;
extern ULONG debug_flags;
extern HMTX common_mutex;
extern BYTE debug_same_sess;
extern HFILE errout_handle;
extern HEV signal_sem;
extern HEV kbd_sem_new;
extern ULONG stack_base;
extern ULONG stack_end;
extern BYTE exe_name[];
extern HFILE exe_fhandle;
extern BYTE exe_heap;
extern BYTE nocore_flag;
extern BYTE no_popup;
extern thread_data *threads[];
extern ULONG layout_flags;
extern BYTE interface;
extern ULONG umask_bits;
extern ULONG umask_bits1;
extern char *startup_env;
extern layout_table *layout;
extern char kbd_started;
extern ULONG my_pid;
extern BYTE fork_flag;
extern ULONG fork_exec_pid;
extern char const signal_valid[NSIG];
extern ULONG uflags;
extern int zombie_count;
extern char sig_flag;
extern BYTE dont_doskillthread;
extern ULONG version_major;
extern ULONG version_minor;
extern ULONG heap_obj_size;
extern unsigned heap_obj_count;
extern char first_heap_obj_fixed;
extern ULONG heap_off;
extern struct heap_obj *top_heap_obj;
extern struct heap_obj heap_objs[MAX_HEAP_OBJS];
extern BYTE ptrace_forking;
extern ULONG ptrace_fork_addr;

#if defined (INCL_DOSPROCESS)
extern PIB *init_pib_ptr;
#endif

#if defined (INCL_DOSEXCEPTIONS)
extern EXCEPTIONREGISTRATIONRECORD *exc_reg_ptr;
#endif

/* The following variables reside in a shared data object and are
   defined in emxdll.asm. */

/* Counter for creating unique names for named pipes. */

extern ULONG pipe_number;

/* Counter for creating unique names for queues. */

extern ULONG queue_number;

/* This array holds reference counts for socket handles:
   sock_proc_count[S] is the number of processes referencing socket
   handle S.  The per-process reference count is maintained in *files,
   see files.h.  Fortunately, the range of socket handles of IBM
   TCP/IP is bound: all socket handles are less than 2048; see
   FD_SETSIZE in IBM's <sys/socket.h>. */

extern ULONG sock_proc_count[MAX_SOCKETS];


/* Access to shared data should be enclosed in LOCK_COMMON and
   UNLOCK_COMMON. */

#define LOCK_COMMON    do_lock_common ()
#define UNLOCK_COMMON  DosReleaseMutexSem (common_mutex)


/* Prototypes. */

void truncate_name (char *dst, const char *src);
ULONG set_error (ULONG rc);
int xlate_errno (int e);
void child_started (void);
void start_cwait_thread (void);
ULONG kbd_avail (void);
int termio_read (ULONG handle, char *dst, unsigned count, int *errnop);
void otext (const char *msg);
void odword (ULONG x);
void core_dump (void);
ULONG core_main (ULONG handle);
void core_regs_i (syscall_frame *frame);
void generate_signal (thread_data *td, int signo);
thread_data *get_thread (void);
int get_tid (void);
int proc_check (int pid);
ULONG querysysinfo (ULONG index);
unsigned long long get_clock (int init_flag);
void init_signal16 (void);
void touch (void *base, ULONG count);
void conout (UCHAR chr);
ULONG load_module (const char *mod_name, HMODULE *phmod);
int verify_memory (ULONG start, ULONG size);

#if defined (INCL_DOSEXCEPTIONS)
void core_regs_e (CONTEXTRECORD *context);
int initthread (EXCEPTIONREGISTRATIONRECORD *registration);
void unwind (EXCEPTIONREGISTRATIONRECORD *registration,
    EXCEPTIONREPORTRECORD *report);
int deliver_pending_signals (thread_data *td, syscall_frame *frame,
    CONTEXTRECORD *context_record);
#endif

void init_fileio (void);
void init_process (void);
void init_fork (const char *s);
void init_exceptions (void);
void initialize1 (void);
void initialize2 (void);
void init2_signal (void);
void term_process (void);
void term_memory (void);
void term_semaphores (void);
void term_tcpip (void);
void term_signal (void);
ULONG setmem (ULONG base, ULONG size, ULONG flags, ULONG error);
ULONG setmem_error (ULONG rc);
void *checked_private_alloc (ULONG size);
ULONG private_alloc (void **mem, ULONG size);
ULONG private_free (void *mem);
void error (ULONG rc, const char *msg);
int conv_hex8 (const char *s, ULONG *dst);
void receive_signals (void);
ULONG reset_event_sem (HEV sem);
sigfun *do_signal (ULONG signo, sigfun *handler, syscall_frame *frame);
void do_raise (int signo, syscall_frame *frame);
int do_kill (int signo, int pid, syscall_frame *frame, int *errnop);
void sig_block_start (void);
void sig_block_end (void);
ULONG set_alarm (ULONG value);
void stop_alarm (void);
ULONG do_wait (ULONG *ptermstatus, ULONG *errnop);
ULONG do_waitpid (ULONG pid, ULONG opt, ULONG *ptermstatus,
    ULONG *errnop);
void init_heap (void);
long do_sbrk (long incr);
long do_brk (ULONG brkp);
int do_fcntl (ULONG handle, ULONG request, ULONG arg, int *errnop);
int do_ioctl1 (ULONG handle, int *errnop);
int do_ioctl2 (ULONG handle, ULONG request, ULONG arg, int *errnop);
int do_dup (ULONG handle, ULONG target, int *errnop);
int do_close (ULONG handle);
ULONG do_read (ULONG handle, void *dst, ULONG count, int *errnop);
ULONG do_write (ULONG handle, void *src, ULONG count, int *errnop);
ULONG do_seek (ULONG handle, ULONG origin, LONG distance, int *errnop);
int do_pipe (int *dst, ULONG pipesize);
int do_ftruncate (ULONG handle, ULONG fsize);
int do_chsize (ULONG handle, ULONG fsize);
int do_fsync (ULONG handle, int *errnop);
int do_filesys (char *dst, ULONG dst_size, const char *drv_name, int *errnop);
int do_mkdir (const char *path);
int do_rmdir (const char *path);
int do_chdir (const char *path);
int do_delete (const char *path);
int do_rename (const char *old_path, const char *new_path);
UCHAR do_getdrive (void);
ULONG do_selectdisk (ULONG drv);
int old_creat (const char *path, ULONG attr, int *errnop);
int old_open (const char *path, ULONG mode, int *errnop);
int sys_open (const char *path, unsigned flags, unsigned long size,
    int *errnop);
int do_stat (const char *path, struct stat *dst, int *errnop);
int do_fstat (ULONG handle, struct stat *dst, int *errnop);
int do_utimes (const char *path, const struct timeval *tv, int *errnop);
int do_ttyname (char *dst, ULONG dst_size, ULONG handle, int *errnop);
void do_lock_common (void);
ULONG create_event_sem (HEV *psem, ULONG attr);
void close_event_sem (HEV sem);
ULONG create_mutex_sem (HMTX *psem);
void request_mutex (HMTX sem);
ULONG create_muxwait_sem (HMUX *psem, ULONG count, SEMRECORD *array,
    ULONG attr);
void close_muxwait_sem (HMUX sem);
ULONG get_attr (const char *path, int *errnop);
ULONG set_attr (const char *path, ULONG attr);
ULONG do_getcwd (char *dst, int drive);
ULONG do_find_first (const char *path, ULONG attr, struct _find *dst);
ULONG do_find_next (struct _find *dst);
ULONG do_get_timestamp (ULONG handle, ULONG *time, ULONG *date);
ULONG do_set_timestamp (ULONG handle, ULONG time, ULONG date);
ULONG do_imphandle (ULONG handle, int *errnop);
int check_npipe (const char *s);
int set_ttyname (char *dst, ULONG dst_size, const char *name, int *errnop);
void fork_register_mem (ULONG start, ULONG end, HMODULE hmod);
void fork_register_dll (HMODULE hmod);
int fork_dll_registered (HMODULE hmod);
void copy_fork (void);
void fileio_fork_parent_init (void);
void fileio_fork_parent_fillin (struct fork_data_done *p);
void fileio_fork_parent_restore (void);
void fileio_fork_child (struct fork_data_done *p);
ULONG new_thread (ULONG tid, int *errnop);
ULONG end_thread (ULONG tid, int *errnop);
void kbd_stop (void);
void kbd_init (void);
void kbd_restart (void);
void quit (ULONG rc) __attribute__ ((noreturn));
void do_ftime (struct timeb *dst);
int do_settime (const struct timeval *tp, int *errnop);
ULONG do_spawn (struct _new_proc *np, ULONG *result);
void do_pause (void);
int do_ptrace (ULONG request, ULONG pid, ULONG addr, ULONG data,
    ULONG *errnop);
ULONG spawn_debug (ULONG pid, ULONG sid);
int do_fork (syscall_frame *frame, ULONG *errnop);
int do_select (struct _select *args, int *errnop);
int execname (char *buf, ULONG bufsize);
void debug_set_wait (ULONG pid, ULONG tid, ULONG wait_ret, ULONG end);
ULONG proc_add_attach (ULONG pid);
void proc_remove_attach (ULONG pid);
void unix2time (struct my_datetime *dst, ULONG src);
ULONG packed2unix (FDATE date, FTIME time);
int kbd_input (UCHAR *dst, int binary_p, int echo_p, int wait_p,
    int check_avail_p);
void conin_string (UCHAR *buf);
int sprintf (char *dst, const char *fmt, ...);
void oprintf (const char *fmt, ...);
ULONG get_dbcs_lead (void);
ULONG query_async (ULONG handle);
ULONG translate_async (ULONG handle);
void init_termio (ULONG handle);
void termio_set (ULONG handle, const struct termio *tio);
void termio_get (ULONG handle, struct termio *tio);
void termios_set (ULONG handle, const struct termios *tio);
void termios_get (ULONG handle, struct termios *tio);
void termio_flush (ULONG handle);
ULONG termio_avail (ULONG handle);
ULONG async_avail (ULONG handle);
ULONG async_writable (ULONG handle);
int do_profil (const struct _profil *p, int *errnop);

void exit_tcpip (void);
int tcpip_init_fork (const struct fork_data_done *p);
void copy_fork_sock (const struct fork_data_done *p);
void tcpip_fork_sock (struct fork_data_done *p);
void tcpip_undo_fork_sock (const struct fork_data_done *p);

int pm_init (void);
void pm_term (void);
void pm_message_box (PCSZ text);

#if defined (_SIGSET_T)
int do_sigaction (int sig, const struct sigaction *iact,
    struct sigaction *oact);
int do_sigpending (sigset_t *set);
int do_sigprocmask (int how, const sigset_t *iset, sigset_t *oset,
    syscall_frame *frame);
int do_sigsuspend (const sigset_t *mask);
#endif

ULONG xf86sup_query (ULONG handle, ULONG *pflags);
int xf86sup_avail (ULONG handle, int *errnop);
int xf86sup_ioctl (ULONG handle, ULONG request, ULONG arg, int *errnop);
int xf86sup_fcntl (ULONG handle, ULONG request, ULONG arg, int *errnop);
int xf86sup_select_add_read (struct select_data *d, int fd);
int xf86sup_ttyname (char *dst, ULONG dst_size, ULONG handle, int *errnop);
ULONG xf86sup_uncond_enadup (ULONG handle, ULONG on);
ULONG xf86sup_maybe_enadup (ULONG handle, ULONG on);
void xf86sup_all_enadup (ULONG on);
