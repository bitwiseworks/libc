/* ld.c -- changed for emx by Eberhard Mattes -- Sep 1998
           changed for Innotek GCC by Andrew Zabolotny -- 2003
           changed for Innotek GCC by knut st. osmundsen -- 2004
           changed for RSX by Rainer Schnitker -- Feb 1996 */

/* Linker `ld' for GNU
   Copyright (C) 1988 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by Richard Stallman with some help from Eric Albert.
   Set, indirect, and warning symbol features added by Randy Smith.  */

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include <ar.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <io.h>
#include <process.h>
#include <errno.h>
#include <utime.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#ifndef USG
#include <sys/time.h>
#include <sys/resource.h>
#endif
#ifdef __EMX__
#include <string.h>
#endif
#include "libiberty.h"

#if !defined(A_OUT)
#define A_OUT
#endif

#ifdef A_OUT
#include <a_out.h>
#endif

#ifndef _System
#define _System
#endif
extern int _System DosCopy (char *, char *, int);

/* We need .data of every module aligned to at least 16 bound
   in order to support the alignments required by SSE. 
   Ditto for constants in .text. */
#define SECTION_ALIGN		16
#define SECTION_ALIGN_MASK	(SECTION_ALIGN-1)

/* If compiled with GNU C, use the built-in alloca */
#ifdef USE_ALLOCA
#include "alloca.h"
#define ALLOCA(size)    alloca (size)
#define FREEA(ptr)      do { (ptr) = NULL; } while (0)
#else
#define ALLOCA(size)    malloc (size)
#define FREEA(ptr)      do { free(ptr); (ptr) = NULL; } while (0)
#endif
#include "getopt.h"

/* Always use the GNU version of debugging symbol type codes, if possible.  */

#include "stab.h"
#define CORE_ADDR unsigned long	/* For symseg.h */
#include "symseg.h"

#include <strings.h>

/* Determine whether we should attempt to handle (minimally)
   N_BINCL and N_EINCL.  */

#if defined (__GNU_STAB__) || defined (N_BINCL)
#define HAVE_SUN_STABS
#endif

#define min(a,b) ((a) < (b) ? (a) : (b))

/* Macro to control the number of undefined references printed */
#define MAX_UREFS_PRINTED	10

/* Size of a page; obtained from the operating system.  */

int page_size;

/* Name this program was invoked by.  */

char *progname;

/* System dependencies */

/* Define this if names etext, edata and end should not start with `_'.  */
/* #define nounderscore 1 */

/* Define NON_NATIVE if using BSD or pseudo-BSD file format on a system
   whose native format is different.  */
/* #define NON_NATIVE */

/* The "address" of the data segment in a relocatable file.
   The text address of a relocatable file is always
   considered to be zero (instead of the value of N_TXTADDR, which
   is what the address is in an executable), so we need to subtract
   N_TXTADDR from N_DATADDR to get the "address" for the input file.  */
#define DATA_ADDR_DOT_O(hdr) (N_DATADDR(hdr) - N_TXTADDR(hdr))

/* Values for 3rd argument to lseek().  */
#ifndef L_SET
#define L_SET 0
#endif
/* This is called L_INCR in BSD, but SEEK_CUR in POSIX.  */
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

/*
 * Ok.  Following are the relocation information macros.  If your
 * system cannot use the default set (below), you must define all of these:

 *   relocation_info: This must be typedef'd (or #define'd) to the type
 * of structure that is stored in the relocation info section of your
 * a.out files.  Often this is defined in the a.out.h for your system.
 *
 *   RELOC_ADDRESS (rval): Offset into the current section of the
 * <whatever> to be relocated.  *Must be an lvalue*.
 *
 *   RELOC_EXTERN_P (rval):  Is this relocation entry based on an
 * external symbol (1), or was it fully resolved upon entering the
 * loader (0) in which case some combination of the value in memory
 * (if RELOC_MEMORY_ADD_P) and the extra (if RELOC_ADD_EXTRA) contains
 * what the value of the relocation actually was.  *Must be an lvalue*.
 *
 *   RELOC_TYPE (rval): For a non-external relocation, this is the
 * segment to relocate for.  *Must be an lvalue.*
 *
 *   RELOC_SYMBOL (rval): For an external relocation, this is the
 * index of its symbol in the symbol table.  *Must be an lvalue*.
 *
 *   RELOC_MEMORY_ADD_P (rval): This should be 1 if the final
 * relocation value output here should be added to memory; 0, if the
 * section of memory described should simply be set to the relocation
 * value.
 *
 *   RELOC_MEMORY_ADD_P (rval): If this is nonzero, the value previously
 * present in the memory location to be relocated is *added*
 * to the relocation value, to produce the final result.
 * Otherwise, the relocation value is stored in the memory location
 * and the value previously found there is ignored.
 * By default, this is always 1.
 *
 *   RELOC_MEMORY_SUB_P (rval): If this is nonzero, the value previously
 * present in the memory location to be relocated is *subtracted*
 * from the relocation value, to produce the final result.
 * By default, this is always 0.
 *
 *   RELOC_ADD_EXTRA (rval): (Optional) This macro, if defined, gives
 * an extra value to be added to the relocation value based on the
 * individual relocation entry.  *Must be an lvalue if defined*.
 *
 *   RELOC_PCREL_P (rval): True if the relocation value described is
 * pc relative.
 *
 *   RELOC_VALUE_RIGHTSHIFT (rval): Number of bits right to shift the
 * final relocation value before putting it where it belongs.
 *
 *   RELOC_TARGET_SIZE (rval): log to the base 2 of the number of
 * bytes of size this relocation entry describes; 1 byte == 0; 2 bytes
 * == 1; 4 bytes == 2, and etc.  This is somewhat redundant (we could
 * do everything in terms of the bit operators below), but having this
 * macro could end up producing better code on machines without fancy
 * bit twiddling.  Also, it's easier to understand/code big/little
 * endian distinctions with this macro.
 *
 *   RELOC_TARGET_BITPOS (rval): The starting bit position within the
 * object described in RELOC_TARGET_SIZE in which the relocation value
 * will go.
 *
 *   RELOC_TARGET_BITSIZE (rval): How many bits are to be replaced
 * with the bits of the relocation value.  It may be assumed by the
 * code that the relocation value will fit into this many bits.  This
 * may be larger than RELOC_TARGET_SIZE if such be useful.
 *
 *
 *		Things I haven't implemented
 *		----------------------------
 *
 *    Values for RELOC_TARGET_SIZE other than 0, 1, or 2.
 *
 *    Pc relative relocation for External references.
 *
 *
 */

/* Default macros */
#ifndef RELOC_ADDRESS
#define RELOC_ADDRESS(r)		((r)->r_address)
#define RELOC_EXTERN_P(r)		((r)->r_extern)
#define RELOC_TYPE(r)		((r)->r_symbolnum)
#define RELOC_SYMBOL(r)		((r)->r_symbolnum)
#define RELOC_MEMORY_SUB_P(r)	0
#define RELOC_MEMORY_ADD_P(r)	1
#undef RELOC_ADD_EXTRA
#define RELOC_PCREL_P(r)		((r)->r_pcrel)
#define RELOC_VALUE_RIGHTSHIFT(r)	0
#define RELOC_TARGET_SIZE(r)		((r)->r_length)
#define RELOC_TARGET_BITPOS(r)	0
#define RELOC_TARGET_BITSIZE(r)	32
#endif

#ifndef MAX_ALIGNMENT
#define	MAX_ALIGNMENT	(sizeof (int))
#endif

#ifdef nounderscore
#define LPREFIX '.'
#else
#define LPREFIX 'L'
#endif


/* Special global symbol types understood by GNU LD.  */

/* The following type indicates the definition of a symbol as being
   an indirect reference to another symbol.  The other symbol
   appears as an undefined reference, immediately following this symbol.

   Indirection is asymmetrical.  The other symbol's value will be used
   to satisfy requests for the indirect symbol, but not vice versa.
   If the other symbol does not have a definition, libraries will
   be searched to find a definition.

   So, for example, the following two lines placed in an assembler
   input file would result in an object file which would direct gnu ld
   to resolve all references to symbol "foo" as references to symbol
   "bar".

	.stabs "_foo",11,0,0,0
	.stabs "_bar",1,0,0,0

   Note that (11 == (N_INDR | N_EXT)) and (1 == (N_UNDF | N_EXT)).  */

#ifndef N_INDR
#define N_INDR 0xa
#endif

/* The following symbols refer to set elements.  These are expected
   only in input to the loader; they should not appear in loader
   output (unless relocatable output is requested).  To be recognized
   by the loader, the input symbols must have their N_EXT bit set.
   All the N_SET[ATDB] symbols with the same name form one set.  The
   loader collects all of these elements at load time and outputs a
   vector for each name.
   Space (an array of 32 bit words) is allocated for the set in the
   data section, and the n_value field of each set element value is
   stored into one word of the array.
   The first word of the array is the length of the set (number of
   elements).  The last word of the vector is set to zero for possible
   use by incremental loaders.  The array is ordered by the linkage
   order; the first symbols which the linker encounters will be first
   in the array.

   In C syntax this looks like:

	struct set_vector {
	  unsigned int length;
	  unsigned int vector[length];
	  unsigned int always_zero;
	};

   Before being placed into the array, each element is relocated
   according to its type.  This allows the loader to create an array
   of pointers to objects automatically.  N_SETA type symbols will not
   be relocated.

   The address of the set is made into an N_SETV symbol
   whose name is the same as the name of the set.
   This symbol acts like a N_DATA global symbol
   in that it can satisfy undefined external references.

   For the purposes of determining whether or not to load in a library
   file, set element definitions are not considered "real
   definitions"; they will not cause the loading of a library
   member.

   If relocatable output is requested, none of this processing is
   done.  The symbols are simply relocated and passed through to the
   output file.

   So, for example, the following three lines of assembler code
   (whether in one file or scattered between several different ones)
   will produce a three element vector (total length is five words;
   see above), referenced by the symbol "_xyzzy", which will have the
   addresses of the routines _init1, _init2, and _init3.

   *NOTE*: If symbolic addresses are used in the n_value field of the
   defining .stabs, those symbols must be defined in the same file as
   that containing the .stabs.

	.stabs "_xyzzy",23,0,0,_init1
	.stabs "_xyzzy",23,0,0,_init2
	.stabs "_xyzzy",23,0,0,_init3

   Note that (23 == (N_SETT | N_EXT)).  */

#ifndef N_SETA
#define	N_SETA	0x14		/* Absolute set element symbol */
#endif				/* This is input to LD, in a .o file.  */

#ifndef N_SETT
#define	N_SETT	0x16		/* Text set element symbol */
#endif				/* This is input to LD, in a .o file.  */

#ifndef N_SETD
#define	N_SETD	0x18		/* Data set element symbol */
#endif				/* This is input to LD, in a .o file.  */

#ifndef N_SETB
#define	N_SETB	0x1A		/* Bss set element symbol */
#endif				/* This is input to LD, in a .o file.  */

/* Macros dealing with the set element symbols defined in a.out.h */
#define	SET_ELEMENT_P(x)	((x)>=N_SETA&&(x)<=(N_SETB|N_EXT))
#define TYPE_OF_SET_ELEMENT(x)	((x)-N_SETA+N_ABS)

#ifndef N_SETV
#define N_SETV	0x1C		/* Pointer to set vector in data area.  */
#endif				/* This is output from LD.  */

/* Check if a symbol is weak */
#define WEAK_SYMBOL(t) (((t) >= N_WEAKU) && ((t) <= N_WEAKB))

/* If a this type of symbol is encountered, its name is a warning
   message to print each time the symbol referenced by the next symbol
   table entry is referenced.

   This feature may be used to allow backwards compatibility with
   certain functions (eg. gets) but to discourage programmers from
   their use.

   So if, for example, you wanted to have ld print a warning whenever
   the function "gets" was used in their C program, you would add the
   following to the assembler file in which gets is defined:

	.stabs "Obsolete function \"gets\" referenced",30,0,0,0
	.stabs "_gets",1,0,0,0

   These .stabs do not necessarily have to be in the same file as the
   gets function, they simply must exist somewhere in the compilation.  */

#ifndef N_WARNING
#define N_WARNING 0x1E		/* Warning message to print if symbol
				   included */
#endif				/* This is input to ld */

#ifndef __GNU_STAB__

/* Line number for the data section.  This is to be used to describe
   the source location of a variable declaration.  */
#ifndef N_DSLINE
#define N_DSLINE (N_SLINE+N_DATA-N_TEXT)
#endif

/* Line number for the bss section.  This is to be used to describe
   the source location of a variable declaration.  */
#ifndef N_BSLINE
#define N_BSLINE (N_SLINE+N_BSS-N_TEXT)
#endif

#endif /* not __GNU_STAB__ */

/* Symbol table */

/* Global symbol data is recorded in these structures,
   one for each global symbol.
   They are found via hashing in 'symtab', which points to a vector of buckets.
   Each bucket is a chain of these structures through the link field.  */

typedef
  struct glosym
    {
      /* Pointer to next symbol in this symbol's hash bucket.  */
      struct glosym *link;
      /* Name of this symbol.  */
      char *name;
      /* Value of this symbol as a global symbol.  */
      long value;
      /* Chain of external 'nlist's in files for this symbol, both defs
	 and refs.  */
      struct nlist *refs;
      /* Any warning message that might be associated with this symbol
         from an N_WARNING symbol encountered. */
      char *warning;
      /* Nonzero means definitions of this symbol as common have been seen,
	 and the value here is the largest size specified by any of them.  */
      int max_common_size;
      /* For OUTPUT_RELOCATABLE, records the index of this global sym in the
	 symbol table to be written, with the first global sym given index 0.*/
      int def_count;
      /* Nonzero means a definition of this global symbol is known to exist.
	 Library members should not be loaded on its account.  */
      char defined;
      /* Nonzero means a reference to this global symbol has been seen
	 in a file that is surely being loaded.
	 A value higher than 1 is the n_type code for the symbol's
	 definition.  */
      char referenced;
      /* A count of the number of undefined references printed for a
	 specific symbol.  If a symbol is unresolved at the end of
	 digest_symbols (and the loading run is supposed to produce
	 relocatable output) do_file_warnings keeps track of how many
	 unresolved reference error messages have been printed for
	 each symbol here.  When the number hits MAX_UREFS_PRINTED,
	 messages stop. */
      unsigned char undef_refs;
      /* 1 means that this symbol has multiple definitions.  2 means
         that it has multiple definitions, and some of them are set
	 elements, one of which has been printed out already.  */
      unsigned char multiply_defined;
      /* Nonzero means print a message at all refs or defs of this symbol */
      char trace;
      /* One of N_WEAKX values */
      char weak;
    }
  symbol;

/* Demangler for C++.  */
#include "demangle.h"
static char *my_cplus_demangle (const char *);
int demangle_options;

/* Demangler function to use.  We unconditionally enable the C++ demangler
   because we assume any name it successfully demangles was probably produced
   by the C++ compiler.  Enabling it only if -lg++ was specified seems too
   much of a kludge.  */
char *(*demangler)() = my_cplus_demangle;

/* Number of buckets in symbol hash table */
#define	TABSIZE	1009

/* The symbol hash table: a vector of TABSIZE pointers to struct glosym. */
symbol *symtab[TABSIZE];

/* Number of symbols in symbol hash table. */
int num_hash_tab_syms = 0;

/* Count the number of nlist entries that are for local symbols.
   This count and the three following counts
   are incremented as as symbols are entered in the symbol table.  */
int local_sym_count;

/* Count number of nlist entries that are for local symbols
   whose names don't start with L. */
int non_L_local_sym_count;

/* Count the number of nlist entries for debugger info.  */
int debugger_sym_count;

/* Count the number of global symbols referenced and not defined.  */
int undefined_global_sym_count;

/* Count the number of global symbols multiply defined.  */
int multiple_def_count;

/* Count the number of defined global symbols.
   Each symbol is counted only once
   regardless of how many different nlist entries refer to it,
   since the output file will need only one nlist entry for it.
   This count is computed by `digest_symbols';
   it is undefined while symbols are being loaded. */
int defined_global_sym_count;

/* Count the number of symbols defined through common declarations.
   This count is kept in symdef_library, linear_library, and
   enter_global_ref.  It is incremented when the defined flag is set
   in a symbol because of a common definition, and decremented when
   the symbol is defined "for real" (ie. by something besides a common
   definition).  */
int common_defined_global_count;

/* Count the number of set element type symbols and the number of
   separate vectors which these symbols will fit into.  See the
   GNU a.out.h for more info.
   This count is computed by 'enter_file_symbols' */
int set_symbol_count;
int set_vector_count;

/* Define a linked list of strings which define symbols which should
   be treated as set elements even though they aren't.  Any symbol
   with a prefix matching one of these should be treated as a set
   element.

   This is to make up for deficiencies in many assemblers which aren't
   willing to pass any stabs through to the loader which they don't
   understand.  */
struct string_list_element {
  char *str;
  struct string_list_element *next;
};

struct string_list_element *set_element_prefixes;

/* Count the number of definitions done indirectly (ie. done relative
   to the value of some other symbol. */
int global_indirect_count;

/* Count the number of warning symbols encountered. */
int warning_count;

/* Total number of symbols to be written in the output file.
   Computed by digest_symbols from the variables above.  */
int nsyms;


/* Nonzero means ptr to symbol entry for symbol to use as start addr.
   -e sets this.  */
symbol *entry_symbol;

/* These can be NULL if we don't actually have such a symbol.  */
symbol *edata_symbol;   /* the symbol _edata */
symbol *etext_symbol;   /* the symbol _etext */
symbol *end_symbol;	/* the symbol _end */
/* We also need __{edata,etext,end} so that they can safely
   be used from an ANSI library.  */
symbol *edata_symbol_alt;
symbol *etext_symbol_alt;
symbol *end_symbol_alt;

/* Kinds of files potentially understood by the linker. */

enum file_type { IS_UNKNOWN, IS_ARCHIVE, IS_A_OUT, IS_MACH_O };

/* Each input file, and each library member ("subfile") being loaded,
   has a `file_entry' structure for it.

   For files specified by command args, these are contained in the vector
   which `file_table' points to.

   For library members, they are dynamically allocated,
   and chained through the `chain' field.
   The chain is found in the `subfiles' field of the `file_entry'.
   The `file_entry' objects for the members have `superfile' fields pointing
   to the one for the library.  */

struct file_entry {
  /* Name of this file.  */
  char *filename;

  /* What kind of file this is. */
  enum file_type file_type;

  /* Name to use for the symbol giving address of text start */
  /* Usually the same as filename, but for a file spec'd with -l
     this is the -l switch itself rather than the filename.  */
  char *local_sym_name;

  /* Describe the layout of the contents of the file.  */

  /* The text section. */
  unsigned long int orig_text_address;
  unsigned long int text_size;
  long int text_offset;

  /* Text relocation. */
  unsigned long int text_reloc_size;
  long int text_reloc_offset;

  /* The data section. */
  unsigned long int orig_data_address;
  unsigned long int data_size;
  long int data_offset;

  /* Data relocation. */
  unsigned long int data_reloc_size;
  long int data_reloc_offset;

  /* The bss section. */
  unsigned long int orig_bss_address;
  unsigned long int bss_size;

  /* The symbol and string tables. */
  unsigned long int syms_size;
  long int syms_offset;
  unsigned long int strs_size;
  long int strs_offset;

  /* The GDB symbol segment, if any. */
  unsigned long int symseg_size;
  long int symseg_offset;

  /* Describe data from the file loaded into core */

  /* Symbol table of the file.  */
  struct nlist *symbols;

  /* Pointer to the string table.
     The string table is not kept in core all the time,
     but when it is in core, its address is here.  */
  char *strings;

  /* Next two used only if OUTPUT_RELOCATABLE or if needed for */
  /* output of undefined reference line numbers. */

  /* Text reloc info saved by `write_text' for `coptxtrel'.  */
  struct relocation_info *textrel;
  /* Data reloc info saved by `write_data' for `copdatrel'.  */
  struct relocation_info *datarel;

  /* Relation of this file's segments to the output file */

  /* Start of this file's text seg in the output file core image.  */
  int text_start_address;
  /* Start of this file's data seg in the output file core image.  */
  int data_start_address;
  /* Start of this file's bss seg in the output file core image.  */
  int bss_start_address;
  /* Offset in bytes in the output file symbol table
     of the first local symbol for this file.  Set by `write_file_symbols'.  */
  int local_syms_offset;

  /* For library members only */

  /* For a library, points to chain of entries for the library members.  */
  struct file_entry *subfiles;
  /* For a library member, offset of the member within the archive.
     Zero for files that are not library members.  */
  int starting_offset;
  /* Size of contents of this file, if library member.  */
  int total_size;
  /* For library member, points to the library's own entry.  */
  struct file_entry *superfile;
  /* For library member, points to next entry for next member.  */
  struct file_entry *chain;

