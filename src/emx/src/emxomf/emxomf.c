/* emxomf.c -- Convert GNU-style a.out object files (with emx extensions)
               to OS/2-style OMF (Object Module Formats) object files
   Copyright (c) 1992-1998 Eberhard Mattes

This file is part of emxomf.

emxomf is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxomf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxomf; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <alloca.h>
#include <ctype.h>
#include <getopt.h>
#include <alloca.h>
#include <sys/param.h>
#include <sys/emxload.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ar.h>
#include <assert.h>

/* Insert private header files. */

#include "defs.h"               /* Common definitions */
#include "emxomf.h"             /* Global definitions for emxomf */
#include "stabshll.h"           /* Convert debug information */
#include "grow.h"               /* Growing objects */

/* This header must come after defs.h */

#include <sys/omflib.h>         /* Handling OMF libraries */

/* Flag bits for FLAGS field of the struct symbol structure. */

#define SF_FAR16        0x01    /* 16:16 pointer */

/* Length of the weak marker. */

#define SYMBOL_WEAK_LENGTH  (3 + 11 + 6)

/* Length of the OMF symbol hash */

#define SYMBOL_HASH_LENGTH  (2 + 6)

/* Max OMF symbol length */

#define SYMBOL_MAX_LENGTH   (255 - SYMBOL_HASH_LENGTH - SYMBOL_WEAK_LENGTH)
/* This structure holds additional data for symbols. */

struct symbol
{
  int index;                    /* EXTDEF index */
  int flags;                    /* SF_FAR16 */
  int hll_type;                 /* HLL type index. */
};

/* This program keeps OMF symbols (names) in a list of lname
   structures. */

struct lname
{
  struct lname *next;           /* Pointer to next element */
  char *name;                   /* Name of the symbol */
  int index;                    /* OMF index of the symbol */
};

/* Names of files to be deleted on program termination are stored in a
   list of delete structures. */

struct delete
{
  struct delete *next;          /* Pointer to next element */
  char *name;                   /* Name of the file to be deleted */
};

/* This structure holds the information about one class (set) of a.out
   set elements.  Set elements are used for constructors and
   destructors.  These structures are chained in a list.  See
   write_set_data() for details. */

struct set
{
  struct set *next;             /* Pointer to next class of set elements */
  char *name;                   /* Name of the set (the a.out symbol) */
  int seg_name[3];              /* Name indices of the three segments */
  int seg_index[3];             /* Segment indices of the three segments */
  int def;                      /* Non-zero if head element encountered */
  int count;                    /* Number of non-head non-tail elements */
  dword *data;                  /* Values of set elements (COUNT elements) */
  byte *seg;                    /* Segments of set elements (COUNT elements) */
};

/* Default library requests (see -i option) are stored in a list of
   libreq structures. */

struct libreq
{
  char *name;                   /* Name of the default library */
  struct libreq *next;          /* Pointer to next element */
};

/* This structure describes an OMF-style line number. */

struct line
{
  dword addr;                   /* Start address of the line */
  int line;                     /* Line number */
  int file_index;               /* File index */
};

/* fetch_modstr() caches the string it has retrieved from other
   modules of the same library.  The cache consists of a a linked list
   of `struct modstr' structures. */

struct modstr
{
  struct modstr *next;          /* Pointer to next node */
  char *symbol;                 /* Symbol attached to the string */
  char *str;                    /* The value of the string */
};


/* The number of symbols in the a.out file. */
int sym_count = 0;

/* The sym_ptr variable points to an array of a.out symbols.  There
   are sym_count table entries.  Note that this table is complemented
   by the sym_more table. */
const struct nlist *sym_ptr = NULL;

/* The str_ptr variable points to the a.out string table.  The
   elements of sym_ptr contain offsets into this table.  The table
   starts with a 32-bit number.  The rest of the table consists of
   null-terminated strings. */
const byte *str_ptr = NULL;

/* The text_ptr variable points to the first byte of the text
   segment.  text_size bytes are available at text_ptr. */
byte *text_ptr = NULL;
long text_size = 0;

/* The data_ptr variable points to the first byte of the data
   segment.  data_size bytes are available at data_ptr. */

static byte *data_ptr;
static long data_size;

/* These varibles point to the text and data relocation tables. */

static const struct relocation_info *text_rel;
static const struct relocation_info *data_rel;

/* Public variables for communication with stabshll.c. */

struct buffer tt = BUFFER_INIT;
struct buffer sst = BUFFER_INIT;
struct buffer sst_reloc = BUFFER_INIT;
struct grow sst_boundary_grow = GROW_INIT;
struct grow tt_boundary_grow = GROW_INIT;
int *sst_boundary = NULL;
int *tt_boundary = NULL;
/** The HLL version of the HLL debuginfo we're generating.
 * Default level is 4.
 *
 * VisualAge 3.65 iLink support HLL v6
 * VisualAge 3.08 iLink support HLL v4
 * Link386              support HLL v3 (?)
 */
int hll_version = 4;

/* Private variables. */

/* The name of the current input file. */
static const char *inp_fname;

/* The name of the current output file. */
static const char *out_fname = NULL;

/* Use this file name for reporting errors.  This is either the name
   of the input file or a library name plus module name:
   library(module). */
const char *error_fname;

/* The output directory.  This is set by the -O option. */
static const char *output_dir = "";

/* The current input file. */
static FILE *inp_file;

/* The current output file. */
static FILE *out_file = NULL;

/* While writing a LIB response file, this variable contains the
   stream pointer.  Otherwise, this variable is NULL. */
static FILE *response_file = NULL;

/* When creating an OMF library (.lib file), this variable contains
   the descriptor used by the omflib library.  Otherwise, this
   variable is NULL. */
static struct omflib *out_lib = NULL;

/* This buffer receives error messages from the omflib library. */
static char lib_errmsg[512];

/* emxomf reads a complete a.out module into memory.  inp_buf points
   to the buffer holding the current a.out module.  This pointer is
   aliased by sym_ptr etc. */
static byte *inp_buf;

/* OMF records are constructed in this buffer.  The first three bytes
   are used for the record type and record length.  One byte at the
   end is used for the checksum. */
static byte out_buf[1+2+MAX_REC_SIZE+1];

/* Index into out_buf (to be more precise: it's a pointer into
   out_data, which is an alias to out_buf+3) for the next byte of the
   OMF record. */
static int out_idx;

/* The list of OMF names.  lnames is the head of the list, lname_add
   is used to add another name to the end of the list. */
static struct lname *lnames;
static struct lname **lname_add;

/* The list of sets.  sets is the head of the list, set_add is used to
   add another set to the end of the list. */
static struct set *sets;
static struct set **set_add;

/* If this variable is TRUE, an a.out archive is split into several
   OMF .obj files (-x option).  If this variable is FALSE, an a.out
   archive is converted into an OMF library file. */
static int opt_x = FALSE;

/* Remove underscores from all symbol names */
static int opt_rmunder = FALSE;

/* This is the page size for OMF libraries.  It is set by the -p
   option. */
static int page_size = 0;

/* The index of the next OMF name, used by find_lname().  Name indices
   are established by LNAMES records written by write_lnames(). */
static int lname_index;

/* The index of the next segment, used by seg_def() for creating
   SEGDEF records. */
static int segdef_index;

/* The index of the next group, used for creating GRPDEF records. */
static int group_index;

/* The index of the next symbol, used for creating EXTDEF and COMDEF
   records. */
static int sym_index;

/* The index of the "WEAK$ZERO" symbol or 0. */
static int weak_zero_index;

/* OMF name indices: segments, classes, groups, etc. */
static int ovl_name;            /* Overlay name, "" */
static int text_seg_name;       /* Text segment, "TEXT32" */
static int data_seg_name;       /* Data segment, "DATA32" */
static int udat_seg_name;       /* Explicit data segment, see -D */
static int stack_seg_name;      /* Stack segment, "STACK" */
static int bss_seg_name;        /* Uninitialized data segment, "BSS32" */
static int symbols_seg_name;    /* Symbols segment, "$$SYMBOLS" */
static int types_seg_name;      /* Types segment */
static int code_class_name;     /* Code class, "CODE" */
static int data_class_name;     /* Data class, "DATA" */
static int stack_class_name;    /* Stack class, "STACK" */
static int bss_class_name;      /* Uninitialized data class, "BSS" */
static int debsym_class_name;   /* Symbols class, "DEBSYM" */
static int debtyp_class_name;   /* Types class, "DEBTYP" */
static int flat_group_name;     /* FLAT group, "FLAT" */
static int dgroup_group_name;   /* DGROUP group, "DGROUP" or "DATAGROUP" */

/* Segment indices for the TEXT32, DATA32, STACK, BSS32, $$SYMBOLS,
   $$TYPES and explicit data segments. */
static int text_index;          /* Text segment, TEXT32 */
static int data_index;          /* Data segment, DATA32 */
static int udat_index;          /* Data segment set by -D or same as above */
static int stack_index;         /* Stack segment, STACK */
static int bss_index;           /* Uninitialized data segment, BSS32 */
static int symbols_index;       /* Symbols segment, $$SYMBOLS */
static int types_index;         /* Types segment, $$TYPES */

/* Group indices for the FLAT and DGROUP groups. */
static int flat_index;          /* FLAT group */
static int dgroup_index;        /* DGROUP group */

/* Next FIXUPP thread index to be used for the frame and target
   thread, respectively. */
static int frame_thread_index;  /* Frame thread */
static int target_thread_index; /* Target thread */

/* We define target threads for referencing the TEXT32, DATA32 (or the
   one set by -D) and BSS32 segments and the FLAT group in FIXUPP
   records.  A thread has not been defined if the variable is -1.
   Otherwise, the value is the thread number. */
static int text_thread;         /* TEXT32 segment */
static int data_thread;         /* Data segment (DATA32 or -D) */
static int bss_thread;          /* BSS32 segment */
static int flat_thread;         /* FLAT group */

/* This variable holds the module name.  See get_mod_name(). */
static char mod_name[256];

/* This variable holds the base(=current) directory.  See get_mod_name(). */
static char base_dir[256];

/* This variable holds the timestamped weak marker for the current module. */
static char weak_marker[SYMBOL_WEAK_LENGTH + 1];

/* This growing array holds the file name table for generating line
   number information.  */
static char **file_list;
static struct grow file_grow;

/* This variable points to the a.out header of the input module. */
static const struct exec *a_out_h;

/* This table contains additional information for the a.out symbols.
   Each entry of the sym_ptr table is complemented by an entry of this
   table. */
static struct symbol *sym_more;

/* The length of the a.out string table, in bytes.  This value
   includes the 32-bit number at the start of the string table. */
static long str_size;

/* omflib_write_module() stored the page number of the module to this
   variable. */
static word mod_page;

/* This variable is TRUE until we have written the first line to the
   LIB response file.  It is used to suppress the line continuation
   character `&'. */
static int response_first = TRUE;

/* This is the list of files to be deleted on program termination.
   See delete_files(). */
static struct delete *files_to_delete = NULL;

/* The module type (-l and -m options).  It is MT_MODULE unless the -l
   or -m option is used. */
static enum {MT_MODULE, MT_MAIN, MT_LIB} mod_type = MT_MODULE;

/* The start (entry point) symbol (-m option).  This is NULL unless
   the -m option (or -l with argument) is used. */
static char *entry_name = NULL;

/* The name of the identifier manipulation DLL.  If this variable is
   NULL, no IDMDLL record is written. */
static char *idmdll_name = "INNIDM";

/* the name of the debug packing DLL. This variable is not used when
   debug info is stripped. If NULL we'll select the default for the
   linker type (EMXOMFLD_TYPE). */
static char *dbgpack_name = NULL;

/* If this variable is TRUE (-b option), we always use the 32-bit
   variant of the OMF records.  If this variable is FALSE, we use the
   16-bit variant whenever possible.  This non-documented feature is
   used for debugging. */
static int force_big = FALSE;

/* The name of the LIB response file (-r and -R options).  This value
   is NULL unless the -r or -R option is used. */
static char *response_fname = NULL;

/* When creating a LIB response file, emit commands for replacing
   modules if this variable is TRUE (-R option).  Otherwise, emit
   commands for adding modules (-r option). */
static int response_replace;

/* Delete input files after successful conversion if this variable is
   TRUE (-d option). */
static int delete_input_files = FALSE;

/* Strip debug information if this variable is TRUE (-s option).
   Otherwise, convert DBX debugging information to HLL debugging
   information. */
static int strip_symbols = FALSE;

/* Print unknown symbol table entries if this variable is TRUE (-u
   option). */
static int unknown_stabs = FALSE;

/* Warning level (-q & -v options). */
int warning_level = 1;

/* The libreq_head variable contains a pointer to the head of the list
   of default libraries.  The -i option is used to add a default
   library request.  The libreq_add pointer is used for appending new
   elements to the tail of the list. */
static struct libreq *libreq_head = NULL;
static struct libreq **libreq_add = &libreq_head;

/* fetch_mod_str() caches the string it has retrieved from other
   modules of the same library.  modstr_cache points to the head of
   the linked list of `struct modstr' structures. */

static struct modstr *modstr_cache = NULL;

/* Simulate the data array of an OMF record.  The data is stored along
   with the record type (1 byte) and record length (2 bytes) in a
   single buffer. */
#define out_data (out_buf+1+2)

/* The data segment name, set by the -D option.  If this value is
   non-NULL, put variables into the named data segment, which isn't a
   member of DGROUP. */

static char *udat_seg_string = NULL;

/* Prototypes for private functions. */

static void doesn_fit (void) NORETURN2;
static void usage (void) NORETURN2;
static int ar_read_header (struct ar_hdr *dst, long pos);
static long ar_member_size (const struct ar_hdr *p);


/* Display an error message on stderr, delete the output file and
   quit.  This function is invoked like printf(): FMT is a
   printf-style format string, all other arguments are formatted
   according to FMT.  The message should not end with a newline. The
   exit code is 2. */

void error (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emxomf: ");
  vfprintf (stderr, fmt, arg_ptr);
  va_end (arg_ptr);
  fputc ('\n', stderr);

  /* If there is an output file, close and delete it. */

  if (out_file != NULL)
    {
      fclose (out_file);
      remove (out_fname);
    }
  exit (2);
}


/* Display a warning message on stderr (and do not quit).  This
   function is invoked like printf(): FMT is a printf-style format
   string, all other arguments are formatted according to FMT. */

void warning (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emxomf warning: ");
  vfprintf (stderr, fmt, arg_ptr);
  va_end (arg_ptr);
  fputc ('\n', stderr);
}


/* Allocate N bytes of memory.  Quit on failure.  This function is
   used like malloc(), but we don't have to check the return value. */

void *xmalloc (size_t n)
{
  void *p;

  p = malloc (n);
  if (p == NULL && n)
    error ("Out of memory");
  return p;
}


/* Change the allocation of PTR to N bytes.  Quit on failure.  This
   function is used like realloc(), but we don't have to check the
   return value. */

void *xrealloc (void *ptr, size_t n)
{
  void *p;

  p = realloc (ptr, n);
  if (p == NULL && n)
    error ("Out of memory");
  return p;
}


/* Create a duplicate of the string S on the heap.  Quit on failure.
   This function is used like strdup(), but we don't have to check the
   return value. */

char *xstrdup (const char *s)
{
  char *p;

  p = xmalloc (strlen (s) + 1);
  strcpy (p, s);
  return p;
}


/* Check if we should remove the leading underscore from a symbol name */
static inline int strip_underscore (const char *name)
{
  if (!opt_rmunder)
    return 0;

  return (*name == '_');
}


/* Find an a.out symbol.  The underscore character `_' is prepended to
   NAME.  On success, a pointer to the symbol table entry (in the
   array pointed to by sym_ptr) is returned.  If the symbol is not
   found, NULL is returned.
   NOT_ENTRY is an entry index which is not to be found.
   FEXT is an indicator on whether or not we can find external symbols or
   not.  */
