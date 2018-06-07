/* init.c -- Initialization
   Copyright (c) 1994-2000 by Eberhard Mattes

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


#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSMODULEMGR
#define INCL_DOSMISC
#define INCL_DOSEXCEPTIONS
#define INCL_DOSERRORS
#define INCL_REXXSAA
#include <os2emx.h>
#include <emx/startup.h>
#include "emxdll.h"
#include "clib.h"
#include "version.h"

/* Pointer to the layout table of the executable. */

layout_table *layout;

/* Flags from layout table. */

ULONG layout_flags;

/* Interface number, taken from the layout table. */

BYTE interface;

/* The top heap object.  This points into heap_objs[] or is NULL.
   While this variable is NULL, no memory has been allocated. */

struct heap_obj *top_heap_obj;

/* This array holds information on all the heap objects.  The heap
   objects are managed in LIFO fashion. */

struct heap_obj heap_objs[MAX_HEAP_OBJS];

/* This is the number of heap objects. */

unsigned heap_obj_count;

/* This variable is true iff the first heap object (heap_objs[0]) is
   an object from the EXE file, that is, has not been allocated with
   DosAllocMem. */

char first_heap_obj_fixed;

/* This is the default size for heap objects. */

ULONG heap_obj_size;

/* Address of the heap (first object) in the executable file. */

ULONG heap_off;

/* Base address of the stack object. */

ULONG stack_base;

/* End address of the stack object (address of first byte after the
   stack object). */

ULONG stack_end;

/* Run debuggee in same session as the debugger. */

BYTE debug_same_sess;

/* Flag bits for debugging, set with the -! option. */

ULONG debug_flags;

/* If this variable is zero, if file names should not be truncated to
   8.3 format.  If bit 0 is set, UNC pathnames are truncated to 8.3
   format.  Bits 1 through 26 control truncation of file names on
   drives A: through Z:.  The user can set this variable by putting
   the -t option into the EMXOPT environment variable. */

ULONG opt_trunc;

/* Default drive: 0=none, "A".."Z", "a".."z": prepend this drive
   letter. */

BYTE opt_drive;

/* The -x option is used for getting the old (0.8h) behavior of not
   quoting arguments passed using the `MKS Korn shell' method (3rd
   argument string starting with `~').  Expanding arguments in that
   case was wrong. */

BYTE opt_expand;

/* The -q option forces quoting of all command line arguments passed
   to child processes. */

BYTE opt_quote;

/* This handle is used for diagnostic output including error
   messages. */

ULONG errout_handle;

/* This flag is TRUE if this is process is a forked process. */

BYTE fork_flag;

/* fork(): EBP of parent process. */

ULONG fork_ebp;

/* fork(): Low end of the stack. */

ULONG fork_stack_page;

/* fork(): Pointer to final data packet from parent process. */

static struct fork_data_done *p_fork_done;

/* Pointer to the PIB. */

PIB *init_pib_ptr;

/* This variable points to the main exception registration record of
   emx.dll. */

EXCEPTIONREGISTRATIONRECORD *exc_reg_ptr;

/* The process ID of the process using this instance of emx.dll. */

ULONG my_pid;

/* Pointer to the command line. */

char *startup_args;

/* Pointer to the environment. */

char *startup_env;

/* Number of environment strings. */

ULONG env_count;

/* Number of arguments. */

ULONG arg_count;

/* Size of argument strings (bytes). */

ULONG arg_size;

/* The third argument string of the program. */

char *argstr3;

/* The name of the EXE file. */

BYTE exe_name[257];

/* The file handle used for reading from the EXE file if the heap is
   being loaded from the EXE file. */

HFILE exe_fhandle;

/* This flag is true if the heap is loaded from the EXE file. */

BYTE exe_heap;

/* This flag is true if we should ignore too small a stack.  It is set
   by the -I option. */

static BYTE ignore_stack;

/* Error message to be displayed by options(). */

static const char *opt_errmsg;