  /* 1 if file is a library. */
  char library_flag;

  /* 1 if file's header has been read into this structure.  */
  char header_read_flag;

  /* 1 means search a set of directories for this file.  */
  char search_dirs_flag;

  /* 1 means this is base file of incremental load.
     Do not load this file's text or data.
     Also default text_start to after this file's bss. */
  char just_syms_flag;

  /* 1 means search for dynamic libs before static.
     0 means search only static libs. */
  char dynamic;
};

/* Vector of entries for input files specified by arguments.
   These are all the input files except for members of specified libraries.  */
struct file_entry *file_table;

/* Length of that vector.  */
int number_of_files;

/* When loading the text and data, we can avoid doing a close
   and another open between members of the same library.

   These two variables remember the file that is currently open.
   Both are zero if no file is open.

   See `each_file' and `file_close'.  */

struct file_entry *input_file;
int input_desc;

/* The name of the file to write; "a.out" by default.  */

char *output_filename;
char *exe_filename;
char *def_filename = NULL;
char *res_filename = NULL;
char *map_filename = NULL;
char *touch_filename = NULL;
int reloc_flag = 0;
int dll_flag = 0;
int exe_flag = 0;
int map_flag = 0;
int stack_size = 0;
int emxbind_strip = 0;
enum exe_bind_type
{
  EMX_DEFAULT, RSXNT_WIN32, RSXNT_RSX, RSXNT_EMX
} rsxnt_linked = EMX_DEFAULT;

/* What kind of output file to write.  */

enum file_type output_file_type;

#ifndef DEFAULT_OUTPUT_FILE_TYPE
#define DEFAULT_OUTPUT_FILE_TYPE IS_A_OUT
#endif

/* What `style' of output file to write.  For BSD a.out files
   this specifies OMAGIC, NMAGIC, or ZMAGIC.  For Mach-O files
   this switches between MH_OBJECT and two flavors of MH_EXECUTE.  */

enum output_style
  {
    OUTPUT_UNSPECIFIED,
    OUTPUT_RELOCATABLE,		/* -r */
    OUTPUT_WRITABLE_TEXT,	/* -N */
    OUTPUT_READONLY_TEXT,	/* -n */
    OUTPUT_DEMAND_PAGED		/* -Z (default) */
  };

enum output_style output_style;

#ifndef DEFAULT_OUTPUT_STYLE
#define DEFAULT_OUTPUT_STYLE OUTPUT_DEMAND_PAGED
#endif

/* Descriptor for writing that file with `mywrite'.  */

int outdesc;

/* The following are computed by `digest_symbols'.  */

int text_size;			/* total size of text of all input files.  */
int text_header_size;		/* size of the file header if included in the
				   text size.  */
int data_size;			/* total size of data of all input files.  */
int bss_size;			/* total size of bss of all input files.  */
int text_reloc_size;		/* total size of text relocation of all input files.  */
int data_reloc_size;		/* total size of data relocation of all input
				   files.  */

/* The following are computed by write_header().  */
long int output_text_offset;	/* file offset of the text section.  */
long int output_data_offset;	/* file offset of the data section.  */
long int output_trel_offset;	/* file offset of the text relocation info.  */
long int output_drel_offset;	/* file offset of the data relocation info.  */
long int output_syms_offset;	/* file offset of the symbol table.  */
long int output_strs_offset;	/* file offset of the string table.  */

/* The following are incrementally computed by write_syms(); we keep
   them here so we can examine their values afterwards.  */
unsigned int output_syms_size;	/* total bytes of symbol table output. */
unsigned int output_strs_size;	/* total bytes of string table output. */

/* This can only be computed after the size of the string table is known.  */
long int output_symseg_offset;	/* file offset of the symbol segment (if any).  */

/* Incrementally computed by write_file_symseg().  */
unsigned int output_symseg_size;

/* Specifications of start and length of the area reserved at the end
   of the text segment for the set vectors.  Computed in 'digest_symbols' */
int set_sect_start;
int set_sect_size;

/* Pointer for in core storage for the above vectors, before they are
   written. */
unsigned long *set_vectors;

int *set_reloc;

/* Amount of cleared space to leave at the end of the text segment.  */

int text_pad;

/* Amount of padding between data segment and set vectors. */
int set_sect_pad;

/* Amount of padding at end of data segment.  This has two parts:
   That which is before the bss segment, and that which overlaps
   with the bss segment.  */
int data_pad;

/* Format of __.SYMDEF:
   First, a longword containing the size of the 'symdef' data that follows.
   Second, zero or more 'symdef' structures.
   Third, a longword containing the length of symbol name strings.
   Fourth, zero or more symbol name strings (each followed by a null).  */

struct symdef {
  int symbol_name_string_index;
  int library_member_offset;
};

/* Record most of the command options.  */

/* Address we assume the text section will be loaded at.
   We relocate symbols and text and data for this, but we do not
   write any padding in the output file for it.  */
int text_start;

/* Address we decide the data section will be loaded at.  */
int data_start;

/* Nonzero if -T was specified in the command line.
   This prevents text_start from being set later to default values.  */
int T_flag_specified;

/* Nonzero if -Tdata was specified in the command line.
   This prevents data_start from being set later to default values.  */
int Tdata_flag_specified;

/* Size to pad data section up to.
   We simply increase the size of the data section, padding with zeros,
   and reduce the size of the bss section to match.  */
int specified_data_size;

/* Nonzero means print names of input files as processed.  */
int trace_files;

/* Which symbols should be stripped (omitted from the output):
   none, all, or debugger symbols.  */
enum { STRIP_NONE, STRIP_ALL, STRIP_DEBUGGER } strip_symbols;

/* Which local symbols should be omitted:
   none, all, or those starting with L.
   This is irrelevant if STRIP_NONE.  */
enum { DISCARD_NONE, DISCARD_ALL, DISCARD_L } discard_locals;

/* 1 => write load map.  */
int write_map;

/* 1 => assign space to common symbols even if OUTPUT_RELOCATABLE. */
int force_common_definition;

/* Standard directories to search for files specified by -l.  */
char *standard_search_dirs[] = { "/usr/lib" };

/* If set STANDARD_SEARCH_DIRS is not searched.  */
int no_standard_dirs;

/* Actual vector of directories to search;
   this contains those specified with -L plus the standard ones.  */
char **search_dirs;

/* Length of the vector `search_dirs'.  */
int n_search_dirs;

/* Non zero means to create the output executable.
   Cleared by nonfatal errors.  */
int make_executable;

/* Force the executable to be output, even if there are non-fatal
   errors */
int force_executable;

/* Whether or not to include .dll in the shared library searching. */
int opt_dll_search;

/* Keep a list of any symbols referenced from the command line (so
   that error messages for these guys can be generated). This list is
   zero terminated. */
struct glosym **cmdline_references;
int cl_refs_allocated;

void *xmalloc (size_t);
void *xrealloc (void *, size_t);
void usage (char *, char *);
void fatal (char *, ...);
void fatal_with_file (char *, struct file_entry *);
void perror_name (char *);
void perror_file (struct file_entry *);
void error (char *, char *, char *, char *);

int parse (char *, char *, char *);
void initialize_text_start (void);
void initialize_data_start (void);
void digest_symbols (void);
void print_symbols (FILE *);
void load_symbols (void);
void decode_command (int, char **);
void write_output (void);
void write_header (void);
void write_text (void);
void read_file_relocation (struct file_entry *);
void write_data (void);
void write_rel (void);
void write_syms (void);
void write_symsegs (void);
void mywrite (void *, int, int, int);
void symtab_init (void);
void padfile (int, int);
char *get_file_name (struct file_entry *);
symbol *getsym (char *), *getsym_soft (char *);
void do_warnings (FILE *);
void check_exe (void);
char *lx_to_aout (const char *pszFilename);
int check_lx_dll(int fd);
void cleanup (void);

void add_cmdline_ref (struct glosym *sp);
void prline_file_name (struct file_entry *entry, FILE *outfile);
void deduce_file_type(int desc, struct file_entry *entry);
void read_a_out_header (int desc, struct file_entry *entry);
void read_header (int desc, struct file_entry *entry);
void read_entry_symbols (int desc, struct file_entry *entry);
void read_entry_strings (int desc, struct file_entry *entry);
unsigned long contains_symbol (struct file_entry *entry, struct nlist *n_ptr);
void process_subentry (int desc, struct file_entry *subentry, struct file_entry *entry, struct file_entry **prev_addr);
void consider_file_section_lengths (struct file_entry *entry);
void relocate_file_addresses (struct file_entry *entry);
void describe_file_sections (struct file_entry *entry, FILE *outfile);
void list_file_locals (struct file_entry *entry, FILE *outfile);
int relocation_entries_relation (struct relocation_info *rel1, struct relocation_info *rel2);
void do_relocation_warnings (struct file_entry *entry, int data_segment, FILE *outfile, unsigned char *nlist_bitvector);
void do_file_warnings (struct file_entry *entry, FILE *outfile);
void initialize_a_out_text_start (void);
void initialize_a_out_data_start (void);
void compute_a_out_section_offsets (void);
void compute_more_a_out_section_offsets (void);
void write_a_out_header (void);
int hash_string (char *key);
void read_file_symbols (struct file_entry *entry);
void compute_section_offsets (void);
void compute_more_section_offsets (void);
void read_relocation (void);
int assign_string_table_index (char *name);
unsigned long check_each_file (register unsigned long (*function)(), register int arg);
static void     gen_deffile (void);




int
main (argc, argv)
     char **argv;
     int argc;
{
  _response (&argc, &argv);
  _wildcard (&argc, &argv);
  page_size = getpagesize ();
  progname = argv[0];

#ifdef RLIMIT_STACK
  /* Avoid dumping core on large .o files.  */
  {
    struct rlimit rl;

    getrlimit (RLIMIT_STACK, &rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit (RLIMIT_STACK, &rl);
  }
#endif

  /* Clear the cumulative info on the output file.  */

  text_size = 0;
  data_size = 0;
  bss_size = 0;
  text_reloc_size = 0;
  data_reloc_size = 0;

  set_sect_pad = 0;
  data_pad = 0;
  text_pad = 0;

  /* Initialize the data about options.  */

  specified_data_size = 0;
  strip_symbols = STRIP_NONE;
  trace_files = 0;
  discard_locals = DISCARD_NONE;
  entry_symbol = 0;
  write_map = 0;
  force_common_definition = 0;
  T_flag_specified = 0;
  Tdata_flag_specified = 0;
  make_executable = 1;
  force_executable = 0;
  set_element_prefixes = 0;

  /* Initialize the cumulative counts of symbols.  */

  local_sym_count = 0;
  non_L_local_sym_count = 0;
  debugger_sym_count = 0;
  undefined_global_sym_count = 0;
  set_symbol_count = 0;
  set_vector_count = 0;
  global_indirect_count = 0;
  warning_count = 0;
  multiple_def_count = 0;
  common_defined_global_count = 0;

  /* Keep a list of symbols referenced from the command line */

  cl_refs_allocated = 10;
  cmdline_references
    = (struct glosym **) xmalloc (cl_refs_allocated
				  * sizeof(struct glosym *));
  *cmdline_references = 0;

  /* Cleanup converted file on exit. */

  atexit (cleanup);

  /* Completely decode ARGV.  */

  decode_command (argc, argv);

  check_exe ();

  /* Load symbols of all input files.
     Also search all libraries and decide which library members to load.  */

  load_symbols ();

  /* Create various built-in symbols.  This must occur after
     all input files are loaded so that a user program can have a
     symbol named etext (for example).  */

  if (output_style != OUTPUT_RELOCATABLE)
    symtab_init ();

  /* Compute where each file's sections go, and relocate symbols.  */

  digest_symbols ();

  /* Print error messages for any missing symbols, for any warning
     symbols, and possibly multiple definitions */

  do_warnings (stderr);

  /* Print a map, if requested.  */

  if (write_map) print_symbols (stdout);

  /* Write the output file.  */

  if (make_executable || force_executable)
    write_output ();

  exit (!make_executable);
}

void add_cmdline_ref ();

static struct option longopts[] =
{
  {"d", 0, 0, 'd'},
  {"dc", 0, 0, 'd'},		/* For Sun compatibility. */
  {"dp", 0, 0, 'd'},		/* For Sun compatibility. */
  {"e", 1, 0, 'e'},
  {"n", 0, 0, 'n'},
  {"noinhibit-exec", 0, 0, 130},
  {"nostdlib", 0, 0, 133},
  {"o", 1, 0, 'o'},
  {"r", 0, 0, 'r'},
  {"s", 0, 0, 's'},
  {"t", 0, 0, 't'},
  {"u", 1, 0, 'u'},
  {"x", 0, 0, 'x'},
  {"z", 0, 0, 'z'},
  {"A", 1, 0, 'A'},
  {"D", 1, 0, 'D'},
  {"M", 0, 0, 'M'},
  {"N", 0, 0, 'N'},
  {"R", 0, 0, 'R'},             /* Create relocatable executable */
  {"Zexe", 0, 0, 135},          /* Create .exe file, touch `output file' */
  {"Zstack", 1, 0, 136},        /* Set stack size */
  {"Zmap", 2, 0, 137},          /* Create .map file */
  {"Zno-demangle", 0, 0, 138},  /* Don't demangle symbols */
  {"Zdemangle-proto", 0, 0, 139},  /* Demangle symbols complete */
  {"Zwin32", 0, 0, 140},        /* Create GUI, CUI Win32 */
  {"Zrsx32", 0, 0, 141},        /* Create Win32/DOS win32 base */
  {"Zemx32", 0, 0, 142},        /* Create Win32/DOS emx base */
  {"S", 0, 0, 'S'},
  {"T", 1, 0, 'T'},
  {"Ttext", 1, 0, 'T'},
  {"Tdata", 1, 0, 132},
  {"V", 1, 0, 'V'},
  {"X", 0, 0, 'X'},
#define OPT_LIBS_STATIC     0x1000
  {"Bstatic", 0, 0, OPT_LIBS_STATIC},
  {"non_shared", 0, 0, OPT_LIBS_STATIC},
  {"dn", 0, 0, OPT_LIBS_STATIC},
  {"static", 0, 0, OPT_LIBS_STATIC},
#define OPT_LIBS_SHARED     0x1001
  {"Bshared", 0, 0, OPT_LIBS_SHARED},
  {"call_shared", 0, 0, OPT_LIBS_SHARED},
  {"dy", 0, 0, OPT_LIBS_SHARED},
#define OPT_ZDLL_SEARCH     0x1008
  {"Zdll-search",0, 0, OPT_ZDLL_SEARCH},
  {0, 0, 0, 0}
};

/* Since the Unix ld accepts -lfoo, -Lfoo, and -yfoo, we must also.
   This effectively prevents any long options from starting with
   one of these letters. */
#define SHORTOPTS "-l:y:L:"

/* Process the command arguments,
   setting up file_table with an entry for each input file,
   and setting variables according to the options.  */

void
decode_command (argc, argv)
     char **argv;
     int argc;
{
  int optc, longind;
  register struct file_entry *p;
  int opt_libs_static = 0;

  number_of_files = 0;
  output_filename = "a.out";

  n_search_dirs = 0;
  search_dirs = (char **) xmalloc (sizeof (char *));

  /* First compute number_of_files so we know how long to make file_table.
     Also process most options completely.  */

  while ((optc = getopt_long_only (argc, argv, SHORTOPTS, longopts, &longind))
	 != EOF)
    {
      if (optc == 0)
	optc = longopts[longind].val;

      switch (optc)
	{
	case '?':
	  usage (0, 0);
	  break;

	case 1:
	  /* Non-option argument. */
	  number_of_files++;
	  break;

	case 'd':
	  force_common_definition = 1;
	  break;

	case 'e':
	  entry_symbol = getsym (optarg);
	  if (!entry_symbol->defined && !entry_symbol->referenced)
	    undefined_global_sym_count++;
	  entry_symbol->referenced = 1;
	  add_cmdline_ref (entry_symbol);
	  break;

	case 'l':
	  number_of_files++;
	  break;

	case 'n':
	  if (output_style && output_style != OUTPUT_READONLY_TEXT)
	    fatal ("illegal combination of -n with -N, -r, or -z", (char *) 0);
	  output_style = OUTPUT_READONLY_TEXT;
	  break;

	case 130:		/* -noinhibit-exec */
	  force_executable = 1;
	  break;

	case 133:		/* -nostdlib */
	  no_standard_dirs = 1;
	  break;

	case 'o':
	  output_filename = optarg;
	  break;

	case 'r':
	  if (output_style && output_style != OUTPUT_RELOCATABLE)
	    fatal ("illegal combination of -r with -N, -n, or -z", (char *) 0);
	  output_style = OUTPUT_RELOCATABLE;
	  text_start = 0;
	  break;

	case 's':
	  strip_symbols = STRIP_ALL;
	  break;

	case 't':
	  trace_files = 1;
	  break;

	case 'u':
	  {
	    register symbol *sp = getsym (optarg);

	    if (!sp->defined && !sp->referenced)
	      undefined_global_sym_count++;
	    sp->referenced = 1;
	    add_cmdline_ref (sp);
	  }
	  break;

	case 'x':
	  discard_locals = DISCARD_ALL;
	  break;

	case 'y':
	  {
	    register symbol *sp = getsym (optarg);

	    sp->trace = 1;
	  }
	  break;

	case 'z':
	  if (output_style && output_style != OUTPUT_DEMAND_PAGED)
	    fatal ("illegal combination of -z with -N, -n, or -r", (char *) 0);
	  output_style = OUTPUT_DEMAND_PAGED;
	  break;

	case 'A':
	  number_of_files++;
	  break;

	case 'D':
	  specified_data_size = parse (optarg, "%x", "invalid argument to -D");
	  break;

	case 'L':
	  n_search_dirs++;
	  search_dirs = (char **)
	    xrealloc (search_dirs, n_search_dirs * sizeof (char *));
	  search_dirs[n_search_dirs - 1] = optarg;
	  break;

	case 'M':
	  write_map = 1;
	  break;

	case 'N':
	  if (output_style && output_style != OUTPUT_WRITABLE_TEXT)
	    fatal ("illegal combination of -N with -n, -r, or -z", (char *) 0);
	  output_style = OUTPUT_WRITABLE_TEXT;
	  break;

        case 135:               /* -Zexe */
          exe_flag = 1;
          break;

        case 136:               /* -Zstack */
	  stack_size = parse (optarg, "%i", "invalid argument to -Zstack");
	  break;

        case 137:               /* -Zmap */
	  map_filename = optarg;
	  map_flag = 1;
	  break;

        case 138:               /* -Zno-demangle */
          demangler = 0;
          break;

        case 139:               /* -Zdemangle-proto */
          demangle_options = DMGL_PARAMS | DMGL_ANSI;
          break;

        case 140:               /* -Zwin32: GUI,CUI Win32 */
          rsxnt_linked = RSXNT_WIN32;
          break;

        case 141:               /* -Zrsx32: Win32/DOS win32 base */
          rsxnt_linked = RSXNT_RSX;
          break;

        case 142:               /* -Zemx32: Win32/DOS emx base */
          rsxnt_linked = RSXNT_EMX;
          break;

        case 'R':
	  reloc_flag = 1;
	  break;

	case 'S':
	  strip_symbols = STRIP_DEBUGGER;
	  break;

	case 'T':
	  text_start = parse (optarg, "%x", "invalid argument to -Ttext");
	  T_flag_specified = 1;
	  break;

	case 132:		/* -Tdata addr */
	  data_start = parse (optarg, "%x", "invalid argument to -Tdata");
	  Tdata_flag_specified = 1;
	  break;

	case 'V':
	  {
	    struct string_list_element *new
	      = (struct string_list_element *)
		xmalloc (sizeof (struct string_list_element));

	    new->str = optarg;
	    new->next = set_element_prefixes;
	    set_element_prefixes = new;
	  }
	  break;

	case 'X':
	  discard_locals = DISCARD_L;
	  break;

        case OPT_ZDLL_SEARCH:
          opt_dll_search = 1;
          break;

        case OPT_LIBS_STATIC:
        case OPT_LIBS_SHARED:
          /* processed later */
          break;
	}
    }

  if (!number_of_files)
    usage ("no input files", 0);

  p = file_table
    = (struct file_entry *) xmalloc (number_of_files * sizeof (struct file_entry));
  bzero (p, number_of_files * sizeof (struct file_entry));

  /* Now scan again and fill in file_table.
     All options except -A and -l are ignored here.  */

  optind = 0;			/* Reset getopt. */
  while ((optc = getopt_long_only (argc, argv, SHORTOPTS, longopts, &longind))
	 != EOF)
    {
      if (optc == 0)
	optc = longopts[longind].val;

      switch (optc)
	{
	case 1:
	  /* Non-option argument. */
	  {
	    char *ext = _getext (optarg);
	    if (ext != NULL && stricmp (ext, ".def") == 0
		&& def_filename == NULL)
	      {
		def_filename = optarg;
		--number_of_files;
		break;
	      }
            else if (ext != NULL && stricmp (ext, ".res") == 0
                     && res_filename == NULL)
	      {
		res_filename = optarg;
		--number_of_files;
		break;
	      }
            else if (ext != NULL && stricmp (ext, ".dll") == 0
                     && res_filename == NULL)
	      { /* convert .dll to temporary import library. */
                p->filename = lx_to_aout (optarg);
                p->local_sym_name = optarg;
                p++;
                break;
	      }
	  }
	  p->filename = optarg;
	  p->local_sym_name = optarg;
	  p++;
	  break;

	case 'A':
	  if (p != file_table)
	    usage ("-A specified before an input file other than the first", NULL);
	  p->filename = optarg;
	  p->local_sym_name = optarg;
	  p->just_syms_flag = 1;
	  p++;
	  break;

	case 'l':
	  p->filename = concat ("", optarg, "", NULL);
	  p->local_sym_name = concat ("-l", optarg, "", NULL);
	  p->search_dirs_flag = 1;
          p->dynamic = !opt_libs_static;
	  p++;
	  break;

        case OPT_LIBS_STATIC:
          opt_libs_static = 1;
          break;
        case OPT_LIBS_SHARED:
          opt_libs_static = 0;
          break;
	}
    }

  if (!output_file_type)
    output_file_type = DEFAULT_OUTPUT_FILE_TYPE;

  if (!output_style)
    output_style = DEFAULT_OUTPUT_STYLE;

#if 0
  /* THIS CONSISTENCY CHECK BELONGS SOMEWHERE ELSE.  */
  /* Now check some option settings for consistency.  */

  if ((output_style == OUTPUT_READONLY_TEXT || output_style == OUTPUT_DEMAND_PAGED)
      && (text_start - text_start_alignment) & (page_size - 1))
    usage ("-T argument not multiple of page size, with sharable output", 0);
#endif

  /* Append the standard search directories to the user-specified ones.  */
  if (!no_standard_dirs)
    {
      int n = sizeof standard_search_dirs / sizeof standard_search_dirs[0];
      n_search_dirs += n;
      search_dirs
	= (char **) xrealloc (search_dirs, n_search_dirs * sizeof (char *));
      bcopy (standard_search_dirs, &search_dirs[n_search_dirs - n],
	     n * sizeof (char *));
    }
}


void
add_cmdline_ref (sp)
     struct glosym *sp;
{
  struct glosym **ptr;

  for (ptr = cmdline_references;
       ptr < cmdline_references + cl_refs_allocated && *ptr;
       ptr++)
    ;

  if (ptr >= cmdline_references + cl_refs_allocated - 1)
    {
      int diff = ptr - cmdline_references;

      cl_refs_allocated *= 2;
      cmdline_references = (struct glosym **)
	xrealloc (cmdline_references,
		 cl_refs_allocated * sizeof (struct glosym *));
      ptr = cmdline_references + diff;
    }

  *ptr++ = sp;
  *ptr = (struct glosym *) 0;
}

static int
set_element_prefixed_p (name)
     char *name;
{
  struct string_list_element *p;
  int i;

  for (p = set_element_prefixes; p; p = p->next)
    {
      for (i = 0; p->str[i] != '\0' && (p->str[i] == name[i]); i++)
	;

      if (p->str[i] == '\0')
	return 1;
    }
  return 0;
}

/* Convenient functions for operating on one or all files being
   loaded.  */
void print_file_name (struct file_entry *entry, FILE *outfile);

/* Call FUNCTION on each input file entry.
   Do not call for entries for libraries;
   instead, call once for each library member that is being loaded.

   FUNCTION receives two arguments: the entry, and ARG.  */

static void
each_file (function, arg)
     register void (*function)();
     register int arg;
{
  register int i;

  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    (*function) (subentry, arg);
	}
      else
	(*function) (entry, arg);
    }
}

