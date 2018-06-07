/* emxbind.h -- Global header file for emxbind
   Copyright (c) 1991-1998 Eberhard Mattes

This file is part of emxbind.

emxbind is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxbind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxbind; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include "a_out.h"
#include <sys/param.h>

#if defined (EXTERN)
#define INIT(X) = X
#define INIT_GROW = GROW_INIT
#else
#define INIT(X)
#define INIT_GROW
#define EXTERN extern
#endif

/* By default, don't implement the -L option (for listing the headers
   of an EXE file). */

/* Put this at the end of a function declaration to tell the compiler
   that the function never returns. */

#define NORETURN2 __attribute__ ((noreturn))

/* Number of characters in a file name. */

#define FNAME_SIZE 512

/* This is the default value for the -h option (heap size). */

#define DEFAULT_HEAP_SIZE   0x02000000      /* 32 MB */

/* This is the default value for the -k option (stack size). */

#define DEFAULT_STACK_SIZE  (1024*1024)

/* A_OUT_OFFSET is the offset from the start of the a.out file to the
   text segment in the file. */

#define A_OUT_OFFSET        0x0400

/* This is the start address (in memory) of the text segment. */

#define TEXT_BASE           0x00010000

/* We use up to 4 memory objects in an LX executable file. */

#define OBJECTS             4

/* Source types for LX fixup records. */

#define NRSTYP      0x0f        /* Mask */
#define NRSBYT      0x00        /* 8-bit byte (8-bits) */
#define NRSSEG      0x02        /* 16-bit selector (16-bits) */
#define NRSPTR      0x03        /* 16:16 pointer (32-bits) */
#define NRSOFF      0x05        /* 16-bit offset (16-bits) */
#define NRPTR48     0x06        /* 16:32 pointer (48-bits) */
#define NROFF32     0x07        /* 32-bit offset (32-bits) */
#define NRSOFF32    0x08        /* 32-bit self-relative offset (32-bits) */

#define NRALIAS     0x10        /* Fixup to 16:16 alias */
#define NRCHAIN     0x20        /* List of source offsets follows */

/* Reference types for LX fixup records. */

#define NRRTYP      0x03        /* Mask */
#define NRRINT      0x00        /* Internal reference */
#define NRRORD      0x01        /* Import by ordinal */
#define NRRNAM      0x02        /* Import by name */

/* Flags for LX fixup records. */

#define NRADD       0x04        /* Additive fixup */
#define NR32BITOFF  0x10        /* 32-bit target offset */
#define NR32BITADD  0x20        /* 32-bit additive fixup */
#define NR16OBJMOD  0x40        /* 16-bit object/module ordinal */
#define NR8BITORD   0x80        /* 8-bit import ordinal */


/* How to open a file.  This type is used for my_open(). */

enum open_mode
{
  open_read, create_write, open_read_write
};

/* A file.  OK is non-zero if the file is open.  DEL is not yet used.
   F is the stream.  NAME is the name of the file. */

struct file
{
  int ok, del;
  FILE *f;
  const char *name;
};


/* Internal representation of fixups. */

#define FIXUP_ABS       0
#define FIXUP_REL       1
#define FIXUP_FAR16     2

#define TARGET_ORD      0
#define TARGET_NAME     1
#define TARGET_ADDR     2

struct fixup
{
  int type;                   /* fixup type, see constants above */
  int target;                 /* fixup target, see constants above */
  dword addr;                 /* address of patch */
  int obj;                    /* object number (where to patch) */
  int mod;                    /* module number or object (target) */
  dword dst;                  /* name or ordinal or address */
  dword add;                  /* additive fixup if non-zero */
};

/* Internal representation of an export definition. */

struct export
{
  char *entryname;
  char *internalname;
  long ord;
  int resident;
  dword offset;
  int object;
  int flags;
};

/* A growable buffer. */

#define GROW_INIT {NULL, 0, 0}

struct grow
{
  byte *data;
  size_t size;
  size_t len;
};


/* This variable holds the command code.  For instance, it is 'b' for
   binding. */

EXTERN int mode INIT (0);

/* Verbosity level. */

EXTERN int verbosity INIT (1);

/* The OS/2 heap size, in bytes.  The default value is 32 MB.  The
   heap size can be changed with the -h option. */

EXTERN long heap_size INIT (DEFAULT_HEAP_SIZE);

/* The OS/2 stack size, in bytes.  The default value is 8 MB.  The
   stack size can be changed with the -k option and the STACKSIZE
   module statement. */

EXTERN long stack_size INIT (DEFAULT_STACK_SIZE);

/* The name of the core dump file (set by the -c option) or NULL (if
   there is no core dump file). */

EXTERN char *opt_c INIT (NULL);