/* User flags, initially 0, set by __uflags(). */

ULONG uflags;

/* Set the following variable to TRUE to avoid using DosKillThread. */

BYTE dont_doskillthread;

/* The major and minor version numbers of OS/2. */

ULONG version_major;
ULONG version_minor;

/* Define our own version of `_osmode' which is uninitialized; do not
   use the library version which would add a non-shared page to
   emx.dll, slowing down loading when it's already loaded. */

unsigned char _osmode;


/* Prototypes. */

static void init_exit_list (void);
static void options (const char *s, const char *errmsg);
static void connect_fork (void);
static void init2_fork (void);


/* This REXX-callable entrypoint returns a string indicating the
   revision index of emx.dll as integer. The number will be
   incremented on each revision and is used only for comparing. */

ULONG emx_revision (PCSZ name, LONG argc, const RXSTRING *argv,
                    PCSZ queuename, PRXSTRING retstr)
{
  static char const revision[] = XSTR(REV_INDEX);

  if (argc != 0)
    return 1;
  strcpy (retstr->strptr, revision);
  retstr->strlength = strlen (revision);
  return 0;
}


/* DLL initialization and termination function.  This function is
   called by the system to initialize the DLL (if FLAG is zero) or to
   terminate the DLL (if FLAG is one). */

ULONG _DLL_InitTerm (ULONG mod_handle, ULONG flag)
{
  switch (flag)
    {
    case 0:                     /* Initialization */
      init_heap ();
      break;

    case 1:                     /* Termination */
#if 0
      /* OS/2 does not terminate DLLs in the correct sequence under
         certain circumstances.  It might happen that EMXLIBCS.DLL is
         terminated *after* EMX.DLL, causing problems in functions of
         EMX.DLL called by the termination code of EMXLIBCS.DLL.
         Don't terminate EMX.DLL -- a memory leak is preferable to a
         crash. */

      pm_term ();
      term_signal ();
      term_process ();
      term_tcpip ();
      term_semaphores ();
      term_memory ();
#endif
      break;
    }
  return 1;                     /* Success! */
}


/* Fetch the flags and the interface level from the layout table. */

static void use_layout (void)
{
  layout_flags = layout->flags;
  interface = (BYTE)(layout_flags >> 24);
}


/* Use the heap stored in the EXE file, if present. */

static void use_heap (void)
{
  if (layout->heap_base == 0 || layout->heap_end == 0 || heap_obj_count != 0)
    return;
  heap_objs[0].base = layout->heap_base;
  heap_objs[0].end = layout->heap_end;
  if (layout->heap_brk != 0)
    {
      ULONG rc, size, flags;

      /* There is a heap stored in the EXE file. */

      heap_off = layout->heap_off;
      heap_objs[0].brk = layout->heap_brk;

      /* Check if the heap object maps the heap pages. */

      size = 0x1000;
      rc = DosQueryMem ((PVOID)heap_objs[0].base, &size, &flags);
      if (rc == 0 && (flags & PAG_COMMIT))
        {
          ULONG next;

          /* The heap pages are mapped by the heap object.  Decommit
             those pages which are not preloaded to improve security
             and to make sbrk() and brk() work. */

          next = ROUND_PAGE (heap_objs[0].brk);
          if (heap_objs[0].end > next)
            setmem (next, heap_objs[0].end - next, PAG_DECOMMIT, 0);
        }
      else
        {
          ULONG action;

          /* The heap pages are not mapped by the heap object.  We
             have to use an exception handler and guard pages to load
             the pages. */

          rc = DosOpen (exe_name, &exe_fhandle, &action, 0, 0,
                        OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                        (OPEN_ACCESS_READONLY | OPEN_SHARE_DENYWRITE
                         | OPEN_FLAGS_RANDOM | OPEN_FLAGS_NOINHERIT), NULL);
          if (rc != 0)
            {
              otext ("Cannot open EXE file\r\n");
              quit (255);
            }
          /* Commit the guard pages. */
          if (heap_objs[0].brk != heap_objs[0].base)
            setmem (heap_objs[0].base, heap_objs[0].brk - heap_objs[0].base,
                    PAG_COMMIT | PAG_GUARD | PAG_READ | PAG_WRITE, 0);
          exe_heap = TRUE;
        }
    }
  else
    heap_objs[0].brk = heap_objs[0].base;
  top_heap_obj = &heap_objs[0];
  heap_obj_count = 1;
  first_heap_obj_fixed = TRUE;

  /* All dynamically heap objects will have at least the same size as
     the heap object of the EXE file. */

  heap_obj_size = heap_objs[0].end - heap_objs[0].base;
}