/* Call FUNCTION on each input file entry until it returns a non-zero
   value.  Return this value.
   Do not call for entries for libraries;
   instead, call once for each library member that is being loaded.

   FUNCTION receives two arguments: the entry, and ARG.  It must be a
   function returning unsigned long (though this can probably be fudged). */

unsigned long
check_each_file (function, arg)
     register unsigned long (*function)();
     register int arg;
{
  register int i;
  register unsigned long return_val;

  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    if ((return_val = (*function) (subentry, arg)))
	      return return_val;
	}
      else
	if ((return_val = (*function) (entry, arg)))
	  return return_val;
    }
  return 0;
}

/* Like `each_file' but ignore files that were just for symbol definitions.  */

static void
each_full_file (function, arg)
     register void (*function)();
     register int arg;
{
  register int i;

  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->just_syms_flag)
	continue;
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    (*function) (subentry, arg);
	}
      else
	(*function) (entry, arg);
    }
}

/* Close the input file that is now open.  */

static void
file_close ()
{
  close (input_desc);
  input_desc = 0;
  input_file = 0;
}

/* Open the input file specified by 'entry', and return a descriptor.
   The open file is remembered; if the same file is opened twice in a row,
   a new open is not actually done.  */

static int
file_open (entry)
     register struct file_entry *entry;
{
  register int desc = -1;

  if (entry->superfile)
    return file_open (entry->superfile);

  if (entry == input_file)
    return input_desc;

  if (input_file) file_close ();

  if (entry->search_dirs_flag && n_search_dirs)
    {
      /* !we're searching for libraries here! */
      static const char *dynamic_dll_suffs[] = { "_dll.a", ".a", ".dll", "_s.a", NULL };
      static const char *dynamic_suffs[]     = { "_dll.a", ".a", "_s.a", NULL };
      static const char *static_suffs[]      = { "_s.a", ".a", NULL };
      const char **suffs = entry->dynamic ? opt_dll_search ? dynamic_dll_suffs : dynamic_suffs  : static_suffs;
      int lenname = strlen (entry->filename);
      int i;

      for (i = 0; i < n_search_dirs; i++)
	{
          int lendir = strlen (search_dirs[i]);
	  register char *string = (char *) xmalloc (lendir + lenname + 4 + 6 + 1);
          int j;

          memcpy (string, search_dirs[i], lendir);
          string[lendir++] = '/';
          for (j = 0; suffs[j]; j++)
            {
              static const char *prefixes[] = { "lib", ""};
              int k;
              for (k = 0; k < sizeof(prefixes) / sizeof(prefixes[0]); k++)
                {
                  int len = strlen (prefixes[k]);
                  memcpy (string + lendir, prefixes[k], len);
                  len += lendir;
                  memcpy (string + len, entry->filename, lenname);
                  len += lenname;
                  strcpy (string + len, suffs[j]);

                  desc = open (string, O_RDONLY|O_BINARY, 0);
                  if (desc > 0)
                    {
                      /* convert? */
                      if (check_lx_dll (desc))
                        {
                          string = lx_to_aout (string);
                          if (!string || (desc = open (string, O_RDONLY|O_BINARY, 0)) < 0)
                            perror_file (entry);
                        }
                      entry->filename = string;
                      entry->search_dirs_flag = 0;
                      input_file = entry;
                      input_desc = desc;
                      return desc;
                    }
                } /* prefix loop */
            } /* suffix loop */
	  free (string);
	} /* dir loop */
    }
  else
    desc = open (entry->filename, O_RDONLY|O_BINARY, 0);

  if (desc > 0)
    {
      input_file = entry;
      input_desc = desc;
      return desc;
    }

  perror_file (entry);
  /* NOTREACHED */
  return -1;
}

/* Print the filename of ENTRY on OUTFILE (a stdio stream),
   and then a newline.  */

void
prline_file_name (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  print_file_name (entry, outfile);
  fprintf (outfile, "\n");
}

/* Print the filename of ENTRY on OUTFILE (a stdio stream).  */

void
print_file_name (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  if (entry->superfile)
    {
      print_file_name (entry->superfile, outfile);
      fprintf (outfile, "(%s)", entry->filename);
    }
  else
    fprintf (outfile, "%s", entry->filename);
}

/* Return the filename of entry as a string (malloc'd for the purpose) */

char *
get_file_name (entry)
     struct file_entry *entry;
{
  char *result, *supfile;
  if (entry->superfile)
    {
      supfile = get_file_name (entry->superfile);
      result = (char *) xmalloc (strlen (supfile)
				 + strlen (entry->filename) + 3);
      sprintf (result, "%s(%s)", supfile, entry->filename);
      free (supfile);
    }
  else
    {
      result = (char *) xmalloc (strlen (entry->filename) + 1);
      strcpy (result, entry->filename);
    }
  return result;
}

/* Medium-level input routines for rel files.  */

/* Determine whether the given ENTRY is an archive, a BSD a.out file,
   a Mach-O file, or whatever.  DESC is the descriptor on which the
   file is open.  */
void
deduce_file_type(desc, entry)
     int desc;
     struct file_entry *entry;
{
  int len;

  {
    char magic[SARMAG];

    lseek (desc, entry->starting_offset, 0);
    len = read (desc, magic, SARMAG);
    if (len == SARMAG && !strncmp(magic, ARMAG, SARMAG))
      {
	entry->file_type = IS_ARCHIVE;
	return;
      }
  }

#ifdef A_OUT
  {
    struct exec hdr;

    lseek (desc, entry->starting_offset, 0);
    len = read (desc, (char *) &hdr, sizeof (struct exec));
    if (len == sizeof (struct exec) && !N_BADMAG (hdr))
      {
	entry->file_type = IS_A_OUT;
	return;
      }
  }
#endif

  fatal_with_file ("malformed input file (not rel or archive) ", entry);
}

#ifdef A_OUT
/* Read an a.out file's header and set up the fields of
   the ENTRY accordingly.  DESC is the descriptor on which
   the file is open.  */
void
read_a_out_header (desc, entry)
     int desc;
     struct file_entry *entry;
{
  struct exec hdr;
  struct stat st;

  lseek (desc, entry->starting_offset, 0);
  read (desc, (char *) &hdr, sizeof (struct exec));

#ifdef READ_HEADER_HOOK
  READ_HEADER_HOOK(hdr.a_machtype);
#endif

  if (entry->just_syms_flag)
    entry->orig_text_address = N_TXTADDR(hdr);
  else
    entry->orig_text_address = 0;
  entry->text_size = hdr.a_text;
  entry->text_offset = N_TXTOFF(hdr);

  entry->text_reloc_size = hdr.a_trsize;
#ifdef N_TRELOFF
  entry->text_reloc_offset = N_TRELOFF(hdr);
#else
#ifdef N_DATOFF
  entry->text_reloc_offset = N_DATOFF(hdr) + hdr.a_data;
#else
  entry->text_reloc_offset = N_TXTOFF(hdr) + hdr.a_text + hdr.a_data;
#endif
#endif

  if (entry->just_syms_flag)
    entry->orig_data_address = N_DATADDR(hdr);
  else
    entry->orig_data_address = entry->text_size;
  entry->data_size = hdr.a_data;
#ifdef N_DATOFF
  entry->data_offset = N_DATOFF(hdr);
#else
  entry->data_offset = N_TXTOFF(hdr) + hdr.a_text;
#endif

  entry->data_reloc_size = hdr.a_drsize;
#ifdef N_DRELOFF
  entry->data_reloc_offset = N_DRELOFF(hdr);
#else
  entry->data_reloc_offset = entry->text_reloc_offset + entry->text_reloc_size;
#endif

#ifdef N_BSSADDR
  if (entry->just_syms_flag)
    entry->orig_bss_address = N_BSSADDR(hdr);
  else
#endif
  entry->orig_bss_address = entry->orig_data_address + entry->data_size;
  entry->bss_size = hdr.a_bss;

  entry->syms_size = hdr.a_syms;
  entry->syms_offset = N_SYMOFF(hdr);
  entry->strs_offset = N_STROFF(hdr);
  lseek(desc, entry->starting_offset + entry->strs_offset, 0);
  if (entry->syms_size &&
      read(desc, (char *) &entry->strs_size, sizeof (unsigned long int))
      != sizeof (unsigned long int))
    fatal_with_file ("failure reading string table size of ", entry);

  if (!entry->superfile)
    {
      fstat(desc, &st);
      if (st.st_size > entry->strs_offset + entry->strs_size)
	{
	  entry->symseg_size = st.st_size - (entry->strs_offset + entry->strs_size);
	  entry->symseg_offset = entry->strs_offset + entry->strs_size;
	}
    }
  else
    if (entry->total_size > entry->strs_offset + entry->strs_size)
      {
	entry->symseg_size = entry->total_size - (entry->strs_offset + entry->strs_size);
	entry->symseg_offset = entry->strs_offset + entry->strs_size;
      }
}
#endif

/* Read a file's header info into the proper place in the file_entry.
   DESC is the descriptor on which the file is open.
   ENTRY is the file's entry.
   Switch in the file_type to determine the appropriate actual
   header reading routine to call.  */

void
read_header (desc, entry)
     int desc;
     register struct file_entry *entry;
{
  if (!entry->file_type)
    deduce_file_type (desc, entry);

  switch (entry->file_type)
    {
    case IS_ARCHIVE:
    default:
      /* Should never happen. */
      abort ();

#ifdef A_OUT
    case IS_A_OUT:
      read_a_out_header (desc, entry);
      break;
#endif
    }

  entry->header_read_flag = 1;
}

/* Read the symbols of file ENTRY into core.
   Assume it is already open, on descriptor DESC.  */

void
read_entry_symbols (desc, entry)
     struct file_entry *entry;
     int desc;
{
  if (!entry->header_read_flag)
    read_header (desc, entry);

  entry->symbols = (struct nlist *) xmalloc (entry->syms_size);

  lseek (desc, entry->syms_offset + entry->starting_offset, 0);
  if (entry->syms_size != read (desc, entry->symbols, entry->syms_size))
    fatal_with_file ("premature end of file in symbols of ", entry);
}

/* Read the string table of file ENTRY into core.
   Assume it is already open, on descriptor DESC.  */

void
read_entry_strings (desc, entry)
     int desc;
     struct file_entry *entry;
{
  if (!entry->header_read_flag)
    read_header (desc, entry);

  lseek (desc, entry->strs_offset + entry->starting_offset, 0);
  if (entry->strs_size != read (desc, entry->strings, entry->strs_size))
    fatal_with_file ("premature end of file in strings of ", entry);
}

/* Read in the symbols of all input files.  */

void enter_file_symbols (struct file_entry *entry);
void enter_global_ref (register struct nlist *nlist_p, char *name, struct file_entry *entry);
void search_library (int desc, struct file_entry *entry);

void
load_symbols (void)
{
  register int i;

  if (trace_files) fprintf (stderr, "Loading symbols:\n\n");

  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      read_file_symbols (entry);
    }

  if (trace_files) fprintf (stderr, "\n");
}

/* If ENTRY is a rel file, read its symbol and string sections into core.
   If it is a library, search it and load the appropriate members
   (which means calling this function recursively on those members).  */

void
read_file_symbols (entry)
     register struct file_entry *entry;
{
  register int desc;

  desc = file_open (entry);

  if (!entry->file_type)
    deduce_file_type (desc, entry);

  if (entry->file_type == IS_ARCHIVE)
    {
      entry->library_flag = 1;
      search_library (desc, entry);
    }
  else
    {
      read_entry_symbols (desc, entry);
      entry->strings = (char *) ALLOCA (entry->strs_size);
      read_entry_strings (desc, entry);
      enter_file_symbols (entry);
      FREEA (entry->strings);
    }

  file_close ();
}

/* Enter the external symbol defs and refs of ENTRY in the hash table.  */

void
enter_file_symbols (entry)
     struct file_entry *entry;
{
  register struct nlist
    *p,
    *end = entry->symbols + entry->syms_size / sizeof (struct nlist);

  if (trace_files) prline_file_name (entry, stderr);

  for (p = entry->symbols; p < end; p++)
    {
      if (p->n_type == (N_SETV | N_EXT)) continue;
      if (p->n_type == (N_IMP1 | N_EXT))
        reloc_flag = 1;
      if (set_element_prefixes
	  && set_element_prefixed_p (p->n_un.n_strx + entry->strings))
	p->n_type += (N_SETA - N_ABS);

      if (SET_ELEMENT_P (p->n_type))
	{
	  set_symbol_count++;
	  if (output_style != OUTPUT_RELOCATABLE)
	    enter_global_ref (p, p->n_un.n_strx + entry->strings, entry);
	}
      else if (p->n_type == N_WARNING)
	{
	  char *name = p->n_un.n_strx + entry->strings;

	  /* Grab the next entry.  */
	  p++;
	  if (p->n_type != (N_UNDF | N_EXT))
	    {
	      fprintf (stderr, "%s: Warning symbol found in %s without external reference following.\n",
		       progname, entry->filename);
	      make_executable = 0;
	      p--;		/* Process normally.  */
	    }
	  else
	    {
	      symbol *sp;
	      char *sname = p->n_un.n_strx + entry->strings;
	      /* Deal with the warning symbol.  */
	      enter_global_ref (p, p->n_un.n_strx + entry->strings, entry);
	      sp = getsym (sname);
	      sp->warning = (char *) xmalloc (strlen(name) + 1);
	      strcpy (sp->warning, name);
	      warning_count++;
	    }
	}
      else if (WEAK_SYMBOL (p->n_type))
	{
	  /* Enter the symbol into the symbol hash table only if it
	     has not already been defined */
	  symbol *s = getsym_soft (p->n_un.n_strx + entry->strings);
	  if (!s || !s->defined)
	    enter_global_ref (p, p->n_un.n_strx + entry->strings, entry);
          else if (s) /* hack! */
              p->n_un.n_name = (char*)s;
#ifdef DEBUG
          else fprintf(stderr, "dbg-warning: %s - sym %d: '%s' no such symbol...\n",
                       entry->filename,
                       p - entry->symbols,
                       p->n_un.n_strx + entry->strings);
#endif
	}
      else if (p->n_type & N_EXT)
	enter_global_ref (p, p->n_un.n_strx + entry->strings, entry);
      else if (p->n_un.n_strx && !(p->n_type & (N_STAB | N_EXT)))
	{
	  if ((p->n_un.n_strx + entry->strings)[0] != LPREFIX)
	    non_L_local_sym_count++;
	  local_sym_count++;
	}
      else debugger_sym_count++;
#ifdef DEBUG_BIRD
      fprintf(stderr, "dbg: %s sym #%3d: un=%08lx typ=%02x\n",
              entry->filename,
              p - entry->symbols,
              p->n_un.n_strx,
              p->n_type);
#endif
    }

   /* Count one for the local symbol that we generate,
      whose name is the file's name (usually) and whose address
      is the start of the file's text.  */

  local_sym_count++;
  non_L_local_sym_count++;
}

/* Enter one global symbol in the hash table.
   NLIST_P points to the `struct nlist' read from the file
   that describes the global symbol.  NAME is the symbol's name.
   ENTRY is the file entry for the file the symbol comes from.

   The `struct nlist' is modified by placing it on a chain of
   all such structs that refer to the same global symbol.
   This chain starts in the `refs' field of the symbol table entry
   and is chained through the `n_name'.  */

void
enter_global_ref (nlist_p, name, entry)
     register struct nlist *nlist_p;
     char *name;
     struct file_entry *entry;
{
  register symbol *sp = getsym (name);
  register int type = nlist_p->n_type;
  const int realtype = type;
  int oldref = sp->referenced;
  int olddef = sp->defined;

  nlist_p->n_un.n_name = (char *) sp->refs;
  sp->refs = nlist_p;

  sp->referenced = 1;

  if (WEAK_SYMBOL (type))
    {
      sp->weak = type;
      /* Switch symbol type so that it can be processed like regular symbols */
      type = nlist_p->n_type =
	(type == N_WEAKU) ? N_UNDF | N_EXT :
	(type == N_WEAKA) ? N_ABS | N_EXT :
	(type == N_WEAKT) ? N_TEXT | N_EXT :
	(type == N_WEAKD) ? N_DATA | N_EXT :
	/*(type == N_WEAKB)*/ N_BSS | N_EXT;
    }