const struct nlist *find_symbol_ex (const char *name, int not_entry, int fext)
{
  int i, j, n, len, t;

  i = 4; len = strlen (name);

  /* Search the string table for a matching string. */
  while (i < str_size)
    {
      int sym_ofs = i;
      if (strip_underscore ((const char *)(str_ptr + i)))
        ++i;
      if (memcmp (name, str_ptr+i, len+1) == 0)
	{
          /* Search the symbol table for an appropriate entry
             referencing the string. */
	  n = sym_count;
	  for (j = 0; j < n; ++j)
	    if (sym_ptr[j].n_un.n_strx == sym_ofs && j != not_entry)
	      {
		t = sym_ptr[j].n_type & ~N_EXT;
		if (   t == N_TEXT || t == N_DATA || t == N_BSS
                    || (fext && sym_ptr[j].n_type == N_EXT)
                    || (sym_ptr[j].n_type >= N_WEAKA && sym_ptr[j].n_type <= N_WEAKB)
                    || (t == 0 && sym_ptr[j].n_value != 0))
		  return sym_ptr+j;
	      }
	}
      i += strlen ((const char *)(str_ptr + i)) + 1;
    }
  return NULL;                  /* Symbol not found */
}

/* Find an a.out symbol.  The underscore character `_' is prepended to
   NAME.  On success, a pointer to the symbol table entry (in the
   array pointed to by sym_ptr) is returned.  If the symbol is not
   found, NULL is returned. */
const struct nlist *find_symbol (const char *name)
{
  return find_symbol_ex (name, -1, 0);
}


/* Set the hll_type of a symbols. */
void set_hll_type (int index, int hll_type)
{
  if (index > sym_count || index < 0)
    error ("Internal error! Invalid index (%d) passed to set_hll_type().", index);

  /* Ignore type indexes we cannot encode (see put_idx), though, issue a
     warning the first time we see this in a file (our repeat detection
     isn't prefect as heap can be reused, but it'll have to do). */
  if (hll_type < 0 || hll_type > 0x7fff)
    {
      static const char *s_pszComplainedFor = NULL;
      if (s_pszComplainedFor != error_fname)
        {
          s_pszComplainedFor = error_fname;
          if (warning_level >= 1)
            warning("Input file '%s' has more HLL debug types than we can index in PUBDEF and EXTDEF records.",
                    error_fname);
        }
      hll_type = 0;
    }

  sym_more[index].hll_type = hll_type;
}


/* Initialize variables for starting a new OMF output file. */

static void init_obj (void)
{
  /* Initialize the list of OMF-style names. */

  lnames = NULL; lname_add = &lnames; lname_index = 1;

  /* Initialize segments. */

  segdef_index = 1; sym_index = 1; group_index = 1; weak_zero_index = 0;

  /* Initialize FIXUPP threads. */

  text_thread = -1; data_thread = -1; bss_thread = -1; flat_thread = -1;
  frame_thread_index = 0; target_thread_index = 0;

  /* Initialize set elements management. */

  sets = NULL; set_add = &sets;

  /* Initialize the buffers used for converting debugging
     information. */

  buffer_init (&tt);
  buffer_init (&sst);
  buffer_init (&sst_reloc);

  /* Init weak markers */

  weak_marker[0] = '\0';
}


/* Find an OMF-style name.  If NAME is not yet a known OMF-style name,
   it is added to the list of OMF-style names.  The index of the name
   is returned.  All the OMF-style names will be defined in LNAMES
   records.  Note: INDEX must be assigned sequentially, append to end
   of list!  Otherwise, write_lnames() would have to sort the list */

static int find_lname (const char *name)
{
  struct lname *p;

/** @todo: change this to use a stringpool for speed reasons! */

  /* Walk through the list of known OMF-style names.  If there is a
     match, return the index of the name. */

  for (p = lnames; p != NULL; p = p->next)
    if (strcmp (name, p->name) == 0)
      return p->index;

  /* The name is not yet known.  Create a new entry. */

  p = xmalloc (sizeof (struct lname));
  p->name = xstrdup (name);
  p->index = lname_index++;

  /* Add the new entry to the tail of the list of names. */

  p->next = NULL;
  *lname_add = p;
  lname_add = &p->next;

  /* Return the index of the new name. */

  return p->index;
}


/* Deallocate the memory used by the list of OMF-style names. */

static void free_lnames (void)
{
  struct lname *p, *q;

  for (p = lnames; p != NULL; p = q)
    {
      q = p->next;
      free (p->name);
      free (p);
    }
}


/* Start a new OMF record.  TYPE is the record type. */

static void init_rec (int type)
{
  out_buf[0] = (byte)type;
  out_idx = 0;
}


/* Write an OMF record.  The record type has been defined by
   init_rec(), the data has been defined by put_8(), put_idx() etc.
   The checksum is computed by this function.  An OMF record has the
   following format:

   struct omf_record
   {
     byte type;                 The record type
     word length;               The record length, exclusive TYPE and LENGTH
     byte data[LENGTH-1];       The record data
     byte checksum;             The check sum.  The sum of all the bytes in
                                the record is 0 (module 256)
   };

   If we're writing a LIB file, we call omflib_write_record().
   Otherwise, we build and write the record here.  An OMF record
   usually contains multiple definitions, see write_lnames() for a
   simple instance of the loop we use for packing multiple definitions
   into OMF records. */

static void write_rec (void)
{
  byte chksum;
  int i;

  if (out_lib != NULL)
    {

      /* Write the record to the library file. */

      if (omflib_write_record (out_lib, out_buf[0], out_idx, out_data,
                               TRUE, lib_errmsg) != 0)
        error (lib_errmsg);
    }
  else
    {

      /* Store the record length, LSB first. */

      out_buf[1] = (byte)(out_idx+1);
      out_buf[2] = (byte)((out_idx+1) >> 8);

      /* Compute the check sum. */

      chksum = 0;
      for (i = 0; i < 1+2+out_idx; ++i)
        chksum += out_buf[i];
      out_data[out_idx] = (byte)(256-chksum);

      /* Write the record. */

      if (fwrite (out_buf, 1+2+out_idx+1, 1, out_file) != 1)
        error ("Write error on output file `%s'", out_fname);
    }
}


/* This macro yields TRUE iff we can put another X bytes into the
   current OMF record. */

#define fits(X) (out_idx + (X) <= MAX_REC_SIZE-4)

/* This function is called if we try to build too big an OMF record.
   This cannot happen unless there is a bug in this program.  Display
   an error message and quit. */

static void doesn_fit (void)
{
  error ("Record too long");
}


/* Calculates the hash for a string using the original djb2 aglorithm. */
  
static unsigned hash_string(const char *pch, size_t cch)
{
    unsigned uHash;
    for (uHash = 5381; cch > 0; pch++, cch--)
        uHash += (uHash << 5) + *pch;
    return uHash;
}

/* Formats a 64-bit number with a fixed with. The purpose is to encode
   a unique number as tiny as possible while keeping within what any 
   linker should accept. The width is fixed, and the string is clipped 
   or padded with zeros to satisfy that. The return value is psz + cchWidth. */

static char *format_u64(uint64_t u64, char *psz, unsigned uRadix, int cchWidth)
{
    static const char       s_achDigits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const unsigned   s_cchDigits = sizeof(s_achDigits) / sizeof(s_achDigits[0]) - 1;
    assert(uRadix <= s_cchDigits);
    while (cchWidth-- > 0)
    {
        unsigned i = u64 % uRadix;
        *psz++ = s_achDigits[i];
        u64 /= uRadix;
    }
    assert(!u64);
    *psz = '\0';
    return psz;
}

/* Put the string pszName into the given buffer pOutBuf (which must be at least
   256 bytes long). The string length must not exceed 255 bytes (OMF format
   requirement); if it is too long, display a warning and truncate the string.
   The string returned in pOutBuf is zero-terminated. The function returns the
   length of the resulting string. */

static int make_nstr (const char *pszName, size_t cch, char *pszOutBuf)
{
    if (    cch > SYMBOL_MAX_LENGTH
        &&  !strstr(pszName + SYMBOL_MAX_LENGTH - SYMBOL_WEAK_LENGTH, "!_")
        &&  !strstr(pszName + SYMBOL_MAX_LENGTH - SYMBOL_WEAK_LENGTH, "$w$"))
    {
        /* Hash the symbol to help making it unique.
         * NOTE that the hash algorithm is not fixed, so is the !_%X marker too.
         *      the weakld is parsing it!
         */
        unsigned    uHash = hash_string(pszName, cch);
        char        szHash[SYMBOL_HASH_LENGTH + 1];
        char       *psz;
        szHash[0] = '!';
        szHash[1] = '_';
        psz = format_u64((uint32_t)uHash, &szHash[2], 62, 6);
        assert(psz - &szHash[0] == SYMBOL_HASH_LENGTH);

        memcpy(pszOutBuf, pszName, SYMBOL_MAX_LENGTH);
        memcpy(pszOutBuf + SYMBOL_MAX_LENGTH, szHash, SYMBOL_HASH_LENGTH);
        pszOutBuf[SYMBOL_MAX_LENGTH + SYMBOL_HASH_LENGTH] = '\0';

        warning ("Truncated symbol '%.*s' to '%.*s%s'", cch, pszName, SYMBOL_MAX_LENGTH, pszName, szHash);

        cch = SYMBOL_MAX_LENGTH + SYMBOL_HASH_LENGTH;
    }
    else
    {
        assert(cch <= 0xff);
        cch &= 0xff;
        memcpy(pszOutBuf, pszName, cch);
        pszOutBuf[cch] = '\0';
    }
    return cch;
}

/* Put the string pszName into the current OMF record.
   In the OMF record, the string is preceded by a length byte.  The
   string length must not exceed 255; if it is too long, display a
   warning and truncate the string.  Moreover, there must be enough
   space left in the OMF record; if there isn't, display an error
   message and abort. */

static void put_nstr(const char *pszName, size_t cch)
{
    char szName[256];

    cch = make_nstr(pszName, cch, szName);
    
    if (!fits(1 + cch))
        doesn_fit();
    out_data[out_idx++] = (byte)cch;
    memcpy(out_data+out_idx, szName, cch);
    out_idx += cch;
}

/* Put the null-terminated string pszName into the current OMF record.
   See put_nstr() for full details on string handling. */

static inline void put_str(const char *pszName)
{
    put_nstr(pszName, strlen(pszName));
}


/* Put the symbol SRC, a null-terminated string, into the current OMF
   record.  If SRC starts with the underscore character `_', that
   character is skipped.  This is where the leading underscore
   character of a.out symbols is removed.  C modules for emx.dll are
   treated specially here: The leading underscore is not removed
   unless the name looks like the name of an OS/2 API function.  The
   symbol length (after removing the leading underscore) must not
   exceed 255 characters.  There must be enough space left in the OMF
   record.  If these conditions are not met, display an error message
   and quit. */

static void put_sym (const char *src)
{
  if (strip_underscore (src))
    ++src;
  put_str (src);
}

/* Put an 8-bit byte into the current OMF record.  This macro does not
   test for buffer / record overflow. */

#define put_8(x) out_data[out_idx++] = (byte)(x)

/* Put a 16-bit word (least significant byte first) into the current
   OMF record.  If there is not enough space left in the OMF record,
   display an error message and quit. */

static void put_16 (int x)
{
  if (!fits (2))
    doesn_fit ();
  out_data[out_idx++] = (byte)x;
  out_data[out_idx++] = (byte)(x >> 8);
}


/* Put a 24-bit word (least significant byte first) into the current
   OMF record.  If there is not enough space left in the OMF record,
   display an error message and quit. */

static void put_24 (long x)
{
  if (!fits (3))
    doesn_fit ();
  out_data[out_idx++] = (byte)x;
  out_data[out_idx++] = (byte)(x >> 8);
  out_data[out_idx++] = (byte)(x >> 16);
}


/* Put a 32-bit word (least significant byte first) into the current
   OMF record.  If there is not enough space left in the OMF record,
   display an error message and quit. */

static void put_32 (long x)
{
  if (!fits (4))
    doesn_fit ();
  out_data[out_idx++] = (byte)x;
  out_data[out_idx++] = (byte)(x >> 8);
  out_data[out_idx++] = (byte)(x >> 16);
  out_data[out_idx++] = (byte)(x >> 24);
}


/* Put the index X into the current OMF record.  Indices are used for
   identifying names (LNAMES), segments (SEGDEF), groups (GRPDEF)
   etc.  Indices are stored in one or two bytes, depending on the
   value.  Small indices (0 through 127) are stored as single byte.
   Large indices (128 through 32767) are stored as two bytes: the
   high-order byte comes first, with bit 7 set; the low-order byte
   comes next.  If there is not enough space left in the OMF record or
   if the value exceeds 32767, display an error message and quit. */

static void put_idx (int x)
{
  if (x <= 0x7f)
    {
      if (!fits (1))
        doesn_fit ();
      out_data[out_idx++] = (byte)x;
    }
  else if (x <=0x7fff)
    {
      if (!fits (2))
        doesn_fit ();
      out_data[out_idx++] = (byte)((x >> 8) | 0x80);
      out_data[out_idx++] = (byte)x;
    }
  else
    error ("Index too large");
}


/* Put SIZE bytes from SRC into the current OMF record.  If there is
   not enough space left in the OMF record, display an error message
   and quit. */

static void put_mem (const void *src, int size)
{
  if (!fits (size))
    doesn_fit ();
  memcpy (out_data+out_idx, src, size);
  out_idx += size;
}


/* Write LNAMES records into the output file for defining all the
   names stored in the LNAMES list.

   LNAMES record:
   �
   �1   String length n
   �n   Name
   �

   Name indices are assigned sequentially. */

static void write_lnames (void)
{
  const struct lname *p;
  int started;

  started = FALSE;
  for (p = lnames; p != NULL; p = p->next)
    {
      if (started && !fits (strlen (p->name)+1))
        {
          write_rec ();
          started = FALSE;
        }
      if (!started)
        {
          init_rec (LNAMES);
          started = TRUE;
        }
      put_str (p->name);
    }
  if (started)
    write_rec ();
}


/* Add a string to the current EXTDEF record or start a new EXTDEF
   record.  PSTARTED points to an object keeping state. */

static void add_extdef (int *pstarted, const char *name, int type)
{
  if (*pstarted)
    {
      size_t cchEncodedName = strlen(name);
      if (   cchEncodedName > SYMBOL_MAX_LENGTH 
          && !strstr(name + SYMBOL_MAX_LENGTH - SYMBOL_WEAK_LENGTH, "$w$"))
        cchEncodedName = SYMBOL_MAX_LENGTH + SYMBOL_HASH_LENGTH;
      if (!fits(cchEncodedName + 3 + (type > 127)))
        {
           write_rec();
           *pstarted = FALSE;
        }
    }
  if (!*pstarted)
    {
      init_rec(EXTDEF);
      *pstarted = TRUE;
    }
  put_sym(name);
  put_idx(type);                        /* type index */
}


/* Write EXTDEF records into the output file for all the external
   symbols stored in the sym_ptr array.

   EXTDEF record:
   �
   �1   String length n
   �n   External name
   �1-2 Type index
   �

   Symbol indices are assigned sequentially.  */

static void write_extdef (void)
{
  const char *name;
  int i, started;

  started = FALSE;
  for (i = 0; i < sym_count; ++i)
    if (sym_ptr[i].n_type == (N_INDR|N_EXT))
      ++i;                       /* Skip immediately following entry */
    else if (sym_ptr[i].n_type == N_EXT && sym_ptr[i].n_value == 0)
      {
        name = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
        sym_more[i].index = sym_index++;
        if (memcmp (name, "_16_", 4) == 0)
          sym_more[i].flags |= SF_FAR16;
        add_extdef (&started, name, sym_more[i].hll_type);
      }
    else if (sym_ptr[i].n_type == N_WEAKU)
      {
        if (weak_zero_index == 0)
          {
            weak_zero_index = sym_index++;
            add_extdef (&started, "WEAK$ZERO", 0);
          }
        sym_more[i].index = sym_index++;
        add_extdef (&started, (const char *)(str_ptr + sym_ptr[i].n_un.n_strx), sym_more[i].hll_type);
      }
#if 1 /* this isn't actually required if only the assembler could do what it's supposed to... */
    else if (sym_ptr[i].n_type >= N_WEAKA && sym_ptr[i].n_type <= N_WEAKB)
      { /* Not convinced about this yet, I mean there should really be
           external records for this, perhaps...
           At least I can't see why this will be wrong. */
        name = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
        sym_more[i].index = sym_index++;
        add_extdef (&started, name, sym_more[i].hll_type);
      }
#endif
  if (started)
    write_rec ();
}

/* Write WKEXT records into the output file for all undefined weak
   symbols stored in the sym_ptr array.

   WKEXT record:
   �
   �1-2 weak EXTDEF index
   �1-2 default resolution EXTDEF index
   �

   */