/* First part of initialization.  After return from initialize1(),
   stacks will be switched and, if this process has been forked, data
   will be copied from the parent process. */

void initialize1 (void)
{
  TIB *tib_ptr;
  ULONG rc, action;
  PSZ str;
  char *p;
  size_t len;

  __asm__ ("cld");

  _osmode = 1;                  /* OS2_MODE */

  use_layout ();

  /* Get pointers to the TIB and PIB, and fetch environment pointers
     and the PID from the PIB. */

  DosGetInfoBlocks (&tib_ptr, &init_pib_ptr);
  startup_args = init_pib_ptr->pib_pchcmd;
  startup_env = init_pib_ptr->pib_pchenv;
  my_pid = init_pib_ptr->pib_ulpid;

  /* Get the version number of OS/2. */

  version_major = querysysinfo (QSV_VERSION_MAJOR);
  version_minor = querysysinfo (QSV_VERSION_MINOR);

  /* Set `dont_doskillthread' depending on the version number of OS/2.
     Assume that DosKillThread works in OS/2 2.11 and later. */

  if (!(version_major > 20 || (version_major == 20 && version_minor >= 11)))
    dont_doskillthread = TRUE;

  /* Retrieve the name of the executable file of the application. */

  rc = DosQueryModuleName (init_pib_ptr->pib_hmte,
                           sizeof (exe_name), exe_name);
  if (rc != 0)
    exe_name[0] = 0;
  exe_fhandle = (HFILE)(-1);

  /* Initialize __clock(). */

  get_clock (TRUE);

  /* Get a pointer to the 3rd argument string.  Check whether we have
     been forked. */

  p = startup_args;
  len = strlen (p);
  p += len + 1;
  len = strlen (p);
  p += len + 1;
  argstr3 = p;
  if (*p == '^')
    init_fork (p);

  /* Set default values for options. */

  heap_obj_size = HEAP_OBJ_SIZE;

  /* Parse emx options put into the executable by emxbind. */

  options (layout->options, "Invalid option in .exe file\r\n");

  /* Parse emx options in the EMXOPT environment variable. */

  rc = DosScanEnv ("EMXOPT", &str);
  if (rc == 0)
    options (str, "Invalid option in EMXOPT\r\n");

  /* Get the stack base and end addresses. */

  stack_base = (ULONG)tib_ptr->tib_pstack;
  stack_end = (ULONG)tib_ptr->tib_pstacklimit;

  /* Refuse to work if the stack is too small. */

  if (stack_end - stack_base <= 16*1024 && !ignore_stack)
    {
      char buf[512];
      ULONG written;

      if (!pm_init ())
        {
          len = sprintf (buf, "emx.dll: Stack size too small.  Run\r\n"
                         "  emxstack -f %s\r\n"
                         "and try again.\r\n", exe_name);
          DosWrite (2, buf, len, &written);
        }
      else
        {
          sprintf (buf, "Stack size too small.  Run \"emxstack -f %s\" "
                   "and try again.", exe_name);
          pm_message_box (buf);
        }

      quit (255);
    }

  /* Initialize errout_handle. */

  if (debug_flags & DEBUG_STDERR)
    {
      rc = DosOpen ("con", &errout_handle, &action, 0, 0,
                    OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                    (OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYNONE
                     | OPEN_FLAGS_NOINHERIT),
                    NULL);
      if (rc != 0)
        errout_handle = 2;      /* In case it was modified by DosOpen */
    }

  /* Initialize the user flags. */

  uflags = 0;

  /* Use the heap stored in the EXE file, if present. */

  use_heap ();

  /* Install the exit list procedure.  This should be done before
     calling copy_fork() in order to be able to adjust the socket
     reference counts if this process terminates prematurely. */

  init_exit_list ();

  /* If this process has been forked, connect to the parent process
     and get the initial block of data.  Among other things, that
     block contains a pointer to the lowest page of stack in use by
     the parent process.  We set the stack pointer to that address
     before copying the stack from the parent process.  As moving the
     stack causes trouble with compiled code, initialization is split
     into two parts, initialize1() and initialize2(), the stack being
     switched between those parts. */

  if (fork_flag)
    connect_fork ();
}