  if (type != (N_UNDF | N_EXT) || nlist_p->n_value)
    {
      if (!sp->defined || sp->defined == (N_UNDF | N_EXT))
	sp->defined = type;

      if (oldref && !olddef)
	/* It used to be undefined and we're defining it.  */
	undefined_global_sym_count--;

      if (!olddef && type == (N_UNDF | N_EXT) && nlist_p->n_value)
	{
	  /* First definition and it's common.  */
	  common_defined_global_count++;
	  sp->max_common_size = nlist_p->n_value;
	}
      else if (olddef && sp->max_common_size && type != (N_UNDF | N_EXT))
	{
	  /* It used to be common and we're defining it as
	     something else.  */
	  common_defined_global_count--;
	  sp->max_common_size = 0;

          fprintf (stderr, "%s: symbol `%s' defined more than once in ",
            progname, name);
          print_file_name (entry, stderr);
          fprintf (stderr, "\n");
          exit (1);
	}
      else if (olddef && sp->max_common_size && type == (N_UNDF | N_EXT)
	  && sp->max_common_size < nlist_p->n_value)
	/* It used to be common and this is a new common entry to
	   which we need to pay attention.  */
	sp->max_common_size = nlist_p->n_value;

      /* Are we defining it as a set element?  */
      if (SET_ELEMENT_P (type)
	  && (!olddef || (olddef && sp->max_common_size)))
	set_vector_count++;
      /* As an indirection?  */
      else if (type == (N_INDR | N_EXT))
	{
	  /* Indirect symbols value should be modified to point
	     a symbol being equivalenced to. */
	  nlist_p->n_value
	    = (unsigned int) getsym ((nlist_p + 1)->n_un.n_strx
				     + entry->strings);
	  if ((symbol *) nlist_p->n_value == sp)
	    {
	      /* Somebody redefined a symbol to be itself.  */
	      fprintf (stderr, "%s: Symbol %s indirected to itself.\n",
		       entry->filename, name);
	      /* Rewrite this symbol as being a global text symbol
		 with value 0.  */
	      nlist_p->n_type = sp->defined = N_TEXT | N_EXT;
	      nlist_p->n_value = 0;
	      /* Don't make the output executable.  */
	      make_executable = 0;
	    }
	  else
	    global_indirect_count++;
	}
    }
  else
    if (!oldref)
      undefined_global_sym_count++;

  if (sp == end_symbol && entry->just_syms_flag && !T_flag_specified)
    text_start = nlist_p->n_value;

#ifdef DEBUG_BIRD
  sp->trace = 1;
#endif
  if (sp->trace)
    {
      register char *reftype;
      int free_reftype = 0;
      switch (realtype & ~N_EXT)
	{
	case N_UNDF:
	  if (nlist_p->n_value)
	    reftype = "defined as common";
	  else reftype = "referenced";
	  break;

	case N_ABS:
	  reftype = "defined as absolute";
	  break;

	case N_TEXT:
	  reftype = "defined in text section";
	  break;

	case N_DATA:
	  reftype = "defined in data section";
	  break;

	case N_BSS:
	  reftype = "defined in BSS section";
	  break;

	case N_SETT:
	  reftype = "is a text set element";
	  break;

	case N_SETD:
	  reftype = "is a data set element";
	  break;

	case N_SETB:
	  reftype = "is a BSS set element";
	  break;

	case N_SETA:
	  reftype = "is an absolute set element";
	  break;

	case N_SETV:
	  reftype = "defined in data section as vector";
	  break;

	case N_INDR:
	  reftype = (char *) ALLOCA (23 + strlen (((symbol *) nlist_p->n_value)->name));
	  sprintf (reftype, "defined equivalent to %s",
		   ((symbol *) nlist_p->n_value)->name);
          free_reftype = 1;
	  break;

        case N_IMP1:
          reftype = "imported";
          break;

	case N_WEAKU & ~N_EXT:
	  reftype = "weak";
	  break;

	case N_WEAKT & ~N_EXT:
	  reftype = "weak text";
	  break;

	case N_WEAKD & ~N_EXT:
	  reftype = "weak data";
	  break;

	default:
	  reftype = "I don't know this type";
	  break;
	}


      fprintf (stderr, "symbol in ");
      print_file_name (entry, stderr);
      fprintf (stderr, ": %3d %s %s\n",
               nlist_p - entry->symbols, sp->name, reftype);
      if (free_reftype)
        FREEA (reftype);
    }
}

/* This return 0 if the given file entry's symbol table does *not*
   contain the nlist point entry, and it returns the files entry
   pointer (cast to unsigned long) if it does. */

unsigned long
contains_symbol (entry, n_ptr)
     struct file_entry *entry;
     register struct nlist *n_ptr;
{
  if (n_ptr >= entry->symbols &&
      n_ptr < (entry->symbols
	       + (entry->syms_size / sizeof (struct nlist))))
    return (unsigned long) entry;
  return 0;
}


/* Searching libraries */

struct file_entry * decode_library_subfile (int desc, struct file_entry *library_entry, int subfile_offset, int *length_loc);
void symdef_library (int desc, struct file_entry *entry, int member_length);
void linear_library (int desc, struct file_entry *entry);

/* Search the library ENTRY, already open on descriptor DESC.
   This means deciding which library members to load,
   making a chain of `struct file_entry' for those members,
   and entering their global symbols in the hash table.  */

void
search_library (desc, entry)
     int desc;
     struct file_entry *entry;
{
  int member_length;
  register char *name;
  register struct file_entry *subentry;

  if (!undefined_global_sym_count) return;

  /* Examine its first member, which starts SARMAG bytes in.  */
  subentry = decode_library_subfile (desc, entry, SARMAG, &member_length);
  if (!subentry) return;

  name = subentry->filename;
  free (subentry);

  /* Search via __.SYMDEF if that exists, else linearly.  */

  if (!strcmp (name, "__.SYMDEF"))
    symdef_library (desc, entry, member_length);
  else
    linear_library (desc, entry);
}

/* Construct and return a file_entry for a library member.
   The library's file_entry is library_entry, and the library is open on DESC.
   SUBFILE_OFFSET is the byte index in the library of this member's header.
   We store the length of the member into *LENGTH_LOC.  */

struct file_entry *
decode_library_subfile (desc, library_entry, subfile_offset, length_loc)
     int desc;
     struct file_entry *library_entry;
     int subfile_offset;
     int *length_loc;
{
  int bytes_read;
  register int namelen;
  int member_length;
  register char *name;
  struct ar_hdr hdr1;
  register struct file_entry *subentry;

  lseek (desc, subfile_offset, 0);

  bytes_read = read (desc, &hdr1, sizeof hdr1);
  if (!bytes_read)
    return 0;		/* end of archive */

  if (sizeof hdr1 != bytes_read)
    fatal_with_file ("malformed library archive ", library_entry);

  if (sscanf (hdr1.ar_size, "%d", &member_length) != 1)
    fatal_with_file ("malformatted header of archive member in ", library_entry);

  subentry = (struct file_entry *) xmalloc (sizeof (struct file_entry));
  bzero (subentry, sizeof (struct file_entry));

  for (namelen = 0;
       namelen < sizeof hdr1.ar_name
       && hdr1.ar_name[namelen] != 0 && hdr1.ar_name[namelen] != ' '
       && hdr1.ar_name[namelen] != '/';
       namelen++);

  name = (char *) xmalloc (namelen+1);
  strncpy (name, hdr1.ar_name, namelen);
  name[namelen] = 0;

  subentry->filename = name;
  subentry->local_sym_name = name;
  subentry->symbols = 0;
  subentry->strings = 0;
  subentry->subfiles = 0;
  subentry->starting_offset = subfile_offset + sizeof hdr1;
  subentry->superfile = library_entry;
  subentry->library_flag = 0;
  subentry->header_read_flag = 0;
  subentry->just_syms_flag = 0;
  subentry->chain = 0;
  subentry->total_size = member_length;

  (*length_loc) = member_length;

  return subentry;
}

int subfile_wanted_p (struct file_entry *);

/* Search a library that has a __.SYMDEF member.
   DESC is a descriptor on which the library is open.
     The file pointer is assumed to point at the __.SYMDEF data.
   ENTRY is the library's file_entry.
   MEMBER_LENGTH is the length of the __.SYMDEF data.  */

void
symdef_library (desc, entry, member_length)
     int desc;
     struct file_entry *entry;
     int member_length;
{
  int *symdef_data = (int *) xmalloc (member_length);
  register struct symdef *symdef_base;
  char *sym_name_base;
  int number_of_symdefs;
  int length_of_strings;
  int not_finished;
  int bytes_read;
  register int i;
  struct file_entry *prev = 0;
  int prev_offset = 0;

  bytes_read = read (desc, symdef_data, member_length);
  if (bytes_read != member_length)
    fatal_with_file ("malformatted __.SYMDEF in ", entry);

  number_of_symdefs = *symdef_data / sizeof (struct symdef);
  if (number_of_symdefs < 0 ||
       number_of_symdefs * sizeof (struct symdef) + 2 * sizeof (int) > member_length)
    fatal_with_file ("malformatted __.SYMDEF in ", entry);

  symdef_base = (struct symdef *) (symdef_data + 1);
  length_of_strings = *(int *) (symdef_base + number_of_symdefs);

  if (length_of_strings < 0
      || number_of_symdefs * sizeof (struct symdef) + length_of_strings
	  + 2 * sizeof (int) != member_length)
    fatal_with_file ("malformatted __.SYMDEF in ", entry);

  sym_name_base = sizeof (int) + (char *) (symdef_base + number_of_symdefs);

  /* Check all the string indexes for validity.  */

  for (i = 0; i < number_of_symdefs; i++)
    {
      register int index = symdef_base[i].symbol_name_string_index;
      if (index < 0 || index >= length_of_strings
	  || (index && *(sym_name_base + index - 1)))
	fatal_with_file ("malformatted __.SYMDEF in ", entry);
    }

  /* Search the symdef data for members to load.
     Do this until one whole pass finds nothing to load.  */

  not_finished = 1;
  while (not_finished)
    {
      not_finished = 0;

      /* Scan all the symbols mentioned in the symdef for ones that we need.
	 Load the library members that contain such symbols.  */

      for (i = 0;
	   (i < number_of_symdefs
	    && (undefined_global_sym_count || common_defined_global_count));
	   i++)
	if (symdef_base[i].symbol_name_string_index >= 0)
	  {
	    register symbol *sp;

	    sp = getsym_soft (sym_name_base
			      + symdef_base[i].symbol_name_string_index);

	    /* If we find a symbol that appears to be needed, think carefully
	       about the archive member that the symbol is in.  */

	    if (sp && ((sp->referenced && !sp->defined)
		       || (sp->defined && sp->max_common_size))
               )
	      {
		int junk;
		register int j;
		register int offset = symdef_base[i].library_member_offset;
		struct file_entry *subentry;

		/* Don't think carefully about any archive member
		   more than once in a given pass.  */

		if (prev_offset == offset)
		  continue;
		prev_offset = offset;

		/* Read the symbol table of the archive member.  */

		subentry = decode_library_subfile (desc, entry, offset, &junk);
		if (subentry == 0)
		  fatal ("invalid offset for %s in symbol table of %s",
			 sym_name_base
			 + symdef_base[i].symbol_name_string_index,
			 entry->filename);
		read_entry_symbols (desc, subentry);
		subentry->strings = (char *) xmalloc (subentry->strs_size);
		read_entry_strings (desc, subentry);

		/* Now scan the symbol table and decide whether to load.  */

		if (!subfile_wanted_p (subentry))
		  {
		    free (subentry->symbols);
		    free (subentry->strings);
		    free (subentry);
		  }
		else
		  {
		    /* This member is needed; load it.
		       Since we are loading something on this pass,
		       we must make another pass through the symdef data.  */

		    not_finished = 1;

		    enter_file_symbols (subentry);

		    if (prev)
		      prev->chain = subentry;
		    else entry->subfiles = subentry;
		    prev = subentry;

		    /* Clear out this member's symbols from the symdef data
		       so that following passes won't waste time on them.  */

		    for (j = 0; j < number_of_symdefs; j++)
		      {
			if (symdef_base[j].library_member_offset == offset)
			  symdef_base[j].symbol_name_string_index = -1;
		      }

		    /* We'll read the strings again if we need them again.  */
		    free (subentry->strings);
		    subentry->strings = 0;
		  }
	      }
	  }
    }

  free (symdef_data);
}


/* Handle a subentry for a file with no __.SYMDEF. */

void
process_subentry (desc, subentry, entry, prev_addr)
     int desc;
     register struct file_entry *subentry;
     struct file_entry **prev_addr, *entry;
{
  read_entry_symbols (desc, subentry);
  subentry->strings = (char *) ALLOCA (subentry->strs_size);
  read_entry_strings (desc, subentry);

  if (!subfile_wanted_p (subentry))
    {
      FREEA (subentry->strings);
      free (subentry->symbols);
      free (subentry);
    }
  else
    {
      enter_file_symbols (subentry);

      if (*prev_addr)
	(*prev_addr)->chain = subentry;
      else
	entry->subfiles = subentry;
      *prev_addr = subentry;
      FREEA (subentry->strings);
    }
}

/* Search a library that has no __.SYMDEF.
   ENTRY is the library's file_entry.
   DESC is the descriptor it is open on.  */

void
linear_library (desc, entry)
     int desc;
     struct file_entry *entry;
{
  struct file_entry *prev = 0;
  register int this_subfile_offset = SARMAG;

  while (undefined_global_sym_count || common_defined_global_count)
    {
      int member_length;
      register struct file_entry *subentry;

      subentry = decode_library_subfile (desc, entry, this_subfile_offset,
					 &member_length);
      if (!subentry) return;

      process_subentry (desc, subentry, entry, &prev);
      this_subfile_offset += member_length + sizeof (struct ar_hdr);
      if (this_subfile_offset & 1) this_subfile_offset++;
    }
}

/* ENTRY is an entry for a library member.
   Its symbols have been read into core, but not entered.
   Return nonzero if we ought to load this member.  */

int
subfile_wanted_p (entry)
     struct file_entry *entry;
{
  register struct nlist *p;
  register struct nlist *end
    = entry->symbols + entry->syms_size / sizeof (struct nlist);

  for (p = entry->symbols; p < end; p++)
    {
      register int type = p->n_type;
      register char *name = p->n_un.n_strx + entry->strings;

      /* If the symbol has an interesting definition, we could
	 potentially want it.  */
      if (((type & N_EXT) || WEAK_SYMBOL (type))
	  && (type != (N_UNDF | N_EXT) || p->n_value)
	  && (type != (N_WEAKU | N_EXT) || p->n_value)
	  && !SET_ELEMENT_P (type)
	  && !set_element_prefixed_p (name))
	{
	  register symbol *sp = getsym_soft (name);

	  /* If this symbol has not been hashed, we can't be looking for it. */

	  if (!sp) continue;

	  if ((sp->referenced && !sp->defined)
	      /* NB.  This needs to be changed so that, e.g., "int pipe;" won't import
		 pipe() from the library.  But the bug fix kingdon made was wrong.  */
	      || (sp->defined && sp->max_common_size
		  && type != (N_INDR | N_EXT)))
	    {
	      /* This is a symbol we are looking for.  It is either
	         not yet defined or defined as a common.  */
	      if (type == (N_UNDF | N_EXT))
		{
		  /* Symbol being defined as common.
		     Remember this, but don't load subfile just for this.  */

		  /* If it didn't used to be common, up the count of
		     common symbols.  */
		  if (!sp->max_common_size)
		    common_defined_global_count++;

		  if (sp->max_common_size < p->n_value)
		    sp->max_common_size = p->n_value;
		  if (!sp->defined)
		    undefined_global_sym_count--;
		  sp->defined = 1;
		  continue;
		}

	      if (write_map)
		{
		  print_file_name (entry, stdout);
		  fprintf (stdout, " needed due to %s\n", sp->name);
		}
	      return 1;
	    }
	}
    }

  return 0;
}

void consider_file_section_lengths (), relocate_file_addresses ();

/* Having entered all the global symbols and found the sizes of sections
   of all files to be linked, make all appropriate deductions from this data.

   We propagate global symbol values from definitions to references.
   We compute the layout of the output file and where each input file's
   contents fit into it.  */

void
digest_symbols (void)
{
  register int i;
  int setv_fill_count = 0;

  if (trace_files)
    fprintf (stderr, "Digesting symbol information:\n\n");

  /* Initialize the text_start address; this depends on the output file formats.  */

  initialize_text_start ();

  text_size = text_header_size;

  /* Compute total size of sections */

  each_file (consider_file_section_lengths, 0);

  /* If necessary, pad text section to full page in the file.
     Include the padding in the text segment size.  */

  if (output_style == OUTPUT_READONLY_TEXT || output_style == OUTPUT_DEMAND_PAGED)
    {
      text_pad = ((text_size + page_size - 1) & (- page_size)) - text_size;
      text_size += text_pad;
    }

  /* Now that the text_size is known, initialize the data start address;
     this depends on text_size as well as the output file format.  */

  initialize_data_start ();

  /* Make sure the set vectors are aligned properly. */
  {
    int new_data_size = ((data_size + sizeof(unsigned long) - 1)
                         & ~(sizeof(unsigned long)-1));

    set_sect_pad += new_data_size - data_size;
    data_size = new_data_size;
  }

  /* Set up the set element vector */

  if (output_style != OUTPUT_RELOCATABLE)
    {
      /* The set sector size is the number of set elements + a word
         for each symbol for the length word at the beginning of the
	 vector, plus a word for each symbol for a zero at the end of
	 the vector (for incremental linking).  */
      set_sect_size
	= (set_symbol_count + 2 * set_vector_count) * sizeof (unsigned long);
      set_sect_start = data_start + data_size;
      data_size += set_sect_size;
      set_vectors = (unsigned long *) xmalloc (set_sect_size);
      set_reloc = (int *) xmalloc (set_sect_size / sizeof (unsigned long)
				   * sizeof (int));
      setv_fill_count = 0;
    }

  /* Make sure bss starts out aligned as much as anyone can want.  */
  {
    int new_data_size = (data_size + SECTION_ALIGN_MASK) & ~SECTION_ALIGN_MASK;

    data_pad += new_data_size - data_size;
    data_size = new_data_size;
  }

  /* Compute start addresses of each file's sections and symbols.  */

  each_full_file (relocate_file_addresses, 0);

  /* Now, for each symbol, verify that it is defined globally at most once.
     Put the global value into the symbol entry.
     Common symbols are allocated here, in the BSS section.
     Each defined symbol is given a '->defined' field
      which is the correct N_ code for its definition,
      except in the case of common symbols with -r.
     Then make all the references point at the symbol entry
     instead of being chained together. */

  defined_global_sym_count = 0;

  for (i = 0; i < TABSIZE; i++)
    {
      register symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  /* For each symbol */
	  register struct nlist *p, *next;
	  int defs = 0, com = sp->max_common_size;
	  struct nlist *first_definition;
	  for (p = sp->refs; p; p = next)
	    {
	      register int type = p->n_type;

	      if (SET_ELEMENT_P (type))
		{
		  if (output_style == OUTPUT_RELOCATABLE)
		    fatal ("internal: global ref to set element with -r");
		  if (!defs++)
		    {
		      sp->value = set_sect_start
			+ setv_fill_count++ * sizeof (unsigned long);
		      sp->defined = N_SETV | N_EXT;
		      first_definition = p;
		    }
		  else if ((sp->defined & ~N_EXT) != N_SETV)
		    {
		      sp->multiply_defined = 1;
		      multiple_def_count++;
		    }
		  set_reloc[setv_fill_count] = TYPE_OF_SET_ELEMENT (type);
		  if ((type & ~N_EXT) != N_SETA)
		    data_reloc_size += sizeof (struct relocation_info);
		  set_vectors[setv_fill_count++] = p->n_value;
		}
	      else if ((type & N_EXT) && type != (N_UNDF | N_EXT)
                       && type != (N_IMP1 | N_EXT))
		{
		  /* non-common definition */
		  if (defs++ && sp->value != p->n_value)
		    {
		      sp->multiply_defined = 1;
		      multiple_def_count++;
		    }
		  sp->value = p->n_value;
		  sp->defined = type;
		  first_definition = p;
		}
	      next = (struct nlist *) p->n_un.n_name;
	      p->n_un.n_name = (char *) sp;
	    }
	  /* Allocate as common if defined as common and not defined for real */
	  if (com && !defs)
	    {
	      if (output_style != OUTPUT_RELOCATABLE || force_common_definition)
		{
		  int align = sizeof (int);

		  /* Round up to nearest sizeof (int).  I don't know
		     whether this is necessary or not (given that
		     alignment is taken care of later), but it's
		     traditional, so I'll leave it in.  Note that if
		     this size alignment is ever removed, ALIGN above
		     will have to be initialized to 1 instead of
		     sizeof (int).  */

		  com = (com + sizeof (int) - 1) & (- sizeof (int));

		  while (!(com & align))
		    align <<= 1;

		  align = align > MAX_ALIGNMENT ? MAX_ALIGNMENT : align;

		  bss_size = ((((bss_size + data_size + data_start)
			      + (align - 1)) & (- align))
			      - data_size - data_start);

		  sp->value = data_start + data_size + bss_size;
		  sp->defined = N_BSS | N_EXT;
		  bss_size += com;
		  if (write_map)
		    printf ("Allocating common %s: %x at %lx\n",
			    sp->name, com, sp->value);
		}
	      else
		{
		  sp->defined = 0;
		  undefined_global_sym_count++;
		}
	    }
	  /* Set length word at front of vector and zero byte at end.
	     Reverse the vector itself to put it in file order.  */
	  if ((sp->defined & ~N_EXT) == N_SETV)
	    {
	      unsigned long length_word_index
		= (sp->value - set_sect_start) / sizeof (unsigned long);
	      unsigned long i, tmp;

	      set_reloc[length_word_index] = N_ABS;
	      set_vectors[length_word_index]
		= setv_fill_count - 1 - length_word_index;

	      /* Reverse the vector.  */
	      for (i = 1;
		   i < (setv_fill_count - length_word_index - 1) / 2 + 1;
		   i++)
		{
		  tmp = set_reloc[length_word_index + i];
		  set_reloc[length_word_index + i]
		    = set_reloc[setv_fill_count - i];
		  set_reloc[setv_fill_count - i] = (int)tmp;

		  tmp = set_vectors[length_word_index + i];
		  set_vectors[length_word_index + i]
		    = set_vectors[setv_fill_count - i];
		  set_vectors[setv_fill_count - i] = tmp;
		}

	      set_reloc[setv_fill_count] = N_ABS;
	      set_vectors[setv_fill_count++] = 0;
	    }
          if (!sp->defined && WEAK_SYMBOL (sp->weak))
            {
              sp->defined = N_ABS;
              sp->value = 0;
              undefined_global_sym_count--;
            }
	  if (sp->defined)
	    defined_global_sym_count++;
	}
    }

  /* Make sure end of bss is aligned as much as anyone can want.  */

  bss_size = (bss_size + SECTION_ALIGN_MASK) & ~SECTION_ALIGN_MASK;

  /* Give values to _end and friends.  */
  {
    int end_value = data_start + data_size + bss_size;
    if (end_symbol)
      end_symbol->value = end_value;
    if (end_symbol_alt)
      end_symbol_alt->value = end_value;
  }

  {
    int etext_value = text_size + text_start;
    if (etext_symbol)
      etext_symbol->value = etext_value;
    if (etext_symbol_alt)
      etext_symbol_alt->value = etext_value;
  }

  {
    int edata_value = data_start + data_size;
    if (edata_symbol)
      edata_symbol->value = edata_value;
    if (edata_symbol_alt)
      edata_symbol_alt->value = edata_value;
  }

  /* Figure the data_pad now, so that it overlaps with the bss addresses.  */

  {
    /* The amount of data_pad that we are computing now.  This is the
       part which overlaps with bss.  What was computed previously
       goes before bss.  */
    int data_pad_additional = 0;

    if (specified_data_size && specified_data_size > data_size)
      data_pad_additional = specified_data_size - data_size;

    if (output_style == OUTPUT_DEMAND_PAGED)
      data_pad_additional =
	((data_pad_additional + data_size + page_size - 1) & (- page_size)) - data_size;

    bss_size -= data_pad_additional;
    if (bss_size < 0) bss_size = 0;

    data_size += data_pad_additional;

    data_pad += data_pad_additional;
  }
}