static void write_wkext (void)
{
  int i;

  for (i = 0; i < sym_count; ++i)
    if (sym_ptr[i].n_type == (N_INDR|N_EXT))
      ++i;                      /* Skip immediately following entry */
    else if (sym_ptr[i].n_type == N_WEAKU)
      {
        init_rec (COMENT);
        put_8 (0x80);
        put_8 (CLASS_WKEXT);
        put_idx (sym_more[i].index);
        put_idx (weak_zero_index);
        write_rec ();
      }
}


/**
 * Generates new names for weak symbols.
 *
 * These names will be catch bye emxomfld and an alias from the generated to
 * the real name will be generated and put in a library before linking.
 *
 * @returns Pointer to pszOrgName or pszName.
 * @param   pSym        Pointer to the symbol in question.
 * @param   pszOrgName  Pointer to the symbols actual name.
 * @param   pachName    Pointer to a character buffer of size cchName where the
 *                      new name will be written.
 * @param   cchName     Size of the buffer pointed to by pachName.
 *                      This must be at least 256, the code make this assumption!
 * @remark I'm sorry this function is written in my coding style - not!
 */
static const char *weak_process_name(const struct nlist *pSym, const char *pszOrgName, char *pachName, int cchName)
{
    switch (pSym->n_type)
    {
        /*
         * Weak external.
         */
        case N_WEAKU:               /* 0x0d  Weak undefined symbol. */
        default:
            break;

        /*
         * These symbols are defined in this module, so we need to
         * make a unique and recognizable name for them.
         */
        case N_WEAKA:               /* 0x0e  Weak absolute symbol. */
        case N_WEAKT:               /* 0x0f  Weak text symbol. */
        case N_WEAKD:               /* 0x10  Weak data symbol. */
        case N_WEAKB:               /* 0x11  Weak bss symbol. */
        {
            int cch;

            /* Init the weak marker if it hasn't already been done for this module. */
            if (!weak_marker[0])
            {
                struct timeval  tv = {0, 0};
                uint64_t        u64;
                char           *psz;

                /* prefix */
                psz = (char *)memcpy(&weak_marker[0], "$w$", sizeof("$w$")) + sizeof("$w$") - 1;

                /* use time-of-day + 4 random bits for the 11 char value */
                gettimeofday(&tv, NULL);
                u64 = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
                u64 <<= 12;
                u64 |= rand() & 0xfff;
                psz = format_u64(u64, psz, 62, 11);

                /* simply hash the current filename for the next 6 chars value. */
                u64 = hash_string(mod_name, strlen(mod_name));
                psz = format_u64((uint32_t)u64, psz, 62, 6);
                assert((psz - &weak_marker[0]) == SYMBOL_WEAK_LENGTH);
            }


            /* generate the weak symbol name. */
            cch = strlen(pszOrgName);
            if (cch <= SYMBOL_MAX_LENGTH)           /* Must cut exactly where add_nstr cuts. */
                snprintf(pachName, cchName, "%s%s", pszOrgName, weak_marker);
            else
            {   /* too long. truncate to: name+!_[hash]$w$[weakmarker] */
                uint32_t u32Hash = hash_string(pszOrgName, cch);
                memcpy(pachName, pszOrgName, SYMBOL_MAX_LENGTH);
                pachName[SYMBOL_MAX_LENGTH + 0] = '!';
                pachName[SYMBOL_MAX_LENGTH + 1] = '_';
                format_u64(u32Hash, &pachName[SYMBOL_MAX_LENGTH + 2], 62, 6);
                memcpy(&pachName[SYMBOL_MAX_LENGTH + SYMBOL_HASH_LENGTH], weak_marker, SYMBOL_WEAK_LENGTH + 1);
                warning("Truncated symbol '%s' to '%s' (weak)", pszOrgName, pachName);
            }

            return pachName;
        }

    }
    /* default is to keep the original name */
    return pszOrgName;
}



/* Write ALIAS records into the output file for all indirect references. */

static void write_alias (void)
{
  int i;
  const char *pub_name;
  char szPubName[256];
  int cchPubName;

  for (i = 0; i < sym_count - 1; ++i)
    if (sym_ptr[i].n_type == (N_INDR|N_EXT) && sym_ptr[i+1].n_type == N_EXT)
      {
        pub_name = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
        if (strip_underscore (pub_name))
          ++pub_name;
        cchPubName = make_nstr (pub_name, strlen(pub_name), szPubName);

        init_rec (ALIAS);
        put_8 (cchPubName);
        put_mem (szPubName, cchPubName);
        put_sym ((const char *)(str_ptr + sym_ptr[i+1].n_un.n_strx));
        write_rec ();

        if (out_lib != NULL)
          {
            if (omflib_add_pub (out_lib, szPubName, mod_page, lib_errmsg) != 0)
              error (lib_errmsg);
          }
      }
}


/* Write PUBDEF records into the output file for the public symbols of
   the a.out-style symbol type TYPE.  Symbols of that type are defined
   in the segment having the OMF segment index INDEX.  Ignore symbols
   not fitting into a 16-bit PUBDEF record if BIG is FALSE.  Ignore
   symbols fitting into a 16-bit PUBDEF record if BIG is TRUE.  START
   is the virtual start address of the segment in the a.out file.

   PUBDEF record:
    1-2 Base group
    1-2 Base segment
    0/2 Base frame
   �
   �1   String length n
   �n   Public name
   �2/4 Public offset (4 bytes for 32-bit PUBDEF record)
   �1-2 Type index
   �

   The base frame field is present only if the base segment field is
   0. */

static void write_pubdef1 (int type, int index, int big, dword start)
{
  int i, started;
  const char *name, *pub_name;
  dword address;
  char  szName[256];
  char  szPubName[256];
  int   cchPubName;

  started = FALSE;
  for (i = 0; i < sym_count; ++i)
    if (    sym_ptr[i].n_type == type
        || ((type < N_WEAKU || type > N_WEAKB) && (sym_ptr[i].n_type & ~N_EXT) == type)
        )
      {
        address = sym_ptr[i].n_value - start;
        if ((address >= 0x10000 || force_big) == big)
          {
            name = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
            if (   (sym_ptr[i].n_type & N_EXT)
                || (type == N_TEXT && strncmp (name, "___POST$", 8) == 0)
                || (sym_ptr[i].n_type >= N_WEAKU && sym_ptr[i].n_type <= N_WEAKB) )
              {
                /* for weaksymbols we may have to decorate the name */
                if ( sym_ptr[i].n_type >= N_WEAKU && sym_ptr[i].n_type <= N_WEAKB )
                    name = weak_process_name(&sym_ptr[i], name, szName, sizeof(szName));
                pub_name = name;
                if (strip_underscore (pub_name))
                  ++pub_name;
                cchPubName = make_nstr (pub_name, strlen (pub_name), szPubName);

                if (    out_lib != NULL
                    &&  (   (   (sym_ptr[i].n_type & N_EXT)
                             && sym_ptr[i].n_type != N_WEAKB
                             && sym_ptr[i].n_type != N_WEAKU)
                         ||  sym_ptr[i].n_type == N_WEAKT
                         ||  sym_ptr[i].n_type == N_WEAKD ) )
                  {
                    if (omflib_add_pub (out_lib, szPubName, mod_page,
                                        lib_errmsg) != 0)
                      error (lib_errmsg);
                  }

                if (started && !fits (strlen (name) + 6 + (sym_more[i].hll_type > 127)))
                  {
                    write_rec ();
                    started = FALSE;
                  }
                if (!started)
                  {
                    init_rec (big ? PUBDEF|REC32 : PUBDEF);
                    put_idx (flat_index);
                    put_idx (index);
                    if (!index)
                        put_16 (0);
                    started = TRUE;
                  }
                put_8 (cchPubName);
                put_mem (szPubName, cchPubName);
                if (big)
                  put_32 (address);
                else
                  put_16 (address);
                put_idx (sym_more[i].hll_type); /* type index */
              }
          }
      }
  if (started)
    write_rec ();
}

/* Write main alias if _main is a pubdef.
   The debugger looks for 'main' not '_main' in the symbol table. */
static void write_pubdef_main ()
{
    int i;

    if (opt_rmunder || strip_symbols || !a_out_h->a_syms)
        return;

    for (i = 0; i < sym_count; ++i)
      if ((sym_ptr[i].n_type & ~N_EXT) == N_TEXT)
        {
          const char * name    = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
          dword        address = sym_ptr[i].n_value;
          if (   (sym_ptr[i].n_type & N_EXT)
              && !strcmp(name, "_main"))
            {
              int big = address >= 0x10000 || force_big;

              name++; /* skip the underscore */
              if (out_lib != NULL)
                {
                  /* Note! make_nstr is not needed here, main is too short. */
                  if (omflib_add_pub (out_lib, name, mod_page, lib_errmsg) != 0)
                    error (lib_errmsg);
                }
              init_rec (big ? PUBDEF|REC32 : PUBDEF);
              put_idx (flat_index);
              put_idx (text_index);
              put_sym (name);
              if (big)
                put_32 (address);
              else
                put_16 (address);
              put_idx (sym_more[i].hll_type);   /* type index */
              write_rec ();
              break;
            }
        }
}

/* Write PUBDEF records into the output file for all the public N_TEXT
   and N_DATA symbols stored in the sym_ptr array.  Common symbols are
   handled by write_comdef() below. */

static void write_pubdef (void)
{
  write_pubdef1 (N_ABS,   0,          FALSE, 0);
  write_pubdef1 (N_ABS,   0,          TRUE, 0);
  write_pubdef1 (N_TEXT,  text_index, FALSE, 0);
  write_pubdef1 (N_TEXT,  text_index, TRUE,  0);
  write_pubdef1 (N_DATA,  udat_index, FALSE, text_size);
  write_pubdef1 (N_DATA,  udat_index, TRUE,  text_size);
  write_pubdef1 (N_BSS,   bss_index,  FALSE, text_size + data_size);
  write_pubdef1 (N_BSS,   bss_index,  TRUE,  text_size + data_size);
  write_pubdef1 (N_WEAKA, 0,          FALSE, 0);
  write_pubdef1 (N_WEAKA, 0,          TRUE, 0);
  write_pubdef1 (N_WEAKT, text_index, FALSE, 0);
  write_pubdef1 (N_WEAKT, text_index, TRUE,  0);
  write_pubdef1 (N_WEAKD, udat_index, FALSE, text_size);
  write_pubdef1 (N_WEAKD, udat_index, TRUE,  text_size);
  write_pubdef1 (N_WEAKB, bss_index,  FALSE, text_size + data_size);
  write_pubdef1 (N_WEAKB, bss_index,  TRUE,  text_size + data_size);
  /* kso #456 2003-06-10: The debugger looks for 'main' not '_main'. */
  write_pubdef_main ();
}


/* Write COMDEF records into the output file for all the common
   symbols stored in the sym_ptr array.  Common symbols are used for
   uninitialized variables.  The TYPE field of these symbols is 0
   (plus N_EXT) and the VALUE field is non-zero (it is the size of the
   variable).

   COMDEF record:
   �
   �1   String length n
   �n   Communal name
   �1-2 Type index
   �1   Data type (0x61: FAR data, 0x62: NEAR data)
   �1-5 Communal length
   �

   The length is encoded in 1 to 5 bytes, depending on the value:

   0 through 0x7f       1 byte, containing the value
   0 through 0xffff     0x81, followed by 16-bit word
   0 through 0xffffff   0x84, followed by 24-bit word
   0 through 0xffffffff 0x88, followed by 32-bit word */

static void write_comdef (void)
{
  int i, started;
  long size;
  const char *name;

  started = FALSE;
  for (i = 0; i < sym_count; ++i)
    if (sym_ptr[i].n_type == (N_INDR|N_EXT))
      ++i;                      /* Skip immediately following entry */
    else if (sym_ptr[i].n_type == N_EXT && sym_ptr[i].n_value != 0)
      {
        name = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
        sym_more[i].index = sym_index++;
        if (memcmp (name, "_16_", 4) == 0)
          sym_more[i].flags |= SF_FAR16;
        size = sym_ptr[i].n_value;
        if (started && !fits (strlen (name) + 12))
          {
            write_rec ();
            started = FALSE;
          }
        if (!started)
          {
            init_rec (COMDEF);
            started = TRUE;
          }
        put_sym (name);
        put_idx (0);                  /* Type index */
        put_8 (0x62);                 /* Data type */
        if (size < 4)
            size = 4; /* VAC308 ilink freaks out if size = 1 or something */
        if (size < 0x80)
          put_8 (size);
        else if (size < 0x10000)
          {
            put_8 (0x81);
            put_16 (size);
          }
        else if (size < 0x1000000)
          {
            put_8 (0x84);
            put_24 (size);
          }
        else
          {
            put_8 (0x88);
            put_32 (size);
          }
      }
  if (started)
    write_rec ();
}


/* Write a SEGDEF record to define a segment.  NAME_INDEX is the name
   index of the segment name.  CLASS_INDEX is the name index of the
   class name.  SIZE is the size of the segment in bytes.  STACK is
   TRUE iff the segment is the stack segment.  seg_def() returns the
   segment index of the new segment.

   SEGDEF record:
    1   Segment attributes
    0/2 Frame number (present only if A=000)
    0/1 Offset (present only if A=000)
    2/4 Segment length (4 bytes for 32-bit SEGDEF record)
    1/2 Segment name index
    1/2 Segment class index
    1/2 Overlay name index

    The segment attributes byte contains the following fields:

    A (bits 5-7)        Alignment (011=relocatable, paragraph (16b) alignment)
                        (before gcc 3.2.2: 101=relocatable, 32-bit alignment)
    C (bits 2-4)        Combination (010=PUBLIC, 101=STACK)
    B (bit 1)           Big (segment length is 64KB)
    P (bit 0)           USE32 */

static int seg_def (int name_index, int class_index, long size, int stack, int is_set)
{
  byte seg_attr;

  seg_attr =  (is_set ? (stack ? 0xb5 : 0xa9) : (stack ? 0x75 : 0x69) );
  if (size > 0x10000 || force_big)
    {
      init_rec (SEGDEF|REC32);
      put_8 (seg_attr);
      put_32 (size);
    }
  else if (size == 0x10000)
    {
      init_rec (SEGDEF);
      put_8 (seg_attr|2);
      put_16 (0);
    }
  else
    {
      init_rec (SEGDEF);
      put_8 (seg_attr);
      put_16 (size);
    }
  put_idx (name_index);
  put_idx (class_index);
  put_idx (ovl_name);
  write_rec ();
  return segdef_index++;
}


/* This function is passed to qsort() for sorting a relocation table.
   X1 and X2 point to reloc structures.  We compare the ADDRESS fields
   of the structures. */

static int reloc_compare (const void *x1, const void *x2)
{
  dword a1, a2;

  a1 = ((struct relocation_info *)x1)->r_address;
  a2 = ((struct relocation_info *)x2)->r_address;
  if (a1 < a2)
    return -1;
  else if (a1 > a2)
    return 1;
  else
    return 0;
}


/* Write the segment contents (data) of one OMF segment to the output
   file.  Create LEDATA and FIXUPP records.  INDEX is the segment
   index.  SEG_NAME is the name index of the segment name (not used).
   SRC points to the data to be written (will be modified for
   fixups!), SEG_SIZE is the number of bytes to be written.  REL
   points to the relocation table (an array of struct relocation_info), REL_SIZE
   is the size of the relocation table, in bytes(!).  SST_FLAG is TRUE
   when writing the symbol segment $$SYMBOLS.  BOUNDARY points to an
   array of indices into SRC where the data can be split between
   records.  That array has BOUNDARY_COUNT entries.  SEG_TYPE is the
   segment type (N_TEXT or N_DATA or -1) and is used for relocation.

   We can split the data at arbitrary points after the last BOUNDARY
   entry.  In consequence, write_seg() splits the data at arbitrary
   points if BOUNDARY_COUNT is zero.

   LEDATA record:
    1-2 Segment index
    2/4 Enumerated data offset (4 bytes for 32-bit LEDATA record)
    n   Data bytes (n is derived from record length)

   FIXUPP record:
   �
   �?   THREAD subrecord or FIXUP subrecord
   �

   THREAD subrecord:
    1   Flags
    0-2 Index (present only for FRAME methods F0, F1 and F2)

   The flags byte contains the following fields:

   0 (bit 7)            always 0 to indicate THREAD subrecord
   D (bit 6)            0=target thread, 1=frame thread
   0 (bit 5)            reserved
   Method (bits 2-4)    method (T0 through T6 and F0 through F6)
   Thred (bits 0-1)     thread number

   FIXUP subrecord:
    2   Locat
    0-1 Fix data
    0-2 Frame datum
    0-2 Target datum
    2/4 target displacement (4 bytes for 32-bit FIXUPP record)

    The first locat byte contains the following fields:

    1 (bit 7)           always 1 to indicate FIXUP subrecord
    M (bit 6)           1=segment-relative fixup, 0=self-relative fixup
    Location (bit 2-5)  Type of location to fix up:
                          0010=16-bit selector
                          0011=32-bit long pointer (16:16)
                          1001=32-bit offset
    Offset (bits 0-1)   Most significant bits of offset into LEDATA record

    The second locat byte contains the least significant bits of the
    offset into the LEDATA record.

    The Fix data byte contains the following fields:

    F (bit 7)           1=frame thread, 0=methods F0 through F5
    Frame (bits 4-6)    frame thread number (F=1) or frame method (F=0)
    T (bit 3)           1=target thread, 1=methods
    P (bit 2)           Bit 2 of target method
    Targt (bits 0-1)    target thread number (T=1) or target method (T=0)  */