/* Second part of initialization.  Here, initialization is continued
   from initialize1(), after switching stacks and, if this process has
   been forked, copying data from the parent process. */

void initialize2 (void)
{
  init_exceptions ();
  receive_signals ();
  get_dbcs_lead ();
  init_fileio ();
  init_process ();
  new_thread (1, NULL);
  if (fork_flag)
    init2_fork ();
  else
    init2_signal ();
}


/* This function is called from the startup code to initialize DLLs.
   Dirty hack: We access our caller's parameters on the stack as
   there's no space in dll0.s for pushing them. */

void dll_init (layout_table *nl, ULONG ret_addr,
               HMODULE hmod, ULONG flag)
{
  if (layout == NULL)
    {
      layout = nl;
      use_layout ();
    }

  /* Register the DLL's data segment for fork(). */

  if (nl->data_base < nl->data_end && nl->bss_base < nl->bss_end
      && nl->bss_base == nl->data_end)
    fork_register_mem (nl->data_base, nl->bss_end, hmod);
  else
    {
      if (nl->data_base < nl->data_end)
        fork_register_mem (nl->data_base, nl->data_end, hmod);
      if (nl->bss_base < nl->bss_end)
        fork_register_mem (nl->bss_base, nl->bss_end, hmod);
    }
  if (flag == 0)                /* Initialization */
    fork_register_dll (hmod);
}


/* This is our exit list procedure. */

static void APIENTRY exit_list_proc (ULONG term_code)
{
  exit_tcpip ();
  DosExitList (EXLST_EXIT, exit_list_proc);
}


/* Install our exit list procedure.  This is always done, whether
   sockets are used or not, to make it run after any exit list
   procedure installed by the application.  If tcpip_init() installed
   the exit list procedure, the invocation order would depend on
   whether the application installs the exit list procedure before or
   after the first call to tcpip_init(). */

static void init_exit_list (void)
{
  ULONG rc;

  /* Note: SO32DLL.DLL installs its exit list procedure with
     invocation order 0x99.  Our exit list procedure should run before
     SO32DLL's to be able to use SO32DLL.  Moreover, it should run
     after any exit list procedure installed by the application, to
     make any sockets available to that exit list procedure.  Assume
     that the invocation orders of the application's exit list
     procedures are less than 0x80 (0x80 through 0xff seem to be
     reserved for OS/2). */

  rc = DosExitList (EXLST_ADD | (0x7f << 8), exit_list_proc);
  if (rc != 0)
    error (rc, "DosExitList");
}


/* There's an error in an option.  Display an error message and abort.
   `opt_errmsg' contains the error message, which depends on where the
   options come from (emxbind or EMXOPT). */

static void opt_error (void)
{
  otext (opt_errmsg);
  quit (1);
}


/* Parse a numeric, decimal, unsigned argument for an option.  Return
   the number and update *P which initially points to the beginning of
   the argument to point to the first character after the number.  The
   number must be terminated by a null character or whitespace.  On
   error, display an error message and abort. */