/* Accumulate the section sizes of input file ENTRY
   into the section sizes of the output file.  */

void
consider_file_section_lengths (entry)
     register struct file_entry *entry;
{
  if (entry->just_syms_flag)
    return;

  entry->text_start_address = text_size;
  /* If there were any vectors, we need to chop them off */
  text_size += (entry->text_size + SECTION_ALIGN_MASK) & ~SECTION_ALIGN_MASK;
  entry->data_start_address = data_size;
  data_size += (entry->data_size + SECTION_ALIGN_MASK) & ~SECTION_ALIGN_MASK;
  entry->bss_start_address = bss_size;
  bss_size += (entry->bss_size + SECTION_ALIGN_MASK) & ~SECTION_ALIGN_MASK;

  text_reloc_size += entry->text_reloc_size;
  data_reloc_size += entry->data_reloc_size;
}

/* Determine where the sections of ENTRY go into the output file,
   whose total section sizes are already known.
   Also relocate the addresses of the file's local and debugger symbols.  */

void
relocate_file_addresses (entry)
     register struct file_entry *entry;
{
  entry->text_start_address += text_start;

  /* Note that `data_start' and `data_size' have not yet been adjusted
     for the portion of data_pad which overlaps with bss.  If they had
     been, we would get the wrong results here.  */
  entry->data_start_address += data_start;
  entry->bss_start_address += data_start + data_size;

  {
    register struct nlist *p;
    register struct nlist *end
      = entry->symbols + entry->syms_size / sizeof (struct nlist);

    for (p = entry->symbols; p < end; p++)
      {
	/* If this belongs to a section, update it by the section's start address */
	register int type = p->n_type & N_TYPE;

	switch (type)
	  {
	  case N_TEXT:
	  case N_SETT:
	    p->n_value += entry->text_start_address - entry->orig_text_address;
	    break;
	  case N_DATA:
	  case N_SETV:
	  case N_SETD:
	    /* Data segment symbol.  Subtract the address of the
	       data segment in the input file, and add the address
	       of this input file's data segment in the output file.  */
	    p->n_value +=
	      entry->data_start_address - entry->orig_data_address;
	    break;
	  case N_BSS:
	  case N_SETB:
	    /* likewise for symbols with value in BSS.  */
	    p->n_value += entry->bss_start_address - entry->orig_bss_address;
	    break;
	  }
      }
  }
}

void describe_file_sections (), list_file_locals ();

/* Print a complete or partial map of the output file.  */

void
print_symbols (outfile)
     FILE *outfile;
{
  register int i;

  fprintf (outfile, "\nFiles:\n\n");

  each_file (describe_file_sections, (int)outfile);

  fprintf (outfile, "\nGlobal symbols:\n\n");

  for (i = 0; i < TABSIZE; i++)
    {
      register symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  if (sp->defined == 1)
	    fprintf (outfile, "  %s: common, length 0x%x\n", sp->name, sp->max_common_size);
	  if (sp->defined)
	    fprintf (outfile, "  %s: 0x%lx\n", sp->name, sp->value);
	  else if (sp->referenced)
	    fprintf (outfile, "  %s: undefined\n", sp->name);
	}
    }

  each_file (list_file_locals, (int)outfile);
}

void
describe_file_sections (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  fprintf (outfile, "  ");
  print_file_name (entry, outfile);
  if (entry->just_syms_flag)
    fprintf (outfile, " symbols only\n");
  else
    fprintf (outfile, " text %x(%lx), data %x(%lx), bss %x(%lx) hex\n",
	     entry->text_start_address, entry->text_size,
	     entry->data_start_address, entry->data_size,
	     entry->bss_start_address, entry->bss_size);
}

void
list_file_locals (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  register struct nlist
    *p,
    *end = entry->symbols + entry->syms_size / sizeof (struct nlist);

  entry->strings = (char *) ALLOCA (entry->strs_size);
  read_entry_strings (file_open (entry), entry);

  fprintf (outfile, "\nLocal symbols of ");
  print_file_name (entry, outfile);
  fprintf (outfile, ":\n\n");

  for (p = entry->symbols; p < end; p++)
    /* If this is a definition,
       update it if necessary by this file's start address.  */
    if (((output_style == OUTPUT_RELOCATABLE) && SET_ELEMENT_P (p->n_type))
     || (!(p->n_type & (N_STAB | N_EXT)) && !SET_ELEMENT_P (p->n_type)))
      fprintf (outfile, "  %s: 0x%lx\n",
	       entry->strings + p->n_un.n_strx, p->n_value);
    else if (SET_ELEMENT_P (p->n_type))
      fprintf (outfile, "  [set element] %s: 0x%lx\n",
	       ((symbol *)p->n_un.n_name)->name, p->n_value);

  FREEA (entry->strings);
}


/* Static vars for do_warnings and subroutines of it */
int list_unresolved_refs;	/* List unresolved refs */
int list_warning_symbols;	/* List warning syms */
int list_multiple_defs;		/* List multiple definitions */

/*
 * Structure for communication between do_file_warnings and it's
 * helper routines.  Will in practice be an array of three of these:
 * 0) Current line, 1) Next line, 2) Source file info.
 */
struct line_debug_entry
{
  int line;
  char *filename;
  struct nlist *sym;
};

int next_debug_entry (int use_data_symbols, struct line_debug_entry state_pointer[3]);
struct line_debug_entry * init_debug_scan (int use_data_symbols, struct file_entry *entry);
int address_to_line (unsigned long address, struct line_debug_entry state_pointer[3]);
void qsort ();
/*
 * Helper routines for do_file_warnings.
 */

/* Return an integer less than, equal to, or greater than 0 as per the
   relation between the two relocation entries.  Used by qsort.  */

int
relocation_entries_relation (rel1, rel2)
     struct relocation_info *rel1, *rel2;
{
  return RELOC_ADDRESS(rel1) - RELOC_ADDRESS(rel2);
}

/* Moves to the next debugging symbol in the file.  USE_DATA_SYMBOLS
   determines the type of the debugging symbol to look for (DSLINE or
   SLINE).  STATE_POINTER keeps track of the old and new locatiosn in
   the file.  It assumes that state_pointer[1] is valid; ie
   that it.sym points into some entry in the symbol table.  If
   state_pointer[1].sym == 0, this routine should not be called.  */

int
next_debug_entry (use_data_symbols, state_pointer)
     register int use_data_symbols;
     /* Next must be passed by reference! */
     struct line_debug_entry state_pointer[3];
{
  register struct line_debug_entry
    *current = state_pointer,
    *next = state_pointer + 1,
    /* Used to store source file */
    *source = state_pointer + 2;
  struct file_entry *entry = (struct file_entry *) source->sym;

  current->sym = next->sym;
  current->line = next->line;
  current->filename = next->filename;

  while (++(next->sym) < (entry->symbols
			  + entry->syms_size/sizeof (struct nlist)))
    {
      /* n_type is a char, and N_SOL, N_EINCL and N_BINCL are > 0x80, so
       * may look negative...therefore, must mask to low bits
       */
      switch (next->sym->n_type & 0xff)
	{
	case N_SLINE:
	  if (use_data_symbols) continue;
	  next->line = next->sym->n_desc;
	  return 1;
	case N_DSLINE:
	  if (!use_data_symbols) continue;
	  next->line = next->sym->n_desc;
	  return 1;
#ifdef HAVE_SUN_STABS
	case N_EINCL:
	  next->filename = source->filename;
	  continue;
#endif
	case N_SO:
	  source->filename = next->sym->n_un.n_strx + entry->strings;
	  source->line++;
#ifdef HAVE_SUN_STABS
	case N_BINCL:
#endif
	case N_SOL:
	  next->filename
	    = next->sym->n_un.n_strx + entry->strings;
	default:
	  continue;
	}
    }
  next->sym = (struct nlist *) 0;
  return 0;
}

/* Create a structure to save the state of a scan through the debug
   symbols.  USE_DATA_SYMBOLS is set if we should be scanning for
   DSLINE's instead of SLINE's.  entry is the file entry which points
   at the symbols to use.  */

struct line_debug_entry *
init_debug_scan (use_data_symbols, entry)
     int use_data_symbols;
     struct file_entry *entry;
{
  struct line_debug_entry
    *state_pointer
      = (struct line_debug_entry *)
	xmalloc (3 * sizeof (struct line_debug_entry));
  register struct line_debug_entry
    *current = state_pointer,
    *next = state_pointer + 1,
    *source = state_pointer + 2; /* Used to store source file */

  struct nlist *tmp;

  for (tmp = entry->symbols;
       tmp < (entry->symbols
	      + entry->syms_size/sizeof (struct nlist));
       tmp++)
    if (tmp->n_type == (int) N_SO)
      break;

  if (tmp >= (entry->symbols
	      + entry->syms_size/sizeof (struct nlist)))
    {
      /* I believe this translates to "We lose" */
      current->filename = next->filename = entry->filename;
      current->line = next->line = -1;
      current->sym = next->sym = (struct nlist *) 0;
      return state_pointer;
    }

  next->line = source->line = 0;
  next->filename = source->filename
    = (tmp->n_un.n_strx + entry->strings);
  source->sym = (struct nlist *) entry;
  next->sym = tmp;

  next_debug_entry (use_data_symbols, state_pointer); /* To setup next */

  if (!next->sym)		/* No line numbers for this section; */
				/* setup output results as appropriate */
    {
      if (source->line)
	{
	  current->filename = source->filename = entry->filename;
	  current->line = -1;	/* Don't print lineno */
	}
      else
	{
	  current->filename = source->filename;
	  current->line = 0;
	}
      return state_pointer;
    }


  next_debug_entry (use_data_symbols, state_pointer); /* To setup current */

  return state_pointer;
}

/* Takes an ADDRESS (in either text or data space) and a STATE_POINTER
   which describes the current location in the implied scan through
   the debug symbols within the file which ADDRESS is within, and
   returns the source line number which corresponds to ADDRESS.  */

int
address_to_line (address, state_pointer)
     unsigned long address;
     /* Next must be passed by reference! */
     struct line_debug_entry state_pointer[3];
{
  struct line_debug_entry
    *current = state_pointer,
    *next = state_pointer + 1;
  struct line_debug_entry *tmp_pointer;

  int use_data_symbols;

  if (next->sym)
    use_data_symbols = (next->sym->n_type & ~N_EXT) == N_DATA;
  else
    return current->line;

  /* Go back to the beginning if we've already passed it.  */
  if (current->sym->n_value > address)
    {
      tmp_pointer = init_debug_scan (use_data_symbols,
				     (struct file_entry *)
				     ((state_pointer + 2)->sym));
      state_pointer[0] = tmp_pointer[0];
      state_pointer[1] = tmp_pointer[1];
      state_pointer[2] = tmp_pointer[2];
      free (tmp_pointer);
    }

  /* If we're still in a bad way, return -1, meaning invalid line.  */
  if (current->sym->n_value > address)
    return -1;

  while (next->sym
	 && next->sym->n_value <= address
	 && next_debug_entry (use_data_symbols, state_pointer))
    ;
  return current->line;
}


/* Macros for manipulating bitvectors.  */
#define	BIT_SET_P(bv, index)	((bv)[(index) >> 3] & 1 << ((index) & 0x7))
#define	SET_BIT(bv, index)	((bv)[(index) >> 3] |= 1 << ((index) & 0x7))

/* This routine will scan through the relocation data of file ENTRY,
   printing out references to undefined symbols and references to
   symbols defined in files with N_WARNING symbols.  If DATA_SEGMENT
   is non-zero, it will scan the data relocation segment (and use
   N_DSLINE symbols to track line number); otherwise it will scan the
   text relocation segment.  Warnings will be printed on the output
   stream OUTFILE.  Eventually, every nlist symbol mapped through will
   be marked in the NLIST_BITVECTOR, so we don't repeat ourselves when
   we scan the nlists themselves.  */

void
do_relocation_warnings (entry, data_segment, outfile, nlist_bitvector)
     struct file_entry *entry;
     int data_segment;
     FILE *outfile;
     unsigned char *nlist_bitvector;
{
  struct relocation_info
    *reloc_start = data_segment ? entry->datarel : entry->textrel,
    *reloc;
  int reloc_size
    = ((data_segment ? entry->data_reloc_size : entry->text_reloc_size)
       / sizeof (struct relocation_info));
  int start_of_segment
    = (data_segment ? entry->data_start_address : entry->text_start_address);
  struct nlist *start_of_syms = entry->symbols;
  struct line_debug_entry *state_pointer
    = init_debug_scan (data_segment != 0, entry);
  register struct line_debug_entry *current = state_pointer;
  /* Assigned to generally static values; should not be written into.  */
  char *errfmt;
  /* Assigned to alloca'd values cand copied into; should be freed
     when done.  */
  char *errmsg;
  int invalidate_line_number;

  /* We need to sort the relocation info here.  Sheesh, so much effort
     for one lousy error optimization. */

  qsort (reloc_start, reloc_size, sizeof (struct relocation_info),
	 (int (*)(const void *, const void *))relocation_entries_relation);

  for (reloc = reloc_start;
       reloc < (reloc_start + reloc_size);
       reloc++)
    {
      register struct nlist *s;
      register symbol *g;

      /* If the relocation isn't resolved through a symbol, continue */
      if (!RELOC_EXTERN_P(reloc))
	continue;

      s = &(entry->symbols[RELOC_SYMBOL(reloc)]);

      /* Local symbols shouldn't ever be used by relocation info, so
	 the next should be safe.
	 This is, of course, wrong.  References to local BSS symbols can be
	 the targets of relocation info, and they can (must) be
	 resolved through symbols.  However, these must be defined properly,
	 (the assembler would have caught it otherwise), so we can
	 ignore these cases.  */
      if (!(s->n_type & N_EXT))
	continue;

      g = (symbol *) s->n_un.n_name;
      errmsg = 0;

      if (!g->defined && list_unresolved_refs) /* Reference */
	{
	  /* Mark as being noted by relocation warning pass.  */
	  SET_BIT (nlist_bitvector, s - start_of_syms);

	  if (g->undef_refs >= MAX_UREFS_PRINTED)    /* Listed too many */
	    continue;

	  /* Undefined symbol which we should mention */

	  if (++(g->undef_refs) == MAX_UREFS_PRINTED)
	    {
	      errfmt = "More undefined symbol %s refs follow";
	      invalidate_line_number = 1;
	    }
	  else
	    {
	      errfmt = "Undefined symbol %s referenced from %s segment";
	      invalidate_line_number = 0;
	    }
	}
      else					     /* Defined */
	{
	  /* Potential symbol warning here */
	  if (!g->warning) continue;

	  /* Mark as being noted by relocation warning pass.  */
	  SET_BIT (nlist_bitvector, s - start_of_syms);

	  errfmt = 0;
	  errmsg = g->warning;
	  invalidate_line_number = 0;
	}


      /* If errfmt == 0, errmsg has already been defined.  */
      if (errfmt != 0)
	{
	  char *nm;

	  if (!demangler || !(nm = (*demangler)(g->name)))
	    nm = g->name;
	  errmsg = (char *) xmalloc (strlen (errfmt) + strlen (nm) + 1);
	  sprintf (errmsg, errfmt, nm, data_segment ? "data" : "text");
	  if (nm != g->name)
	    free (nm);
	}

      address_to_line (RELOC_ADDRESS (reloc) + start_of_segment,
		       state_pointer);

      if (current->line >=0)
	{
	  fprintf (outfile, "%s:%d (", current->filename,
		   invalidate_line_number ? 0 : current->line);
	  print_file_name (entry, outfile);
	  fprintf (outfile, "): %s\n", errmsg);
	}
      else
	{
	  print_file_name(entry, outfile);
	  fprintf(outfile, ": %s\n", errmsg);
	}

      if (errfmt != 0)
	free (errmsg);
    }

  free (state_pointer);
}

/* Print on OUTFILE a list of all warnings generated by references
   and/or definitions in the file ENTRY.  List source file and line
   number if possible, just the .o file if not. */

void
do_file_warnings (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  int number_of_syms = entry->syms_size / sizeof (struct nlist);
  unsigned char *nlist_bitvector
    = (unsigned char *) ALLOCA ((number_of_syms >> 3) + 1);
  struct line_debug_entry *text_scan, *data_scan;
  int i;
  char *errfmt, *file_name = NULL;
  int line_number = 0;
  int dont_allow_symbol_name;

  bzero (nlist_bitvector, (number_of_syms >> 3) + 1);

  /* Read in the files strings if they aren't available */
  if (!entry->strings)
    {
      int desc;

      entry->strings = (char *) ALLOCA (entry->strs_size);
      desc = file_open (entry);
      read_entry_strings (desc, entry);
    }

  read_file_relocation (entry);

  /* Do text warnings based on a scan through the relocation info.  */
  do_relocation_warnings (entry, 0, outfile, nlist_bitvector);

  /* Do data warnings based on a scan through the relocation info.  */
  do_relocation_warnings (entry, 1, outfile, nlist_bitvector);

  /* Scan through all of the nlist entries in this file and pick up
     anything that the scan through the relocation stuff didn't.  */

  text_scan = init_debug_scan (0, entry);
  data_scan = init_debug_scan (1, entry);

  for (i = 0; i < number_of_syms; i++)
    {
      struct nlist *s;
      struct glosym *g;

      s = entry->symbols + i;

      if (WEAK_SYMBOL (s->n_type) || !(s->n_type & N_EXT))
	continue;

      g = (symbol *) s->n_un.n_name;
      dont_allow_symbol_name = 0;

      if (list_multiple_defs && g->multiply_defined)
	{
	  errfmt = "Definition of symbol %s (multiply defined)";
	  switch (s->n_type)
	    {
	    case N_TEXT | N_EXT:
	      line_number = address_to_line (s->n_value, text_scan);
	      file_name = text_scan[0].filename;
	      break;
	    case N_BSS | N_EXT:
	    case N_DATA | N_EXT:
	      line_number = address_to_line (s->n_value, data_scan);
	      file_name = data_scan[0].filename;
	      break;
	    case N_SETA | N_EXT:
	    case N_SETT | N_EXT:
	    case N_SETD | N_EXT:
	    case N_SETB | N_EXT:
	      if (g->multiply_defined == 2)
		continue;
	      errfmt = "First set element definition of symbol %s (multiply defined)";
	      break;
	    default:
	      continue;		/* Don't print out multiple defs
				   at references.  */
	    }
	}
      else if (BIT_SET_P (nlist_bitvector, i))
	continue;
      else if (list_unresolved_refs && !g->defined)
	{
	  if (g->undef_refs >= MAX_UREFS_PRINTED)
	    continue;

	  if (++(g->undef_refs) == MAX_UREFS_PRINTED)
	    errfmt = "More undefined \"%s\" refs follow";
	  else
	    errfmt = "Undefined symbol \"%s\" referenced";
	  line_number = -1;
	}
      else if (g->warning)
	{
	  /* There are two cases in which we don't want to
	     do this.  The first is if this is a definition instead of
	     a reference.  The second is if it's the reference used by
	     the warning stabs itself.  */
	  if (s->n_type != (N_EXT | N_UNDF)
	      || (i && (s-1)->n_type == N_WARNING))
	    continue;

	  errfmt = g->warning;
	  line_number = -1;
	  dont_allow_symbol_name = 1;
	}
      else
	continue;

      if (line_number == -1)
	{
	  print_file_name (entry, outfile);
	  fprintf (outfile, ": ");
	}
      else
	{
	  fprintf (outfile, "%s:%d (", file_name, line_number);
	  print_file_name (entry, outfile);
	  fprintf (outfile, "): ");
	}

      if (dont_allow_symbol_name)
	fprintf (outfile, "%s", errfmt);
      else
	{
	  char *nm;

	  if (!demangler || !(nm = (*demangler)(g->name)))
	    fprintf (outfile, errfmt, g->name);
	  else
	    {
	      fprintf (outfile, errfmt, nm);
	      free (nm);
	    }
	}

      fputc ('\n', outfile);
    }
  free (text_scan);
  free (data_scan);
  FREEA (entry->strings);
  FREEA (nlist_bitvector);
}