static void write_seg (int index, int seg_name, byte *src, long seg_size,
                       const struct relocation_info *rel, long rel_size, int sst_flag,
                       const int *boundary, int boundary_count, int seg_type)
{
  long n, off, tmp, end;
  int i, reloc_count, reloc_idx, target_index, ok, data_off, far16;
  int boundary_idx, started, *threadp;
  struct relocation_info *reloc_tab;
  const struct relocation_info *r;
  byte locat;
  dword start_data, start_bss;

  target_index = 0; threadp = NULL; /* keep the compiler happy */

  /* Copy and sort the relocation table. */

  reloc_count = rel_size / sizeof (struct relocation_info);
  reloc_tab = xmalloc (reloc_count * sizeof (struct relocation_info));
  memcpy (reloc_tab, rel, reloc_count * sizeof (struct relocation_info));
  qsort (reloc_tab, reloc_count, sizeof (struct relocation_info), reloc_compare);

  /* First pass: apply fixups to data.  Adjust fixup frames for OMF
     fixups.  In a.out files, frames are relative to address 0, in OMF
     files, frames are relative to the start of the segment.  The
     following two variables are used for doing these adjustments. */

  start_data = text_size;
  start_bss = start_data + data_size;

  /* Scan the relocation table for entries applying to this segment. */

  for (i = 0, r = reloc_tab; i < reloc_count; ++i, ++r)
    if (r->r_length == 2)
      {

        /* Here we have a 32-bit relocation. */

        if (r->r_extern)
          {

            /* The relocation refers to a symbol.  Look into the
               symbol table to find the fixup type and target
               address. */
            const struct nlist *sym = &sym_ptr[r->r_symbolnum];
/*#ifdef DEBUG
            const char *psz = sym->n_un.n_strx + str_ptr;
#endif*/
            switch (sym->n_type)
              {
              case N_TEXT:
              case N_WEAKD:
              case N_WEAKB:
              case N_WEAKA:
                break;

              case N_EXT:
              case N_WEAKU:
              case N_TEXT|N_EXT:
              case N_WEAKT:
                if (r->r_pcrel)
                  { /* example: r_address = 0xc, dw=0xfffffff6. disp=6 */
                    dword dw = *(dword *)(src + r->r_address);
                    dw += r->r_address + 4;
                    *(dword *)(src + r->r_address) = dw;
                  }
                break;
              case N_DATA:
              case N_DATA|N_EXT:
                *(dword *)(src + r->r_address) -= start_data;
                break;
              case N_BSS:
              case N_BSS|N_EXT:
                *(dword *)(src + r->r_address) -= start_bss;
                break;

              default:
                error ("write_seg: Invalid symbol type (0x%.2x)",
                       sym_ptr[r->r_symbolnum].n_type);
              }
          }
        else if (!(r->r_pcrel && (r->r_symbolnum & ~N_EXT) == seg_type))
          {

            /* The relocation does not refer to a symbol, it's an
               internal relocation.  Get the fixup type from the
               relocation table. */

            switch (r->r_symbolnum & ~N_EXT)
              {
              /* kso #465 2003-06-04: WEAK hack - bogus */
              case N_WEAKT:
              case N_TEXT:
                break;
              /* kso #465 2003-06-04: WEAK hack - bogus */
              case N_WEAKD:
              case N_DATA:
                *(dword *)(src + r->r_address) -= start_data;
                break;
              /* kso #465 2003-06-04: WEAK hack - bogus */
              case N_WEAKB:
              case N_BSS:
                *(dword *)(src + r->r_address) -= start_bss;
                break;
              /* kso #465 2003-06-04: WEAK hack - bogus */
              case N_WEAKA:
                break;
              default:
                error ("write_seg: Invalid relocation type (0x%.2x)", r->r_symbolnum);
              }
          }
      }

  /* Second pass: write LEDATA and FIXUPP records. */

  off = 0; reloc_idx = 0; boundary_idx = 0;
  while (seg_size > 0)
    {

      /* Compute the maximum number of bytes in the next LEDATA
         record, depending on the maximum record size, the record type
         (16-bit or 32-bit) and the number of bytes remaining.  The
         number of bytes will be adjusted later to avoid splitting
         entries of the $$SYMBOLS and $$TYPES segments. */

      n = MAX_REC_SIZE - 5;
      n -= (off >= 0x10000 || force_big ? 4 : 2);
      if (seg_size < n)
        n = seg_size;

      /* Adjust the number of bytes to avoid splitting a fixup.  Find
         the last relocation table entry which applies to this chunk.
         Then, lower the chunk size to stop at the start of the
         frame. */

      i = reloc_idx; end = off + n;
      while (i < reloc_count && reloc_tab[i].r_address < end)
        ++i;
      if (i > reloc_idx)
        {
          --i;
          tmp = reloc_tab[i].r_address + (1 << reloc_tab[i].r_length) - off;
          if (tmp > n)
            n = reloc_tab[i].r_address - off;
        }

      /* Consult the BOUNDARY table to find the last point where we
         are allowed to split the data into multiple LEDATA records.
         This must be done after adjusting for relocation table
         entries. */

      end = off + n;
      while (boundary_idx < boundary_count && boundary[boundary_idx] < end)
        ++boundary_idx;
      #if 1/* kso #456 2003-06-05: This must be wrong cause we're splitting unneedingly.
            *                      Check if we acutally hit the '< end' check.  */
      if (boundary_idx > 0 && boundary_idx < boundary_count)
      #else
      if (boundary_idx > 0)
      #endif
        {
          tmp = boundary[boundary_idx-1] - off;
          if (tmp > 0)
            n = tmp;
        }

      /* Write the LEDATA record.  This is simple. */

      if (off >= 0x10000 || force_big)
        {
          init_rec (LEDATA|REC32);
          put_idx (index);
          put_32 (off);
        }
      else
        {
          init_rec (LEDATA);
          put_idx (index);
          put_16 (off);
        }
      put_mem (src, n);
      write_rec ();

      /* Write the FIXUPP records for this LEDATA record.  Quite
         hairy. */

      end = off + n;
      started = FALSE;
      r = &reloc_tab[reloc_idx];

      /* Look at all relocation table entries which apply to the
         current LEDATA chunk. */

      while (reloc_idx < reloc_count && r->r_address < end)
        {

          /* Set ok to TRUE if we should create a fixup for this
             relocation table entry.  First, ignore all but 32-bit
             relocations.  In the $$SYMBOLS segment, we also have
             16-bit selector fixups.  far16 is later set to TRUE for
             16:16 fixups. */

          ok = (r->r_length == 2 || (sst_flag && r->r_length == 1));
          far16 = FALSE;

          if (r->r_extern)
            {

              /* The relocation refers to a symbol -- we won't use a
                 target thread.  If the symbol is a 16:16 symbol, we
                 set far16 to true to generate a 16:16 fixup. */

              threadp = NULL;
              if (sym_more[r->r_symbolnum].flags & SF_FAR16)
                far16 = TRUE;
            }
          else if (r->r_pcrel && (r->r_symbolnum & ~N_EXT) == seg_type)
            {
              ok = FALSE;
              if (warning_level > 0)
                warning ("Internal PC-relative relocation ignored");
            }
          else
            {

              /* The relocation does not refer to a symbol -- we use
                 an appropriate target thread.  The target thread
                 number is taken from or stored to *threadp.
                 target_index is the OMF segment index. */

              switch (r->r_symbolnum & ~N_EXT)
                {
                case N_TEXT:
                  threadp = &text_thread;
                  target_index = text_index;
                  break;
                case N_DATA:
                  threadp = &data_thread;
                  target_index = udat_index;
                  break;
                case N_BSS:
                  threadp = &bss_thread;
                  target_index = bss_index;
                  break;
                case N_ABS:
                default:
                  ok = FALSE;
                  break;
                }
            }

          if (ok)
            {

              /* Now we build the FIXUPP record. */

              if (started && !fits (32))
                {
                  write_rec ();
                  started = FALSE;
                }
              if (!started)
                {
                  init_rec (FIXUPP|REC32);
                  started = TRUE;
                }

              /* If no frame thread has been defined for the FLAT
                 group, define it now. */

              if (flat_thread < 0 && !far16)
                {
                  if (frame_thread_index >= 4)
                    error ("Too many frame threads");
                  /* THREAD: D=1, METHOD=F1 */
                  put_8 (0x44 | frame_thread_index);
                  put_idx (flat_index);
                  flat_thread = frame_thread_index++;
                }

              /* If we want to use a target thread and the target
                 thread is not yet defined, define it now. */

              if (threadp != NULL && *threadp < 0)
                {
                  if (target_thread_index >= 4)
                    error ("Too many target threads");
                  /* THREAD: D=0, METHOD=T4 */
                  put_8 (0x10 | target_thread_index);
                  put_idx (target_index);
                  *threadp = target_thread_index++;
                }

              /* Compute and write the locat word. */

              data_off = r->r_address - off;
              if (far16)
                locat = 0x8c;   /* Method 3: 16:16 far pointer */
              else if (sst_flag && r->r_length == 1)
                locat = 0x88;   /* Method 2: selector */
              else
                locat = 0xa4;   /* Method 9: 32-bit offset */
              locat |= ((data_off >> 8) & 0x03);
              if (!r->r_pcrel)
                locat |= 0x40;
              put_8 (locat);
              put_8 (data_off);

              /* Write the rest of the FIXUP subrecord. */

              if (far16)
                {
                  /* F=0, FRAME=F2, T=0, P=1, TARGT=T2 */
                  put_8 (0x26);
                  put_idx (sym_more[r->r_symbolnum].index);
                  put_idx (sym_more[r->r_symbolnum].index);
                }
              else if (r->r_extern)
                {
                  /* F=1, FRAME=F1, T=0, P=1, TARGT=T2 */
                  put_8 (0x86 | (flat_thread << 4));
                  put_idx (sym_more[r->r_symbolnum].index);
                }
              else
                {
                  /* F=1, FRAME=F1, T=1, P=1, TARGT=T4 */
                  put_8 (0x8c | (flat_thread << 4) | *threadp);
                }
            }
          ++reloc_idx; ++r;
        }
      if (started)
        write_rec ();

      /* Adjust pointers and counters for the next chunk. */

      src += n; off += n; seg_size -= n;
    }

  /* Deallocate the sorted relocation table. */

  free (reloc_tab);
}


/* Write a default library request record to the output file.  The
   linker will search the library NAME to resolve external
   references.  Create a COMENT record of class 0x9f.  The name is
   stored without leading byte count, the linker gets the length of
   the name from the record length. */

static void request_lib (const char *name)
{
  init_rec (COMENT);
  put_8 (0x40);
  put_8 (0x9f);
  put_mem (name, strlen (name));
  write_rec ();
}


/* Write default library request records for all the -i options.  The
   library names are stored in a list.  libreq_head points to the head
   of the list. */

static void write_libs (void)
{
  struct libreq *p;

  for (p = libreq_head; p != NULL; p = p->next)
    request_lib (p->name);
}


/* Write a record identifying the debug information style to the
   output file.  The style we use is called HLL version 3.  Create a
   COMENT record of class 0xa1. */

static void write_debug_style (void)
{
  if (strip_symbols || !a_out_h->a_syms)
    return;
  init_rec (COMENT);
  put_8 (0x80);
  put_8 (0xa1);                 /* Debug info style */
  put_8 (hll_version);          /* Version */
  put_mem ("HL", 2);            /* HLL style debug tables */
  write_rec ();
}


/* Write a link pass separator to the output file.  The linker makes
   two passes through the object modules.  The first pass stops when
   encountering the link pass separator.  This is used for improving
   linking speed.  The second pass ignores the link pass separator.
   The following records must precede the link pass separator: ALIAS,
   CEXTDEF COMDEF, EXTDEF, GRPDEF, LCOMDEF, LEXTDEF, LNAMES, LPUBDEF,
   PUBDEF, SEGDEF, TYPDEF, and most of the COMENT classes.  Create a
   COMENT record of class 0xa2. */

static void write_pass2 (void)
{
  init_rec (COMENT);
  put_8 (0x40);
  put_8 (0xa2);                 /* Link pass separator */
  put_8 (0x01);
  write_rec ();
}


/* Create segment names for all the sets.  It is here where sets are
   created.  See write_set_data() for details. */

static void define_set_names (void)
{
  int i, j;
  struct set *set_ptr;
  char tmp[512];
  const char *name;
  byte type;

  /* Loop through the symbol table. */

  for (i = 0; i < sym_count; ++i)
    {
      type = sym_ptr[i].n_type & ~N_EXT;
      if ((type == N_SETA && sym_ptr[i].n_value == 0xffffffff)
          || type == N_SETT || type == N_SETD)
        {

          /* This is a set element.  If type is N_SETA the symbol is
             the head of the set, otherwise it is an ordinary set
             element.  Search the table of sets to find out whether
             this set is already known. */

          name = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
          for (set_ptr = sets; set_ptr != NULL; set_ptr = set_ptr->next)
            if (strcmp (set_ptr->name, name) == 0)
              break;
          if (set_ptr == NULL)
            {

              /* The set is not yet known.  Create a new table
                 entry. */

              set_ptr = xmalloc (sizeof (struct set));
              set_ptr->name = xstrdup (name);
              for (j = 0; j < 3; ++j)
                {
                  set_ptr->seg_name[j] = -1;
                  set_ptr->seg_index[j] = -1;
                }
              set_ptr->count = 0;
              set_ptr->data = NULL;
              set_ptr->seg = NULL;
              set_ptr->def = 0;
              set_ptr->next = NULL;
              *set_add = set_ptr;
              set_add = &set_ptr->next;
            }
          else if (type == N_SETA && set_ptr->def)
            error ("Set `%s' defined more than once", name);

          if (type == N_SETA)
            set_ptr->def = 1;   /* Write SET1 and SET3 segments */
          else
            {

              /* Add the element to the set. */

              ++set_ptr->count;
              set_ptr->data = xrealloc (set_ptr->data, set_ptr->count * 4);
              set_ptr->seg = xrealloc (set_ptr->seg, set_ptr->count);
              set_ptr->data[set_ptr->count-1] = sym_ptr[i].n_value;
              set_ptr->seg[set_ptr->count-1]
                = (type == N_SETT ? N_TEXT : N_DATA);
            }

          /* Define the OMF segment names for this set, if not yet
             done. */

          if (set_ptr->seg_name[0] < 0)
            {
              strcpy (tmp, "SET#");
              strcat (tmp, name);
              for (j = 0; j < 3; ++j)
                {
                  tmp[3] = (char)('1' + j);
                  set_ptr->seg_name[j] = find_lname (tmp);
                }
            }
        }
    }
}


/* Define three segments for each set.  The segment names have already
   been defined, now write the SEGDEF records. */

static void write_set_segs (void)
{
  int j;
  struct set *set_ptr;

  for (set_ptr = sets; set_ptr != NULL; set_ptr = set_ptr->next)
    for (j = 0; j < 3; ++j)
      set_ptr->seg_index[j] =
        seg_def (set_ptr->seg_name[j], data_class_name,
                 4 * (j == 1 ? set_ptr->count : set_ptr->def), FALSE, TRUE);
}