/* Create a full-screen application.  This flag is set by the -f
   option. */

EXTERN int opt_f INIT (FALSE);

/* Create a Presentation Manager application.  This flag is set by the
   -p option. */

EXTERN int opt_p INIT (FALSE);

/* The name of the map file (set by the -m option) or NULL (if no .map
   file should be written). */

EXTERN char *opt_m INIT (NULL);

/* The name of the binary resource file (set by the -r option) or NULL
   (if there is no binary resource file). */

EXTERN char *opt_r INIT (NULL);

/* Strip symbols.  This flag is set by the -s option. */

EXTERN int opt_s INIT (FALSE);

/* Create a windowed application.  This flag is set by the -w
   option. */

EXTERN int opt_w INIT (FALSE);

/* The module name. */

EXTERN char *module_name INIT (NULL);

/* The application type. */

EXTERN int app_type INIT (_MDT_DEFAULT);

/* INITINSTANCE, INITGLOBAL, TERMINSTANCE and TERMGLOBAL. */

EXTERN int init_global INIT (TRUE);
EXTERN int term_global INIT (TRUE);

/* The description string. */

EXTERN char *description INIT (NULL);

/* The emx options for DOS and OS/2, respectively, are stored in these
   buffers, before being written to the emxbind headers. */

EXTERN char options_for_dos[512];
EXTERN char options_for_os2[512];

/* The name of the output file. */

EXTERN char out_fname[FNAME_SIZE];

/* File structure for the input executable.  Note that the `ok' field
   is initialized to FALSE to avoid closing an unopened file. */

EXTERN struct file inp_file INIT ({FALSE});

/* File structure for the binary resource file (-r option).  Note that
   the `ok' field is initialized to FALSE to avoid closing an unopened
   file. */

EXTERN struct file res_file INIT ({FALSE});

/* File structure for the destination executable file.  Note that the
   `ok' field is initialized to FALSE to avoid closing an unopened
   file. */

EXTERN struct file out_file INIT ({FALSE});

/* File structure for the DOS stub file.  Note that the `ok' field
   is initialized to FALSE to avoid closing an unopened file. */

EXTERN struct file stub_file INIT ({FALSE});

/* File structure for the core dump file (-c option).  Note that the
   `ok' field is initialized to FALSE to avoid closing an unopened
   file. */

EXTERN struct file core_file INIT ({FALSE});

/* The DOS EXE headers read from the input executable. */

EXTERN struct exe1_header inp_h1;
EXTERN struct exe2_header inp_h2;

/* The a.out header read from the source a.out (sub)file. */

EXTERN struct exec a_in_h;

/* The fixed part of the LX header. */

EXTERN struct os2_header os2_h;

/* The location of the a.out header in the source file. */

EXTERN long a_in_pos;

/* The size of the string table in the source a.out (sub)file. */

EXTERN long a_in_str_size;

/* The offsets of the data section and symbol table, respectively, of
   the source a.out (sub)file. */

EXTERN long a_in_data;
EXTERN long a_in_sym;

/* This table contains all the fixups of the module, in an internal
   format.  fixup_size is the number of entries allocated, fixup_len
   is the number of entries used. */

EXTERN struct fixup *fixup_data INIT (NULL);
EXTERN int fixup_size INIT (0);
EXTERN int fixup_len INIT (0);

/* procs holds the import procedure name table (table of import symbol
   names) of the executable, in LX EXE format, while it is being
   built. */

EXTERN struct grow procs INIT_GROW;

/* Create a relocatable executable if this flag is true.  This flag is
   set when creating a DLL. */

EXTERN int relocatable INIT (FALSE);

/* The base address of the initialized data segment. */

EXTERN dword data_base;

/* Add one of these offsets to an text or data address, respectively,
   to get the location in the input executable. */

EXTERN long text_off;
EXTERN long data_off;

/* These arrays contain the text and data sections, respectively, of
   the source a.out file. */

EXTERN byte *text_image INIT (NULL);
EXTERN byte *data_image INIT (NULL);

/* These arrays contain the text and data relocation tables,
   respectively, as read from the source a.out file. */

EXTERN struct relocation_info *tr_image INIT (NULL);
EXTERN struct relocation_info *dr_image INIT (NULL);

/* This array contains the symbol table, as read from the source a.out
   file. */

EXTERN struct nlist *sym_image INIT (NULL);

/* This array contains the string table, as read from the source a.out
   file. */

EXTERN byte *str_image INIT (NULL);

/* This is the number of symbols of the source a.out file. */

EXTERN int sym_count INIT (-1);

/* This array holds the object table entries.  Usually, the entries
   are accessed via aliases, such as obj_text. */

EXTERN struct object obj_h[OBJECTS];

/* These are indices for the obj_h array. */