static ULONG opt_number (const char **p)
{
  const char *s;
  ULONG n, d;
  char flag;

  s = *p;
  flag = FALSE; n = 0;
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
      /* Base 16 */
      s += 2;
      for (;;)
        {
          if (*s >= '0' && *s <= '9')
            d = *s - '0';
          else if (*s >= 'a' && *s <= 'f')
            d = *s - 'a' + 10;
          else if (*s >= 'A' && *s <= 'F')
            d = *s - 'A' + 10;
          else
            break;
          if (n >= 0x10000000)
            opt_error ();
          n = n * 16 + d;
          flag = TRUE; ++s;
        }
    }
  else
    {
      /* Base 10 */
      while (*s >= '0' && *s <= '9')
        {
          /* This check for overflow isn't perfect -- it rejects some
             valid numbers.  However, those numbers aren't valid
             arguments, anyway. */

          if (n >= 429496728)
            opt_error ();
          n = n * 10 + (*s - '0');
          flag = TRUE; ++s;
        }
    }
  if (!flag || (*s != 0 && *s != ' ' && *s != '\t'))
    opt_error ();
  *p = s;
  return n;
}


#define ISLETTER(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define ISEND(c)    ((c) == 0 || (c) == ' ' || (c) == '\t')

/* Parse options.  S points to the null-terminated string of options,
   ERRMSG points to the error message to be used. */

static void options (const char *s, const char *errmsg)
{
  char c;
  ULONG n;

  /* Make the pointer to the error message available to
     opt_error(). */

  opt_errmsg = errmsg;

  /* Parse the options, one by one.  Options must be separated by
     whitespace (blank or tab). */

  do
    {
      /* Skip whitespace preceding the first option or following any
         other option.  Return when reaching the end of the string. */

      do
        {
          c = *s++;
          if (c == 0)
            return;
        } while (c == ' ' || c == '\t');

      /* All options start with `-'. */

      if (c != '-')
        opt_error ();

      /* Parse the various options. */

      c = *s++;
      switch (c)
        {
        case 'c':

          /* Suppress core dumps. */

          nocore_flag = TRUE;
          break;

        case 'f':

          /* This option was used for setting the maximum stack frame
             size.  Parse the value, and discard it.  As there are
             still some programs which have, say, -f0 set with
             emxbind, we have to accept this option though the maximum
             stack frame size is no longer used. */

          n = opt_number (&s);
          if (n != 0 && (n < 4 || n > 32768))
            opt_error ();
          break;

        case 'h':

          /* Set the number of file handles.  Note that DosSetMaxFH
             cannot lower the number of file handles below the initial
             value for this process (DosSetRelMaxFH can). */

          n = opt_number (&s);
          if (n < 10 || n > 65536)
            opt_error ();
          DosSetMaxFH (n);
          break;

        case 'n':

          /* Don't display a harderror popup.  This is done by
             terminating the process instead of returning
             XCPT_CONTINUE_SEARCH from the exception handler. */

          no_popup = TRUE;
          break;

        case 'q':

          /* Quote all command line arguments passed to child
             processes. */

          opt_quote = TRUE;
          break;

        case 'r':

          /* Set the default drive for path names starting with `/' or
             `\'.  The argument of this option is a drive letter. */

          if (ISLETTER (*s))
            opt_drive = *s++;
          else
            opt_error ();
          break;

        case 't':

          /* Truncate all components of certain pathnames to 8.3
             format. `-t' enables truncation on all drives, `-t-'
             disables truncation on all drives, `-tabc' enables
             truncation on drives A, B, and C, `-t-abc' disables
             truncation on drives A, B, and C.  UNC pathnames are
             specified by the special `drive name' `/'. */

          if (ISEND (*s))
            opt_trunc = TRUNC_ALL;
          else if (*s == '-')
            {
              ++s;
              if (ISEND (*s))
                opt_trunc = 0;
              else
                do
                  {
                    c = *s++;
                    if (ISEND (c))
                      break;
                    if (ISLETTER (c))
                      opt_trunc &= ~TRUNC_DRV (c);
                    else if (c == '/')
                      opt_trunc &= ~TRUNC_UNC;
                    else
                      opt_error ();
                  } while (!ISEND (*s));
            }
          else
            do
              {
                c = *s++;
                if (ISLETTER (c))
                  opt_trunc |= TRUNC_DRV (c);
                else if (c == '/')
                  opt_trunc |= TRUNC_UNC;
                else
                  opt_error ();
              } while (!ISEND (*s));
          break;

        case 'x':

          /* Expand all command line arguments passed with the `MKS
             Korn shell' method to this process. */

          opt_expand = TRUE;
          break;

        case 'E':

          /* Run debugees in the same session as the debugger. */

          debug_same_sess = TRUE;
          break;

        case 'K':

          /* Don't use DosKillThread. */

          dont_doskillthread = TRUE;
          break;

        case 'I':

          /* Don't abort when the stack size is too small.  This is
             required for old programs which assume that emx.dll
             creates a private stack object if the stack size is too
             small. */

          ignore_stack = TRUE;
          break;

        case 'V':

          /* Display the emx banner. */

          otext ("emx " VERSION " (rev " XSTR (REV_INDEX) ")"
                 " -- Copyright (c) 1992-2000 by Eberhard Mattes\r\n");
          break;

        case '!':

          /* Set debugging flags.  The argument as a (decimal) number,
             interpreted bitwise.  See emxdll.h for the meaning of the
             bits (DEBUG_STDERR, for instance). */

          debug_flags = opt_number (&s);
          break;

        default:

          /* The option is not known, abort. */

          opt_error ();
        }
    } while (*s == ' ' || *s == '\t');

  /* If there are characters remaining after an option, complain and
     abort. */

  if (*s != 0)
    opt_error ();
}