void
do_warnings (outfile)
     FILE *outfile;
{
  list_unresolved_refs = output_style != OUTPUT_RELOCATABLE && undefined_global_sym_count;
  list_warning_symbols = warning_count;
  list_multiple_defs = multiple_def_count != 0;

  if (!(list_unresolved_refs ||
	list_warning_symbols ||
	list_multiple_defs      ))
    /* No need to run this routine */
    return;

  each_file (do_file_warnings, (int)outfile);

  if (list_unresolved_refs || list_multiple_defs)
    make_executable = 0;
}

#ifdef A_OUT

/* Stuff pertaining to creating a.out files. */

/* The a.out header. */

struct exec outheader;

/* Compute text_start and text_header_size for an a.out file.  */

void
initialize_a_out_text_start (void)
{
  int magic = 0;

  switch (output_style)
    {
    case OUTPUT_RELOCATABLE:
    case OUTPUT_WRITABLE_TEXT:
      magic = OMAGIC;
      break;
    case OUTPUT_READONLY_TEXT:
#ifdef NMAGIC
      magic = NMAGIC;
      break;
#endif
    case OUTPUT_DEMAND_PAGED:
      magic = ZMAGIC;
      break;
    default:
      fatal ("unknown output style found (bug in ld)", (char *) 0);
      break;
    }

  /* Determine whether to count the header as part of
     the text size, and initialize the text size accordingly.
     This depends on the kind of system and on the output format selected.  */
  N_SET_MAGIC (outheader, magic);
  N_SET_MACHTYPE (outheader, M_386);
#ifdef INITIALIZE_HEADER
  INITIALIZE_HEADER;
#endif

  text_header_size = sizeof (struct exec);
  if (text_header_size <= N_TXTOFF (outheader))
    text_header_size = 0;
  else
    text_header_size -= N_TXTOFF (outheader);

#ifdef _N_BASEADDR
  /* SunOS 4.1 N_TXTADDR depends on the value of outheader.a_entry. */
  outheader.a_entry = N_PAGSIZ(outheader);
#endif

  if (!T_flag_specified && output_style != OUTPUT_RELOCATABLE)
    text_start = N_TXTADDR (outheader);
}

/* Compute data_start once text_size is known. */

void
initialize_a_out_data_start (void)
{
  outheader.a_text = text_size;
  if (! Tdata_flag_specified)
    data_start = N_DATADDR (outheader) + text_start - N_TXTADDR (outheader);
}

/* Compute offsets of various pieces of the a.out output file.  */

void
compute_a_out_section_offsets (void)
{
  outheader.a_data = data_size;
  outheader.a_bss = bss_size;
  outheader.a_entry = (entry_symbol ? entry_symbol->value
		       : text_start + text_header_size);

  if (strip_symbols == STRIP_ALL)
    nsyms = 0;
  else
    {
      nsyms = (defined_global_sym_count
	       + undefined_global_sym_count);
      if (discard_locals == DISCARD_L)
	nsyms += non_L_local_sym_count;
      else if (discard_locals == DISCARD_NONE)
	nsyms += local_sym_count;
      /* One extra for following reference on indirects */
      if (output_style == OUTPUT_RELOCATABLE)
	nsyms += set_symbol_count + global_indirect_count;
    }

  if (strip_symbols == STRIP_NONE)
    nsyms += debugger_sym_count;

  outheader.a_syms = nsyms * sizeof (struct nlist);

  if (output_style == OUTPUT_RELOCATABLE || reloc_flag)
    {
      outheader.a_trsize = text_reloc_size;
      outheader.a_drsize = data_reloc_size;
    }
  else
    {
      outheader.a_trsize = 0;
      outheader.a_drsize = 0;
    }

  /* Initialize the various file offsets.  */

  output_text_offset = N_TXTOFF (outheader);
#ifdef N_DATOFF
  output_data_offset = N_DATOFF (outheader);
#else
  output_data_offset = output_text_offset + text_size;
#endif
#ifdef N_TRELOFF
  output_trel_offset = N_TRELOFF (outheader);
#else
  output_trel_offset = output_data_offset + data_size;
#endif
#ifdef N_DRELOFF
  output_drel_offset = N_DRELOFF (outheader);
#else
  output_drel_offset = output_trel_offset + text_reloc_size;
#endif
  output_syms_offset = N_SYMOFF (outheader);
  output_strs_offset = N_STROFF (outheader);
}

/* Compute more section offsets once the size of the string table is known.  */

void
compute_more_a_out_section_offsets (void)
{
  output_symseg_offset = output_strs_offset + output_strs_size;
}

/* Write the a.out header once everything else is known.  */

void
write_a_out_header (void)
{
  lseek (outdesc, 0L, 0);
  mywrite (&outheader, sizeof (struct exec), 1, outdesc);
}

#endif

/* The following functions are simple switches according to the
   output style.  */

/* Compute text_start and text_header_size as appropriate for the
   output format.  */

void
initialize_text_start (void)
{
#ifdef A_OUT
  if (output_file_type == IS_A_OUT)
    {
      initialize_a_out_text_start ();
      return;
    }
#endif
  fatal ("unknown output file type (enum file_type)", (char *) 0);
}

/* Initialize data_start as appropriate to the output format, once text_size
   is known.  */

void
initialize_data_start (void)
{
#ifdef A_OUT
  if (output_file_type == IS_A_OUT)
    {
      initialize_a_out_data_start ();
      return;
    }
#endif
  fatal ("unknown output file type (enum file_type)", (char *) 0);
}

/* Compute offsets of the various sections within the output file.  */

void
compute_section_offsets (void)
{
#ifdef A_OUT
  if (output_file_type == IS_A_OUT)
    {
      compute_a_out_section_offsets ();
      return;
    }
#endif
  fatal ("unknown output file type (enum file_type)", (char *) 0);
}

/* Compute more section offsets, once the size of the string table
   is finalized.  */
void
compute_more_section_offsets (void)
{
#ifdef A_OUT
  if (output_file_type == IS_A_OUT)
    {
      compute_more_a_out_section_offsets ();
      return;
    }
#endif
  fatal ("unknown output file type (enum file_type)", (char *) 0);
}

/* Write the output file header, once everything is known.  */
void
write_header (void)
{
#ifdef A_OUT
  if (output_file_type == IS_A_OUT)
    {
      write_a_out_header ();
      return;
    }
#endif
  fatal ("unknown output file type (enum file_type)", (char *) 0);
}

/* Parse output_filename and decide whether to create an exe file or not */

void check_exe (void)
{
  char *ext;

  if (exe_flag)
    {
      ext = _getext (output_filename);
      if ((ext != NULL) && (stricmp (ext, ".exe") == 0))
      {
        exe_filename = output_filename;
        exe_flag = 0;
      } else
      {
        touch_filename = output_filename;
        exe_filename = concat (output_filename, ".exe", NULL);
      }
    }
  else
    {
      ext = _getext2 (output_filename);
      if (stricmp (ext, ".dll") == 0)
        {
          reloc_flag = 1; dll_flag = 1;
        }
      else if (stricmp (ext, ".exe") != 0)
        {
          exe_filename = NULL;
          return;
        }
      exe_filename = output_filename;
    }

  /* Create a temporary a.out executable file. */

  output_filename = make_temp_file ("ldXXXXXX");

  if (rsxnt_linked != EMX_DEFAULT && strip_symbols != STRIP_NONE) /* RSXNT */
    {
      strip_symbols = STRIP_NONE;
      emxbind_strip = 1;
    }
  else if (strip_symbols == STRIP_ALL)
    {
      strip_symbols = STRIP_DEBUGGER;
      emxbind_strip = 1;
    }
  unlink (exe_filename);
  if (touch_filename != NULL)
    unlink (touch_filename);
}

/* Write the output file */