/* Write the PUBDEF records for all the sets.  One PUBDEF record is
   generated for each set, it defines the set name. */

static void write_set_pub (void)
{
  struct set *set_ptr;

  for (set_ptr = sets; set_ptr != NULL; set_ptr = set_ptr->next)
      if (set_ptr->def)
        {
          init_rec (force_big ? PUBDEF|REC32 : PUBDEF);
          put_idx (flat_index);
          put_idx (set_ptr->seg_index[0]);
          put_sym (set_ptr->name);
          if (force_big)
            put_32 (0);
          else
            put_16 (0);
          put_idx (0);        /* Type index */
          write_rec ();
        }
}


/* Write the segment contents for all the sets.  One or three segments
   are written for each set XXX:

   SET1XXX      long start = -2
   SET2XXX      long table[]
   SET3XXX      long end = 0

   SET1XXX and SET3XXX are written only if there is a N_SETA symbol
   for the set.  The N_SETA symbol defines the start of the set and
   must have a value of -1 (see crt0.s).  SET2XXX is written for all
   modules which contain N_SETT or N_SETD symbols.  All three segments
   belong to the group GROUPXXX.  The linker combines all the segments
   of GROUPXXX into a single object.  SET1XXX comes first, followed by
   the SET2XXX segments of all modules of the program, followed by
   SET3XXX.  That way, we obtain a table of all set elements of a
   given set, preceded by -2 and terminated by 0.  A symbol XXX is
   defined which points to the start of SET1XXX.  See
   /emx/lib/gcc/main.c for code which uses sets. */

static void write_set_data (void)
{
  struct set *set_ptr;
  dword x;
  int i, max_count;
  dword *buf;
  struct relocation_info *reloc_tab;

  max_count = 0; buf = NULL; reloc_tab = NULL;
  for (set_ptr = sets; set_ptr != NULL; set_ptr = set_ptr->next)
    {

      /* Write the first segment.  It consists of a 32-bit word
         containing the number -2.  This is used by the startup code
         to detect a delimited set. */

      if (set_ptr->def)
        {
          x = 0xfffffffe;
          write_seg (set_ptr->seg_index[0], set_ptr->seg_name[0],
                     (byte *)&x, sizeof (x), NULL, 0, FALSE, NULL, 0, -1);
        }

      /* Write the second segment.  It consists of the set elements as
         taken from the a.out module.  The elements are assumed to be
         pointers into the text segment. */

      if (set_ptr->count >= 1)
        {

          /* Enlarge the relocation table, if required. */

          if (set_ptr->count > max_count)
            {
              max_count = set_ptr->count;
              buf = xrealloc (buf, 4 * max_count);
              memset (buf, 0, 4 * max_count);
              reloc_tab = xrealloc (reloc_tab,
                                    max_count * sizeof (struct relocation_info));
            }

          /* Create one relocation table entry for each set
             element. */

          for (i = 0; i < set_ptr->count; ++i)
            {
              buf[i] = set_ptr->data[i];
              reloc_tab[i].r_address = i * 4;
              reloc_tab[i].r_symbolnum = set_ptr->seg[i];
              reloc_tab[i].r_pcrel = 0;
              reloc_tab[i].r_length = 2;
              reloc_tab[i].r_extern = 0;
              reloc_tab[i].r_pad = 0;
            }

          /* Write the segment data and fixups. */

          write_seg (set_ptr->seg_index[1], set_ptr->seg_name[1],
                     (byte *)buf, set_ptr->count * 4, reloc_tab,
                     set_ptr->count * sizeof (struct relocation_info), FALSE, NULL, 0,
                     -1);
        }

      /* Write the third segment.  It consists of a 32-bit word
         containing the value 0, marking the end of the table. */

      if (set_ptr->def)
        {
          x = 0;
          write_seg (set_ptr->seg_index[2], set_ptr->seg_name[2],
                     (byte *)&x, sizeof (x), NULL, 0, FALSE, NULL, 0, -1);
        }
    }
  if (buf != NULL)
    free (buf);
}


/* Find a relocation table entry by fixup address. */

static const struct relocation_info *find_reloc_fixup (const struct relocation_info *rel,
                                             long rel_size, dword addr)
{
  int i, count;

  count = rel_size / sizeof (struct relocation_info);
  for (i = 0; i < count; ++i)
    if (rel[i].r_address == addr)
      return &rel[i];
  return NULL;
}


static void free_modstr_cache (void)
{
  struct modstr *p1, *p2;

  for (p1 = modstr_cache; p1 != NULL; p1 = p2)
    {
      p2 = p1->next;
      free (p1->symbol);
      free (p1->str);
      free (p1);
    }
  modstr_cache = NULL;
}


/* Fetch a null-terminated string from a module.  REL points to the
   relocation table entry of the pointer, ADDR is the pointer.  Copy
   the string to DST, a buffer of DST_SIZE bytes.  Return TRUE if
   successful, FALSE on failure. */

static int fetch_modstr (const struct relocation_info *rel, dword addr,
                         char *dst, size_t dst_size)
{
  const byte *seg_ptr;
  dword seg_size;

  if (rel == NULL) return FALSE;
  if (rel->r_extern)
    {
      struct modstr *p;
      byte *buf;
      const char *sym_name;
      const struct nlist *m_sym = NULL;
      const struct exec *m_ao = NULL;

      /* We have to take the string from another module.  First try to
         get the string from the cache. */

      sym_name = (const char *)(str_ptr + sym_ptr[rel->r_symbolnum].n_un.n_strx);
      for (p = modstr_cache; p != NULL; p = p->next)
        if (strcmp (p->symbol, sym_name) == 0)
          break;
      if (p == NULL)
        {
          long pos, size, m_str_size;
          struct ar_hdr ar;
          byte *t;
          size_t buf_size, len;
          const byte *m_str;
          int i, m_sym_count, found;

          /* Search all modules until finding the symbol. */

          buf = NULL; buf_size = 0; found = FALSE;
          pos = SARMAG;
          while (ar_read_header (&ar, pos))
            {
              size = ar_member_size (&ar);
              pos += (sizeof (ar) + size + 1) & -2;
              if (strcmp (ar.ar_name, "__.SYMDEF") == 0
                  || strcmp (ar.ar_name, "__.IMPORT") == 0
                  || memcmp (ar.ar_name, "IMPORT#", 7) == 0)
                continue;
              if (size > buf_size)
                {
                  if (buf != NULL) free (buf);
                  buf_size = size;
                  buf = xmalloc (buf_size);
                }
              size = fread (buf, 1, size, inp_file);
              if (ferror (inp_file))
                error ("Cannot read `%s'", inp_fname);
              m_ao = (const struct exec *)buf;
              if (size < sizeof (struct exec) || N_MAGIC(*m_ao) != OMAGIC)
                break;
              t = buf + sizeof (struct exec);
              t += m_ao->a_text; t += m_ao->a_data;
              t += m_ao->a_trsize; t += m_ao->a_drsize;
              m_sym = (const struct nlist *)t; t += m_ao->a_syms;
              m_str = t;
              if (m_str + 4 - buf > size)
                break;
              m_str_size = *(long *)m_str;
              m_sym_count = m_ao->a_syms / sizeof (struct nlist);
              if (m_str + m_str_size - buf > size)
                break;
              for (i = 0; i < m_sym_count; ++i)
                switch (m_sym[i].n_type)
                  {
                  case N_TEXT|N_EXT:
                  case N_DATA|N_EXT:
                    if (strcmp ((const char *)(m_str + m_sym[i].n_un.n_strx), sym_name) == 0)
                      {
                        m_sym += i; found = TRUE; break;
                      }
                  }
              if (found)
                break;
            }
          if (!found)
            error ("Symbol `%s' not found", sym_name);

          addr = m_sym->n_value;
          switch (m_sym->n_type)
            {
            case N_TEXT|N_EXT:
              seg_ptr = buf + sizeof (struct exec);
              seg_size = m_ao->a_text;
              break;
            case N_DATA|N_EXT:
              seg_ptr = buf + sizeof (struct exec) + m_ao->a_text;
              seg_size = m_ao->a_data;
              break;
            default:
              abort ();
            }

          len = 0;
          for (;;)
            {
              if (addr + len >= seg_size)
                error ("String extends beyond end of segment");
              if (seg_ptr[addr + len] == 0)
                break;
              ++len;
            }

          p = xmalloc (sizeof (struct modstr));
          p->symbol = xstrdup (sym_name);
          p->str = xmalloc (len + 1);
          memcpy (p->str, seg_ptr + addr, len + 1);
          p->next = modstr_cache;
          modstr_cache = p;
          if (buf != NULL) free (buf);
        }
      if (strlen (p->str) >= dst_size)
        return FALSE;
      strcpy (dst, p->str);
      return TRUE;
    }
  else
    {
      dword si;

      switch (rel->r_symbolnum)
        {
        case N_TEXT:
          seg_ptr = text_ptr;
          seg_size = text_size;
          break;
        case N_DATA:
          seg_ptr = data_ptr;
          seg_size = data_size;
          break;
        default:
          return FALSE;
        }
      for (si = 0; addr + si < seg_size && si < dst_size; ++si)
        {
          dst[si] = seg_ptr[addr + si];
          if (seg_ptr[addr + si] == 0)
            return TRUE;
        }
      return FALSE;
    }
}


/* Create an import record and a PUBDEF record. */

static void make_import (const char *pub_name, const char *proc_name,
                         long ord, const char *mod)
{
  char szPubName[256];
  int  cchPubName;

  /* Skip a leading underscore character if present. */

  if (strip_underscore (pub_name))
    ++pub_name;
  cchPubName = make_nstr (pub_name, strlen (pub_name), szPubName);

  /* Make the symbol public in the output library. */

  if (omflib_add_pub (out_lib, szPubName, mod_page, lib_errmsg) != 0)
    error (lib_errmsg);

  /* Write the import definition record. */

  init_rec (COMENT);
  put_8 (0x00);
  put_8 (IMPDEF_CLASS);
  put_8 (IMPDEF_SUBTYPE);
  put_8 (proc_name == NULL ? 0x01 : 0x00); /* Import by ordinal or by name */
  put_8 (cchPubName);
  put_mem (szPubName, cchPubName);
  put_str (mod);
  if (proc_name == NULL)
    put_16 (ord);
  else if (strcmp (proc_name, pub_name) == 0)
    put_8 (0);
  else {
    put_str (proc_name);
  }
  write_rec ();
  init_rec (MODEND|REC32);
  put_8 (0x00);                 /* Non-main module without start address */
  write_rec ();
}


/* If the input file is an import module (method (I2) as created by
   emximp), create an OMF-style import module and return TRUE.
   Otherwise, return FALSE. */

static int handle_import_i2 (void)
{
  int i, len1, mod_len;
  long ord;
  const char *name1, *name2, *proc_name, *p;
  char mod[256], *q;

  /* Search the symbol table for N_IMP1 and N_IMP2 symbols.  These are
     unique to import modules. */

  name1 = NULL; name2 = NULL;
  for (i = 0; i < sym_count; ++i)
    switch (sym_ptr[i].n_type)
      {
      case N_IMP1|N_EXT:
        name1 = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
        break;
      case N_IMP2|N_EXT:
        name2 = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
        break;
      default:
        return FALSE;
      }

  /* If no N_IMP1 and N_IMP2 symbols have been found, the module is
     not an import module. */

  if (name1 == NULL || name2 == NULL)
    return FALSE;

  /* Parse the special symbols into module name and ordinal number.
     name2 should look like

       SYMBOL=MODULE.ORDINAL

     where SYMBOL is name1, MODULE is the module name and ORDINAL is
     the ordinal number. */

  len1 = strlen (name1);
  if (memcmp (name1, name2, len1) != 0)
    error ("Invalid import record: names don't match");
  name2 += len1;
  if (*name2 != '=')
    error ("Invalid import record: missing `='");
  ++name2;
  p = strchr (name2, '.');
  if (p == NULL)
    error ("Invalid import record: missing `.'");
  mod_len = p - name2;
  memcpy (mod, name2, mod_len);
  mod[mod_len] = 0;
  proc_name = NULL;
  errno = 0;
  ord = strtol (p + 1, &q, 10);
  if (q != p + 1 && *q == 0 && errno == 0)
    {
      if (ord < 1 || ord > 65535)
        error ("Invalid import record: invalid ordinal");
    }
  else
    {
      ord = -1;
      proc_name = p + 1;
      if (*proc_name == 0)
        error ("Invalid import record: invalid name");
    }

  make_import (name1, proc_name, ord, mod);
  return TRUE;
}

/* Convert import entries. */
static void write_import_i2 (void)
{
  int i;
  const char *name1, *name2;

  /* Search the symbol table for N_IMP1 and N_IMP2 symbols.  These are
     unique to import modules. */

  name1 = NULL; name2 = NULL;
  for (i = 0; i < sym_count; ++i)
    {
      switch (sym_ptr[i].n_type)
        {
        case N_IMP1|N_EXT:
          name1 = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
          break;
        case N_IMP2|N_EXT:
          name2 = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
          break;
        }
      if (name1 && name2)
        {
          int len1, mod_len;
          long ord;
          const char *proc_name, *p;
          char mod[256], *q;

          /* Parse the special symbols into module name and ordinal number.
             name2 should look like

               SYMBOL=MODULE.ORDINAL

             where SYMBOL is name1, MODULE is the module name and ORDINAL is
             the ordinal number. */

          len1 = strlen (name1);
          if (memcmp (name1, name2, len1) != 0)
            error ("Invalid import record: names don't match");
          name2 += len1;
          if (*name2 != '=')
            error ("Invalid import record: missing `='");
          ++name2;
          p = strchr (name2, '.');
          if (p == NULL)
            error ("Invalid import record: missing `.'");
          mod_len = p - name2;
          memcpy (mod, name2, mod_len);
          mod[mod_len] = '\0';
          proc_name = NULL;
          errno = 0;
          ord = strtol (p + 1, &q, 10);
          if (q != p + 1 && *q == 0 && errno == 0)
            {
              if (ord < 1 || ord > 65535)
                error ("Invalid import record: invalid ordinal");
            }
          else
            {
              ord = -1;
              proc_name = p + 1;
              if (*proc_name == 0)
                error ("Invalid import record: invalid name");
            }

          /* Write the import definition record. */

          init_rec (COMENT);
          put_8 (0x00);
          put_8 (IMPDEF_CLASS);
          put_8 (IMPDEF_SUBTYPE);
          put_8 (proc_name == NULL ? 0x01 : 0x00); /* Import by ordinal or by name */
          put_str (name1);                         /* Underscore already removed above */
          put_str (mod);
          if (proc_name == NULL)
            put_16 (ord);
          else if (strcmp (proc_name, name1) == 0)
            put_8 (0);
          else
            put_str (proc_name);
          write_rec ();

          name1 = NULL;
          name2 = NULL;
        }
    }
}


/* If the input file is an import module (method (I1) as created by
   emximp), create an OMF-style import module and return TRUE.
   Otherwise, return FALSE. */

static int handle_import_i1 (void)
{
  int i;
  const char *pub_name;
  char proc_name[257], dll_name[257];
  long table_off, ord, off;
  long *table;
  const struct relocation_info *rel;

  /* There must be exactly one public symbol (which must defined for
     N_TEXT), otherwise we cannot safely eliminate all the code.
     Moreover, there must be an N_SETT|N_EXT entry for `__os2dll'. */

  pub_name = NULL; table_off = -1;
  for (i = 0; i < sym_count; ++i)
    switch (sym_ptr[i].n_type)
      {
      case N_TEXT|N_EXT:
        if (pub_name != NULL) return FALSE;
        pub_name = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
        break;

      case N_DATA|N_EXT:
      case N_BSS|N_EXT:
        return FALSE;

      case N_SETT|N_EXT:
        if (strcmp ((const char *)(str_ptr + sym_ptr[i].n_un.n_strx), "__os2dll") != 0)
          return FALSE;
        table_off = sym_ptr[i].n_value;
        break;
      }

  if (pub_name == NULL || table_off == -1)
    return FALSE;

  /* The table must be completely present. */

  if (table_off < 0 || table_off + 4 * 4 > text_size)
    error ("Invalid import table: beyond end of segment");

  /* Fetch the procedure name or the ordinal. */

  table = (long *)(text_ptr + table_off);
  if (table[0] == 0)
    {
      ord = 0;

      /* Check and fetch the procedure name. */

      rel = find_reloc_fixup (text_rel, a_out_h->a_trsize, table_off + 3 * 4);
      if (!fetch_modstr (rel, table[3], proc_name, sizeof (proc_name)))
        error ("Invalid import table: invalid pointer to procedure name");
    }
  else if (table[0] == 1)
    {
      ord = table[3];
      if (ord < 1 || ord > 65535)
        error ("Invalid import table: invalid ordinal number");
    }
  else
    error ("Invalid import table: invalid flag");

  /* Check the fixup address -- there must be a JMP instruction. */

  off = table[1];
  if (off < 1 || off + 4 > text_size)
    error ("Invalid import table: invalid fixup address");
  if (text_ptr[off-1] != 0xe9)
    error ("Invalid import table: fixup does not belong to a JMP instruction");

  /* Check and fetch the module name. */

  rel = find_reloc_fixup (text_rel, a_out_h->a_trsize, table_off + 2 * 4);
  if (!fetch_modstr (rel, table[2], dll_name, sizeof (dll_name)))
    error ("Invalid import table: invalid pointer to module name");

  make_import (pub_name, ord != 0 ? NULL : proc_name, ord, dll_name);
  return TRUE;
}