/* Parse the command line.  Store the argument strings at POOL (unless
   POOL is NULL).  The pointers are stored at VEC (unless VEC is
   NULL). */

#define BEGIN    do {
#define END      } while (0)
#define WHITE(C) ((C) == ' ' || (C) == '\t')
#define PUTC(C)  BEGIN ++arg_size; if (pool != NULL) *pool++ = (C); END
#define PUTV     BEGIN ++arg_count; if (vec != NULL) *vec++ = pool; END

static void parse_arg (char *pool, char **vec, const char *str1,
                       const char *str3)
{
  const char *s;
  char *flag_ptr;
  int bs, quote;

  arg_count = 0; arg_size = 0;

  /* Look for the 3rd string -- if it starts with ~ we can get the
     parsed argument words from that string and the following
     strings. */

  if (str3[0] == '~' && strcmp (str3 + 1, str1) == 0)
    {
      char flag;

      if (opt_expand)
        flag = _ARG_NONZERO;
      else
        flag = _ARG_NONZERO|_ARG_DQUOTE;
      s = str3 + 1;
      while (*s != 0)
        {
          if (*s == '~')
            ++s;
          PUTC (flag); PUTV;
          do
            {
              PUTC (*s);
            } while (*s++ != 0);
        }
    }
  else
    {
      s = str1;

      /* argv[0] contains the program name. */

      PUTC (_ARG_NONZERO); PUTV;
      do
        {
          PUTC (*s);
        } while (*s++ != 0);

      /* Now scan the arguments, one by one. */

      for (;;)
        {
          /* Skip leading whitespace. */

          while (WHITE (*s))
            ++s;

          /* Work is completed when reaching the end of the string. */

          if (*s == 0)
            break;

          /* Flags will be stored to `*flag_ptr' while parsing the
             current argument.  Initially, no flags are set. */

          flag_ptr = pool;
          PUTC (_ARG_NONZERO);
          PUTV;
          bs = 0; quote = 0;
          for (;;)
            {
              if (*s == '"')
                {
                  /* Backslashes preceding a double quote character
                     are treated specially: A backslash escapes either
                     a backslash or a double quote character. */

                  while (bs >= 2)
                    {
                      PUTC ('\\');
                      bs -= 2;
                    }
                  if (bs & 1)
                    PUTC ('"');
                  else
                    {
                      /* The number of backslashes preceding the
                         double quote character is even (including
                         zero), therefore this double quote character
                         starts or ends a quoted string. */

                      quote = !quote;
                      if (flag_ptr != NULL)
                        *flag_ptr |= _ARG_DQUOTE;
                    }

                  /* We have eaten all backslashes. */

                  bs = 0;
                }
              else if (*s == '\\')
                {
                  /* Instead of looking ahead to learn whether the
                     backslash is followed by (backslashes and) a
                     double quote character, we count the number of
                     successive backslashes and consider them when
                     processing another character. */

                  ++bs;
                }
              else
                {
                  /* Process backslashes preceding this character. */

                  while (bs != 0)
                    {
                      PUTC ('\\');
                      --bs;
                    }

                  /* Whitespace ends the current argument unless we
                     are inside a quoted string. */

                  if (*s == 0 || (WHITE (*s) && !quote))
                    break;
                  PUTC (*s);
                }
              ++s;
            }

          /* Mark the end of the argument string. */

          PUTC (0);
        }
    }

  /* Mark the end of the vector of pointers to argument strings. */

  if (vec != NULL)
    *vec = NULL;
}