void
write_output (void)
{
  struct stat statbuf;
  int filemode, mask;

  /* Remove the old file in case it is owned by someone else.
     This prevents spurious "not owner" error messages.
     Don't check for errors from unlink; we don't really care
     whether it worked.

     Note that this means that if the output file is hard linked,
     the other names will still have the old contents.  This is
     the way Unix ld works; I'm going to consider it a feature.  */
  (void) unlink (output_filename);

  outdesc = open (output_filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
  if (outdesc < 0) perror_name (output_filename);

  if (fstat (outdesc, &statbuf) < 0)
    perror_name (output_filename);

  filemode = statbuf.st_mode;

  chmod (output_filename, filemode & ~0111);

  if (reloc_flag)
    global_indirect_count = 0;

  /* Calculate the offsets of the various pieces of the output file.  */
  compute_section_offsets ();

  /* Output the text and data segments, relocating as we go.  */
  write_text ();
  write_data ();

  /* Output the merged relocation info, if requested with `-r'.  */
  if (output_style == OUTPUT_RELOCATABLE || reloc_flag)
    write_rel ();

  /* Output the symbol table (both globals and locals).  */
  write_syms ();

  /* At this point the total size of the symbol table and string table
     are finalized.  */
  compute_more_section_offsets ();

  /* Copy any GDB symbol segments from input files.  */
  write_symsegs ();

  /* Now that everything is known about the output file, write its header.  */
  write_header ();

  close (outdesc);

  mask = umask (0);
  umask (mask);

  if (chmod (output_filename, filemode | (0111 & ~mask)) == -1)
    perror_name (output_filename);

  if (rsxnt_linked == EMX_DEFAULT && exe_filename != NULL)
    {
      char *nargv[11];
      char *freeav[11];
      int i, j, saved_errno;

      i = j = 0;
      nargv[i++] = "emxbind";
      nargv[i++] = "-bq";
      if (emxbind_strip)
        nargv[i++] = "-s";
      if (dll_flag && !def_filename)
          gen_deffile();
      if (def_filename != NULL)
	{
	  freeav[j++] = nargv[i] = ALLOCA (strlen (def_filename) + 3);
	  strcpy (nargv[i], "-d");
	  strcat (nargv[i], def_filename);
	  i++;
	}
      else if (dll_flag)
	  nargv[i++] = "-d";
      if (stack_size != 0)
	{
	  freeav[j++] = nargv[i] = ALLOCA (20);
          sprintf (nargv[i], "-k0x%x", stack_size);
	  i++;
	}
      if (map_flag)
	{
	  if (map_filename == NULL)
	    {
	      freeav[j++] = map_filename = ALLOCA (strlen (exe_filename) + 5);
              strcpy (map_filename, exe_filename);
	      _remext (map_filename);
	      strcat (map_filename, ".map");
	    }
	  freeav[j++] = nargv[i] = ALLOCA (strlen (map_filename) + 3);
          strcpy (nargv[i], "-m");
          strcat (nargv[i], map_filename);
	  i++;
	}
      if (res_filename != NULL)
	{
	  freeav[j++] = nargv[i] = ALLOCA (strlen (res_filename) + 3);
	  strcpy (nargv[i], "-r");
	  strcat (nargv[i], res_filename);
	  i++;
	}
      nargv[i++] = "-o";
      nargv[i++] = exe_filename;
      nargv[i++] = output_filename;
      nargv[i] = NULL;
      if (trace_files)
        {
          fprintf(stderr, "Invoking emxbind:");
          for (i = 0; nargv[i]; i++)
            fprintf(stderr, " %s", nargv[i]);
          fprintf(stderr, "\n");
        }
      i = spawnvp (P_WAIT, "emxbind", nargv);
      saved_errno = errno; unlink (output_filename); errno = saved_errno;
      if (i < 0)
        perror_name ("emxbind");
      else if (i != 0)
        fatal ("emxbind failed\n", NULL);
      if (chmod (exe_filename, filemode | (0111 & ~mask)) == -1)
	perror_name (exe_filename);
      if (touch_filename != NULL)
        {
          char execname[512];
          _execname(execname, sizeof (execname));
          strcpy(_getname(execname), "ldstub.bin");
          /* Copy stub into file */
          if (DosCopy(execname, touch_filename, 4))
          {
            errno = EACCES;
            perror_name (execname);
          }
          /* Now touch it */
          if (utime(touch_filename, NULL))
            perror_name (touch_filename);
        }
      while (j-- > 0)
        FREEA (freeav[j]);
    }
  else if (exe_filename != NULL) /* RSXNT */
    {
      char *nargv[10];
      char *freea = NULL;
      int i, saved_errno;

      i = 0;
      nargv[i++] = "ntbind";
      nargv[i++] = output_filename;
      nargv[i++] = "-o";
      nargv[i++] = exe_filename;
      nargv[i++] = "-s";
      if (rsxnt_linked == RSXNT_WIN32)
        nargv[i++] = (emxbind_strip) ? "dosstub.dos" : "dosstub.dbg";
      else
        {
        nargv[i++] = "dosstub.rsx";
        if (emxbind_strip)
          nargv[i++] = "-strip";
        }
      if (def_filename != NULL)
	{
          nargv[i++] = "-d";
	  freea = nargv[i] = ALLOCA (strlen (def_filename) + 3);
          strcpy(nargv[i], def_filename);
          i++;
	}
      nargv[i] = NULL;

      i = spawnvp (P_WAIT, "ntbind", nargv);
      saved_errno = errno; unlink (output_filename); errno = saved_errno;
      if (i < 0)
        perror_name ("ntbind");
      else if (i != 0)
        fatal ("ntbind failed\n", NULL);
      if (chmod (exe_filename, filemode | (0111 & ~mask)) == -1)
	perror_name (exe_filename);
      if (touch_filename != NULL)
        {
          i = open (touch_filename,
                    O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
          if (i < 0)
            perror_name (touch_filename);
          close (i);
        }
      FREEA (freea);
    }
}

void copy_text (struct file_entry *entry);
void copy_data (struct file_entry *entry);
void perform_relocation ( char *data, int pc_relocation, int data_size,
    struct relocation_info *reloc_info, int reloc_size, struct file_entry *entry);
#ifdef DEBUG_BIRD
void dbg_dump_rel(struct file_entry *entry, struct relocation_info * rel,
                  size_t reloc_size, const char *desc);
#endif

/* Relocate the text segment of each input file
   and write to the output file.  */

void
write_text ()
{
  if (trace_files)
    fprintf (stderr, "Copying and relocating text:\n\n");

  lseek (outdesc, output_text_offset + text_header_size, 0);

  each_full_file (copy_text, 0);
  file_close ();

  if (trace_files)
    fprintf (stderr, "\n");

  padfile (text_pad, outdesc);
}

/* Read in all of the relocation information */

void
read_relocation (void)
{
  each_full_file (read_file_relocation, 0);
}

/* Read in the relocation sections of ENTRY if necessary */

void
read_file_relocation (entry)
     struct file_entry *entry;
{
  register struct relocation_info *reloc;
  int desc;
  int read_return;

  desc = -1;
  if (!entry->textrel)
    {
      reloc = (struct relocation_info *) xmalloc (entry->text_reloc_size);
      desc = file_open (entry);
      lseek (desc, entry->starting_offset + entry->text_reloc_offset, L_SET);
      if (entry->text_reloc_size != (read_return = read (desc, reloc, entry->text_reloc_size)))
	{
	  fprintf (stderr, "Return from read: %d\n", read_return);
	  fatal_with_file ("premature eof in text relocation of ", entry);
	}
      entry->textrel = reloc;
    }

  if (!entry->datarel)
    {
      reloc = (struct relocation_info *) xmalloc (entry->data_reloc_size);
      if (desc == -1) desc = file_open (entry);
      lseek (desc, entry->starting_offset + entry->data_reloc_offset, L_SET);
      if (entry->data_reloc_size != read (desc, reloc, entry->data_reloc_size))
	fatal_with_file ("premature eof in data relocation of ", entry);
      entry->datarel = reloc;
    }
#ifdef DEBUG_BIRD
  dbg_dump_rel(entry, entry->textrel, entry->text_reloc_size, "text");
  dbg_dump_rel(entry, entry->datarel, entry->data_reloc_size, "data");
#endif
}

/* Read the text segment contents of ENTRY, relocate them,
   and write the result to the output file.
   If `-r', save the text relocation for later reuse.  */

void
copy_text (entry)
     struct file_entry *entry;
{
  register char *bytes;
  register int desc;
  register struct relocation_info *reloc;
  int free_reloc = 0;

  if (trace_files)
    prline_file_name (entry, stderr);

  desc = file_open (entry);

  /* Allocate space for the file's text section */

  bytes = (char *) ALLOCA (entry->text_size);

  /* Deal with relocation information however is appropriate */

  if (entry->textrel)  reloc = entry->textrel;
  else if (output_style == OUTPUT_RELOCATABLE || reloc_flag)
    {
      read_file_relocation (entry);
      reloc = entry->textrel;
    }
  else
    {
      free_reloc = 1;
      reloc = (struct relocation_info *) ALLOCA (entry->text_reloc_size);
      lseek (desc, entry->starting_offset + entry->text_reloc_offset, L_SET);
      if (entry->text_reloc_size != read (desc, reloc, entry->text_reloc_size))
	fatal_with_file ("premature eof in text relocation of ", entry);
    }

  /* Read the text section into core.  */

  lseek (desc, entry->starting_offset + entry->text_offset, L_SET);
  if (entry->text_size != read (desc, bytes, entry->text_size))
    fatal_with_file ("premature eof in text section of ", entry);

  /* Relocate the text according to the text relocation.  */

  perform_relocation (bytes, entry->text_start_address - entry->orig_text_address,
		      entry->text_size, reloc, entry->text_reloc_size, entry);

  /* Write the relocated text to the output file.  */

  mywrite (bytes, 1, entry->text_size, outdesc);
  padfile ((SECTION_ALIGN - entry->text_size) & SECTION_ALIGN_MASK, outdesc);

  FREEA (bytes);
  if (free_reloc)
    FREEA (reloc);
}

#ifdef DEBUG_BIRD
void dbg_dump_rel(struct file_entry *entry, struct relocation_info * rel,
                  size_t reloc_size, const char *desc)
{
  struct relocation_info   *relend = (struct relocation_info *)((char*)rel + reloc_size);
  int                       i;

  fprintf(stderr,
          "dbg: %s relocations %s:\n"
          "dbg: rel - Address  len sym#   attrs\n",
          desc, entry->filename);
  i = 0;
  while (rel < relend)
    {
      fprintf(stderr, "dbg: %3d - %08x 2^%d %06x %s%s",
              i,
              rel->r_address,
              rel->r_length,
              rel->r_symbolnum,
              rel->r_extern ? "Extrn " : "      ",
              rel->r_pcrel  ? "PCRel " : "      "
              );
      if (rel->r_extern)
        { /* find the symbol. */
          struct nlist *s = &entry->symbols[rel->r_symbolnum];
          fprintf(stderr, " s.val:%08lx s.typ:%02x s.des:%04x s.oth:%02x",
                  s->n_value,
                  (unsigned)s->n_type,
                  s->n_desc,
                  (unsigned)s->n_other);
          if (s->n_un.n_strx < 0x20000)
            fprintf(stderr, " !!bad symbol ptr %p", s->n_un.n_name);
          else
            {
              struct glosym *sym = (struct glosym *)s->n_un.n_name;
              fprintf(stderr, " %s", sym->name);
            }
        }
      fprintf(stderr, "\n");
      /* next */
      i++;
      rel++;
    }
}
#endif

/* Relocate the data segment of each input file
   and write to the output file.  */

void
write_data ()
{
  if (trace_files)
    fprintf (stderr, "Copying and relocating data:\n\n");

  lseek (outdesc, output_data_offset, 0);

  each_full_file (copy_data, 0);
  file_close ();

  /* Write out the set element vectors.  See digest symbols for
     description of length of the set vector section.  */

  padfile (set_sect_pad, outdesc);

  if (set_vector_count)
    mywrite (set_vectors, set_symbol_count + 2 * set_vector_count,
	     sizeof (unsigned long), outdesc);

  if (trace_files)
    fprintf (stderr, "\n");

  padfile (data_pad, outdesc);
}

/* Read the data segment contents of ENTRY, relocate them,
   and write the result to the output file.
   If `-r', save the data relocation for later reuse.
   See comments in `copy_text'.  */

void
copy_data (entry)
     struct file_entry *entry;
{
  register struct relocation_info *reloc;
  register char *bytes;
  register int desc;
  int free_reloc = 0;

  if (trace_files)
    prline_file_name (entry, stderr);

  desc = file_open (entry);

  bytes = (char *) ALLOCA (entry->data_size);

  if (entry->datarel) reloc = entry->datarel;
  else if (output_style == OUTPUT_RELOCATABLE || reloc_flag) /* Will need this again */
    {
      read_file_relocation (entry);
      reloc = entry->datarel;
    }
  else
    {
      free_reloc = 1;
      reloc = (struct relocation_info *) ALLOCA (entry->data_reloc_size);
      lseek (desc, entry->starting_offset + entry->data_reloc_offset, L_SET);
      if (entry->data_reloc_size != read (desc, reloc, entry->data_reloc_size))
	fatal_with_file ("premature eof in data relocation of ", entry);
    }

  lseek (desc, entry->starting_offset + entry->data_offset, L_SET);
  if (entry->data_size != read (desc, bytes, entry->data_size))
    fatal_with_file ("premature eof in data section of ", entry);

  perform_relocation (bytes, entry->data_start_address - entry->orig_data_address,
		      entry->data_size, reloc, entry->data_reloc_size, entry);

  mywrite (bytes, 1, entry->data_size, outdesc);
  padfile ((SECTION_ALIGN - entry->data_size) & SECTION_ALIGN_MASK, outdesc);

  FREEA (bytes);
  if (free_reloc)
    FREEA (reloc);
}

/* Relocate ENTRY's text or data section contents.
   DATA is the address of the contents, in core.
   DATA_SIZE is the length of the contents.
   PC_RELOCATION is the difference between the address of the contents
     in the output file and its address in the input file.
   RELOC_INFO is the address of the relocation info, in core.
   RELOC_SIZE is its length in bytes.  */
/* This version is about to be severly hacked by Randy.  Hope it
   works afterwards. */
void
perform_relocation (data, pc_relocation, data_size, reloc_info, reloc_size, entry)
     char *data;
     struct relocation_info *reloc_info;
     struct file_entry *entry;
     int pc_relocation;
     int data_size;
     int reloc_size;
{
  register struct relocation_info *p = reloc_info;
  struct relocation_info *end
    = reloc_info + reloc_size / sizeof (struct relocation_info);
  int text_relocation = entry->text_start_address - entry->orig_text_address;
  int data_relocation = entry->data_start_address - entry->orig_data_address;
  int bss_relocation = entry->bss_start_address - entry->orig_bss_address;

  for (; p < end; p++)
    {
      register int relocation = 0;
      register int addr = RELOC_ADDRESS(p);
      register unsigned int mask = 0;

      if (addr >= data_size)
	fatal_with_file ("relocation address out of range in ", entry);

      if (RELOC_EXTERN_P(p))
	{
	  int symindex = RELOC_SYMBOL (p) * sizeof (struct nlist);
	  symbol *sp = ((symbol *)
			(((struct nlist *)
			  (((char *)entry->symbols) + symindex))
			 ->n_un.n_name));

#ifdef N_INDR
	  /* Resolve indirection */
	  if ((sp->defined & ~N_EXT) == N_INDR)
	    sp = (symbol *) sp->value;
#endif

	  if (symindex >= entry->syms_size)
	    fatal_with_file ("relocation symbolnum out of range in ", entry);

	  /* If the symbol is undefined, leave it at zero.  */
	  if (! sp->defined)
	    relocation = 0;
	  else
	    relocation = sp->value;
	}
      else switch (RELOC_TYPE(p))
	{
	case N_TEXT:
	case N_TEXT | N_EXT:
	  relocation = text_relocation;
	  break;

	case N_DATA:
	case N_DATA | N_EXT:
	  relocation = data_relocation;
	  break;

	case N_BSS:
	case N_BSS | N_EXT:
	  relocation = bss_relocation;
	  break;

	case N_ABS:
	case N_ABS | N_EXT:
	  /* Don't know why this code would occur, but apparently it does.  */
	  break;

	default:
	  fatal_with_file ("nonexternal relocation code invalid in ", entry);
	}

      if (RELOC_PCREL_P(p))
	relocation -= pc_relocation;

#ifdef RELOC_ADD_EXTRA
      relocation += RELOC_ADD_EXTRA(p);
      if (output_style == OUTPUT_RELOCATABLE)
	{
	  /* If this RELOC_ADD_EXTRA is 0, it means that the
	     symbol was external and the relocation does not
	     need a fixup here.  */
	  if (RELOC_ADD_EXTRA (p))
	    {
	      if (! RELOC_PCREL_P (p))
		RELOC_ADD_EXTRA (p) = relocation;
	      else
		RELOC_ADD_EXTRA (p) -= pc_relocation;
	    }
#if 0
	  if (! RELOC_PCREL_P (p))
	    {
	      if ((int)p->r_type <= RELOC_32
		  || RELOC_EXTERN_P (p) == 0)
		RELOC_ADD_EXTRA (p) = relocation;
	    }
	  else if (RELOC_EXTERN_P (p))
	    RELOC_ADD_EXTRA (p) -= pc_relocation;
#endif
	  continue;
	}
#endif

      relocation >>= RELOC_VALUE_RIGHTSHIFT(p);

      /* Unshifted mask for relocation */
#if WORD_BIT == RELOC_TARGET_BITSIZE(p)
      mask = ~0;
#else
      mask = (1 << RELOC_TARGET_BITSIZE(p)) - 1;
#endif
      relocation &= mask;

      /* Shift everything up to where it's going to be used */
      relocation <<= RELOC_TARGET_BITPOS(p);
      mask <<= RELOC_TARGET_BITPOS(p);

      switch (RELOC_TARGET_SIZE(p))
	{
	case 0:
	  if (RELOC_MEMORY_SUB_P(p))
	    relocation -= mask & *(char *) (data + addr);
	  else if (RELOC_MEMORY_ADD_P(p))
	    relocation += mask & *(char *) (data + addr);
	  *(char *) (data + addr) &= ~mask;
	  *(char *) (data + addr) |= relocation;
	  break;

	case 1:
	  if (RELOC_MEMORY_SUB_P(p))
	    relocation -= mask & *(short *) (data + addr);
	  else if (RELOC_MEMORY_ADD_P(p))
	    relocation += mask & *(short *) (data + addr);
	  *(short *) (data + addr) &= ~mask;
	  *(short *) (data + addr) |= relocation;
	  break;

	case 2:
#ifdef CROSS_LINKER
	  /* This is necessary if the host has stricter alignment
	     than the target.  Too slow to use all the time.
	     Also doesn't deal with differing byte-order.  */
	  {
	    /* Thing to relocate.  */
	    long thing;
	    bcopy (data + addr, &thing, sizeof (thing));
	    if (RELOC_MEMORY_SUB_P (p))
	      relocation -= mask & thing;
	    else if (RELOC_MEMORY_ADD_P (p))
	      relocation += mask & thing;
	    thing = (thing & ~mask) | relocation;
	    bcopy (&thing, data + addr, sizeof (thing));
	  }
#else /* not CROSS_LINKER */
	  if (RELOC_MEMORY_SUB_P(p))
	    relocation -= mask & *(long *) (data + addr);
	  else if (RELOC_MEMORY_ADD_P(p))
	    relocation += mask & *(long *) (data + addr);
	  *(long *) (data + addr) &= ~mask;
	  *(long *) (data + addr) |= relocation;
#endif /* not CROSS_LINKER */
	  break;

	default:
	  fatal_with_file ("Unimplemented relocation field length in ", entry);
	}
    }
}

/* For OUTPUT_RELOCATABLE only: write out the relocation,
   relocating the addresses-to-be-relocated.  */

void coptxtrel (struct file_entry *entry);
void copdatrel (struct file_entry *entry);

void
write_rel ()
{
  register int i;
  register int count = 0;

  if (trace_files)
    fprintf (stderr, "Writing text relocation:\n\n");

  /* Assign each global symbol a sequence number, giving the order
     in which `write_syms' will write it.
     This is so we can store the proper symbolnum fields
     in relocation entries we write.  */

  for (i = 0; i < TABSIZE; i++)
    {
      symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	if (sp->referenced || sp->defined)
	  {
	    sp->def_count = count++;
	    /* Leave room for the reference required by N_INDR, if
	       necessary.  */
	    if ((sp->defined & ~N_EXT) == N_INDR)
	      if (!reloc_flag)
	        count++;
	  }
    }
  /* Correct, because if (OUTPUT_RELOCATABLE), we will also be writing
     whatever indirect blocks we have.  */
  if (count != defined_global_sym_count
      + undefined_global_sym_count + global_indirect_count)
    fatal ("internal error");

  /* Write out the relocations of all files, remembered from copy_text.  */

  lseek (outdesc, output_trel_offset, 0);
  each_full_file (coptxtrel, 0);

  if (trace_files)
    fprintf (stderr, "\nWriting data relocation:\n\n");

  lseek (outdesc, output_drel_offset, 0);
  each_full_file (copdatrel, 0);
  if (reloc_flag)
    {
      int i;
      int n = set_sect_size / sizeof (unsigned long);
      struct relocation_info reloc;

      memset (&reloc, 0, sizeof (reloc));
      RELOC_PCREL_P (&reloc) = 0;
      RELOC_TARGET_SIZE (&reloc) = 2;
      RELOC_EXTERN_P (&reloc) = 0;
      for (i = 0; i < n; ++i)
	switch (set_reloc[i] & ~N_EXT)
	  {
	  case N_TEXT:
	  case N_DATA:
	    RELOC_SYMBOL (&reloc) = set_reloc[i] & ~N_EXT;
	    RELOC_ADDRESS (&reloc)
	      = set_sect_start + i * sizeof (unsigned long) - data_start;
	    mywrite (&reloc, sizeof (reloc), 1, outdesc);
	    break;
	  case N_ABS:
	    break;
	  default:
	    fatal ("N_SETB not supported", (char *)0);
	  }
    }

  if (trace_files)
    fprintf (stderr, "\n");
}

void
coptxtrel (entry)
     struct file_entry *entry;
{
  register struct relocation_info *p, *end;
  register int reloc = entry->text_start_address - text_start;

  p = entry->textrel;
  end = (struct relocation_info *) (entry->text_reloc_size + (char *) p);
  while (p < end)
    {
      RELOC_ADDRESS(p) += reloc;
      if (RELOC_EXTERN_P(p))
	{
	  register int symindex = RELOC_SYMBOL(p) * sizeof (struct nlist);
	  symbol *symptr = ((symbol *)
			    (((struct nlist *)
			      (((char *)entry->symbols) + symindex))
			     ->n_un.n_name));

	  if (symindex >= entry->syms_size)
	    fatal_with_file ("relocation symbolnum out of range in ", entry);

#ifdef N_INDR
	  /* Resolve indirection.  */
	  if ((symptr->defined & ~N_EXT) == N_INDR)
	    symptr = (symbol *) symptr->value;
#endif

	  /* If the symbol is now defined, change the external relocation
	     to an internal one.  */

	  if (symptr->defined && symptr->defined != (N_IMP1 | N_EXT))
	    {
	      RELOC_EXTERN_P(p) = 0;
	      RELOC_SYMBOL(p) = (symptr->defined & ~N_EXT);
	      if (RELOC_SYMBOL (p) == N_SETV)
		RELOC_SYMBOL (p) = N_DATA;
#ifdef RELOC_ADD_EXTRA
	      /* If we aren't going to be adding in the value in
	         memory on the next pass of the loader, then we need
		 to add it in from the relocation entry.  Otherwise
	         the work we did in this pass is lost.  */
	      if (!RELOC_MEMORY_ADD_P(p))
		RELOC_ADD_EXTRA (p) += symptr->value;
#endif
	    }
	  else
	    /* Debugger symbols come first, so have to start this
	       after them.  */
	      RELOC_SYMBOL(p) = (symptr->def_count + nsyms
				 - defined_global_sym_count
				 - undefined_global_sym_count
				 - global_indirect_count);
	}
      p++;
    }

  mywrite (entry->textrel, 1, entry->text_reloc_size, outdesc);
}

void
copdatrel (entry)
     struct file_entry *entry;
{
  register struct relocation_info *p, *end;
  /* Relocate the address of the relocation.
     Old address is relative to start of the input file's data section.
     New address is relative to start of the output file's data section.

     So the amount we need to relocate it by is the offset of this
     input file's data section within the output file's data section.  */
  register int reloc = entry->data_start_address - data_start;

  p = entry->datarel;
  end = (struct relocation_info *) (entry->data_reloc_size + (char *) p);
  while (p < end)
    {
      RELOC_ADDRESS(p) += reloc;
      if (RELOC_EXTERN_P(p))
	{
	  register int symindex = RELOC_SYMBOL(p);
	  symbol *symptr = (symbol *)(entry->symbols [symindex].n_un.n_name);
	  int symtype;

	  if (symindex >= (entry->syms_size / sizeof (struct nlist)))
	    fatal_with_file ("relocation symbolnum out of range in ", entry);

#ifdef N_INDR
	  /* Resolve indirection.  */
	  if ((symptr->defined & ~N_EXT) == N_INDR)
	    symptr = (symbol *) symptr->value;
#endif

	  symtype = symptr->defined & ~N_EXT;

	  if (force_common_definition
	      || (reloc_flag && symtype != N_IMP1)
	      || symtype == N_DATA || symtype == N_TEXT
              || symtype == N_ABS || symtype == N_BSS)
	    {
	      if (symtype == N_SETV)
		symtype = N_DATA;
	      RELOC_EXTERN_P(p) = 0;
	      RELOC_SYMBOL(p) = symtype;
	    }
	  else
	    /* Debugger symbols come first, so have to start this
	       after them.  */
	    RELOC_SYMBOL(p) = (symptr->def_count
		 + nsyms - defined_global_sym_count
		 - undefined_global_sym_count
		 - global_indirect_count);

#if 0
          if ((symtype = symptr->defined) != (N_IMP1 | N_EXT))
	    symtype = symptr->defined & ~N_EXT;

	  if ((force_common_definition || reloc_flag)
           && (symtype != (N_IMP1 | N_EXT)))
          {
            if (force_common_definition
                || symtype == N_DATA || symtype == N_TEXT || symtype == N_ABS)
              {
                if (symtype == N_SETV)
                  symtype = N_DATA;
                RELOC_EXTERN_P(p) = 0;
                RELOC_SYMBOL(p) = symtype;
              }
            else
              /* Debugger symbols come first, so have to start this
                 after them.  */
              RELOC_SYMBOL(p) = (symptr->def_count
                   + nsyms - defined_global_sym_count
                   - undefined_global_sym_count
                   - global_indirect_count);
          }
#endif
	}
      p++;
    }

  mywrite (entry->datarel, 1, entry->data_reloc_size, outdesc);
}

void write_file_syms (struct file_entry *entry, int *syms_written_addr);
void write_string_table (void);

/* Total size of string table strings allocated so far,
   including strings in `strtab_vector'.  */
int strtab_size;

/* Vector whose elements are strings to be added to the string table.  */
char **strtab_vector;

/* Vector whose elements are the lengths of those strings.  */
int *strtab_lens;

/* Index in `strtab_vector' at which the next string will be stored.  */
int strtab_index;

/* Add the string NAME to the output file string table.
   Record it in `strtab_vector' to be output later.
   Return the index within the string table that this string will have.  */

int
assign_string_table_index (name)
     char *name;
{
  register int index = strtab_size;
  register int len = strlen (name) + 1;

  strtab_size += len;
  strtab_vector[strtab_index] = name;
  strtab_lens[strtab_index++] = len;

  return index;
}

FILE *outstream = (FILE *) 0;

/* Write the contents of `strtab_vector' into the string table.
   This is done once for each file's local&debugger symbols
   and once for the global symbols.  */

void
write_string_table (void)
{
  register int i;

  lseek (outdesc, output_strs_offset + output_strs_size, 0);

  if (!outstream)
    outstream = fdopen (outdesc, "wb");

  for (i = 0; i < strtab_index; i++)
    {
      fwrite (strtab_vector[i], 1, strtab_lens[i], outstream);
      output_strs_size += strtab_lens[i];
    }

  fflush (outstream);

  /* Report I/O error such as disk full.  */
  if (ferror (outstream))
    perror_name (output_filename);
}

/* Write the symbol table and string table of the output file.  */

void
write_syms ()
{
  /* Number of symbols written so far.  */
  int syms_written = 0;
  register int i;
  register symbol *sp;

  /* Buffer big enough for all the global symbols.  One
     extra struct for each indirect symbol to hold the extra reference
     following. */
  struct nlist *buf = (struct nlist *) ALLOCA ((defined_global_sym_count
				+ undefined_global_sym_count
				+ global_indirect_count)
				* sizeof (struct nlist));
  /* Pointer for storing into BUF.  */
  register struct nlist *bufp = buf;

  /* Size of string table includes the bytes that store the size.  */
  strtab_size = sizeof strtab_size;

  output_syms_size = 0;
  output_strs_size = strtab_size;

  if (strip_symbols == STRIP_ALL)
    return;

  /* Write the local symbols defined by the various files.  */

  each_file (write_file_syms, (int)&syms_written);
  file_close ();

  /* Now write out the global symbols.  */

  /* Allocate two vectors that record the data to generate the string
     table from the global symbols written so far.  This must include
     extra space for the references following indirect outputs. */

  strtab_vector = (char **) ALLOCA ((num_hash_tab_syms
				     + global_indirect_count) * sizeof (char *));
  strtab_lens = (int *) ALLOCA ((num_hash_tab_syms
				 + global_indirect_count) * sizeof (int));
  strtab_index = 0;

  /* Scan the symbol hash table, bucket by bucket.  */

  for (i = 0; i < TABSIZE; i++)
    for (sp = symtab[i]; sp; sp = sp->link)
      {
	struct nlist nl;

#ifdef N_SECT
	nl.n_sect = 0;
#else
	nl.n_other = 0;
#endif
	nl.n_desc = 0;

	/* Compute a `struct nlist' for the symbol.  */

	if (sp->defined || sp->referenced)
	  {
	    /* common condition needs to be before undefined condition */
	    /* because unallocated commons are set undefined in */
	    /* digest_symbols */
	    if (sp->defined > 1) /* defined with known type */
	      {
		/* If the target of an indirect symbol has been
		   defined and we are outputting an executable,
		   resolve the indirection; it's no longer needed */
		if (output_style != OUTPUT_RELOCATABLE
		    && ((sp->defined & ~N_EXT) == N_INDR)
		    && (((symbol *) sp->value)->defined > 1))
		  {
		    symbol *newsp = (symbol *) sp->value;
		    nl.n_type = newsp->defined;
		    nl.n_value = newsp->value;
		  }
		else
		  {
		    nl.n_type = sp->defined;
		    if (sp->defined != (N_INDR | N_EXT))
		      nl.n_value = sp->value;
		    else
		      nl.n_value = 0;
		  }
	      }
	    else if (sp->max_common_size) /* defined as common but not allocated. */
	      {
		/* happens only with -r and not -d */
		/* write out a common definition */
		nl.n_type = N_UNDF | N_EXT;
		nl.n_value = sp->max_common_size;
	      }
            else if (!sp->defined && sp->weak)
	      {
		nl.n_type = sp->weak;
		nl.n_value = 0;
	      }
	    else if (!sp->defined)	      /* undefined -- legit only if -r */
	      {
		nl.n_type = N_UNDF | N_EXT;
		nl.n_value = 0;
	      }
	    else
	      fatal ("internal error: %s defined in mysterious way", sp->name);

	    /* Allocate string table space for the symbol name.  */

	    nl.n_un.n_strx = assign_string_table_index (sp->name);

	    /* Output to the buffer and count it.  */

	    *bufp++ = nl;
	    syms_written++;
	    if (nl.n_type == (N_INDR | N_EXT))
	      {
		struct nlist xtra_ref;
		xtra_ref.n_type = N_EXT | N_UNDF;
		xtra_ref.n_un.n_strx
		  = assign_string_table_index (((symbol *) sp->value)->name);
#ifdef N_SECT
		xtra_ref.n_sect = 0;
#else
		xtra_ref.n_other = 0;
#endif
		xtra_ref.n_desc = 0;
		xtra_ref.n_value = 0;
		*bufp++ = xtra_ref;
		syms_written++;
	      }
	  }
      }

  /* Output the buffer full of `struct nlist's.  */

  lseek (outdesc, output_syms_offset + output_syms_size, 0);
  mywrite (buf, sizeof (struct nlist), bufp - buf, outdesc);
  output_syms_size += sizeof (struct nlist) * (bufp - buf);

  if (syms_written != nsyms)
    fatal ("internal error: wrong number of symbols written into output file", 0);

  /* Now the total string table size is known, so write it into the
     first word of the string table.  */

  lseek (outdesc, output_strs_offset, 0);
  mywrite (&strtab_size, sizeof (int), 1, outdesc);

  /* Write the strings for the global symbols.  */

  write_string_table ();
  FREEA (strtab_vector);
  FREEA (strtab_lens);
  FREEA (buf);
}

/* Write the local and debugger symbols of file ENTRY.
   Increment *SYMS_WRITTEN_ADDR for each symbol that is written.  */

/* Note that we do not combine identical names of local symbols.
   dbx or gdb would be confused if we did that.  */

void
write_file_syms (entry, syms_written_addr)
     struct file_entry *entry;
     int *syms_written_addr;
{
  register struct nlist *p = entry->symbols;
  register struct nlist *end = p + entry->syms_size / sizeof (struct nlist);

  /* Buffer to accumulate all the syms before writing them.
     It has one extra slot for the local symbol we generate here.  */
  struct nlist *buf
    = (struct nlist *) ALLOCA (entry->syms_size + sizeof (struct nlist));
  register struct nlist *bufp = buf;

  /* Upper bound on number of syms to be written here.  */
  int max_syms = (entry->syms_size / sizeof (struct nlist)) + 1;

  /* Make tables that record, for each symbol, its name and its name's length.
     The elements are filled in by `assign_string_table_index'.  */

  strtab_vector = (char **) ALLOCA (max_syms * sizeof (char *));
  strtab_lens = (int *) ALLOCA (max_syms * sizeof (int));
  strtab_index = 0;

  /* Generate a local symbol for the start of this file's text.  */

  if (discard_locals != DISCARD_ALL)
    {
      struct nlist nl;

      nl.n_type = N_TEXT;
#ifdef EMX /* fix GCC/ld/GDB problem */
      {
        char *tmp;
        int len;

        tmp = entry->local_sym_name;
        len = strlen (tmp);
        if (strncmp (tmp, "-l", 2) != 0
            && (len < 2 || strcmp (tmp+len-2, ".o") != 0))
          tmp = concat (tmp, ".o", NULL);   /* Needed by GDB */
        nl.n_un.n_strx = assign_string_table_index (tmp);
      }
#else /* !EMX */
      nl.n_un.n_strx = assign_string_table_index (entry->local_sym_name);
#endif /* !EMX */
      nl.n_value = entry->text_start_address;
      nl.n_desc = 0;
#ifdef N_SECT
      nl.n_sect = 0;
#else
      nl.n_other = 0;
#endif
      *bufp++ = nl;
      (*syms_written_addr)++;
      entry->local_syms_offset = *syms_written_addr * sizeof (struct nlist);
    }

  /* Read the file's string table.  */

  entry->strings = (char *) ALLOCA (entry->strs_size);
  read_entry_strings (file_open (entry), entry);

  for (; p < end; p++)
    {
      register int type = p->n_type;
      register int write = 0;

      /* WRITE gets 1 for a non-global symbol that should be written.  */

      if (SET_ELEMENT_P (type))	/* This occurs even if global.  These */
				/* types of symbols are never written */
				/* globally, though they are stored */
				/* globally.  */
        write = output_style == OUTPUT_RELOCATABLE;
      else if (WEAK_SYMBOL (type))
        ;
      else if (!(type & (N_STAB | N_EXT)))
        /* ordinary local symbol */
	write = ((discard_locals != DISCARD_ALL)
		 && !(discard_locals == DISCARD_L &&
		      (p->n_un.n_strx + entry->strings)[0] == LPREFIX)
		 && type != N_WARNING);
      else if (!(type & N_EXT))
	/* debugger symbol */
        write = (strip_symbols == STRIP_NONE);

      if (write)
	{
	  /* If this symbol has a name,
	     allocate space for it in the output string table.  */

	  if (p->n_un.n_strx)
	    p->n_un.n_strx = assign_string_table_index (p->n_un.n_strx
							+ entry->strings);

	  /* Output this symbol to the buffer and count it.  */

	  *bufp++ = *p;
	  (*syms_written_addr)++;
	}
    }

  /* All the symbols are now in BUF; write them.  */

  lseek (outdesc, output_syms_offset + output_syms_size, 0);
  mywrite (buf, sizeof (struct nlist), bufp - buf, outdesc);
  output_syms_size += sizeof (struct nlist) * (bufp - buf);

  /* Write the string-table data for the symbols just written,
     using the data in vectors `strtab_vector' and `strtab_lens'.  */

  write_string_table ();
  FREEA (entry->strings);
  FREEA (strtab_vector);
  FREEA (strtab_lens);
  FREEA (buf);
}

/* Copy any GDB symbol segments from the input files to the output file.
   The contents of the symbol segment is copied without change
   except that we store some information into the beginning of it.  */

void write_file_symseg (struct file_entry *entry);

void
write_symsegs ()
{
  lseek (outdesc, output_symseg_offset, 0);
  each_file (write_file_symseg, 0);
}

void
write_file_symseg (entry)
     struct file_entry *entry;
{
  char buffer[4096];
  struct symbol_root root;
  int indesc, len, total;

  if (entry->symseg_size == 0)
    return;

  output_symseg_size += entry->symseg_size;

  /* This entry has a symbol segment.  Read the root of the segment.  */

  indesc = file_open (entry);
  lseek (indesc, entry->symseg_offset + entry->starting_offset, 0);
  if (sizeof root != read (indesc, &root, sizeof root))
    fatal_with_file ("premature end of file in symbol segment of ", entry);

  /* Store some relocation info into the root.  */

  root.ldsymoff = entry->local_syms_offset;
  root.textrel = entry->text_start_address - entry->orig_text_address;
  root.datarel = entry->data_start_address - entry->orig_data_address;
  root.bssrel = entry->bss_start_address - entry->orig_bss_address;
  root.databeg = entry->data_start_address - root.datarel;
  root.bssbeg = entry->bss_start_address - root.bssrel;

  /* Write the modified root into the output file.  */

  mywrite (&root, sizeof root, 1, outdesc);

  /* Copy the rest of the symbol segment unchanged.  */

  total = entry->symseg_size - sizeof root;

  while (total > 0)
    {
      len = read (indesc, buffer, min (sizeof buffer, total));

      if (len != min (sizeof buffer, total))
	fatal_with_file ("premature end of file in symbol segment of ", entry);
      total -= len;
      mywrite (buffer, len, 1, outdesc);
    }

  file_close ();
}

/* Define a special symbol (etext, edata, or end).  NAME is the
   name of the symbol, with a leading underscore (whether or not this
   system uses such underscores).  TYPE is its type (e.g. N_DATA | N_EXT).
   Store a symbol * for the symbol in *SYM if SYM is non-NULL.  */
static void
symbol_define (name, type, sym)
     /* const */ char *name;
     int type;
     symbol **sym;
{
  symbol *thesym;

#if defined(nounderscore)
  /* Skip the leading underscore.  */
  name++;
#endif

  thesym = getsym (name);
  if (thesym->defined)
    {
      /* The symbol is defined in some input file.  Don't mess with it.  */
      if (sym)
	*sym = 0;
    }
  else
    {
      if (thesym->referenced)
	/* The symbol was not defined, and we are defining it now.  */
	undefined_global_sym_count--;
      thesym->defined = type;
      thesym->referenced = 1;
      if (sym)
	*sym = thesym;
    }
}

/* Create the symbol table entries for `etext', `edata' and `end'.  */

void
symtab_init ()
{
  symbol_define ("_edata", N_DATA | N_EXT, &edata_symbol);
  symbol_define ("_etext", N_TEXT | N_EXT, &etext_symbol);
  symbol_define ("_end", N_BSS | N_EXT, &end_symbol);

  /* Either _edata or __edata (C names) is OK as far as ANSI is concerned
     (see section 4.1.2.1).  In general, it is best to use __foo and
     not worry about the confusing rules for the _foo namespace.
     But HPUX 7.0 uses _edata, so we might as weel be consistent.  */
  symbol_define ("__edata", N_DATA | N_EXT, &edata_symbol_alt);
  symbol_define ("__etext", N_TEXT | N_EXT, &etext_symbol_alt);
  symbol_define ("__end", N_BSS | N_EXT, &end_symbol_alt);
}

/* Compute the hash code for symbol name KEY.  */

int
hash_string (key)
     char *key;
{
  register char *cp;
  register int k;

  cp = key;
  k = 0;
  while (*cp)
    k = (((k << 1) + (k >> 14)) ^ (*cp++)) & 0x3fff;

  return k;
}

/* Get the symbol table entry for the global symbol named KEY.
   Create one if there is none.  */

symbol *
getsym (key)
     char *key;
{
  register int hashval;
  register symbol *bp;

  /* Determine the proper bucket.  */

  hashval = hash_string (key) % TABSIZE;

  /* Search the bucket.  */

  for (bp = symtab[hashval]; bp; bp = bp->link)
    if (! strcmp (key, bp->name))
      return bp;

  /* Nothing was found; create a new symbol table entry.  */

  bp = (symbol *) xmalloc (sizeof (symbol));
  bp->refs = 0;
  bp->name = (char *) xmalloc (strlen (key) + 1);
  strcpy (bp->name, key);
  bp->defined = 0;
  bp->referenced = 0;
  bp->trace = 0;
  bp->value = 0;
  bp->max_common_size = 0;
  bp->warning = 0;
  bp->undef_refs = 0;
  bp->multiply_defined = 0;
  bp->weak = 0;

  /* Add the entry to the bucket.  */

  bp->link = symtab[hashval];
  symtab[hashval] = bp;

  ++num_hash_tab_syms;

  return bp;
}

/* Like `getsym' but return 0 if the symbol is not already known.  */

symbol *
getsym_soft (key)
     char *key;
{
  register int hashval;
  register symbol *bp;

  /* Determine which bucket.  */

  hashval = hash_string (key) % TABSIZE;

  /* Search the bucket.  */

  for (bp = symtab[hashval]; bp; bp = bp->link)
    if (! strcmp (key, bp->name))
      return bp;

  return 0;
}

/* Report a usage error.
   Like fatal except prints a usage summary.  */

void
usage (string, arg)
     char *string, *arg;
{
  if (string)
    {
      fprintf (stderr, "%s: ", progname);
      fprintf (stderr, string, arg);
      fprintf (stderr, "\n");
    }
  fprintf (stderr, "\
Usage: %s [-d] [-dc] [-dp] [-e symbol] [-l lib] [-n] [-noinhibit-exec]\n\
       [-nostdlib] [-o file] [-r] [-s] [-t] [-u symbol] [-x] [-y symbol]\n\
       [-z] [-A file] [-Bstatic] [-D size] [-L libdir] [-M] [-N]\n\
       [-S] [-T[{text,data}] addr] [-V prefix] [-X] [-Zdll-search]\n\
       [file...]\n", progname);
  exit (1);
}

/* Report a fatal error.
   STRING is a printf format string and ARG is one arg for it.  */

void fatal (char *string, ...)
{
  va_list args;
  fprintf (stderr, "%s: ", progname);
  va_start (args, string);
  vfprintf (stderr, string, args);
  va_end (args);
  fprintf (stderr, "\n");
  exit (1);
}

/* Report a fatal error.  The error message is STRING
   followed by the filename of ENTRY.  */

void
fatal_with_file (string, entry)
     char *string;
     struct file_entry *entry;
{
  fprintf (stderr, "%s: ", progname);
  fprintf (stderr, string);
  print_file_name (entry, stderr);
  fprintf (stderr, "\n");
  exit (1);
}

/* Report a fatal error using the message for the last failed system call,
   followed by the string NAME.  */

void
perror_name (name)
     char *name;
{
  char *s;

  if (errno < sys_nerr)
    s = concat ("", sys_errlist[errno], " for %s", NULL);
  else
    s = "cannot open %s";
  fatal (s, name);
}

/* Report a fatal error using the message for the last failed system call,
   followed by the name of file ENTRY.  */

void
perror_file (entry)
     struct file_entry *entry;
{
  char *s;

  if (errno < sys_nerr)
    s = concat ("", sys_errlist[errno], " for ", NULL);
  else
    s = "cannot open ";
  fatal_with_file (s, entry);
}

/* Report a nonfatal error.
   STRING is a format for printf, and ARG1 ... ARG3 are args for it.  */

void
error (string, arg1, arg2, arg3)
     char *string, *arg1, *arg2, *arg3;
{
  fprintf (stderr, "%s: ", progname);
  fprintf (stderr, string, arg1, arg2, arg3);
  fprintf (stderr, "\n");
}


/* Output COUNT*ELTSIZE bytes of data at BUF
   to the descriptor DESC.  */

void
mywrite (buf, count, eltsize, desc)
     void *buf;
     int count;
     int eltsize;
     int desc;
{
  register int val;
  register int bytes = count * eltsize;

  while (bytes > 0)
    {
      val = write (desc, buf, bytes);
      if (val <= 0)
	perror_name (output_filename);
      buf = (char*)buf + val;
      bytes -= val;
    }
}

/* Output PADDING zero-bytes to descriptor OUTDESC.
   PADDING may be negative; in that case, do nothing.  */

void
padfile (padding, outdesc)
     int padding;
     int outdesc;
{
  register char *buf;
  if (padding <= 0)
    return;

  buf = (char *) ALLOCA (padding);
  bzero (buf, padding);
  mywrite (buf, padding, 1, outdesc);
  FREEA (buf);
}

/* Parse the string ARG using scanf format FORMAT, and return the result.
   If it does not parse, report fatal error
   generating the error message using format string ERROR and ARG as arg.  */

int
parse (arg, format, error)
     char *arg, *format, *error;
{
  int x;
  if (1 != sscanf (arg, format, &x))
    fatal (error, arg);
  return x;
}

/* Like malloc but get fatal error if memory is exhausted.  */

void *
xmalloc (size)
     size_t size;
{
  register char *result = malloc (size);
  if (!result)
    fatal ("virtual memory exhausted", 0);
  return result;
}

/* Like realloc but get fatal error if memory is exhausted.  */

void *
xrealloc (ptr, size)
     void *ptr;
     size_t size;
{
  register char *result = realloc (ptr, size);
  if (!result)
    fatal ("virtual memory exhausted", 0);
  return result;
}

char *my_cplus_demangle (const char *mangled)
{
  if (*mangled == '_')
    ++mangled;
  return cplus_demangle (mangled, demangle_options);
}


/**
 * Generates an unique temporary file.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pszFile     Where to put the filename.
 * @param   pszPrefix   Prefix.
 * @param   pszSuffix   Suffix.
 * @remark  The code is nicked from the weak linker.
 * @remark sorry about the convention here, this is borrowed from elsewhere.
 */
static int      make_tempfile(char *pszFile, const char *pszPrefix, const char *pszSuffix)
{
    struct stat     s;
    unsigned        c = 0;
    pid_t           pid = getpid();
    const char *    pszTmp = getenv("TMP");
    if (!pszTmp)    pszTmp = getenv("TMPDIR");
    if (!pszTmp)    pszTmp = getenv("TEMP");
    if (!pszTmp)    pszTmp = ".";

    do
    {
        struct timeval  tv = {0,0};
        if (c++ >= 200)
            return -1;
        gettimeofday(&tv, NULL);
        sprintf(pszFile, "%s\\%s%x%lx%d%lx%s", pszTmp, pszPrefix, pid, tv.tv_sec, c, tv.tv_usec, pszSuffix);
    } while (!stat(pszFile, &s));

    return 0;
}

/* list of converted libraries and objects which must be removed upon exit. */
struct lx_tmp
{
    struct lx_tmp *next;
    char *name;
}   *conv_list = NULL;

/** Converts a .DLL to an temporary import library.
 * @remark sorry about the convention here, this is borrowed from elsewhere.
 */
char *lx_to_aout (const char *pszFilename)
{
    int             rc;
    char *          pszNewFile;
    struct lx_tmp  *pName;

    /*
     * Make temporary file.
     */
    pName = (struct lx_tmp *)xmalloc(sizeof(*pName));
    pName->name = pszNewFile = xmalloc(_MAX_PATH);
    if (make_tempfile(pszNewFile, "ldconv", ".a"))
    {
        free(pszNewFile);
        return NULL;
    }

    /*
     * Do the conversion.
     */
    rc = spawnlp(P_WAIT, "emximp.exe", "emximp.exe",
                 "-o", pszNewFile, pszFilename, NULL);
    if (!rc)
    {
        /* add to auto delete list for removal on exit(). */
        pName->next = conv_list;
        conv_list = pName;
        return pName->name;

    }
    free(pszNewFile);
    free(pName);

    fprintf (stderr, "emxomfld: a.out to omf conversion failed for '%s'.\n",
             pszFilename);
    exit (2);
    return NULL;
}


/**
 * Checks if the file FD is an LX DLL.
 *
 * @returns 1 if LX DLL.
 * @returns 0 if not LX DLL.
 * @param   fd  Handle to file to check.
 */
int check_lx_dll(int fd)
{
    unsigned long   ul;
    char            achMagic[2];

    if (    lseek(fd, 0, SEEK_SET)
        ||  read(fd, &achMagic, 2) != 2)
        goto thats_not_it;

    if (!memcmp(achMagic, "MZ", 2))
    {
        if (    lseek(fd, 0x3c, SEEK_SET)
            ||  read(fd, &ul, 4) != 4 /* offset of the 'new' header */
            ||  ul < 0x40
            ||  ul >= 0x10000000 /* 512MB stubs sure */
            ||  lseek(fd, ul, SEEK_SET)
            ||  read(fd, &achMagic, 2) != 2)
            goto thats_not_it;
    }

    if (    memcmp(achMagic, "LX", 2)
        ||  lseek(fd, 14, SEEK_CUR)
        ||  read(fd, &ul, 4) != 4) /*e32_mflags*/
        goto thats_not_it;

#define E32MODDLL        0x08000L
#define E32MODPROTDLL    0x18000L
#define E32MODMASK       0x38000L
    if (   (ul & E32MODMASK) != E32MODDLL
        && (ul & E32MODMASK) != E32MODPROTDLL)
        goto thats_not_it;

    /* it's a LX DLL! */
    lseek(fd,  0, SEEK_SET);
    return 1;


thats_not_it:
    lseek(fd, 0, SEEK_SET);
    return 0;
}

/* Generates a definition file for a dll which doesn't have one. */

static void gen_deffile(void)
{
    char *          psz;
    struct lx_tmp  *pName;

    /*
     * Make temporary file.
     */
    pName = (struct lx_tmp *)xmalloc(sizeof(*pName));
    pName->name = psz = (char *)xmalloc(_MAX_PATH);
    if (!make_tempfile(psz, "lddef", ".def"))
    {
        FILE *pFile = fopen(psz, "w");
        if (pFile)
        {
            const char *pszName = _getname(exe_filename);
            size_t cchName = strlen(pszName);
            if (cchName > 4 && !stricmp(pszName + cchName - 4, ".dll"))
                cchName -= 4;
            fprintf(pFile,
                    ";; Autogenerated by ld\n"
                    "LIBRARY %.*s INITINSTANCE TERMINSTANCE\n"
                    "DATA MULTIPLE\n"
                    "CODE SHARED\n"
                    "\n",
                    cchName, pszName);
            if (trace_files)
                fprintf(stderr,
                        "--- Generated def-file %s:\n"
                        ";; Autogenerated by ld\n"
                        "LIBRARY %.*s INITINSTANCE TERMINSTANCE\n"
                        "DATA MULTIPLE NONSHARED\n"
                        "CODE SINGLE SHARED\n"
                        "---- End of generated def-file.\n",
                        psz, cchName, pszName);
            fclose(pFile);
            def_filename = psz;

            /* add to auto delete list for removal on exit(). */
            pName->next = conv_list;
            conv_list = pName;
            return;
        }
    }
    free(psz);
    free(pName);
}


/* atexit worker */
void cleanup (void)
{
  for (; conv_list; conv_list = conv_list->next)
    unlink (conv_list->name);
}