/**
 * Emits a OMF export record if the symbols is defined in this module.
 */
static void make_export(const char *pszSymbol, size_t cchSymbol,
                        const char *pszExpName, size_t cchExpName, unsigned iOrdinal)
{
    const struct nlist *pSym;
    char *pszSym = alloca(cchSymbol + 1);
    memcpy(pszSym, pszSymbol, cchSymbol);
    pszSym[cchSymbol] = '\0';

    pSym = find_symbol(pszSym);
    if (pSym)
    {
        /*
         * Write the export definition record.
         */
        uint8_t fFlags = 0;
        if (iOrdinal)
            fFlags |= 1 << 7; /* have ordinal */
        if (cchExpName && !iOrdinal)
           fFlags |= 1 << 6; /* resident name */

        init_rec(COMENT);
        put_8(0x00);
        put_8(CLASS_OMFEXT);
        put_8(OMFEXT_EXPDEF);
        put_8(fFlags);
        put_nstr(pszExpName, cchExpName);
        put_nstr(pszSymbol, cchSymbol);
        if (iOrdinal)
            put_16(iOrdinal);
        write_rec();
    }
}


/* Convert export entries.

   The exports are encoded as N_EXT (0x6c) stab entries. The value is -42,
   and the symbol name is on the form: "<exportname>,<ordinal>=<symbol>,code|data"
   The exportname can be empty if ordinal is not 0. Ordinal 0 means not exported by
   any special ordinal number.

   Export entries for symbols which are not defined in the object are ignored. */
static void write_export (void)
{
    int i;

    /* Search the symbol table for N_IMP1 and N_IMP2 symbols.  These are
       unique to import modules. */

    for (i = 0; i < sym_count; ++i)
    {
        const char *pszName;
        size_t      cchName;
        const char *pszOrdinal;
        const char *pszSymbol;
        size_t      cchSymbol;
        const char *pszType;
        int         iOrdinal;
        char       *pszOrdinalEnd;
        if (sym_ptr[i].n_type != N_EXP)
            continue;
        pszName = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);

        /*
         * Parse it.
         */
        /* find equal sign first, we'll use this for validating the ordinal. */
        pszSymbol = strchr(pszName, '=');
        if (!pszSymbol)
            error("Invalid export record: missing `='. \nstabs: %s", pszName);

        /* ordinal */
        pszOrdinal = strchr(pszName, ',');
        if (!pszOrdinal || pszOrdinal >= pszSymbol)
            error("Invalid export record: missing ordinal.\nstabs: %s", pszName);
        cchName = pszOrdinal - pszName;
        pszOrdinal++;
        iOrdinal = strtol(pszOrdinal, &pszOrdinalEnd, 0);
        if (iOrdinal < 0 || iOrdinal >= 0x10000)
            error("Invalid export record: ordinal value is out of range (0-65k): %d\nstabs:%s",
                  iOrdinal, pszName);
        if (pszOrdinalEnd != pszSymbol)
            error("Invalid export record: ordinal field doesn't end at `=.\nstabs:%s", pszName);

        /* type and symbol */
        pszSymbol++;
        pszType = strchr(pszSymbol, ',');
        if (!pszType)
            error("Invalid export record: Symbol type is missing\nstabs:%s", pszName);
        cchSymbol = pszType - pszSymbol;
        pszType++;
        if (strcmp(pszType, "code") && strcmp(pszType, "data") && strcmp(pszType, "bss") && strcmp(pszType, "common"))
            error("Invalid export record: Invalid symbol type: %s\nstabs:%s", pszType, pszName);
        if (!cchSymbol)
            error("Invalid export record: No (internal) symbol name.\nstabs:%s", pszName);

        /*
         * Hand it on to the worker which looks up the symbol
         * and emits the OMF export record if found.
         */
        make_export(pszSymbol, cchSymbol, pszName, cchName, iOrdinal);
    }
}


/* Convert a filename from Unix format to OS/2 format.  All that has
   to be done is replacing slashes with backslashes as IPMD doesn't
   like slashes in file names.  The conversion is done in place. */

static void convert_filename (char *name)
{
  while (*name != 0)
    {
      if (*name == '/')
        *name = '\\';
      ++name;
    }
}

#if 0
/* Converts dashed filenames to somewhat absolute ones assuming that
   the current directory is what they're relative to. */

static int abspath_filename(char *dst, const char *src, int size)
{
  int   rc = -1;
  char *psz;

  if (src[1] != ':' && src[0] != '\\' && src[0] != '/')
    rc = _abspath(dst, src, size);
  if (rc)
  {
      *dst = '\0';
      strncat(dst, src, size);
  }

  for (psz = dst; *psz; psz++)
    if (*psz == '/')
      *psz = '\\';
  return psz - dst;
}
#endif

/* Derive the module name and store it in the mod_name variable.
   Derive the current director and store it in the base_dir variable.

   Search the symbol table for N_SO symbols. There are two kinds, base dir
   and main source file name. The base dir one ends with a slash.

   We assume that the base dir name comes first, and that is it's prefect
   but maybe for a drive letter.

   If there is no such symbol, use the output file name. */

static void get_mod_name (void)
{
  int i, len, ok;
  const char *p1, *p2;

  base_dir[0] = '\0';
  ok = FALSE;
  for (i = 0; i < sym_count; ++i)
    if (sym_ptr[i].n_type == N_SO)
      {
        p1 = (const char *)(str_ptr + sym_ptr[i].n_un.n_strx);
        len = strlen (p1);
        if (len > 0 && p1[len-1] == '/' )
          {
            if (!base_dir[0])
              {
                _strncpy (base_dir, p1, sizeof(base_dir));
                convert_filename (base_dir);
              }
          }
        else if (len > 0)
          {
            ok = TRUE;
            _strncpy (mod_name, p1, sizeof (mod_name));
            convert_filename (mod_name);
            /* The base dir doesn't currently have any drive letter.
               steal that from the source filename if possible.
               ASSUME: that it's more like that source is right than curdir
                       bacause of stuff like makeomflibs.cmd and autoconv. */
            if (   (base_dir[0] == '\\' ||  base_dir[0] == '/')
                &&  base_dir[1] != '\\' &&  base_dir[1] != '/' /* unc */
                &&  mod_name[1] == ':')
              {
                len = strlen(base_dir) + 1;
                memmove(base_dir + 2, base_dir, len);
                base_dir[0] = mod_name[0];
                base_dir[1] = mod_name[1];
              }
            break;
          }
      }
  if (!ok && out_lib == NULL)
    {
      p1 = out_fname;
      for (p2 = p1; *p2 != 0; ++p2)
        if (*p2 == '/' || *p2 == '\\' || *p2 == ':')
          p1 = p2 + 1;
      _strncpy (mod_name, p1, sizeof (mod_name));
    }

  /* make sure we have base_dir and that it's an abspath */
  if (!base_dir[0])
    {
      getcwd(base_dir, sizeof(base_dir));
      len = strlen(base_dir);
      base_dir[len++] = '\\';
      base_dir[len] = '\0';
    }
  else if (   (base_dir[0] == '\\' ||  base_dir[0] == '/')
           &&  base_dir[1] != '\\' &&  base_dir[1] != '/') /* unc */
    {   /* make it absolute using current drive */
      len = strlen(base_dir) + 1;
      memmove(base_dir + 2, base_dir, len);
      base_dir[0] = _getdrive();
      base_dir[1] = ':';
    }
}


/* Write the translator module header record.  When creating a .LIB
   file, this function does nothing.  Otherwise, the module name is
   expected in the mod_name variable. */

static void write_theadr (void)
{
  if (out_lib == NULL)
    {
      init_rec (THEADR);
      put_str (mod_name);
      write_rec ();
    }
}

/* Tell LINK386 what identifier manipulator DLL to call.  The name of
   the DLL is given by `idmdll_name', the default value of which is
   GPPDEMID.  The name can be changed with the -I option.  If -I- is
   given, no IDMDLL record will be written.  The IDMDLL record will
   also be suppressed if the program hasn't been compiled by the GNU
   C++ compiler. */

static void write_idmdll ()
{
  /* kso #465 2003-06-04: This test doesn't work any longer, sorry.
   *                      Pretend everything is C++ */
  #if 1
  if (idmdll_name != NULL)
  #else
  if (idmdll_name != NULL && find_symbol ("__gnu_compiled_cplusplus") != NULL)
  #endif
    {
      init_rec (COMENT);
      put_8 (0x00);
      put_8 (CLASS_IDMDLL);
      put_str (idmdll_name);
      put_str ("");             /* Initialization parameter */
      write_rec ();
    }
}


/* Tell ilink which TIS (Tools I Standard) version we follow.
   (At least that's what I think this comment record is good for). */
static void write_tis ()
{
  unsigned short ver = 0;
  char *type = getenv ("EMXOMFLD_TYPE");
  if (type && !stricmp (type, "VAC308"))
      ver = 0x0100;
  else if (!type || !stricmp (type, "VAC365"))
      ver = 0x0101;
  /* else: no TIS record for link386! */

  if (ver)
    {
      init_rec (COMENT);
      put_8 (0x00);
      put_8 (CLASS_TIS);
      put_mem ("TIS", 3);
      put_16 (ver);
      write_rec ();
    }
}


/* Tell ilink which dll to use when /DBGPACK is specificed. The dllname
   is given without extension. */
static void write_dbgpack ()
{
  const char *name = dbgpack_name;
  if (strip_symbols || !a_out_h->a_syms)
    return;
  if (!name)
    {
      char *type = getenv ("EMXOMFLD_TYPE");
      if (type && !stricmp (type, "VAC308"))
          name = "LNKOH410";
      else if (!type || !stricmp (type, "VAC365"))
          name = NULL /* hll_version == 4 ?  "CPPLH436" : "CPPLH636"
                        - this linker figures it out by it self. */;
      /* no DLL for link386! */
    }

  if (name)
    {
      init_rec (COMENT);
      put_8 (0x80);
      put_8 (CLASS_DBGPACK);
      put_str (name);
      write_rec ();
    }
}


/* Find a file name for creating line number information. */

static int file_find (const char *name)
{
  int i;
  char *psz;

  /* canonize the filename - slashes and possibly abs path. */
  if (name[0] != '/' && name[0] != '\\' && name[1] != ':')
    { /* need to add base_dir. */
      int cch1 = strlen(base_dir);
      int cch2 = strlen(name) + 1;
      psz = xmalloc(cch1 + cch2);
      memcpy(psz, base_dir, cch1);
      memcpy(psz + cch1, name, cch2);
    }
  else
    psz = xstrdup(name);
  convert_filename(psz);

  /* search for previous instances */
  for (i = 0; i < file_grow.count; ++i)
    if (!strcmp(file_list[i], psz))
      {
        free(psz);
        return i;
      }

  /* new source file, add it if possible. */
  if (hll_version <= 3 && file_grow.count >= 255)
    {
      warning ("Too many source files, cannot convert line number info");
      free(psz);
      return file_grow.count;
    }
  grow_by (&file_grow, 1);
  i = file_grow.count++;
  file_list[i] = psz;
  return i;
}


/* This function is passed to qsort() for sorting line numbers.  X1
   and X2 point to struct line structures.  Line number entries are
   sorted by address, file and line number. */

static int line_compare (const void *x1, const void *x2)
{
  dword a1, a2;
  int i1, i2;

  a1 = ((const struct line *)x1)->addr;
  a2 = ((const struct line *)x2)->addr;
  if (a1 < a2)
    return -1;
  else if (a1 > a2)
    return 1;
  i1 = ((const struct line *)x1)->file_index;
  i2 = ((const struct line *)x2)->file_index;
  if (i1 < i2)
    return -1;
  else if (i1 > i2)
    return 1;
  i1 = ((const struct line *)x1)->line;
  i2 = ((const struct line *)x2)->line;
  if (i1 < i2)
    return -1;
  else if (i1 > i2)
    return 1;
  return 0;
}

/* Write linenumber fixups for HLL v4 records
 * We need fixups for segment idx and offset in the special
 * first entry. */
static void write_linenumfixup(void)
{
  /* ASSUME! flat_thread is defined. */
  /* ASSUME! base seg idx of linnum rec < 128. */
  init_rec (FIXUPP|REC32);
  /* LOC=16-bit sel;I=1;M=1; */
  put_8 (0xc8);
  /* offset 6 (segment number). */
  put_8 (0x06);
  /* F=1;Frame=flat_thread;T=0;P=1;TARGT=0; */
  put_8 (0x84 | (flat_thread << 4));
  /* target seg index? */
  put_8 (text_index);

  /* LOC=32-offset sel;I=1;M=1; */
  put_8 (0xe4);
  /* offset 8 (offset). */
  put_8 (0x08);
  /* F=1;Frame=flat_thread;T=0;P=1;TARGT=0; */
  put_8 (0x84 | (flat_thread << 4));
  /* target seg index? */
  put_8 (text_index);
  write_rec ();
}


/* Write line number information to the output file.  Unfortunately,
   the line number information format currently used by IPMD is not
   documented.  This code is based on experimentation. */