/* Build the table of environment pointers; don't store if VEC is
   NULL. */

static char **parse_env (char **vec)
{
  char *s;

  env_count = 0;
  s = startup_env;
  while (*s != 0)
    {
      ++env_count;
      if (vec != NULL)
        *vec++ = s;
      while (*s != 0)
        ++s;
      ++s;
    }
  if (vec != NULL)
    *vec++ = NULL;
  return vec;
}


/* Compute the number and size of argument strings and environment
   strings. */

void count_arg_env (void)
{
  parse_env (NULL);
  parse_arg (NULL, NULL, startup_args, argstr3);
}


/* Build the tables of argument strings and environment strings. */

void build_arg_env (char *str, char **vec)
{
  vec = parse_env (vec);
  parse_arg (str, vec, startup_args, argstr3);
}


/* Semaphores for communicating with the parent process after a fork. */

static HEV fork_req_sem;
static HEV fork_ack_sem;

/* Various values reveived from the parent process. */

static ULONG fork_msize;
static ULONG fork_addr;
static ULONG fork_ppid;


/* This process seems to have been forked off; parse the arguments
   passed on the command line. */

void init_fork (const char *s)
{
  ++s;                          /* Skip the caret */
  if (!conv_hex8 (s, &fork_addr))
    return;
  s += 8;
  if (*s != ' ')
    return;
  ++s;
  if (!conv_hex8 (s, &fork_ppid))
    return;
  s += 8;
  if (*s != 0)
    return;
  fork_flag = TRUE;
}


/* Final initializations for a forked process. */

static void init2_fork (void)
{
  thread_data *td;

  /* Copy the signal handlers from the shared memory object to the
     thread data block for thread 1. */

  td = threads[1];
  memcpy (td->sig_table, p_fork_done->sig_actions, sizeof (td->sig_table));

  /* Install socket handles inherited from the parent process. */

  copy_fork_sock (p_fork_done);

  /* Initalize other file handles.  This must be done after calling
     copy_fork_sock()! */

  fileio_fork_child (p_fork_done);

  /* Now free the shared memory object, it's no longer needed. */

  DosFreeMem ((void *)fork_addr);
  p_fork_done = NULL;
}


/* Connect to the parent process and process the initial block of
   data. */