#define OBJ_TEXT 0
#define OBJ_DATA 1
#define OBJ_HEAP 2
#define OBJ_STK0 3

/* Define shortcuts for the elements of the obj_h array. */

#define obj_text obj_h[OBJ_TEXT]
#define obj_data obj_h[OBJ_DATA]
#define obj_heap obj_h[OBJ_HEAP]
#define obj_stk0 obj_h[OBJ_STK0]

/* This is the number of resource objects. */

EXTERN int res_obj_count;

/* This is the number of resources. */

EXTERN int res_count;

/* This is the number of resource pages. */

EXTERN int res_pages;

/* This is the number of preload resource pages. */

EXTERN int res_preload_pages;

/* entry_tab holds the entry table (table of exports) of the
   executable, in LX EXE format, while it is being built. */

EXTERN struct grow entry_tab INIT_GROW;

/* resnames holds the resident name table (table of export symbol
   names) of the executable, in LX EXE format, while it is being
   built. */

EXTERN struct grow resnames INIT_GROW;

/* nonresnames holds the non-resident name table (table of export
   symbol names) of the executable, in LX EXE format, while it is
   being built. */

EXTERN struct grow nonresnames INIT_GROW;

/* The DOS EXE headers to be written to the output executable. */

EXTERN struct exe1_header out_h1;
EXTERN struct exe2_header out_h2;

/* The location of the LX header of the output executable. */

EXTERN long os2_hdr_pos;

/* The location of the LX header in the input executable. */

EXTERN long inp_os2_pos;

/* The location of the a.out header in the output executable. */

EXTERN long a_out_pos;

/* The number of zero bytes to insert between the emx.exe or emxl.exe
   image and the LX header. */

EXTERN long fill2;

/* This flag is set if a LIBRARY statement is seen in the module
   definition file. */

EXTERN int dll_flag INIT (FALSE);


/* Round up to the next multiple of 4. */

#define round_4(X) ((((X)-1) & ~3) + 4)

/* Divide by the page size, rounding down. */

#define div_page(X) ((X) >> 12)

/* Compute the number of pages required for X bytes. */

#define npages(X) div_page (round_page (X))

/* Return the low 16-bit word of a 32-bit word. */

#define LOWORD(X) ((X) & 0xffff)

/* Return the high 16-bit word of a 32-bit word. */

#define HIWORD(X) ((X) >> 16)

/* Combine two 16-bit words (low word L and high word H) into a 32-bit
   word. */

#define COMBINE(L,H) ((L) | (long)((H) << 16))


/* exec.c */

void read_stub (void);
void read_a_out_header (void);
void read_core (void);
void read_segs (void);
void read_reloc (void);
void read_sym (void);
void read_os2_header (void);
void exe_flags (void);
void set_options (void);
void set_exe_header (void);
void put_header_byte (byte x);
void put_header_word (word x);
void put_header_dword (dword x);
void put_header_bytes (const void *src, size_t size);
void init_os2_header (void);
void set_os2_header (void);
void write_header (void);
void write_nonres (void);
void copy_a_out (void);
void fill (long count);
void copy (struct file *src, long size);

/* fixup.c */

void build_sym_hash_table (void);
struct nlist *find_symbol (const char *name);
void sort_fixup (void);
void create_fixup (const struct fixup *fp, int neg);
void relocations (void);
void os2_fixup (void);
void put_impmod (void);

/* export.c */

void add_export (const struct export *exp);
void entry_name (struct grow *table, const char *name, int ord);
void exports (void);
const struct export *get_export (int i);

/* resource.c */

void read_res (void);
void put_rsctab (void);
void put_res_obj (int map);
void write_res (void);

/* cmd.c */

void cmd (int mode);

/* list.c */

/* map.c */

void write_map (const char *fname);
void map_import (const char *sym_name, const char *mod, const char *name,
    int ord);

/* utils.c */

void error (const char *fmt, ...) NORETURN2;
void warning (const char *fmt, ...);
void my_read (void *dst, size_t size, struct file *f);
void my_read_str (byte *dst, size_t size, struct file *f);
void my_write (const void *src, size_t size, struct file *f);
void my_seek (struct file *f, long pos);
void my_skip (struct file *f, long pos);
long my_size (struct file *f);
long my_tell (struct file *f);
void my_trunc (struct file *f);
void my_change (struct file *dst, struct file *src);
void my_open (struct file *f, const char *name, enum open_mode mode);
void my_close (struct file *f);
void my_remove (struct file *f);
int my_readable (const char *name);
void *xmalloc (size_t n);
void *xrealloc (void *p, size_t n);
char *xstrdup (const char *p);
void put_grow (struct grow *dst, const void *src, size_t size);
const char *plural_s (long n);