static void write_linnum (void)
{
  int i, started, len, file_index;
  int valid_lines;
  struct line *line_list;
  struct grow line_grow;
  char buf[256];

  /* Initialize growing arrays for file names and line numbers. */

  grow_init (&file_grow, &file_list, sizeof (*file_list), 8);
  grow_init (&line_grow, &line_list, sizeof (*line_list), 64);

  /* Define file index for main source file. */

  file_index = file_find (mod_name);
  started = FALSE;

  /* Go through the symbol table, defining line numbers and additional
     source files. */

  for (i = 0; i < sym_count; ++i)
    switch (sym_ptr[i].n_type)
      {
      case N_SOL:
        file_index = file_find ((const char *)(str_ptr + sym_ptr[i].n_un.n_strx));
        break;
      case N_SLINE:
        grow_by (&line_grow, 1);
        line_list[line_grow.count].file_index = file_index;
        line_list[line_grow.count].line = sym_ptr[i].n_desc;
        line_list[line_grow.count].addr = sym_ptr[i].n_value;
        ++line_grow.count;
      }

  /* Sort the line numbers by address, file and line number. */

  qsort (line_list, line_grow.count, sizeof (*line_list), line_compare);

  /* If there are multiple line numbers assigned to the same address,
     keep but the last line number.  Delete line numbers by setting
     the line number to -1. */

  for (i = 0; i < line_grow.count - 1; ++i)
    if (line_list[i].line >= 0 && line_list[i+1].line >= 0
        && line_list[i].addr == line_list[i+1].addr)
      line_list[i].line = -1;

  /* Count the number of valid line numbers, that is, non-negative
     line numbers. */

  valid_lines = 0;
  for (i = 0; i < line_grow.count; ++i)
    if (line_list[i].line >= 0)
      ++valid_lines;


  /*
   * If no lines or no files, we don't want to issue any LINNUM records.
   */
  if (valid_lines <= 0 || file_grow.count <= 0)
      return;

  /* Compute the size of the file names table. */

  len = 3 * 4;
  for (i = 0; i < file_grow.count; ++i)
    len += 1 + strlen (file_list[i]);

  /*
   * This is the VAC way, too bad link386 doesn't fancy it.
   */
  if (hll_version >= 4)
    {
      int first_linnum;
      /* Filename table first  -  a first entry like visual age does it and does hints on */

      init_rec (LINNUM|REC32);
      started = TRUE;
      put_idx (0);                  /* Base Group */
      put_idx (0);                  /* Base Segment */

      put_16 (0);                   /* Line number = 0 (special entry) */
      put_8 (3);                    /* Entry type: some visual age stuff I believe */
      put_8 (0);                    /* Reserved */
      put_16 (file_grow.count);     /* Count of line number entries */
      put_16 (0);                   /* Segment number */
      put_32 (len);                 /* Size of file names table */
      /* no linenumber */
      put_32 (0);                   /* First column */
      put_32 (0);                   /* Number of columns */
      put_32 (file_grow.count);     /* Number of source and listing files */

      for (i = 0; i < file_grow.count; ++i)
        {
          len = strlen(file_list[i]);
          if (started && !fits (1 + len))
            {
              write_rec ();
              started = FALSE;
            }
          if (!started)
            {
              init_rec (LINNUM|REC32);
              put_idx (0);          /* Base Group */
              put_idx (text_index); /* Base Segment */
              started = TRUE;
            }
          put_8 (len);
          put_mem (file_list[i], len);
        }

      if (started)
        write_rec ();


      /* Write the line number table. */
      first_linnum = 1;
      init_rec (LINNUM|REC32);
      started = TRUE;
      put_idx (0);                  /* Base Group */
      put_idx (text_index);         /* Base Segment */

      put_16 (0);                   /* Line number = 0 (special entry) */
      put_8 (0);                    /* Entry type: source and offset */
      put_8 (0);                    /* Reserved */
      put_16 (valid_lines);         /* Count of line number entries */
      put_16 (0);                   /* Segment number - Fixup required */
      put_32 (0);                   /* Segment offset - Fixup required */

      for (i = 0; i < line_grow.count; ++i)
        if (line_list[i].line >= 0)
          {
            if (started && !fits (8))
              {
                write_rec ();
                started = FALSE;
                if (first_linnum)
                  {
                    first_linnum = 0;
                    write_linenumfixup();
                  }
              }
            if (!started)
              {
                init_rec (LINNUM|REC32);
                put_idx (0);          /* Base Group */
                put_idx (text_index); /* Base Segment */
                started = TRUE;
              }
            put_16 (line_list[i].line);
            put_16 (line_list[i].file_index + 1);
            put_32 (line_list[i].addr);
          }

      if (started)
        {
          write_rec ();
          if (first_linnum)
              write_linenumfixup();
        }
    }
  else
    { /* hll version 3 */
      /* Write the line number table. */

      init_rec (LINNUM|REC32);
      started = TRUE;
      put_idx (0);                  /* Base Group */
      put_idx (text_index);         /* Base Segment */

      put_16 (0);                   /* Line number = 0 (special entry) */
      put_8 (0);                    /* Entry type: source and offset */
      put_8 (0);                    /* Reserved */
      put_16 (valid_lines);         /* Count of line number entries */
      put_16 (0);                   /* Segment number */
      put_32 (len);                 /* Size of file names table */

      for (i = 0; i < line_grow.count; ++i)
        if (line_list[i].line >= 0)
          {
            if (started && !fits (8))
              {
                write_rec ();
                started = FALSE;
              }
            if (!started)
              {
                init_rec (LINNUM|REC32);
                put_idx (0);          /* Base Group */
                put_idx (text_index); /* Base Segment */
                started = TRUE;
              }
            put_16 (line_list[i].line);
            put_16(line_list[i].file_index + 1);
            put_32 (line_list[i].addr);
          }

      /* Now write the file names table. */

      if (started && !fits (12))
        {
          write_rec ();
          started = FALSE;
        }
      if (!started)
        {
          init_rec (LINNUM|REC32);
          put_idx (0);          /* Base Group */
          put_idx (text_index); /* Base Segment */
          started = TRUE;
        }
      put_32 (0);                   /* First column */
      put_32 (0);                   /* Number of columns */
      put_32 (file_grow.count);     /* Number of source and listing files */

      for (i = 0; i < file_grow.count; ++i)
        {
          len = strlen (file_list[i]);
          if (len > sizeof (buf) - 1)
            len = sizeof (buf) - 1;
          memcpy (buf, file_list[i], len);
          buf[len] = 0;
          convert_filename (buf);
          if (started && !fits (1 + len))
            {
              write_rec ();
              started = FALSE;
            }
          if (!started)
            {
              init_rec (LINNUM|REC32);
              put_idx (0);          /* Base Group */
              put_idx (text_index); /* Base Segment */
              started = TRUE;
            }
          put_8 (len);
          put_mem (buf, len);
        }

      if (started)
        write_rec ();
    }

  grow_free (&line_grow);
  grow_free (&file_grow);
}


/* Print unknown symbol table entries (-u option). */

static void list_unknown_stabs (void)
{
  int i;

  for (i = 0; i < sym_count; ++i)
    switch (sym_ptr[i].n_type)
      {
      case 0:       case 0     |N_EXT:
      case N_ABS:   case N_ABS |N_EXT:
      case N_TEXT:  case N_TEXT|N_EXT:
      case N_DATA:  case N_DATA|N_EXT:
      case N_BSS:   case N_BSS |N_EXT:
                    case N_INDR|N_EXT:
                    case N_WEAKU:
                    case N_WEAKA:
                    case N_WEAKT:
                    case N_WEAKD:
                    case N_WEAKB:
      case N_SETA:  case N_SETA|N_EXT:
      case N_SETT:  case N_SETT|N_EXT:
      case N_SETD:  case N_SETD|N_EXT:
      case N_GSYM:
      case N_FUN:
      case N_STSYM:
      case N_LCSYM:
      case N_RSYM:
      case N_SLINE:
      case N_SO:
                    case N_IMP1|N_EXT:
                    case N_IMP2|N_EXT:
      case N_EXP:   case N_EXP|N_EXT:
      case N_LSYM:
      case N_SOL:
      case N_PSYM:
      case N_LBRAC:
      case N_RBRAC:
        break;
      default:
        fprintf (stderr, "Unknown symbol table entry: 0x%.2x \"%s\"\n",
                 sym_ptr[i].n_type, str_ptr + sym_ptr[i].n_un.n_strx);
        break;
      }
}

/* Convert an a.out module to an OMF module.  SIZE is the number of
   bytes in the input file inp_file. */

static void o_to_omf (long size)
{
  byte *t;
  const struct nlist *entry_symbol;
  struct set *set_ptr;
  int j;

  /* Simplify things by reading the complete a.out module into
     memory. */

  inp_buf = xmalloc (size);
  size = fread (inp_buf, 1, size, inp_file);
  if (ferror (inp_file))
    goto read_error;

  /* Set up pointers to various sections of the module read into
     memory. */

  a_out_h = (struct exec *)inp_buf;
  if (size < sizeof (struct exec) || N_MAGIC(*a_out_h) != OMAGIC)
    error ("Input file `%s' is not an a.out file", error_fname);
  text_size = a_out_h->a_text;
  data_size = a_out_h->a_data;
  t = inp_buf + sizeof (struct exec);
  text_ptr = t; t += text_size;
  data_ptr = t; t += data_size;
  text_rel = (const struct relocation_info *)t; t += a_out_h->a_trsize;
  data_rel = (const struct relocation_info *)t; t += a_out_h->a_drsize;
  sym_ptr = (const struct nlist *)t; t += a_out_h->a_syms;
  str_ptr = t;
  if (a_out_h->a_syms == 0)
    str_size = 0;
  else
    {
      if (str_ptr + 4 - inp_buf > size)
        goto invalid;
      str_size = *(long *)str_ptr;
    }
  sym_count = a_out_h->a_syms / sizeof (struct nlist);
  if (str_ptr + str_size - inp_buf > size)
    goto invalid;

  /* Build the complementary array of symbol data. */

  sym_more = xmalloc (sym_count * sizeof (struct symbol));
  memset(sym_more, 0, sym_count * sizeof (struct symbol));

  /* Print unknown symbol table entries if the -u option is given. */

  if (unknown_stabs)
    list_unknown_stabs ();

  /* Find the start symbol if converting a main module. */

  if (entry_name != NULL)
    {
      entry_symbol = find_symbol (entry_name);
      if (entry_symbol == NULL)
        error ("Entry symbol not found");
      if ((entry_symbol->n_type & ~N_EXT) != N_TEXT)
        error ("Entry symbol not in text segment");
    }
  else
    entry_symbol = NULL;        /* keep the compiler happy */

  /* Initialize variables for a new OMF module. */

  init_obj ();

  /* If the a.out module is an import module as created by emximp,
     create an OMF import module.  Everything is done by
     handle_import_i2() or handle_import_i1() in that case, therefore
     we can simply return. */

  if (out_lib != NULL && (handle_import_i2 () || handle_import_i1 ()))
    return;

  /* Initialize growing arrays for debug information conversion.
     These two arrays contain indices at which the $$SYMBOLS and
     $$TYPES segments, respectively, can be split into LEDATA records
     by write_seg(). */

  grow_init (&sst_boundary_grow, &sst_boundary, sizeof (*sst_boundary), 32);
  grow_init (&tt_boundary_grow, &tt_boundary, sizeof (*tt_boundary), 32);

  /* Create some OMF names. */

  ovl_name = find_lname ("");
  text_seg_name = find_lname ("TEXT32");
  data_seg_name = find_lname ("DATA32");
  bss_seg_name = find_lname ("BSS32");
  if (udat_seg_string != NULL)
    udat_seg_name = find_lname (udat_seg_string);
  else
    udat_seg_name = data_seg_name;
  if (!strip_symbols && a_out_h->a_syms)
    {
      symbols_seg_name = find_lname ("$$SYMBOLS");
      types_seg_name = find_lname ("$$TYPES");
    }
  code_class_name = find_lname ("CODE");
  data_class_name = find_lname ("DATA");
  bss_class_name = find_lname ("BSS");
  if (!strip_symbols && a_out_h->a_syms)
    {
      debsym_class_name = find_lname ("DEBSYM");
      debtyp_class_name = find_lname ("DEBTYP");
    }
  define_set_names ();
  if (mod_type == MT_MAIN)
    {
      stack_seg_name = find_lname ("STACK");
      stack_class_name = find_lname ("STACK");
    }
  flat_group_name = find_lname ("FLAT");
  dgroup_group_name = find_lname ("DGROUP");

  /* Write the THREADR record. */

  get_mod_name ();
  write_theadr ();

  /* Tell ilink what TIS standard we follow. */

  write_tis ();

  /* Write default library requests and the debug information style
     record (COMENT records). */

  write_libs ();
  write_debug_style ();

  /* Tell ilink what DBGPACK DLL to use. */

  write_dbgpack ();

  /* Tell LINK386 what identifier manipulator DLL to use. */

  write_idmdll ();

  /* Define all the OMF names (LNAMES record).  Of course, we must not
     define new OMF names after this point. */

  write_lnames ();

  /* Define segments (SEGDEF records).  This must be done after
     defining the names and before defining the groups. */

  text_index = seg_def (text_seg_name, code_class_name, text_size,
                        FALSE, FALSE);
  if (udat_seg_string != NULL)
    udat_index = seg_def (udat_seg_name, data_class_name, data_size,
                          FALSE, FALSE);
  data_index = seg_def (data_seg_name, data_class_name,
                        (udat_seg_string == NULL ? data_size : 0),
                        FALSE, FALSE);
  if (udat_seg_string == NULL)
    udat_index = data_index;

  write_set_segs ();

  bss_index = seg_def (bss_seg_name, bss_class_name, a_out_h->a_bss,
                       FALSE, FALSE);

  if (mod_type == MT_MAIN)
    stack_index = seg_def (stack_seg_name, stack_class_name, 0x8000, TRUE, FALSE);

  if (!strip_symbols && a_out_h->a_syms)
    {
      convert_debug ();         /* After seg_def of text, data & bss */
      symbols_index = seg_def (symbols_seg_name, debsym_class_name,
                               sst.size, FALSE, FALSE);
      types_index = seg_def (types_seg_name, debtyp_class_name,
                             tt.size, FALSE, FALSE);
    }

  /* Define groups (GRPDEF records).  This must be done after defining
     segments.
     We lazily assumes that the number of sets will not make the GRPDEF
     record too big. Rather safe unless a hundred setvectors are used... */

  flat_index = group_index++;
  init_rec (GRPDEF);
  put_idx (flat_group_name);
  write_rec ();

  dgroup_index = group_index++;
  init_rec (GRPDEF);
  put_idx (dgroup_group_name);
  put_8 (0xff); put_idx (bss_index);
  put_8 (0xff); put_idx (data_index);
  for (set_ptr = sets; set_ptr != NULL; set_ptr = set_ptr->next)
      for (j = 0; j < 3; ++j)
        {
          put_8 (0xff); put_idx (set_ptr->seg_index[j]);
        }
  write_rec ();

  /* Convert imports and exports. These show up very early in VAC
     generated OMF files. */
  write_import_i2 ();
  write_export ();

  /* Define external, communal and public symbols (EXTDEF, WKEXT,
     COMDEF, PUBDEF, and ALIAS records).  This must be done after
     defining the groups. */

  write_extdef ();
  write_wkext ();
  write_comdef ();
  write_pubdef ();
  write_set_pub ();
  write_alias ();

  /* Write link pass separator. */

  write_pass2 ();

  /* Write segment contents (LEDATA and FIXUPP records) and line
     number information (LINNUM record). */

  write_seg (text_index, text_seg_name, text_ptr, text_size,
             text_rel, a_out_h->a_trsize, FALSE, NULL, 0, N_TEXT);
  write_seg (udat_index, udat_seg_name, data_ptr, data_size,
             data_rel, a_out_h->a_drsize, FALSE, NULL, 0, N_DATA);

  write_set_data ();

  if (!strip_symbols && a_out_h->a_syms)
    {
      write_seg (types_index, types_seg_name, tt.buf, tt.size, NULL, 0, FALSE,
                 tt_boundary, tt_boundary_grow.count, -1);
      write_seg (symbols_index, symbols_seg_name, sst.buf, sst.size,
                 (const struct relocation_info *)sst_reloc.buf, sst_reloc.size, TRUE,
                 sst_boundary, sst_boundary_grow.count, -1);
      write_linnum ();
    }

  /* End of module. */

  init_rec (MODEND|REC32);
  if (entry_name != NULL)
    {
      put_8 (0xc1);             /* Main module with start address */
      put_8 (0x50);             /* ENDDAT: F5, T0 */
      put_idx (text_index);
      put_32 (entry_symbol->n_value);
    }
  else
    put_8 (0x00);               /* Non-main module without start address */
  write_rec ();

  /* Clean up memory. */

  free_lnames ();
  free (inp_buf);
  free (sym_more);
  buffer_free (&tt);
  buffer_free (&sst);
  buffer_free (&sst_reloc);
  grow_free (&sst_boundary_grow);
  grow_free (&tt_boundary_grow);
  return;

read_error:
  error ("Cannot read `%s'", error_fname);

invalid:
  error ("Malformed a.out file `%s'", error_fname);
}


/* Display some hints on using this program, then quit. */

static void usage (void)
{
  puts ("emxomf " VERSION VERSION_DETAILS
        "\nCopyright (c) 1992-1995 by Eberhard Mattes\n" VERSION_COPYRIGHT "\n");
  puts ("Usage:");
  puts ("  emxomf [-dgqs] [-l[<symbol>]] [-m <symbol>] [-p <page_size>]");
  puts ("         [-i <default_lib>] [-I <idmdll>] [-D <dataseg>]");
  puts ("         -o <output_file> <input_file>");
  puts ("  emxomf [-dgqsx] [-l[<symbol>]] [-m <symbol>] [-p <page_size>]");
  puts ("         [-O <directory>] [-r|R <response_file>] [-i <default_lib>]");
  puts ("         [-I <idmdll>] [-D <dataseg>] <input_file>...");
  puts ("\nOptions:");
  puts ("  -d                 Delete input files except for archives");
  puts ("  -i <default_lib>   Add default library request");
  puts ("  -l[<symbol>]       Convert library modules, with optional entrypoint");
  puts ("  -m <symbol>        Convert main module with entrypoint <symbol>");
  puts ("  -o <output_file>   Write output to <output_file>");
  puts ("  -p <page_size>     Set page size for LIB files");
  puts ("  -q                 Suppress certain warnings");
  puts ("  -r <response_file> Write response file for adding modules");
  puts ("  -R <response_file> Write response file for replacing modules");
  puts ("  -s                 Strip debugging information");
  puts ("  -u                 List symbol table entries unknown to emxomf");
  puts ("  -x                 Extract archive members");
  puts ("  -D <dataseg>       Change the name of the data segment");
  puts ("  -I <idmdll>        Name the identifier manipulation DLL");
  puts ("  -O <directory>     Extract files to <directory>");
  puts ("  -z                 Remove underscores from all symbol names");
  puts ("  -P <dbgpackdll>    Name the dbgpack DLL (ilink)");
  exit (1);
}