static void connect_fork (void)
{
  ULONG rc;
  struct fork_data_init *pi;

  rc = DosGetSharedMem ((void *)fork_addr, PAG_READ);
  if (rc != 0)
    {
      error (rc, "DosGetSharedMem");
      quit (255);
    }

  pi = (struct fork_data_init *)fork_addr;
  fork_msize = pi->msize;

  fork_req_sem = pi->req_sem;
  rc = DosOpenEventSem (NULL, &fork_req_sem);
  if (rc != 0)
    {
      error (rc, "DosOpenEventSem");
      quit (255);
    }

  fork_ack_sem = pi->ack_sem;
  rc = DosOpenEventSem (NULL, &fork_ack_sem);
  if (rc != 0)
    {
      error (rc, "DosOpenEventSem");
      quit (255);
    }

  heap_objs[0].brk = pi->brk;
  if (heap_objs[0].brk != 0)
    {
      ULONG size;

      size = heap_objs[0].brk - heap_objs[0].base;
      if (size != 0)
        setmem (heap_objs[0].base, size, PAG_DEFAULT | PAG_COMMIT, 0);
    }

  if (pi->stack_base != stack_base)
    {
      /* Moving the stack is no longer supported. */
      error (0, "fork stack problem");
      quit (255);
    }
  fork_stack_page = pi->stack_page;
  fork_ebp = pi->reg_ebp;
  umask_bits = pi->umask;
  umask_bits1 = pi->umask1;
  uflags = pi->uflags;
}


/* Copy data from the parent process. */

void copy_fork (void)
{
  fork_data *p;
  ULONG rc;
  HMODULE hmod;
  static HMODULE cache_hmod;    /* = NULLHANDLE */
  static int cache_loaded;      /* = whatever (0) */

  for (;;)
    {
      reset_event_sem (fork_req_sem);
      DosPostEventSem (fork_ack_sem);
      do
        {
          rc = DosWaitEventSem (fork_req_sem, 500);

          /* Repeat if interrupted by a signal.  Repeat if timed out
             and the parent process is still the same (that is, it is
             still alive). */

        } while (rc == ERROR_INTERRUPT
                 || (rc == ERROR_TIMEOUT
                     && init_pib_ptr->pib_ulppid == fork_ppid));
      if (rc != 0)
        {
          error (rc, "DosWaitEventSem");
          quit (255);
        }

      /* Don't move this assignment outside the loop, unless you make
         `p' a volatile pointer. */

      p = (fork_data *)fork_addr;

      switch (p->req_code)
        {
        case FORK_REQ_DONE:
          /* Last transaction. */
          if (p->done.sock_count != 0)
            {
              /* To keep reference counts correct in case TCP/IP
                 initialization fails, we have to initialize now. */

              if (!tcpip_init_fork (&p->done))
                quit (255);
            }

          /* Fork completed.  Don't free the shared memory object now,
             as we need the information on signals and sockets later,
             after proceeding with initializing. */

          p_fork_done = &p->done;
          DosPostEventSem (fork_ack_sem);

          /* Note: Do not call close_event_sem() as these semaphores
             have been opened with DosOpenEventSem, not created with
             create_event_sem()! */

          DosCloseEventSem (fork_req_sem);
          DosCloseEventSem (fork_ack_sem);
          return;

        case FORK_REQ_DLL:
          /* Load a DLL.  There's not much point in checking if the
             module is already loaded (by querying the name for the
             module handle) -- if it is already loaded, the following
             code will be redundant. */

          rc = load_module (p->dll.path, &hmod);
          if (rc != 0)
            {
              error (rc, "DosLoadModule");
              quit (255);
            }
          if (hmod != p->dll.hmod)
            {
              oprintf ("HMODULE mismatch: %u vs. %u\r\n",
                       (unsigned)hmod, (unsigned)p->dll.hmod);
              quit (255);
            }
          break;

        case FORK_REQ_MEM:
          /* Receive memory from parent process. */
          if (p->mem.hmod != 0 && p->mem.hmod != cache_hmod)
            {
              cache_hmod = p->mem.hmod;
              cache_loaded = fork_dll_registered (p->mem.hmod);
            }
          if (p->mem.hmod == 0 || cache_loaded)
            memcpy ((void *)p->mem.address, p->mem.shared, p->mem.count);
          break;

        default:
          /* Ignore unknown request codes. */
          break;
        }
    }
}