/* Create the output file.  If the -r or -R option is used, create the
   LIB resonse file. */

static void open_output (void)
{
  char *tmp, *p;

  out_file = fopen (out_fname, "wb");
  if (out_file == NULL)
    error ("Cannot create output file `%s'", out_fname);
  if (response_file != NULL)
    {
      if (response_first)
        response_first = FALSE;
      else
        fprintf (response_file, " &\n");
      tmp = alloca (strlen (out_fname) + 1);
      strcpy (tmp, out_fname);
      for (p = tmp; *p != 0; ++p)
        if (*p == '/')
          *p = '\\';
      fprintf (response_file, "%s %s", (response_replace ? "-+" : "+"), tmp);
    }
}


/* Close the output file.  Display an error message and quit on
   failure. */

static void close_output (void)
{
  if (fflush (out_file) != 0 || fclose (out_file) != 0)
    {
      out_file = NULL;
      error ("Write error on output file `%s'", out_fname);
    }
  out_file = NULL;
}


/* Define or build the name of the output file.  If DST_FNAME is not
   NULL, use it as name of the output file.  Otherwise, build the name
   from INP_FNAME (the input file name) and EXT (the extension).  If
   an output directory has been specified with the -O option, remove
   the directory part of INP_FNAME and use output_dir instead. */

static void make_out_fname (const char *dst_fname, const char *inp_fname,
                            const char *ext)
{
  static char tmp1[MAXPATHLEN+3];
  char tmp2[MAXPATHLEN+3];

  if (dst_fname == NULL)
    {
      if (*output_dir != 0)
        _splitpath (inp_fname, NULL, NULL, tmp2, NULL);
      else
        _strncpy (tmp2, inp_fname, MAXPATHLEN);
      if (strlen (output_dir) + strlen (tmp2) + 5 > MAXPATHLEN)
        error ("File name `%s' too long", inp_fname);
      strcpy (tmp1, output_dir);
      strcat (tmp1, tmp2);
      _remext (tmp1);
      strcat (tmp1, ext);
      out_fname = tmp1;
    }
  else
    out_fname = dst_fname;
}


/* Read the header of an archive member at file position POS.  Return
   FALSE when reaching the end of the file. */

static int ar_read_header (struct ar_hdr *dst, long pos)
{
  int size, i;

  fseek (inp_file, pos, SEEK_SET);
  size = fread  (dst, 1, sizeof (struct ar_hdr), inp_file);
  if (size == 0)
    return FALSE;
  else if (size != sizeof (struct ar_hdr))
    error ("Malformed archive `%s'", inp_fname);

  /* Remove trailing blanks from the member name. */

  i = sizeof (dst->ar_name) - 1;
  while (i > 0 && dst->ar_name[i-1] == ' ')
    --i;
  dst->ar_name[i] = 0;
  return TRUE;
}


/* Retrieve the size from the header of an archive member. */

static long ar_member_size (const struct ar_hdr *p)
{
  long size;
  char *e;

  errno = 0;
  size = strtol (p->ar_size, &e, 10);
  if (e == p->ar_size || errno != 0 || size <= 0 || *e != ' ')
    error ("Malformed archive header in `%s'", inp_fname);
  return size;
}


/* Convert the a.out file SRC_FNAME to an OMF file named DST_FNAME.
   If DST_FNAME is NULL, the output file name is derived from
   SRC_FNAME. */

static void convert (const char *src_fname, const char *dst_fname)
{
  char tmp1[MAXPATHLEN+3], tmp2[MAXPATHLEN+3];
  char *p = NULL, *name, *end;
  long size, index;
  static char ar_magic[SARMAG+1] = ARMAG;
  char ar_test[SARMAG];
  struct ar_hdr ar;
  long ar_pos;
  struct delete *del;
  char *long_names = NULL;
  size_t long_names_size = 0;

  inp_fname = src_fname;
  error_fname = inp_fname;
  mod_name[0] = 0;
  inp_file = fopen (inp_fname, "rb");
  if (inp_file == NULL)
    error ("Cannot open input file `%s'", inp_fname);

  /* Read some bytes from the start of the file to find out whether
     this is an archive (.a) file or not. */

  if (fread (ar_test, sizeof (ar_test), 1, inp_file) != 1)
    error ("Cannot read input file `%s'", inp_fname);

  /* Create a LIB response file if requested and not yet done. */

  if (response_fname != NULL && response_file == NULL)
    {
      response_file = fopen (response_fname, "wt");
      if (response_file == NULL)
        error ("Cannot create response file `%s'", response_fname);
    }

  /* The rest of this function (save closing the input file) depends
     on whether the input file is an archive or not. */

  if (memcmp (ar_test, ar_magic, SARMAG) == 0)
    {

      /* The input file is an archive.  We cannot procede if the user
         has specified an output file name and wants one OMF file to
         be written for each module in the archive (well, we could do
         it if there's exactly one module in the archive, but we
         don't). */

      if (dst_fname != NULL && opt_x)
        error ("Cannot process archive if -o and -x are used");
      ar_pos = SARMAG;

      /* Create an OMF library file (.LIB file) if the -x option is
         not given. */

      if (!opt_x)
        {
          /* calculate a page size based on the size of the aout library.
             Since we don't yet know the number of files it contains, we'll
             assum the converted library isn't more than the double it's size.
             Or at least we can hope it's not... */
          struct stat s;
          int calculated_page_size = page_size;
          if (calculated_page_size <= 0)
            {
              /* For a better calculation we would need the file count. */
              calculated_page_size = 16;
              if (!stat(src_fname, &s))
                {
                  int cbPage = (s.st_size * 2) / 65536;
                  /* Don't allow page size larger than 32K since omflib
                     won't allow that ... */
                  while ((calculated_page_size < cbPage)
                      && (calculated_page_size <= 16384))
                      calculated_page_size <<= 1;
                }
            }

          make_out_fname (dst_fname, inp_fname, ".lib");
          out_lib = omflib_create (out_fname, calculated_page_size, lib_errmsg);
          if (out_lib == NULL)
            error (lib_errmsg);
          if (omflib_header (out_lib, lib_errmsg) != 0)
            error (lib_errmsg);
        }

      /* Loop over all the members of the archive. */

      while (ar_read_header (&ar, ar_pos))
        {
          /* Decode the header. */

          size = ar_member_size (&ar);
          ar_pos += (sizeof (ar) + size + 1) & -2;

          if (strcmp (ar.ar_name, "ARFILENAMES/") == 0)
            {
              size_t i;

              /* The "ARFILENAMES/" member contains the long file
                 names, each one is terminated with a newline
                 character.  Member names starting with a space are
                 also considered long file names because a leading
                 space is used for names pointing into the
                 "ARFILENAMES/" table.  Read the "ARFILENAMES/" member
                 to LONG_NAMES. */

              if (size != 0)
                {
                  long_names_size = (size_t)size;
                  long_names = xmalloc (long_names_size);
                  size = fread (long_names, 1, long_names_size, inp_file);
                  if (ferror (inp_file))
                    error ("Cannot read `%s'", inp_fname);
                  if (size != long_names_size)
                    error ("%s: ARFILENAMES/ member is truncated", inp_fname);

                  /* Replace the newlines with nulls to make
                     processing a bit more convenient. */

                  for (i = 0; i < long_names_size; ++i)
                    if (long_names[i] == '\n')
                      long_names[i] = 0;
                  if (long_names[long_names_size-1] != 0)
                    error ("%s: ARFILENAMES/ member corrupt", inp_fname);
                }
            }

          /* Ignore the __.SYMDEF and __.IMPORT members.  Ignore
             import modules unless creating a library file. */

          else if (strcmp (ar.ar_name, "__.SYMDEF") != 0
                   && strcmp (ar.ar_name, "__.IMPORT") != 0
                   && (memcmp (ar.ar_name, "IMPORT#", 7) != 0 || !opt_x))
            {

              /* Convert the current member to OMF.  First, fetch the
                 name of the member.  If the ar_name starts with a
                 space, the decimal number following that space is an
                 offset into the "ARFILENAMES/" member.  The number
                 may be followed by a space and a substring of the
                 long file name. */

              name = ar.ar_name;
              if (name[0] == ' ' && long_names != NULL
                  && (index = strtol (name + 1, &end, 10)) >= 0
                  && index < long_names_size - 1
                  && (*end == 0 || *end == ' ')
                  && (index == 0 || long_names[index-1] == 0))
                name = long_names + index;

              /* Extract the base name of the member. */

              _splitpath (name, NULL, NULL, tmp2, NULL);
              if (strlen (output_dir) + strlen (tmp2) + 5 > MAXPATHLEN)
                error ("File name `%s' too long", tmp2);

              /* Provide for informative error message.  Memory leak. */

              p = xmalloc (strlen (inp_fname) + 3 + strlen (name));
              sprintf (p, "%s(%s)", inp_fname, name);
              error_fname = p;

              /* Construct the module name for the THREADR record
                 etc. */

              _strncpy (mod_name, tmp2, sizeof (mod_name));
              strcat (tmp2, ".obj");

              /* Create the output file (-x option) or add a new
                 module to the output library (no -x option). */

              if (opt_x)
                {
                  strcpy (tmp1, output_dir);
                  strcat (tmp1, tmp2);
                  out_fname = tmp1;
                  open_output ();
                }
              else
                {
                  if (omflib_write_module (out_lib, tmp2, &mod_page,
                                           lib_errmsg) != 0)
                    error (lib_errmsg);
                }

              /* Convert the member and close the output file. */

              o_to_omf (size);
              if (opt_x)
                close_output ();
            }
          if (p != NULL) free (p);
        }

      /* Finish and close the library library if creating a library
         file. */

      if (!opt_x)
        {
          if (omflib_finish (out_lib, lib_errmsg) != 0
              || omflib_close (out_lib, lib_errmsg) != 0)
            error (lib_errmsg);
          out_lib = NULL;
        }
    }
  else
    {

      /* The input file is not an archive.  We assume it being an
         a.out object file.  Get the size of the file for
         o_to_omf(). */

      if (fseek (inp_file, 0L, SEEK_END) != 0)
        error ("Input file `%s' is not seekable", inp_fname);
      size = ftell (inp_file);
      fseek (inp_file, 0L, SEEK_SET);

      /* Convert the file. */

      make_out_fname (dst_fname, inp_fname, ".obj");
      open_output ();
      o_to_omf (size);
      close_output ();

      /* If input files are to be deleted (-d option), add the current
         input file to the list of files to be deleted. */

      if (delete_input_files)
        {
          del = xmalloc (sizeof (struct delete));
          del->name = xstrdup (inp_fname);
          del->next = files_to_delete;
          files_to_delete = del;
        }
    }
  fclose (inp_file);
  free_modstr_cache ();
  if (long_names != NULL)
    free (long_names);
}


/* Delete all the files stored in the FILES_TO_DELETE list.  While
   deleting the files, the list elements are deallocated. */

static void delete_files (void)
{
  struct delete *p, *q;

  for (p = files_to_delete; p != NULL; p = q)
    {
      q = p->next;
      remove (p->name);
      free (p->name);
      free (p);
    }
}


/* Main function of emxomf.  Parse the command line and perform the
   requested actions. */

int main (int argc, char *argv[])
{
  int c, i;
  char *opt_o, *opt_O, *tmp;
  struct libreq *lrp;

  /* Keep emxomf in memory for the number of minutes indicated by the
     GCCLOAD environment variable. */

  _emxload_env ("GCCLOAD");

  /* Get options from the EMXOMFOPT environment variable, expand
     response files (@filename) and wildcard (*.o) on the command
     line. */

  _envargs (&argc, &argv, "EMXOMFOPT");
  _response (&argc, &argv);
  _wildcard (&argc, &argv);

  /* Set default values of some options. */

  opt_o = NULL; opt_O = NULL;
  opterr = FALSE;

  tmp = getenv("EMXOMFLD_TYPE");
  if (tmp)
    {
      if (!stricmp (tmp, "VAC308"))
        hll_version = 4;
      else if (!stricmp (tmp, "LINK386"))
        hll_version = 3;
    }

  /* Parse the command line options. */

  while ((c = getopt_long (argc, argv, "bdD:h:i:jI:m:l::o:P:p:qO:r:R:tsuxwz", NULL, NULL)) != EOF) /* use long for getting optional -l argument working (FLAG_PERMUTE). */
    switch (c)
      {
      case 'b':
        force_big = TRUE;
        break;
      case 'd':
        delete_input_files = TRUE;
        break;
      case 'D':
        udat_seg_string = optarg;
        break;
      case 'h':
        hll_version = optarg ? atoi(optarg) : 4;
        if (hll_version != 4 && hll_version != 3 && hll_version != 6)
          {
            printf ("syntax error: Invalid HLL version specified (%d)\n", hll_version);
            usage ();
          }
        break;
      case 'i':
        lrp = xmalloc (sizeof (*lrp));
        lrp->next = NULL;
        lrp->name = xstrdup (optarg);
        *libreq_add = lrp;
        libreq_add = &lrp->next;
        break;
      case 'I':
        if (strcmp (optarg, "-") == 0)
          idmdll_name = NULL;
        else
          idmdll_name = optarg;
        break;
      case 'l':
        if (mod_type != MT_MODULE)
          usage ();
        mod_type = MT_LIB;
        entry_name = optarg;
        break;
      case 'm':
        if (mod_type != MT_MODULE)
          usage ();
        mod_type = MT_MAIN;
        entry_name = optarg;
        break;
      case 'o':
        if (opt_o != NULL || opt_O != NULL)
          usage ();
        opt_o = optarg;
        break;
      case 'P':
        if (strcmp (optarg, "-") == 0)
          dbgpack_name = NULL;
        else
          dbgpack_name = optarg;
        break;
      case 'p':
        errno = 0;
        page_size = (int)strtol (optarg, &tmp, 0);
        if (tmp == optarg || errno != 0 || *tmp != 0
            || page_size < 16 || page_size > 32768
            || (page_size & (page_size - 1)) != 0)
          usage ();
        break;
      case 'q':
        warning_level--;
        break;
      case 'O':
        if (opt_o != NULL || opt_O != NULL)
          usage ();
        opt_O = optarg;
        break;
      case 'r':
      case 'R':
        if (response_fname != NULL)
          usage ();
        response_fname = optarg;
        response_replace = (c == 'R');
        break;
      case 's':
        strip_symbols = TRUE;
        break;
      case 'u':
        unknown_stabs = TRUE;
        break;
      case 'v':
        warning_level++;
        break;
      case 'x':
        opt_x = TRUE;
        break;
      case 'z':
        opt_rmunder = TRUE;
        break;
      default:
        usage ();
      }

  /* Check for non-option arguments. */

  if (argc - optind == 0)
    usage ();

  if (opt_o != NULL)
    {
      /* If the -o option is used, there must be exactly one input
         file name. */

      if (argc - optind != 1)
        usage ();
      convert (argv[optind], opt_o);
    }
  else
    {

      /* The -o option is not used.  If the -O option is used, set up
         output_dir. */

      if (opt_O != NULL)
        {
          i = strlen (opt_O);
          tmp = xmalloc (i + 2);
          strcpy (tmp, opt_O);
          if (i > 0 && strchr (":\\/", tmp[i-1]) == NULL)
            strcat (tmp, "/");
          output_dir = tmp;
        }

      /* Convert all the files named on the command line. */

      for (i = optind; i < argc; ++i)
        convert (argv[i], NULL);
    }

  /* If a LIB response file has been created, finish and close it. */

  if (response_file != NULL)
    {
      if (!response_first)
        fprintf (response_file, "\n");
      if (fflush (response_file) != 0 || fclose (response_file) != 0)
        error ("Write error on response file `%s'", response_fname);
    }

  /* If the user wants input files to be deleted, delete them now. */

  if (delete_input_files)
    delete_files ();
  return 0;
}
