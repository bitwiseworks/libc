/* stabshll.c -- Convert GNU-style a.out debug information into
                 IBM OS/2 HLL 3 debug information
   Copyright (c) 1993-1998 Eberhard Mattes

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
//#define HLL_DEBUG 1

#define DEMANGLE_PROC_NAMES 1

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <alloca.h>
#include <ctype.h>
#ifdef DEMANGLE_PROC_NAMES
# include <demangle.h>
#endif
#include "defs.h"
#include "emxomf.h"
#include "grow.h"
#include "stabshll.h"


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/* Field ID values for the type table. */

#define FID_nil         0x80    /* No data */
#define FID_void        0x81    /* Type void */
#define FID_string      0x82    /* A length-prefixed string follows */
#define FID_index       0x83    /* A type index follows */
#define FID_span_16u    0x85    /* An unsigned 16-bit number follows */
#define FID_span_32u    0x86    /* An unsigned 32-bit number follows */
#define FID_span_8s     0x88    /* A signed 8-bit number follows */
#define FID_span_16s    0x89    /* A signed 16-bit number follows */
#define FID_span_32s    0x8a    /* A signed 32-bit number follows */
#define FID_span_8u     0x8b    /* An unsigned 8-bit number follows */

/* Static scope table subrecord types. */

#define SST_begin       0x00    /* Begin block */
#define SST_proc        0x01    /* Procedure */
#define SST_end         0x02    /* End block or end procedure */
#define SST_auto        0x04    /* Scoped variable */
#define SST_static      0x05    /* Static variable */
#define SST_reg         0x0d    /* Register variable */
#define SST_changseg    0x11    /* Change default segment */
#define SST_tag         0x16    /* Structure, union, enum tags */
#define SST_tag2        0x19    /* Extended tag for long names (C++ class). */
#define SST_memfunc     0x1a    /* C++ member function */
#define SST_CPPproc     0x1d    /* C++ function */
#define SST_CPPstatic   0x1e    /* C++ static variable */
#define SST_cuinfo      0x40    /* Compile unit information */

/* Class  visibility. */

#define VIS_PRIVATE     0x00    /* private */
#define VIS_PROTECTED   0x01    /* protected */
#define VIS_PUBLIC      0x02    /* public */

#define MEMBER_VIS      0x03    /* Member visibility mask */
#define MEMBER_FUNCTION 0x08    /* It's a member function */
#define MEMBER_VIRTUAL  0x10    /* Member function / base class is virtual */
#define MEMBER_VOLATILE 0x20    /* Member function is volatile */
#define MEMBER_CONST    0x40    /* Member function is const */
#define MEMBER_STATIC   0x80    /* Member is static */
#define MEMBER_CTOR     0x100   /* Member function is a constructor */
#define MEMBER_DTOR     0x200   /* Member function is a destructor */
#define MEMBER_BASE     0x400   /* It's a base class */

/* Flags for ty_struc. */

#define STRUC_FORWARD   0x01    /* Structure declared forward */


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
enum type_tag
{
  ty_stabs_ref,                 /* Reference to stabs type */
  ty_prim,                      /* Primitive type */
  ty_alias,                     /* Equivalent type */
  ty_pointer,                   /* Pointer type */
  ty_func,                      /* Function */
  ty_args,                      /* Function arguments */
  ty_struc,                     /* Structure or union */
  ty_types,                     /* Type list for structure/union */
  ty_fields,                    /* Field list for structure/union */
  ty_enu,                       /* Enumeration type */
  ty_values,                    /* Enumeration values */
  ty_array,                     /* Array */
  ty_bits,                      /* Bit field / bit string */
  ty_ref,                       /* Reference type (C++) */
  ty_member,                    /* Member of a C++ class */
  ty_class,                     /* A C++ class */
  ty_memfunc,                   /* A C++ member function */
  ty_baseclass,                 /* A C++ base class */
  ty_max
};

/* A structure field. */

struct field
{
  dword offset;                 /* Offset, in bytes */
  const char *name;             /* Field name */
};

/* An enumeration value. */

struct value
{
  long index;                   /* The value */
  const char *name;             /* The name */
};

/* Temporary representation of a structure field. */

struct tmp_field
{
  struct type *type;            /* The internal type */
  dword offset;                 /* The offset of the field */
  const char *name;             /* The field name */
  const char *sname;            /* Static name */
  const char *mnglname;         /* Mangled name for methods. (#456) */
  int flags;                    /* Member flags (visibility) */
};


/* Internal representation of a type. */

struct type
{
  enum type_tag tag;            /* The type of this type */
  int index;                    /* The HLL index for this type */
  struct type *next;            /* All types are chained in a single list */
  struct type *nexthash;        /* All types are chained in the hash list */
  union
    {
      int stabs_ref;            /* Reference a stabs type */
      struct type *pointer;     /* Pointer to another type */
      struct type *alias;       /* Alias for another type */
      struct type *ref;         /* Reference to another type */
      struct
        {                       /* Structure */
          struct type *types;   /* Types of the fields */
          struct type *fields;  /* Field names and offsets */
          dword size;           /* Size of the structure, in bytes */
          word count;           /* Number of fields */
          word flags;           /* Flags (STRUC_FORWARD) */
          const char *name;     /* Name of the structure */
        } struc;
      struct
        {                       /* Class */
          struct type *members; /* Members */
          dword size;           /* Size of the class, in bytes */
          word count;           /* Number of fields */
          const char *name;     /* Name of the class */
        } class;
      struct
        {                       /* Enumeration */
          struct type *type;    /* Base type */
          struct type *values;  /* Values */
          long first;           /* Smallest value */
          long last;            /* Biggest value */
          const char *name;     /* Name of the enum */
        } enu;
      struct
        {                       /* Array */
          struct type *itype;   /* Index type */
          struct type *etype;   /* Element type */
          long first;           /* First index */
          long last;            /* Last index */
        } array;
      struct
        {                       /* Bitfield */
          struct type *type;    /* Base type */
          long start;           /* First bit */
          long count;           /* Number of bits */
        } bits;
      struct
        {                       /* Function */
          struct type *ret;     /* Return type */
          struct type *args;    /* Argument types */
          struct type *domain;  /* Domain type (C++ member) (#456) */
          int arg_count;        /* Number of arguments */
        } func;
      struct
        {                       /* Function arguments */
          struct type **list;   /* Array of argument types */
          int count;            /* Number of arguments */
        } args;
      struct
        {                       /* List of types */
          struct type **list;   /* Array of types */
          int count;            /* Number of types */
        } types;
      struct
        {                       /* Structure fields */
          struct field *list;   /* The fields */
          int count;            /* Number of fields */
        } fields;
      struct
        {                       /* Class member */
          struct type *type;    /* Type of the member */
          dword offset;         /* Offset of the member, in bytes */
          int flags;            /* Member flags (visibility etc.) */
          const char *name;     /* Name of the member */
          const char *sname;    /* Static name */
        } member;
      struct
        {                       /* Enumeration values */
          struct value *list;   /* Array of values */
          int count;            /* Number of values */
        } values;
      struct
        {                       /* Member function */
          struct type *type;    /* Function type */
          dword offset;         /* Offset in the virtual table */
          int flags;            /* Member flags (visibility etc.) */
          const char *name;     /* Name of the member function */
          const char *mnglname; /* Mangled Name of the member function. (#456) */
        } memfunc;
      struct
        {                       /* Base class */
          struct type *type;    /* The base class */
          dword offset;         /* Offset of the base class */
          int flags;            /* Visibility, virtual */
        } baseclass;
    } d;
};


/* A block. */

struct block
{
  dword addr;                   /* Start address of the block */
  size_t patch_ptr;             /* Index into SST buffer for patching */
};

/* Timestamp for the compiler unit information SST subrecord. */

#pragma pack(1)

struct timestamp
{
  byte hours;
  byte minutes;
  byte seconds;
  byte hundredths;
  byte day;
  byte month;
  word year;
};

#pragma pack()


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/* This variable points to the next character of a stabs type being
   parsed. */

static const char *parse_ptr;

/* Start of the stabs string we're parsing. Useful when printing
   messages. */
static const char *parse_start;

/* Current stabs index. Useful when printing messages. */
static int        *parse_pindex;

/* The next HLL type index.  When creating a complex type, this index
   is used.  Then, this variable is incremented. */

static int hll_type_index;

/* We keep all internal types in a single list.  When creating a new
   type, we first look through the list to find out whether an
   identical type already exists.  Doing a linear search is
   inefficient, but that doesn't matter here, according to
   experience.  This variable points to the first type of the list. */

static struct type *type_head;
#define TYPE_HASH_MAX   211
static struct type *atype_tag[TYPE_HASH_MAX];
#ifdef HASH_DEBUG
static int          ctypes = 0;
static int          ctype_tag[TYPE_HASH_MAX];
#endif

/* This growing array keeps track of stabs vs. internal types.  It is
   indexed by stabs type numbers. */

static struct type **stype_list;
static struct grow stype_grow;

/* We collect function arguments in this growing array. */

static struct type **args_list;
static struct grow args_grow;

/* This stack (implemented with a growing array) keeps track of
   begin/end block nesting. */

static struct block *block_stack;
static struct grow block_grow;

/* This string pool contains field names etc. */

static struct strpool *str_pool;

/* This variable holds the sym_ptr index of the most recently
   encountered N_LBRAC symbol. */

static int lbrac_index;

/* The index (in sst) of the most recently started SST subrecord. */

static size_t subrec_start;

/* The proc_patch_base contains the index where the base address and
   function length are to be patched into the SST. */

static size_t proc_patch_base;

/* The base address used by the most recently proc record in the
   SST. */

static size_t proc_start_addr;

/* The internal type of the most recently defined function.  Valid
   only if last_fun_valid is TRUE. */

static struct type last_fun_type;

/* This variable is TRUE if a function has recently been defined. */

static int last_fun_valid;

/* The length of the prologue of the most recently defined
   function. */

static int prologue_length;

/* The address of the most recently defined function. */

static dword last_fun_addr;

/* This variable is non-zero when converting a translated C++
   program. */

static int cplusplus_flag;

/* Unnamed structures are numbered sequentially, using this
   counter. */

static int unnamed_struct_number;

/* kso #456 2003-06-11: Reversed quiet workaround. */
#define no_warning warning_parse


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void parse_typedef (int *index);
static void warning_parse (const char *pszFormat, ...);


/**
 * Report an warning in which might be connecting to parsing.
 *
 * @param   pszFormat   Message string.
 * @param   ...         More arguments
 */
static void warning_parse (const char *pszFormat, ...)
{
  va_list args;

  if (warning_level < 2)
    {
      static const char *s_pszComplainedFor = NULL;
      if (warning_level == 1 && s_pszComplainedFor == error_fname)
        {
          s_pszComplainedFor = error_fname;
          fprintf (stderr, "emxomf warning: debug info conversion issues for '%s'. (-v for details.)\n", error_fname);
        }
    }
  else
    {
      va_start(args, pszFormat);
      fprintf (stderr, "emxomf warning: ");
      vfprintf (stderr, pszFormat, args);
      va_end (args);
      fputc ('\n', stderr);

      if (parse_ptr && parse_start && parse_ptr >= parse_start)
        {
          if (parse_pindex && *parse_pindex >= 0 && *parse_pindex < sym_count)
            fprintf (stderr, "emxomf info: parsing sym no.%d type=%d at char '%c' in position %d:\n%s\n",
                     *parse_pindex, sym_ptr[*parse_pindex].n_type,
                     *parse_ptr, parse_ptr - parse_start, parse_start);
          else
            fprintf (stderr, "emxomf info: parsing '%c' at position %d:\n%s\n",
                     *parse_ptr, parse_ptr - parse_start, parse_start);
        }
    }
}


/* Start a type table record of type TYPE, with type qualifier QUAL.
   tt_end() must be called to finish the record after writing the
   record contents. */

static void tt_start (byte type, byte qual)
{
  subrec_start = tt.size;
  grow_by (&tt_boundary_grow, 1);
  tt_boundary[tt_boundary_grow.count++] = tt.size;
  buffer_word (&tt, 0);    /* Length of sub-record */
  buffer_byte (&tt, type); /* Type of sub-record */
  buffer_byte (&tt, qual); /* Type qualifier */
}


/* Finish a type table record.  This function must be called after
   tt_start(). */

static void tt_end (void)
{
  *(word *)(tt.buf + subrec_start) = tt.size - subrec_start - 2;
}


/* Put the type index TI into the type table.  TI = -1 encodes the
   `nil' type (no type), 0x97 encodes the `void' type.  All other
   values are type indices. */

static void tt_index (int ti)
{
  if (ti == -1)                 /* nil */
    buffer_byte (&tt, FID_nil);
  else if (ti == 0x97)          /* void */
    buffer_byte (&tt, FID_void);
  else
    {
      buffer_byte (&tt, FID_index);
      buffer_word (&tt, ti);
    }
}


/* Put an 8-bit byte into the type table. */

static void tt_byte (byte x)
{
  buffer_byte (&tt, x);
}


/* Put a 16-bit word into the type table. */

static void tt_word (word x)
{
  buffer_word (&tt, x);
}


/* Put a 32-bit double word into the type table. */

static void tt_dword (dword x)
{
  buffer_dword (&tt, x);
}


/* Put the name S into the type table. */

static void tt_name (const char *s)
{
  buffer_byte (&tt, FID_string);
  buffer_nstr (&tt, s);
}


/* Put the mangled name S into the type table. */

static void tt_enc (const char *s)
{
  buffer_enc (&tt, s);
}


/* Put the unsigned number N into the type table.  Depending on the
   magnitude of N, different encodings are used. */

static void tt_unsigned_span (dword n)
{
  if (n <= 0xff)
    {
      buffer_byte (&tt, FID_span_8u);
      buffer_byte (&tt, n);
    }
  else if (n <= 0xffff)
    {
      buffer_byte (&tt, FID_span_16u);
      buffer_word (&tt, n);
    }
  else
    {
      buffer_byte (&tt, FID_span_32u);
      buffer_dword (&tt, n);
    }
}


/* Put the signed number N into the type table.  Depending on the
   magnitude of N, different encodings are used. */

static void tt_signed_span (long n)
{
  if (-127 <= n && n <= 127)
    {
      buffer_byte (&tt, FID_span_8s);
      buffer_byte (&tt, n);
    }
  else if (-32767 <= n && n <= 32767)
    {
      buffer_byte (&tt, FID_span_16s);
      buffer_word (&tt, n);
    }
  else
    {
      buffer_byte (&tt, FID_span_32s);
      buffer_dword (&tt, n);
    }
}


/* Start a static scope table subrecord of type TYPE.  sst_end() must
   be called to finish the subrecord after writing the subrecord
   data. */

static void sst_start (byte type)
{
  subrec_start = sst.size;
  grow_by (&sst_boundary_grow, 1);
  sst_boundary[sst_boundary_grow.count++] = sst.size;
  /* Length of sub-record - bird: not sure if HL03 likes this. */
  buffer_byte (&sst, 0x80);
  buffer_byte (&sst, 0);
  /* Type of sub-record */
  buffer_byte (&sst, type);
}


/* Finish a static scope table subrecord.  This function must be
   called after sst_start(). */

static void sst_end (void)
{
  int len = sst.size - subrec_start - 2;
  /* -- experimental...
  if (len > 1000)
    error ("sst_end: Record too big");
  */
  sst.buf[subrec_start] = (len >> 8) | 0x80;
  sst.buf[subrec_start + 1] = 0xff & len;
}


/* Define a static variable in the static scope table.  NAME is the
   name of the variable, ADDR is the address of the variable (segment
   offset), TYPE_INDEX is the HLL type index.  If EXT is zero, SYM_SEG
   is the segment (N_DATA, for instance).  If EXT is non-zero, SYM_SEG
   is the stabs symbol index (for the relocation table).  This
   function also creates appropriate relocation table entries. */

static void sst_static (const char *name, dword addr, int type_index,
                        int sym_seg, int ext, const struct nlist *psym)
{
  struct relocation_info r;
  size_t len = strlen (name);

  if (!psym)
    {
      psym = find_symbol_ex (name, -1, ext);
      if (!psym)
        {
          char *psz = alloca (len + 2);
          *psz = '_';
          memcpy(psz + 1, name, len + 1);
          psym = find_symbol_ex (psz, -1, ext);
        }
    }
  if (psym)
    set_hll_type (psym - sym_ptr, type_index);

  if (len > 255)
    sst_start (SST_CPPstatic);
  else
    sst_start (SST_static);
  r.r_address = sst.size;
  buffer_dword (&sst, addr);      /* Segment offset */
  buffer_word (&sst, 0);          /* Segment address */
  buffer_word (&sst, type_index); /* Type index */
  if (len > 255)
    buffer_enc (&sst, name);      /* Symbol name - encoded. */
  else
    buffer_nstr (&sst, name);     /* Symbol name */
  sst_end ();
  r.r_extern = ext;
  r.r_length = 2;
  r.r_symbolnum = sym_seg;
  r.r_pcrel = 0;
  r.r_pad = 0;
  buffer_mem (&sst_reloc, &r, sizeof (r));
  r.r_address += 4;
  r.r_length = 1;
  buffer_mem (&sst_reloc, &r, sizeof (r));
}

/**
 * Try calculate a good hash (yeah sure).
 *
 * @returns hash index.
 * @param   t     type
 */
static inline unsigned type_hash (struct type *t)
{
  register unsigned uhash = 0;
  switch (t->tag)
    {
      case ty_alias:        uhash = (unsigned)t->d.alias; break;
      case ty_stabs_ref:    uhash = (unsigned)t->d.stabs_ref; break;
      case ty_prim:         uhash = (unsigned)t->index; break;
      case ty_pointer:      uhash = (unsigned)t->d.pointer; break;
      case ty_ref:          uhash = (unsigned)t->d.ref; break;
      case ty_struc:        uhash = (unsigned)t->d.struc.name; break;
      case ty_class:        uhash = (unsigned)t->d.class.name; break;
      case ty_enu:          uhash = (unsigned)t->d.enu.name; break;
      case ty_array:        uhash = (unsigned)t->d.array.etype; break;
      case ty_bits:         uhash = (unsigned)t->d.bits.type; break;
      case ty_func:         uhash = (unsigned)t->d.func.args ^ (unsigned)t->d.func.arg_count ^ (unsigned)t->d.func.ret ^ (unsigned)t->d.func.domain; break;
      case ty_types:        uhash = (unsigned)t->d.types.count  << 3; break;
      case ty_args:         uhash = (unsigned)t->d.args.count   << 3; break;
      case ty_fields:       uhash = (unsigned)t->d.fields.count << 3; break;
      case ty_member:       uhash = (unsigned)t->d.member.type ^ (unsigned)t->d.member.name >> 7; break;
      case ty_values:       uhash = (unsigned)t->d.values.count << 3; break;
      case ty_memfunc:      uhash = (unsigned)t->d.memfunc.type; break;
      case ty_baseclass:    uhash = (unsigned)t->d.baseclass.type; break;
      default:break; /* shut up warnings. */
    }
  uhash += t->tag;
  return (uhash ^ uhash >> 13) % TYPE_HASH_MAX;
}


/**
 * Search a list for a type.
 * @returns pointer to the type we found.
 * @returns NULL if not found.
 * @param   p       Pointer to the head list.
 * @param   src     Type to find.
 * @param   fhash   Whether or not to follow the hash list or the main list.
 */
#ifdef HASH_DEBUG
static struct type *type_find (struct type *p, const struct type *src, int fhash)
#else
static struct type *type_find (struct type *p, const struct type *src)
#endif
{
  int i;
  for (; p != NULL;
#ifdef HASH_DEBUG
       p = fhash ? p->nexthash : p->next )
#else
       p = p->nexthash                   )
#endif
    if (p->tag == src->tag)
      switch (p->tag)
        {
        case ty_alias:
          if (p->d.alias == src->d.alias)
            return p;
          break;
        case ty_stabs_ref:
          if (p->d.stabs_ref == src->d.stabs_ref)
            return p;
          break;
        case ty_prim:
          if (p->index == src->index)
            return p;
          break;
        case ty_pointer:
          if (p->d.pointer == src->d.pointer)
            return p;
          break;
        case ty_ref:
          if (p->d.ref == src->d.ref)
            return p;
          break;
        case ty_struc:
          if (p->d.struc.fields == src->d.struc.fields
              && p->d.struc.types == src->d.struc.types
              && p->d.struc.name == src->d.struc.name
              && p->d.struc.flags == src->d.struc.flags)
            return p;
          break;
        case ty_class:
          if (p->d.class.members == src->d.class.members
              && p->d.class.name == src->d.class.name)
            return p;
          break;
        case ty_enu:
          if (p->d.enu.type == src->d.enu.type
              && p->d.enu.values == src->d.enu.values
              && p->d.enu.name == src->d.enu.name)
            return p;
          break;
        case ty_array:
          if (p->d.array.etype == src->d.array.etype
              && p->d.array.itype == src->d.array.itype
              && p->d.array.first == src->d.array.first
              && p->d.array.last == src->d.array.last)
            return p;
          break;
        case ty_bits:
          if (p->d.bits.type == src->d.bits.type
              && p->d.bits.start == src->d.bits.start
              && p->d.bits.count == src->d.bits.count)
            return p;
          break;
        case ty_func:
          if (p->d.func.ret == src->d.func.ret
              && p->d.func.args == src->d.func.args
              && p->d.func.domain == src->d.func.domain
              && p->d.func.arg_count == src->d.func.arg_count)
            return p;
          break;
        case ty_args:
          if (p->d.args.count == src->d.args.count)
            {
              for (i = 0; i < p->d.args.count; ++i)
                if (p->d.args.list[i] != src->d.args.list[i])
                  break;
              if (i >= p->d.args.count)
                return p;
            }
          break;
        case ty_types:
          if (p->d.types.count == src->d.types.count)
            {
              for (i = 0; i < p->d.types.count; ++i)
                if (p->d.types.list[i] != src->d.types.list[i])
                  break;
              if (i >= p->d.types.count)
                return p;
            }
          break;
        case ty_fields:
          if (p->d.fields.count == src->d.fields.count)
            {
              for (i = 0; i < p->d.fields.count; ++i)
                if (p->d.fields.list[i].offset != src->d.fields.list[i].offset
                    || p->d.fields.list[i].name != src->d.fields.list[i].name)
                  break;
              if (i >= p->d.fields.count)
                return p;
            }
          break;
        case ty_member:
          if (p->d.member.type == src->d.member.type
              && p->d.member.offset == src->d.member.offset
              && p->d.member.flags == src->d.member.flags
              && p->d.member.name == src->d.member.name
              && p->d.member.sname == src->d.member.sname)
            return p;
          break;
        case ty_values:
          if (p->d.values.count == src->d.values.count)
            {
              for (i = 0; i < p->d.values.count; ++i)
                if (p->d.values.list[i].index != src->d.values.list[i].index
                    || p->d.values.list[i].name != src->d.values.list[i].name)
                  break;
              if (i >= p->d.values.count)
                return p;
            }
          break;
        case ty_memfunc:
          if (p->d.memfunc.type == src->d.memfunc.type
              && p->d.memfunc.offset == src->d.memfunc.offset
              && p->d.memfunc.flags == src->d.memfunc.flags
              && p->d.memfunc.name == src->d.memfunc.name
              && p->d.memfunc.mnglname == src->d.memfunc.mnglname)
            return p;
          break;
        case ty_baseclass:
          if (p->d.baseclass.type == src->d.baseclass.type
              && p->d.baseclass.offset == src->d.baseclass.offset
              && p->d.baseclass.flags == src->d.baseclass.flags)
            return p;
          break;
        default:
          abort ();
        }
  return NULL;
}


/* Add a new internal type to the list of internal types.  If an
   identical type already exists, a pointer to that type is returned.
   Otherwise, a copy of the new type is added to the list and a
   pointer to that type is returned.  It isn't worth my while to
   improve the speed of this function, for instance by using
   hashing.

   Actually it is worth while optimizing it as it takes 85% of the
   time here... or rather it was.

   ctypes=25301 */

static struct type *type_add (struct type *src)
{
  struct type *p;
  unsigned ihash;
  int i;

  ihash = type_hash(src);
#ifdef HASH_DEBUG
  p = type_find (atype_tag[ihash], src, 1);
#else
  p = type_find (atype_tag[ihash], src);
#endif
  if (p)
    {
      return p;
    }

#ifdef HASH_DEBUG
/*  {
    struct type *pcheck = type_find (type_head, src, 0);
    if (pcheck)
      {
        printf("\n\thash algorithm is borked! tag=%d\n", pcheck->tag);
        return pcheck;
      }
  } */
  ctype_tag[ihash]++;
  ctypes++;
#endif

  p = xmalloc (sizeof (*p));
  *p = *src;
  p->next = type_head;
  type_head = p;
  p->nexthash = atype_tag[ihash];
  atype_tag[ihash] = p;
  switch (src->tag)
    {
    case ty_args:
      i = src->d.args.count * sizeof (*(src->d.args.list));
      p->d.args.list = xmalloc (i);
      memcpy (p->d.args.list, src->d.args.list, i);
      break;
    case ty_types:
      i = src->d.types.count * sizeof (*(src->d.types.list));
      p->d.types.list = xmalloc (i);
      memcpy (p->d.types.list, src->d.types.list, i);
      break;
    case ty_fields:
      i = src->d.fields.count * sizeof (*(src->d.fields.list));
      p->d.fields.list = xmalloc (i);
      memcpy (p->d.fields.list, src->d.fields.list, i);
      break;
    case ty_values:
      i = src->d.values.count * sizeof (*(src->d.values.list));
      p->d.values.list = xmalloc (i);
      memcpy (p->d.values.list, src->d.values.list, i);
      break;
    default:
      break;
    }
  return p;
}


/* Associate the stabs type number NUMBER with the internal type
   HLL. */

static void stype_add (int number, struct type *hll)
{
  int i;

  /* Stabs type numbers are greater than zero. */

  if (number < 1)
    error ("stype_add: Invalid type index %d", number);

  /* Remember the current size of the table and grow the table as
     required. */

  i = stype_grow.alloc;
  grow_to (&stype_grow, number);

  /* Initialize the new table entries. */

  while (i < stype_grow.alloc)
    stype_list[i++] = NULL;

  /* Put the type into the table. */

#ifdef HLL_DEBUG
  printf ("  type: stabs %d => HLL 0x%x\n", number, hll->index);
#endif
#if 0 /* not good test... as we do some types twice. */
  if (stype_list[number-1] != NULL && stype_list[number-1] != hll)
    warning ("Type stabs %d allready mapped to HLL 0x%x, new HLL 0x%x!!\n", number, stype_list[number-1]->index, hll->index);
#endif

  stype_list[number-1] = hll;
}


/* Find the internal type associated with the stabs type number
   NUMBER.  If there is no such type, return NULL. */

static struct type *stype_find (int number)
{
  /* Stabs type numbers are greater than zero. */

  if (number < 1)
    error ("stype_find: Invalid type index %d", number);

  /* Adjust the index and check if it is within the current size of
     the table. */

  --number;
  if (number >= stype_grow.alloc)
    return NULL;
  else
    return stype_list[number];
}


/* Dereference stabs references until finding a non-reference. */

static const struct type *follow_refs (const struct type *t)
{
  const struct type *t2;

  while (t->tag == ty_stabs_ref)
    {
      t2 = stype_find (t->d.stabs_ref);
      if (t2 == NULL)
        break;
      t = t2;
    }

  return t;
}


#if defined (HLL_DEBUG)

/* This function is used to print an internal type for debugging. */

static void show_type (struct type *tp)
{
  /* precaution so we don't iterate for ever here when showing classes. */
  static int    fShowClass;
  int i;

  /* Currently we crash when showing FILE (stdio.h). So, better not crash here..  */
  if (!tp)
    {
    printf("!!! error! NULL type pointer! !!!");
    return;
    }

  switch (tp->tag)
    {
    case ty_prim:
      printf ("%#x", tp->index);
      break;
    case ty_stabs_ref:
      printf ("t%d", tp->d.stabs_ref);
      break;
    case ty_alias:
      show_type (tp->d.alias);
      break;
    case ty_pointer:
      printf ("(");
      show_type (tp->d.pointer);
      printf (")*");
      break;
    case ty_ref:
      printf ("(");
      show_type (tp->d.ref);
      printf (")&");
      break;
    case ty_struc:
      printf ("{");
      show_type (tp->d.struc.types);
      printf ("}");
      break;
    case ty_class:
      printf ("{");
      if (!fShowClass)
        {
          fShowClass = 1;
          show_type (tp->d.class.members);
          fShowClass = 0;
        }
      else
        printf ("<-!-!->");
      printf ("}");
      break;
    case ty_enu:
      printf ("{");
      show_type (tp->d.enu.values);
      printf ("}");
      break;
    case ty_array:
      printf ("(");
      show_type (tp->d.array.etype);
      printf ("[%ld]", tp->d.array.last + 1 - tp->d.array.first);
      break;
    case ty_bits:
      show_type (tp->d.bits.type);
      printf (":%ld:%ld", tp->d.bits.start, tp->d.bits.count);
      break;
    case ty_func:
      show_type (tp->d.func.ret);
      printf ("(");
      if (tp->d.func.args != NULL)
        show_type (tp->d.func.args);
      printf (")");
      break;
    case ty_args:
      for (i = 0; i < tp->d.args.count; ++i)
        {
          if (i != 0)
            printf (", ");
          show_type (tp->d.args.list[i]);
        }
      break;
    case ty_types:
      for (i = 0; i < tp->d.types.count; ++i)
        {
          if (i != 0)
            printf (", ");
          show_type (tp->d.types.list[i]);
        }
      break;
    case ty_fields:
      for (i = 0; i < tp->d.fields.count; ++i)
        {
          if (i != 0)
            printf (", ");
          printf ("%s", tp->d.fields.list[i].name);
        }
      break;
    case ty_member:
      printf ("%s", tp->d.member.name);
      break;
    case ty_values:
      for (i = 0; i < tp->d.values.count; ++i)
        {
          if (i != 0)
            printf (", ");
          printf ("%s", tp->d.values.list[i].name);
        }
      break;
    case ty_memfunc:
      show_type (tp->d.memfunc.type);
      break;
    case ty_baseclass:
      printf ("{");
      show_type (tp->d.baseclass.type);
      printf ("}");
      break;
    default:
      error ("show_type: Unknown type: %d", tp->tag);
    }
}

#endif


/**
 * Tries to complete a struct forward reference.
 *
 * @returns 0 on success
 * @returns 1 on failure.
 * @param   tp  Pointer to the struct/class to complete.
 * @remark  Nasy hack for strstreambuf.
 */
static int try_complete_struct(const struct type *tp)
{
    int         cch;
    const char *pszName;
    int         i;

    if (tp->tag == ty_struc)
        return 0;
    if (tp->d.struc.flags != STRUC_FORWARD)
        return 1;

    pszName = tp->d.struc.name;
    cch = strlen(pszName);
    for (i = 0; i < sym_count; ++i)
        switch (sym_ptr[i].n_type)
        {
            case N_LSYM:
            case N_LCSYM:
            case N_GSYM:
            case N_PSYM:
            case N_RSYM:
            case N_STSYM:
            case N_FUN:
                if (    !memcmp(str_ptr + sym_ptr[i].n_un.n_strx, pszName, cch)
                    &&  *(char*)(str_ptr + sym_ptr[i].n_un.n_strx + cch) == ':'
                    )
                {
                    parse_typedef(&i);
                    return 1;
                } /* if */
        } /* switch */

    return 0;
}



/* Return the size of the internal type TP, in bytes. */

static dword type_size (const struct type *tp)
{
  switch (tp->tag)
    {
    case ty_prim:

      /* Primitive types. */

      if (tp->index >= 0xa0 && tp->index <= 0xbf)
        return 4;               /* Near pointer */
      switch (tp->index)
        {
        case 0x97:              /* void */
          return 0;
        case 0x80:              /* 8 bit signed */
        case 0x84:              /* 8 bit unsigned */
        case 0x90:              /* 8 bit boolean */
          return 1;
        case 0x81:              /* 16 bit signed */
        case 0x85:              /* 16 bit unsigned */
          return 2;
        case 0x82:              /* 32 bit signed */
        case 0x86:              /* 32 bit unsigned */
        case 0x88:              /* 32 bit real */
          return 4;
        case 0x89:              /* 64 bit real */
        case 0x8c:              /* 64 bit complex */
          return 8;
        case 0x8a:              /* 80 bit real (96 bits of storage) */
          return 12;
        case 0x8d:              /* 128 bit complex */
          return 16;
        case 0x8e:              /* 160 bit complex */
          return 24;
        case 0x8f:              /* 256 bit complex (birds invention) */
          return 32;
        }
      error ("type_size: Unknown primitive type: %d", tp->index);

    case ty_stabs_ref:

      #if 0
      /* This should not happen. */

      no_warning ("stabs type %d not defined", tp->d.stabs_ref);

      return 4;
      #else
      /* This seems to happen although it shouldn't... Try do something at least. */

      tp = follow_refs (tp);
      if (!tp || tp->tag == ty_stabs_ref)
        {
          no_warning ("stabs type %d not defined", tp->d.stabs_ref);
          return 4;
        }
      return type_size (tp);
      #endif

    case ty_alias:

      /* An alias.  Return the size of the referenced type. */

      return type_size (tp->d.alias);

    case ty_pointer:
    case ty_ref:

      /* Pointers and references are 32-bit values. */

      return 4;

    case ty_struc:

      /* The size of a structure is stored in the type structure. */

      if (   tp->d.struc.flags & STRUC_FORWARD
          && try_complete_struct(tp))
        {
          no_warning ("size of incomplete structure %s is unknown (off %ld)\n stabs: %s",
                      tp->d.struc.name, parse_ptr - parse_start, parse_start);
          return 0;
        }
      return tp->d.struc.size;

    case ty_class:

      /* The size of a class is stored in the type structure. */

      return tp->d.class.size;

    case ty_enu:

      /* Return the size of the base type for an enumeration type. */

      return type_size (tp->d.enu.type);

    case ty_array:

      /* Multiply the element size with the number of array elements
         for arrays. */

      return ((tp->d.array.last + 1 - tp->d.array.first) *
              type_size (tp->d.array.etype));

    case ty_bits:

      /* Use the size of the base type for bitfields. */

      return type_size (tp->d.bits.type);

    case ty_member:

      /* Use the size of the member's type for members. */

      return type_size (tp->d.member.type);

    case ty_baseclass:

      /* The size of a base class is the size of the base class :-) */

      return type_size (tp->d.baseclass.type);

    case ty_func:
    case ty_args:
    case ty_types:
    case ty_fields:
    case ty_values:
    case ty_memfunc:

      /* This cannot not happen. */

      error ("type_size: cannot compute size for tag %d", tp->tag);

    default:

      /* This cannot happen. */

      error ("type_size: unknown type: %d", tp->tag);
    }
}


/* Turn a struct into a class.  This needs to be done when a struct is
   referenced as base class (unfortunately, a simple class looks like
   a struct). */

static void struct_to_class (struct type *tp)
{
  struct type t, *t1, **list, **types;
  struct field *fields;
  int i, n;

  if (tp->index != -1)
    {
      warning ("Base class cannot be converted from struct");
      return;
    }
  n = tp->d.struc.count;
  if (n == 0)
    list = NULL;
  else
    {
      if (tp->d.struc.fields->tag != ty_fields
          || tp->d.struc.fields->d.fields.count != n)
        {
          warning ("Invalid struct field list");
          return;
        }
      fields = tp->d.struc.fields->d.fields.list;
      if (tp->d.struc.types->tag != ty_types
          || tp->d.struc.types->d.types.count != n)
        {
          warning ("Invalid struct types list");
          return;
        }
      types = tp->d.struc.types->d.types.list;
      list = xmalloc (n * sizeof (*list));
      for (i = 0; i < n; ++i)
        {
          t.tag = ty_member;
          t.index = -1;
          t.d.member.type = types[i];
          t.d.member.offset = fields[i].offset;
          t.d.member.flags = VIS_PUBLIC;
          t.d.member.name = fields[i].name;
          t.d.member.sname = NULL;
          list[i] = type_add (&t);
        }
    }

  t.tag = ty_types;
  t.index = -1;
  t.d.types.count = n;
  t.d.types.list = list;
  t1 = type_add (&t);
  free (list);

  t = *tp;
  tp->tag = ty_class;
  tp->d.class.members = t1;
  tp->d.class.size = t.d.struc.size;
  tp->d.class.count = t.d.struc.count;
  tp->d.class.name = t.d.struc.name;
}


/* Build and return the internal representation of the `long long'
   type.  As IPMD doesn't know about `long long' we use the following
   structure instead:

     struct _long_long
     {
       unsigned long lo, hi;
     };

   This function could be optimized by remembering the type in a
   global variable. */

static struct type *make_long_long (void)
{
  struct type t, *t1, *t2, *t3, *types[2];
  struct field fields[2];

  /* Let t1 be `unsigned long'. */

  t.tag = ty_prim;
  t.index = 0x86;               /* 32 bit unsigned */
  t1 = type_add (&t);

  /* Let t2 be the field types: unsigned long, unsigned long. */

  types[0] = t1;
  types[1] = t1;
  t.tag = ty_types;
  t.index = -1;
  t.d.types.count = 2;
  t.d.types.list = types;
  t2 = type_add (&t);

  /* Let t3 be the fields: lo (at 0), hi (at 4). */

  fields[0].offset = 0; fields[0].name = strpool_add (str_pool, "lo");
  fields[1].offset = 4; fields[1].name = strpool_add (str_pool, "hi");
  t.tag = ty_fields;
  t.index = -1;
  t.d.fields.count = 2;
  t.d.fields.list = fields;
  t3 = type_add (&t);

  /* Build the structure type from t1, t2 and t3. */

  t.tag = ty_struc;
  t.d.struc.count = 2;
  t.d.struc.size = 8;
  t.d.struc.types = t2;
  t.d.struc.fields = t3;
  t.d.struc.name = strpool_add (str_pool, "_long_long");
  t.d.struc.flags = 0;
  return type_add (&t);
}


/* Parse a (signed) long long number in a stabs type.  The number is
   given in octal or decimal notation.  The result is stored to
   *NUMBER.  This function returns TRUE iff successful.  We don't
   check for overflow. */

static int parse_long_long (long long *number)
{
  long long n;
  int neg, base;

  n = 0; neg = FALSE; base = 10;
  if (*parse_ptr == '-')
    {
      neg = TRUE;
      ++parse_ptr;
    }
  if (*parse_ptr == '0')
    base = 8;
  if (!(*parse_ptr >= '0' && *parse_ptr < base + '0'))
    {
      *number = 0;
      return FALSE;
    }
  while (*parse_ptr >= '0' && *parse_ptr < base + '0')
    {
      n = n * base + (*parse_ptr - '0');
      ++parse_ptr;
    }
  *number = (neg ? -n : n);
  return TRUE;
}


/* Parse a (signed) number in a stabs type.  The number is given in
   octal or decimal notation.  The result is stored to *NUMBER.  This
   function returns TRUE iff successful.  We don't check for
   overflow. */

static int parse_number (long *number)
{
  long long n;
  int result;

  result = parse_long_long (&n);
  *number = (long)n;
  return result;
}


/* Check the next character in a stabs type for C.  If the next
   character matches C, the character is skipped and TRUE is
   returned.  Otherwise, a warning message is displayed and FALSE is
   returned. */

static int parse_char (char c)
{
  if (*parse_ptr != c)
    {
      no_warning ("Invalid symbol data: `%c' expected, found '%c' (off %ld)\n stabs: %s",
                  c, *parse_ptr, parse_ptr - parse_start, parse_start);
      return FALSE;
    }
  ++parse_ptr;
  return TRUE;
}


/* Parse mangled arguments of a member functions.  For instance, ic
   means an `int' argument and a `char' argument.  Return FALSE on
   failure. */

static int parse_mangled_args (void)
{
  while (*parse_ptr != ';')
    {
      switch (*parse_ptr)
        {
        case 0:
          warning ("Missing semicolon after mangled arguments");
          return FALSE;
        default:
          /* TODO */
          break;
        }
      ++parse_ptr;
    }
  ++parse_ptr;                  /* Skip semicolon */
  return TRUE;
}


/* Parse the visibility of a class or member. */

static int parse_visibility (void)
{
  switch (*parse_ptr)
    {
    case '0':
      ++parse_ptr;
      return VIS_PRIVATE;
    case '1':
      ++parse_ptr;
      return VIS_PROTECTED;
    case '2':
      ++parse_ptr;
      return VIS_PUBLIC;
    default:
      ++parse_ptr;
      no_warning ("Invalid visibility: %c", *parse_ptr);
      if (*parse_ptr != 0)
        ++parse_ptr;
      return VIS_PUBLIC;
    }
}


/* Add TYPE_NAME to `str_pool', replacing a NULL string with a unique
   string. */

static const char *add_struct_name (const char *type_name)
{
  char tmp[32];

  /* Some names have only spaces for gdb safety, let's make an
     fake name for those. */
  if (type_name != NULL)
    {
      const char *psz = type_name;
      while (*psz == ' ' || *psz == '\t')
          psz++;
      if (!*psz)
          type_name = NULL;
    }

  if (type_name == NULL)
    {
      sprintf (tmp, "__%d", unnamed_struct_number++);
      type_name = tmp;
    }
  return strpool_add (str_pool, type_name);
}

/**
 * Finds the next colon in the string and returns pointer to it.
 * For C++ template support, colons within <> are not considered.
 *
 * @returns Pointer to next colon within psz.
 * @returns NULL if not found.
 * @param   pszString   String to search.
 * @param   chType      Type we're parsing. Use '\0' if unknown or it
 *                      should be ignored
 * @remark  Still a bad hack this.
 */
#ifdef COLON_DEBUG
static const char *nextcolon2(const char *pszString, char chType)
{
    static const char *_nextcolon2(const char *pszString, char chType);
    const char *psz = _nextcolon2(pszString, chType);
    fprintf(stderr, "COLON: %.100s\n%.32s\n", pszString, psz);
    return psz;
}
static const char *_nextcolon2(const char *pszString, char chType)
#else
static const char *nextcolon2(const char *pszString, char chType)
#endif
{
    const char *pszColon;
    const char *psz;

    /* permanent operator hack */
    if (    pszString[0] == 'o'
        &&  pszString[1] == 'p'
        &&  pszString[2] == 'e'
        &&  pszString[3] == 'r'
        &&  pszString[4] == 'a'
        &&  pszString[5] == 't'
        &&  pszString[6] == 'o'
        &&  pszString[7] == 'r')
    {
        psz = pszString + 8;
        while (*psz && !isalnum(*psz) && *psz != '_') /* potential operator chars */
        {
            if (*psz == ':')
            {
                if (    psz - pszString >= 1 + 8
                    &&  psz - pszString <= 3 + 8)
                    return psz;
                break;
            }
            psz++;
        }

    }

    /* normal life */
    pszColon = strchr(pszString, ':');
    if (!pszColon)
        return NULL;
    psz = strchr(pszString, '<');

    if (psz && psz < pszColon && pszColon[1] == ':')
    {
        int   cNesting = 0;             /* <> level */

        for (;;psz++)
        {
            switch (*psz)
            {
                case '<':   cNesting++; break;
                case '>':   cNesting--; break;
                case ':':
                    if (cNesting > 0)
                        break;
                    /* hack: continue on ">::" (but not on "operator...") */
                    if (    chType != 'x'
                        &&  psz[1] == ':'
                        &&  psz > pszString && psz[-1] == '>'
                            )
                    {
                        psz++;
                        break;
                    }
                    return psz;
                case '\0':
                    return NULL;
            }
        }
    }
    else
    {
        if (chType != 'x')
        {
            /* skip '::' (hack:) if not followed by number. */
            while (*pszColon && pszColon[1] == ':' && (pszColon[2] > '9' || pszColon[2] < '0'))
            {
                pszColon += 2;
                while (*pszColon != ':' && *pszColon)
                    pszColon++;
            }
        }
    }

    return pszColon;
}

/* old version */
static inline const char *nextcolon(const char *pszString)
{
    return nextcolon2(pszString, '\0');
}



/* Parse a stabs type (which is named TYPE_NAME; TYPE_NAME is NULL for
   an unnamed type) and return the internal representation of that
   type. */

static struct type *parse_type (const char *type_name)
{
  const char *saved_ptr, *p1;
  char ch;
  struct type t, *result, *t1, *t2, *t3, **tlist;
  int i, n, class_flag, is_void;
  long num1, num2, size, lo, hi, offset, width, code;
  long long range_lo, range_hi;
  struct tmp_field *tmp_fields, *tf;
  struct value *tmp_values, *tv;
  struct grow g1 = GROW_INIT;
  enum type_tag tt;

  result = NULL;
  t.index = -1;
  switch (*parse_ptr)
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if (!parse_number (&num1))
        goto syntax;

      /* Special case: self reference => void */
      is_void = FALSE;
      if (*parse_ptr == '=' && parse_ptr[1] >= '0' && parse_ptr[1] <= '9')
        {
          saved_ptr = parse_ptr++;
          if (!parse_number (&num2))
            goto syntax;
          if (num2 == num1)
            is_void = TRUE;
          else
            parse_ptr = saved_ptr;
        }
      if (is_void)
        {
          t.tag = ty_prim;
          t.index = 0x97;       /* void */
          result = type_add (&t);
          stype_add (num1, result);
        }
      else if (*parse_ptr == '=')
        {
          /* Nested definition */
          ++parse_ptr;
          result = parse_type (type_name);
          stype_add (num1, result);
        }
      else
        {
          result = stype_find (num1);
          if (result == NULL)
            {
              t.tag = ty_stabs_ref;
              t.d.stabs_ref = num1;
            }
        }
      break;

    case 'r':

      /* A range type: r<base_type>;<lower_bound>;<upper_bound>;

         Special cases:

         lower_bound | upper_bound | type
         ------------+-------------+------------------------
         n > 0       | 0           | floating point, n bytes

         */

      ++parse_ptr;
      if (!parse_number (&num1) || !parse_char (';')
          || !parse_long_long (&range_lo) || !parse_char (';')
          || !parse_long_long (&range_hi) || !parse_char (';'))
        goto syntax;
      t.tag = ty_prim;
      if (range_lo == -128 && range_hi == 127)
        t.index = 0x80;         /* 8 bit signed */
      else if (range_lo == -32768 && range_hi == 32767)
        t.index = 0x81;         /* 16 bit signed */
      else if (range_lo == -2147483647 - 1 && range_hi == 2147483647)
        t.index = 0x82;         /* 32 bit signed */
      else if (range_lo == 0 && range_hi == 127)
        t.index = 0x84;         /* 8 bit character */
      else if (range_lo == 0 && range_hi == 255)
        t.index = 0x84;         /* 8 bit unsigned */
      else if (range_lo == 0 && range_hi == 65535)
        t.index = 0x85;         /* 16 bit unsigned */
      else if (range_lo == 0 && range_hi == 0xffffffff)
        t.index = 0x86;         /* 32 bit unsigned */
      else if (range_lo == 0 && range_hi == -1)
        {
          if (type_name != NULL
              && (strcmp (type_name, "long long unsigned int") == 0
                  || strcmp (type_name, "long long int") == 0))
            result = make_long_long ();
          else
            t.index = 0x86;     /* 32 bit unsigned */
        }
      else if ((unsigned long long)range_lo == 0x8000000000000000ULL
               && range_hi == 0x7fffffffffffffffLL)
        result = make_long_long ();
      else if (range_lo == 4 && range_hi == 0)
        t.index = 0x88;         /* 32 bit real */
      else if (range_lo == 8 && range_hi == 0)
        t.index = 0x89;         /* 64 bit real */
      else if (range_lo == 12 && range_hi == 0)
        t.index = 0x8a;         /* 80 bit real */
      else
        {
          no_warning ("Unknown range type: %lld..%lld", range_lo, range_hi);
          goto syntax;
        }
      break;

    case 'R':

      /* Complex number: R<?base_type?>;<size bytes>;<0>; */

      ++parse_ptr;
      if (!parse_number (&num1) || !parse_char (';')
          || !parse_number (&width) || !parse_char (';')
          || !parse_number (&code) || !parse_char (';'))
        goto syntax;

      /*
       * The 192bit type is not defined in the HLL specs, but we assume it for
       * convenince right now.
       *
       * It seems like no debugger acutally supports these types. So, we should,
       * when somebody requires it for debugging complex math, make structs out
       * of these declarations like GCC does for the 'complex int' type.
       *
       * .stabs	"FakeComplexInt:t21=22=s8real:1,0,32;imag:1,32,32;;",128,0,0,0
       * .stabs "FakeComplexFloat:t23=24=s8real:12,0,32;imag:12,32,32;;",128,0,0,0
       * .stabs "FakeComplexDouble:t25=26=s16real:13,0,64;imag:13,64,64;;",128,0,0,0
       * .stabs "FakeComplexLongDouble:t27=28=s24real:14,0,96;imag:14,96,96;;",128,0,0,0
       *
       */
      t.tag = ty_prim;
      if (width == 8 && code == 0)
        t.index = 0x8c;
      else if (width == 16 && code == 0)
        t.index = 0x8d ;
      else if (width == 20 && code == 0)
        t.index = 0x8e;
      else if (width == 24 && code == 0)
        t.index = 0x8f; //TODO: bogus!!!
      else
        {
          no_warning ("Unknown complex type: %ld/%ld", width, code);
          goto syntax;
        }
      break;

    case '*':

      /* A pointer type: *<type> */

      ++parse_ptr;
      t1 = parse_type (NULL);
      t.tag = ty_pointer;
      t.d.pointer = t1;
      break;

    case 'k':

      /* A const type: k<type>
         HLL don't have const typedefs afaik, so we just make it an alias. */

      ++parse_ptr;
      t1 = parse_type (NULL);
      t.tag = ty_alias;
      t.d.alias = t1;
      break;

   case 'B':

      /* A volatile type: B<type>
         This is a SUN extension, and right now we don't care to generate
         specific HLL for it and stick with an alias. */

      ++parse_ptr;
      t1 = parse_type (NULL);
      t.tag = ty_alias;
      t.d.alias = t1;
      break;


    case '&':

      /* A reference type (C++): &<type> */

      ++parse_ptr;
      t1 = parse_type (NULL);
      t.tag = ty_ref;
      t.d.ref = t1;
      break;

    case '#':

      /* A member function:

         short format: ##<return_type>;

         long format:  #<domain_type>,<return_type>{,<arg_type>};

         */

      ++parse_ptr;
      args_grow.count = 0;  /* ASSUMES we can safely use args_grow/args_list. */
      t2 = NULL;            /* t2=Domain type; t1=Return type; */
      if (*parse_ptr == '#')
        {
          ++parse_ptr;
          t1 = parse_type (NULL);
          if (!parse_char (';'))
            goto syntax;
          no_warning ("We don't understand '##' methods.");
        }
      else
        {
          t2 = parse_type (NULL); /* Domain type */
          if (!parse_char (','))
            goto syntax;
          t1 = parse_type (NULL); /* Return type */
          /* Arguments */
          while (*parse_ptr != ';')
            {
              if (!parse_char (','))
                goto syntax;
              grow_by (&args_grow, 1);
              args_list[args_grow.count++] = parse_type (NULL); /* Argument type */
            }
          ++parse_ptr;
        }

      t3 = NULL;
      #if 0 /* this doesn't really seems to be required, but can just as well leave it in. */
      if (args_grow.count)
        {
          t.tag = ty_args;
          t.index = -1;
          t.d.args.count = args_grow.count;
          t.d.args.list = xmalloc (args_grow.count * sizeof (*args_list));
          memcpy (t.d.args.list, args_list, args_grow.count * sizeof (*args_list));
          t3 = type_add (&t);
          free (t.d.args.list);
        }
      #endif

      /* Make a function type from the return type t1 */
      t.tag = ty_func;
      t.d.func.ret = t1;
      t.d.func.domain = t2;
      t.d.func.args = t3;
      t.d.func.arg_count = args_grow.count;
      args_grow.count = 0;
      t1 = type_add(&t);

      break;

    case '@':

      /* Special GNU-defined types:

         @s<bits>;<code>;

         BITS:          Number of bits
         CODE:          Identifies the type:
                        -16              bool
                        -20              char

         @s<bits>;<range record>
            Used for 64 bit ints.

         @s<bits>;<num records>
            Used for non-standard enums.

         @<basetype>,<membertype>
            Used for addressing class/struct/union members.
      */

      ++parse_ptr;
      switch (*parse_ptr)
        {
        case 's':
          ++parse_ptr;
          if (!parse_number (&size) || !parse_char (';'))
            goto syntax;
          if (*parse_ptr == 'r')
            { /* nested */
              return parse_type(type_name);
            }
          if (*parse_ptr == 'e')
            goto l_parse_enum;

          if (!parse_number (&code))
            goto syntax;
          if (size == 8 && code == -16)
            {
#if 0 /* debugger doesn't understand this game */
              t.tag = ty_prim;
              t.index = 0x90;   /* 8 bit boolean */
#else
              t.tag = ty_values;
              t.d.values.count = 2;
              t.d.values.list = xmalloc (2 * sizeof (*t.d.values.list));
              t.d.values.list[0].index = 0;
              t.d.values.list[0].name  = strpool_add (str_pool, "false");
              t.d.values.list[1].index = 1;
              t.d.values.list[1].name  = strpool_add (str_pool, "true");
              t1 = type_add (&t);
              free (t.d.values.list);

              t.tag = ty_prim;
              t.index = 0x80;
              t2 = type_add (&t);

              t.tag = ty_enu;
              t.index = -1;
              t.d.enu.values = t1;
              t.d.enu.type = t2;
              t.d.enu.name = add_struct_name (NULL);
              t.d.enu.first = 0;
              t.d.enu.last = 1;
#endif
            }
          else
            {
              warning ("Unknown GNU extension type code: %ld, %ld bits",
                       code, size);
              goto syntax;
            }
          if (!parse_char (';'))
            goto syntax;
          break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          {
            /* just make it an integer - see dbxout.c OFFSET_TYPE case. */
            if (    parse_number (&num1)
                &&  *parse_ptr == ','
                &&  ++parse_ptr
                &&  parse_number (&num2)
                )
              {
                t.tag = ty_prim;
                t.index = 0x82;         /* 32 bit signed */
              }
            else
              {
                warning_parse ("Invalid BASE_OFFSET GNU extension format!");
                goto syntax;
              }
            break;
          }

        default:
          warning ("Unknown GNU extension type: @%c", *parse_ptr);
          goto syntax;
        }
      break;

    case 'x':

      /* Incomplete type: xs<name>:
                          xu<name>:
                          xe<name>: */

      ++parse_ptr;
      ch = *parse_ptr++;
      p1 = nextcolon2 (parse_ptr, 'x');
      if (p1 == NULL)
        {
          warning ("Invalid forward reference: missing colon");
          goto syntax;
        }

      switch (ch)
        {
        case 's':
        case 'u':
          {
            /* make a fake structure. */
            t.tag = ty_prim;
            t.index = 0x80;
            tlist = xmalloc (1 * sizeof (*t.d.types.list));
            tlist[0] = type_add (&t);
            t.tag = ty_types;
            t.index = -1;
            t.d.types.count = 1;
            t.d.types.list = tlist;
            t1 = type_add (&t);
            free (tlist);

            t.tag = ty_fields;
            t.index = -1;
            t.d.fields.count = 1;
            t.d.fields.list = xmalloc (1 * sizeof (*t.d.fields.list));
            t.d.fields.list[0].offset = 0;
            t.d.fields.list[0].name = strpool_add (str_pool, "This_Is_an_Incomplete_Structure_SORRY");
            t2 = type_add (&t);
            free (t.d.fields.list);

            t.tag = ty_struc;
            t.index = -1;
            t.d.struc.types = t1;
            t.d.struc.fields = t2;
            t.d.struc.size = 1;
            t.d.struc.count = 1;
            t.d.struc.name = strpool_addn (str_pool, parse_ptr, p1 - parse_ptr);
            t.d.struc.flags = STRUC_FORWARD;

            /* Avoid adding an incomplete type if the complete one is
               already defined.  We're ignoring scoping (TODO). */

            for (t3 = type_head; t3 != NULL; t3 = t3->next)
              if ((t3->tag == ty_struc && t3->d.struc.name == t.d.struc.name
                   && !(t3->d.struc.flags & STRUC_FORWARD))
                  || (t3->tag == ty_class && t3->d.class.name == t.d.struc.name))
                {
                  result = t3;
                  break;
                }
            break;
          }

        case 'e':
          { /* just make it a primary. */
            /** @todo make an enum perhaps? */
            t.tag = ty_prim;
            t.index = 0x82;             /* 32 bit signed */
            break;
          }

        default:
          warning ("Unknown forward reference: %c", ch);
          parse_ptr = p1 + 1;
          goto syntax;
        }
      parse_ptr = p1 + 1;
      break;

    case 's':                           /* struct, class */
    case 'u':                           /* union */

      /* Structures, unions and classes:

         s<size>[<baseclasses>]{<field>}{<method>}

         BASECLASSES: !<#baseclasses>,{<baseclass>;}
         BASECLASS:   <virtuality><visibility><offset>,<type>
         FIELD:       <name>:[/0|1|2]<type>,<offset>,<width>;
         FIELD:       <name>:[/0|1|2]<type>:<static_name>;
         METHOD:      <name>::{<type>:<args>};

         */

      ++parse_ptr;
      class_flag = FALSE;
      if (!parse_number (&size))
        goto syntax;
      grow_init (&g1, &tmp_fields, sizeof (*tmp_fields), 8);

      /* Parse the base classes, if present. */

      if (*parse_ptr == '!')
        {
          long i, n;

          ++parse_ptr;
          class_flag = TRUE;
          if (!parse_number (&n) || !parse_char (','))
            goto syntax;
          for (i = 0; i < n; ++i)
            {
              grow_by (&g1, 1);
              tf = &tmp_fields[g1.count++];
              tf->flags = MEMBER_BASE;
              tf->name = NULL;
              tf->sname = NULL;
              switch (*parse_ptr)
                {
                case '0':
                  ++parse_ptr;
                  break;
                case '1':
                  tf->flags |= MEMBER_VIRTUAL;
                  ++parse_ptr;
                  break;
                default:
                  warning ("Invalid base class visibility: %c", *parse_ptr);
                  if (*parse_ptr != 0)
                    ++parse_ptr;
                }

              tf->flags |= parse_visibility ();
              if (!parse_number (&offset) || !parse_char (','))
                goto syntax;
              tf->offset = offset >> 3;
              t1 = parse_type (NULL);
              switch (t1->tag)
                {
                case ty_class:
                  break;
                case ty_struc:
                  /* kso #456 2003-06-11: Don't mess with forward defines! */
                  if (!(   (t1->d.struc.flags & STRUC_FORWARD)
                        /*&& t1->d.struc.types == NULL
                        && t1->d.struc.fields == NULL*/))
                    struct_to_class (t1);
                  break;
                default:
                  warning ("Invalid base class type");
                }
              tf->type = t1;
              if (!parse_char (';'))
                goto syntax;
            }
        }

      while (*parse_ptr != ';')
        {
          p1 = nextcolon (parse_ptr);
          if (p1 == NULL)
            {
              warning ("Invalid structure type: missing colon");
              goto syntax;
            }
          n = p1 - parse_ptr;
          grow_by (&g1, 1);
          tf = &tmp_fields[g1.count++];
          tf->type = NULL;
          tf->offset = 0;
          tf->flags = VIS_PUBLIC;
          tf->name = strpool_addn (str_pool, parse_ptr, n);
          tf->mnglname = tf->sname = NULL;
          parse_ptr = p1 + 1;
          if (*parse_ptr == ':')
            {
              class_flag = TRUE;
              ++parse_ptr;
              do
                {
                  const char * pszMangled;

                  t1 = parse_type (NULL);
                  offset = 0;
                  if (t1 == NULL || t1->tag != ty_func)
                    {
                      no_warning ("Invalid member function type");
                      goto syntax;
                    }

                  if (*parse_ptr != ':')
                    {
                      no_warning ("Arguments for member function missing");
                      goto syntax;
                    }
                  ++parse_ptr;
                  pszMangled = parse_ptr;
                  if (!parse_mangled_args ())
                    goto syntax;
                  tf->mnglname = strpool_addn(str_pool, pszMangled, parse_ptr - pszMangled - 1); /* ASSUMES no extra spaces! */
                  tf->flags = parse_visibility () | MEMBER_FUNCTION;
                  switch (*parse_ptr)
                    {
                    case 'A':
                      ++parse_ptr;
                      break;
                    case 'B':
                      tf->flags |= MEMBER_CONST;
                      ++parse_ptr;
                      break;
                    case 'C':
                      tf->flags |= MEMBER_VOLATILE;
                      ++parse_ptr;
                      break;
                    case 'D':
                      tf->flags |= MEMBER_CONST | MEMBER_VOLATILE;
                      ++parse_ptr;
                      break;
                    default:
                      no_warning ("Unknown member function qualifier: %c",
                               *parse_ptr);
                      if (*parse_ptr != 0)
                        ++parse_ptr;
                    }
                  switch (*parse_ptr)
                    {
                    case '.':
                      ++parse_ptr;
                      break;
                    case '*':
                      tf->flags |= MEMBER_VIRTUAL;
                      ++parse_ptr;
                      if (!parse_number (&offset) || !parse_char (';'))
                        {
                          warning ("Invalid data for virtual member function");
                          goto syntax;
                        }
                      parse_type (NULL);
                      if (!parse_char (';'))
                        {
                          warning ("Invalid data for virtual member function");
                          goto syntax;
                        }
                      break;
                    case '?':
                      tf->flags |= MEMBER_STATIC;
                      ++parse_ptr;
                      break;
                    default:
                      no_warning ("Unknown member function qualifier: %c",
                               *parse_ptr);
                      if (*parse_ptr != 0)
                        ++parse_ptr;
                    }
                  /* Contstructor / Destructor hack -
                   * TODO: verify that this doesn't really work with overloaded constructors and fix it.
                   */
                  if (!(tf->flags & MEMBER_STATIC))
                    {
                      /* using the strpool is much faster! */
                      static const char *pszCompCtor, *pszBaseCtor, *pszCompDtor, *pszBaseDtor = NULL;
                      if (!pszBaseDtor)
                        {
                          pszCompCtor = strpool_addn(str_pool, "__comp_ctor", 11);
                          pszBaseCtor = strpool_addn(str_pool, "__base_ctor", 11);
                          pszCompDtor = strpool_addn(str_pool, "__comp_dtor", 11);
                          pszBaseDtor = strpool_addn(str_pool, "__base_dtor", 11);
                        }

                      if (tf->name == pszCompCtor || tf->name == pszBaseCtor)
                          tf->flags |= MEMBER_CTOR;
                      else if (tf->name == pszCompDtor || tf->name == pszBaseDtor)
                          tf->flags |= MEMBER_DTOR;
                    }
                  tf->type = t1;
                  tf->offset = offset;
                  if (*parse_ptr != ';')
                    {
                      /* Another member function with the same name
                         follows. */

                      grow_by (&g1, 1);
                      tf = &tmp_fields[g1.count++];
                      tf->flags = VIS_PUBLIC | MEMBER_FUNCTION;
                      tf->name = tf[-1].name;
                    }
                } while (*parse_ptr != ';');
              ++parse_ptr;
            }
          else
            {
              if (*parse_ptr == '/')
                {
                  class_flag = TRUE;
                  ++parse_ptr;
                  tf->flags = parse_visibility ();
                }
              t1 = parse_type (NULL);
              if (*parse_ptr == ':')
                {
                  /* Static member */

                  ++parse_ptr;
                  tf->flags |= MEMBER_STATIC;
                  p1 = strchr (parse_ptr, ';');
                  if (p1 == NULL)
                    {
                      warning ("Invalid static member: missing semicolon");
                      goto syntax;
                    }
                  n = p1 - parse_ptr;
                  tf->sname = strpool_addn (str_pool, parse_ptr, n);
                  parse_ptr = p1 + 1;
                  tf->offset = 0;
                }
              else
                {
                  if (!parse_char (',') || !parse_number (&offset))
                    goto syntax;
                  if (*parse_ptr == ',')
                    {
                      ++parse_ptr;
                      if (!parse_number (&width))
                        goto syntax;
                    }
                  else
                    width = 0;      /* vtable pointer field */
                  if (!parse_char (';'))
                    goto syntax;
                  if ((offset & 7) != 0 || width != 8 * type_size (t1))
                    {
                      t.tag = ty_bits;
                      t.d.bits.type = t1;
                      t.d.bits.start = offset & 7;
                      t.d.bits.count = width;
                      t1 = type_add (&t);
                    }
                  tf->offset = offset >> 3;
                }
              tf->type = t1;
            }

          /* the debuggers croak if the field doesn't have a name. */

          if ((!tf->name || !*tf->name) 
           && (!tf->sname || !*tf->sname)
           && (!tf->mnglname || !*tf->mnglname))
            tf->name = strpool_add (str_pool, "<anonymous>");
        }
      ++parse_ptr;
      if (*parse_ptr == '~')
        {
          /* Type reference to the first base class.  This field is
             present for classes which contain virtual functions. */

          ++parse_ptr;
          if (!parse_char ('%'))
            goto syntax;
          t1 = parse_type (NULL);
          if (!parse_char (';'))
            goto syntax;
        }

      if (class_flag)
        {
          tlist = xmalloc (g1.count * sizeof (*t.d.types.list));
          for (i = 0; i < g1.count; ++i)
            {
              if (tmp_fields[i].flags & MEMBER_FUNCTION)
                {
                  t2 = tmp_fields[i].type;
                  tt = follow_refs (t2)->tag;
                  /* TODO: check later if ty_stabs_ref */
                  if (tt != ty_func && tt != ty_stabs_ref)
                    error ("Invalid type for member function %s",
                           tmp_fields[i].name);
                  t.tag = ty_memfunc;
                  t.d.memfunc.type = t2;
                  t.d.memfunc.name = tmp_fields[i].name;
                  t.d.memfunc.mnglname = tmp_fields[i].mnglname;
                  t.d.memfunc.flags = tmp_fields[i].flags;
                  t.d.memfunc.offset = tmp_fields[i].offset;
                }
              else if (tmp_fields[i].flags & MEMBER_BASE)
                {
                  t.tag = ty_baseclass;
                  t.d.baseclass.type = tmp_fields[i].type;
                  t.d.baseclass.flags = tmp_fields[i].flags;
                  t.d.baseclass.offset = tmp_fields[i].offset;
                }
              else
                {
                  t.tag = ty_member;
                  t.d.member.type = tmp_fields[i].type;
                  t.d.member.offset = tmp_fields[i].offset;
                  t.d.member.flags = tmp_fields[i].flags;
                  t.d.member.name = tmp_fields[i].name;
                  t.d.member.sname = tmp_fields[i].sname;
                }
              t2 = type_add (&t);
              tlist[i] = t2;
            }
          t.tag = ty_types;
          t.d.types.count = g1.count;
          t.d.types.list = tlist;
          t1 = type_add (&t);
          free (tlist);

          t.tag = ty_class;
          t.d.class.members = t1;
          t.d.class.size = size;
          t.d.class.count = g1.count;
          t.d.class.name = add_struct_name (type_name);

          /* Check if there is an incomplete type waiting to be filled
             in (forward reference).  We're ignoring scoping (TODO). */

          for (t3 = type_head; t3 != NULL; t3 = t3->next)
            if (t3->tag == ty_struc && t3->d.struc.name == t.d.class.name
                /*&& t3->d.struc.types == NULL && t3->d.struc.fields == NULL */
                && (t3->d.struc.flags & STRUC_FORWARD))
              {
                if (t3->index != -1)
                  warning ("This cannot happen, case 1");
                result = t3;
                t3->tag = ty_class;
                t3->d.class = t.d.class;
                break;
              }
        }
      else
        {
          t.tag = ty_types;
          t.d.types.count = g1.count;
          t.d.types.list = xmalloc (g1.count * sizeof (*t.d.types.list));
          for (i = 0; i < g1.count; ++i)
            t.d.types.list[i] = tmp_fields[i].type;
          t1 = type_add (&t);
          free (t.d.types.list);

          t.tag = ty_fields;
          t.d.fields.count = g1.count;
          t.d.fields.list = xmalloc (g1.count * sizeof (*t.d.fields.list));
          for (i = 0; i < g1.count; ++i)
            {
              t.d.fields.list[i].offset = tmp_fields[i].offset;
              t.d.fields.list[i].name = tmp_fields[i].name;
            }
          t2 = type_add (&t);
          free (t.d.fields.list);

          t.tag = ty_struc;
          t.d.struc.count = g1.count;
          t.d.struc.size = size;
          t.d.struc.types = t1;
          t.d.struc.fields = t2;
          t.d.struc.name = add_struct_name (type_name);
          t.d.struc.flags = 0;

          /* Check if there is an incomplete type waiting to be filled
             in (forward reference).  We're ignoring scoping (TODO). */

          for (t3 = type_head; t3 != NULL; t3 = t3->next)
            if (t3->tag == ty_struc && t3->d.struc.name == t.d.struc.name
                /* && t3->d.struc.types == NULL && t3->d.struc.fields == NULL */
                && (t3->d.struc.flags & STRUC_FORWARD))
              {
                if (t3->index != -1)
                  warning ("This cannot happen, case 1");
                result = t3;
                t3->d.struc = t.d.struc;
                break;
              }
        }
      grow_free (&g1);
      break;

    case 'e':
      size = 32;
l_parse_enum:

      /* Enumeration type: e{<name>:<value>,}; */

      ++parse_ptr;
      grow_init (&g1, &tmp_values, sizeof (*tmp_values), 8);
      while (*parse_ptr != ';')
        {
          p1 = nextcolon (parse_ptr);
          if (p1 == NULL)
            {
              warning ("Invalid enum type: missing colon");
              goto syntax;
            }
          n = p1 - parse_ptr;
          grow_by (&g1, 1);
          tv = &tmp_values[g1.count++];
          tv->name = strpool_addn (str_pool, parse_ptr, n);
          parse_ptr = p1 + 1;
          if (!parse_number (&tv->index) || !parse_char (','))
            goto syntax;
        }
      ++parse_ptr;
      t.tag = ty_values;
      t.d.values.count = g1.count;
      t.d.values.list = xmalloc (g1.count * sizeof (*t.d.values.list));
      if (g1.count == 0)
        lo = hi = 0;
      else
        lo = hi = tmp_values[0].index;
      for (i = 0; i < g1.count; ++i)
        {
          t.d.values.list[i].index = tmp_values[i].index;
          t.d.values.list[i].name = tmp_values[i].name;
          if (tmp_values[i].index < lo)
            lo = tmp_values[i].index;
          if (tmp_values[i].index > hi)
            hi = tmp_values[i].index;
        }
      t1 = type_add (&t);
      free (t.d.values.list);

      t.tag = ty_prim;
      switch (size)
        {
        case 8:  t.index = 0x80; break;   /* 8 bit signed */
        case 16: t.index = 0x81; break;   /* 16 bit signed */
        case 32: t.index = 0x82; break;   /* 32 bit signed */
        case 64: result = t2 = make_long_long (); break;
        default: goto syntax;
        }
      if (!result)
        t2 = type_add (&t);

      t.tag = ty_enu;
      t.index = -1;
      t.d.enu.values = t1;
      t.d.enu.type = t2;
      t.d.enu.name = add_struct_name (type_name);
      t.d.enu.first = lo;
      t.d.enu.last = hi;
      grow_free (&g1);
      break;

    case 'a':
      /* An array: ar<index_type>;<lower_bound>;<upper_bound>;<el_type> */

      ++parse_ptr;
      if (!parse_char ('r'))
        goto syntax;
      if (strncmp (parse_ptr, "0;", 2) == 0)
        {
          /* This invalid type index seems to be caused by a bug of
             GCC 2.8.1.  Replace with a 32-bit signed integer. */

          t.tag = ty_prim;
          t.index = 0x82;       /* 32 bit signed */
          t1 = type_add (&t);
          ++parse_ptr;
        }
      else
        t1 = parse_type (NULL); /* Index type */
      if (!parse_char (';')
          || !parse_number (&lo) || !parse_char (';')   /* Lower bound */
          || !parse_number (&hi) || !parse_char (';'))  /* Upper bound */
        goto syntax;
      t2 = parse_type (NULL);   /* Elements type */
      if (lo == 0 && hi == 0)   /* Variable-length array */
        {
          t.tag = ty_pointer;   /* Turn into pointer */
          t.d.pointer = t2;
        }
      else
        {
          t.tag = ty_array;
          t.d.array.first = lo;
          t.d.array.last = hi;
          t.d.array.itype = t1;
          t.d.array.etype = t2;
        }
      break;

    case 'f':

      /* A function: f<return_type> */

      ++parse_ptr;
      t1 = parse_type (NULL);   /* Return type */
      t.tag = ty_func;
      t.d.func.ret = t1;
      t.d.func.domain = NULL;
      t.d.func.args = NULL;
      t.d.func.arg_count = 0;
      break;

    default:
      no_warning ("Unknown type: %c", *parse_ptr);
      goto syntax;
    }

  /* Add the type to the tables. */

  if (result == NULL)
    result = type_add (&t);
  return result;

syntax:
  no_warning ("syntax error in stabs: %c (off %ld)\n stabs: %s",
              *parse_ptr, parse_ptr - parse_start, parse_start);

  grow_free (&g1);
  t.tag = ty_prim;
  t.index = 0x82;               /* 32 bit signed */
  return type_add (&t);
}


/* Parse a stabs type (the name of the type is TYPE_NAME, which is
   NULL for an unnamed type) and check for end of the string.  If not
   all characters of the string have been parsed, a warning message is
   displayed.  Return the internal representation of the type. */

static struct type *parse_complete_type (const char *type_name)
{
  struct type *tp;

  tp = parse_type (type_name);
  if (*parse_ptr != 0)
      no_warning ("unexpected character at end of stabs type: %c (off %ld)\n stabs: %s",
                  *parse_ptr, parse_ptr - parse_start, parse_start);
  return tp;
}


/* Create an HLL type number for type TP. If a type number has already
   been assigned to TP, use that number.  Otherwise, allocate a new
   (complex) type number and define the type in the $$TYPES segment.
   The HLL type number us stored to *HLL.

   Note: This function must not be called between sst_start() and
   sst_end() or tt_start() and tt_end() as entries are made both in
   the SST ($$SYMBOLS) and the type table ($$TYPES). */

static void make_type (struct type *tp, int *hll)
{
  struct type *t1;
  int ti_1, ti_2, ti_3, i, qual;
  int patch1;

#define RETURN(X) do {*hll = (X); return;} while (0)

  if (tp->index <= -2)
    {
      /* bird: A hack where we allow a 2nd recursion of rectain types.
       * The normal issue is C++ class references. */
      if (tp->tag != ty_alias || tp->index <= -3)
        {
          warning_parse ("Cycle detected by make_type.");
          RETURN (0);
        }
    }
  if (tp->index > -1)
    RETURN (tp->index);
  tp->index--;
  switch (tp->tag)
    {
    case ty_stabs_ref:
      t1 = stype_find (tp->d.stabs_ref);
      if (t1 == NULL)
        {
          warning_parse ("Undefined stabs type %d referenced", tp->d.stabs_ref);
          RETURN (0x82);        /* 32 bit signed */
        }
      make_type (t1, &tp->index);
      *hll = tp->index;
      break;

    case ty_alias:
      make_type (tp->d.alias, &tp->index);
      *hll = tp->index;
      break;

    case ty_pointer:
      ti_1 = tp->d.pointer->index;
      if (ti_1 >= 0x80 && ti_1 <= 0xa0)
        {
          tp->index = ti_1 + 0x20;
          RETURN (tp->index);
        }
      *hll = tp->index = hll_type_index++;
      tt_start (0x7a, 0x01);    /* 32-bit near pointer */
      patch1 = tt.size;
      tt_index (0);
      tt_end ();
      make_type (tp->d.pointer, &ti_1);
      buffer_patch_word (&tt, patch1+1, ti_1);
      break;

    case ty_ref:
      *hll = tp->index = hll_type_index++;
      tt_start (0x48, 0x00);    /* Reference type (C++) */
      patch1 = tt.size;
      tt_word (0);
      tt_end ();
      make_type (tp->d.ref, &ti_1);
      buffer_patch_word (&tt, patch1, ti_1);
      break;

    case ty_struc:
    {
      struct field *fields = NULL;
      struct type **types = NULL;
      int           cFields = tp->d.struc.count;

      if (    tp->d.struc.fields
          &&  tp->d.struc.fields->tag == ty_fields
          &&  tp->d.struc.fields->d.fields.count == cFields)
          fields = tp->d.struc.fields->d.fields.list;
      if (   tp->d.struc.types
          && tp->d.struc.types->tag == ty_types
          && tp->d.struc.types->d.types.count == cFields)
          types = tp->d.struc.types->d.types.list;

      if (cplusplus_flag && ((types && fields) || !cFields))
        {
          struct type   t;
          struct type **list;
          int           i;
          *hll = tp->index = hll_type_index++;
          tt_start (0x40, 0x01);      /* Class is-struct */
          tt_dword (tp->d.struc.size);
          tt_word (tp->d.struc.count);
          patch1 = tt.size;
          tt_word (0);                /* Members */
          tt_enc (tp->d.struc.name);  /* Class name */
          tt_end ();

          list = xmalloc (cFields * sizeof (*list));
          for (i = 0; i < cFields; ++i)
            {
              t.tag             = ty_member;
              t.index           = -1;
              t.d.member.type   = types[i];
              t.d.member.offset = fields[i].offset;
              t.d.member.flags  = VIS_PUBLIC;
              t.d.member.name   = fields[i].name;
              t.d.member.sname  = NULL;
              list[i] = type_add (&t);
            }

          t.tag = ty_types;
          t.index = -1;
          t.d.types.count = cFields;
          t.d.types.list = list;
          t1 = type_add (&t);
          free (list);

          make_type (t1, &ti_1);
          buffer_patch_word (&tt, patch1, ti_1);
        }
      else
        {
          tt_start (0x79, 0x00);    /* Structure/Union */
          tt_dword (tp->d.struc.size);
          tt_word (tp->d.struc.count);
          patch1 = tt.size;
          tt_index (0);
          tt_index (0);
          tt_name (tp->d.struc.name);
          tt_end ();
          *hll = tp->index = hll_type_index++;
          if (tp->d.struc.types == NULL && tp->d.struc.fields == NULL)
            {
              ti_1 = 0;
              ti_2 = 1;
            }
          else
            {
              make_type (tp->d.struc.types, &ti_1);
              make_type (tp->d.struc.fields, &ti_2);
            }
          buffer_patch_word (&tt, patch1+1, ti_1);
          buffer_patch_word (&tt, patch1+4, ti_2);
        }
      break;
    }

    case ty_class:
      *hll = tp->index = hll_type_index++;
      tt_start (0x40, 0x00);      /* Class */
      tt_dword (tp->d.class.size);
      tt_word (tp->d.class.count);
      patch1 = tt.size;
      tt_word (0);                /* Members */
      tt_enc (tp->d.class.name);  /* Class name */
      tt_end ();
      make_type (tp->d.class.members, &ti_1);
      buffer_patch_word (&tt, patch1, ti_1);
      break;

    case ty_member:
      qual = 0;
      if (tp->d.member.flags & MEMBER_STATIC)
        qual |= 0x01;
      tt_start (0x46, qual);
      tt_byte (tp->d.member.flags & MEMBER_VIS);
      patch1 = tt.size;
      tt_word (0);
      tt_unsigned_span (tp->d.member.offset);
      if (tp->d.member.flags & MEMBER_STATIC)
        tt_enc (tp->d.member.sname);
      else
        tt_enc ("");
      tt_enc (tp->d.member.name);
      tt_end ();
      *hll = tp->index = hll_type_index++;

      make_type (tp->d.member.type, &ti_1);
      buffer_patch_word (&tt, patch1, ti_1);
      break;

    case ty_enu:
      make_type (tp->d.enu.type, &ti_1);
      make_type (tp->d.enu.values, &ti_2);
      tt_start (0x7b, 0x00);            /* Enum */
      tt_index (ti_1);                  /* Element's data type */
      tt_index (ti_2);                  /* List of values */
      tt_signed_span (tp->d.enu.first); /* Minimum index */
      tt_signed_span (tp->d.enu.last);  /* Maximum index */
      tt_name (tp->d.enu.name);
      tt_end ();
      *hll = tp->index = hll_type_index++;
      break;

    case ty_array:
      make_type (tp->d.array.itype, &ti_1);
      make_type (tp->d.array.etype, &ti_2);
      tt_start (0x78, 0x00);    /* Array */
      tt_dword (type_size (tp));
      tt_index (ti_1);
      tt_index (ti_2);
      tt_name ("");             /* Name */
      tt_end ();
      *hll = tp->index = hll_type_index++;
      break;

    case ty_bits:
      tt_start (0x5c, 0x0a);    /* Bit string */
      tt_byte (tp->d.bits.start);
      tt_unsigned_span (tp->d.bits.count);
      tt_end ();
      *hll = tp->index = hll_type_index++;
      break;

    case ty_func:
      tt_start (0x54, 0x05);          /* Function */
      tt_word (tp->d.func.arg_count); /* Number of parameters */
      tt_word (tp->d.func.arg_count); /* Max. Number of parameters */
      patch1 = tt.size;
      tt_index (0);                   /* Return value */
      tt_index (0);                   /* Argument list */
      tt_end ();
      *hll = tp->index = hll_type_index++;

      if (tp->d.func.domain)
        make_type (tp->d.func.domain, &ti_3);
      make_type (tp->d.func.ret, &ti_1);
      if (tp->d.func.args == NULL)
        ti_2 = 0;
      else
        make_type (tp->d.func.args, &ti_2);
      buffer_patch_word (&tt, patch1+1, ti_1);
      buffer_patch_word (&tt, patch1+4, ti_2);
      break;

    case ty_args:
      if (tp->d.args.count == 0)
        {
          tp->index = 0;
          RETURN (tp->index);
        }
      for (i = 0; i < tp->d.args.count; ++i)
        make_type (tp->d.args.list[i], &ti_1);
      tt_start (0x7f, 0x04);    /* List (function parameters) */
      for (i = 0; i < tp->d.args.count; ++i)
        {
          tt_byte (0x01);        /* Flag: pass by value, no descriptor */
          tt_index (tp->d.args.list[i]->index);
        }
      tt_end ();
      *hll = tp->index = hll_type_index++;
      break;

    case ty_types:
      if (tp->d.types.count == 0)
        {
          tp->index = 0;
          RETURN (tp->index);
        }
      *hll = tp->index = hll_type_index++;
      tt_start (0x7f, 0x01);    /* List (structure field types) */
      patch1 = tt.size;
      for (i = 0; i < tp->d.types.count; ++i)
        tt_index (0);
      tt_end ();
      for (i = 0; i < tp->d.types.count; ++i)
        {
          make_type (tp->d.types.list[i], &ti_1);
          buffer_patch_word (&tt, patch1 + 1 + 3*i, ti_1);
        }
      break;

    case ty_fields:
      if (tp->d.fields.count == 0)
        {
          tp->index = 0;
          RETURN (tp->index);
        }
      tt_start (0x7f, 0x02);    /* List (structure field names) */
      for (i = 0; i < tp->d.fields.count; ++i)
        {
          tt_name (tp->d.fields.list[i].name);
          tt_unsigned_span (tp->d.fields.list[i].offset);
        }
      tt_end ();
      *hll = tp->index = hll_type_index++;
      break;

    case ty_values:
      if (tp->d.values.count == 0)
        {
          tp->index = -1;
          RETURN (tp->index);
        }
      tt_start (0x7f, 0x03);    /* Enum values */
      for (i = 0; i < tp->d.values.count; ++i)
        {
          tt_name (tp->d.values.list[i].name);
          tt_unsigned_span (tp->d.values.list[i].index);
        }
      tt_end ();
      *hll = tp->index = hll_type_index++;
      break;

    case ty_memfunc:
      make_type (tp->d.memfunc.type, &ti_1);
      qual = 0;
      if (tp->d.memfunc.flags & MEMBER_STATIC)
        qual |= 0x01;
      if (tp->d.memfunc.flags & MEMBER_CONST)
        qual |= 0x04;
      if (tp->d.memfunc.flags & MEMBER_VOLATILE)
        qual |= 0x08;
      if (tp->d.memfunc.flags & MEMBER_VIRTUAL)
        qual |= 0x10;
      tt_start (0x45, qual);          /* Member Function */
      tt_byte (tp->d.memfunc.flags & MEMBER_VIS); /* Protection */
      if (tp->d.memfunc.flags & MEMBER_CTOR)
        tt_byte (1);                  /* Constructor */
      else if (tp->d.memfunc.flags & MEMBER_DTOR)
        tt_byte (2);                  /* Destructor */
      else
        tt_byte (0);                  /* Regular member function */
      tt_word (ti_1);                 /* Type index of function */
      if (tp->d.memfunc.flags & MEMBER_VIRTUAL)
        tt_unsigned_span (tp->d.memfunc.offset); /* Index into vtable */
      tt_enc (tp->d.memfunc.name);    /* Name of the function */
      tt_end ();
      *hll = tp->index = hll_type_index++;
      break;

    case ty_baseclass:
      make_type (tp->d.baseclass.type, &ti_1);
      qual = 0;
      if (tp->d.baseclass.flags & MEMBER_VIRTUAL)
        qual |= 0x01;
      tt_start (0x41, qual);      /* Base Class */
      tt_byte (tp->d.baseclass.flags & MEMBER_VIS);
      tt_word (ti_1);             /* Type */
      tt_unsigned_span (tp->d.baseclass.offset);
      tt_end ();
      *hll = tp->index = hll_type_index++;
      break;

    default:
      error ("make_type: unknown tag %d", tp->tag);
    }
}


/* Parse an unnamed stabs type and return an HLL type index for it. */

static int hll_type (void)
{
  struct type *tp;
  int ti;

  tp = parse_complete_type (NULL);
  make_type (tp, &ti);
  return ti;
}


/* Return the symbol name of symbol *INDEX, concatenating the names of
   successive symbol table entries if necessary.  A pointer to the
   string is stored to *STR.  If symbols are concatenated, *INDEX is
   adjusted to refer to the last symbol table entry used, in order to
   refer to the next symbol after incrementing by one.  If TRUE is
   returned, the caller should free the string when it is no longer
   needed.  If FALSE is returned, the string must not be freed. */

static int concat_symbols (int *index, const char **str)
{
  int i, n, len;
  char *new;
  const char *p;

  len = 0;
  for (i = *index; i < sym_count; ++i)
    {
      p = (char *)str_ptr + sym_ptr[i].n_un.n_strx;
      n = strlen (p);
      if (n > 0 && p[n-1] == '\\')
        len += n - 1;
      else
        {
          len += n;
          break;
        }
    }
  if (i == *index)
    {
      *str = (char *)str_ptr + sym_ptr[i].n_un.n_strx;
      return FALSE;
    }
  else
    {
      new = xmalloc (len + 1);
      *str = new;
      for (i = *index; i < sym_count; ++i)
        {
          p = (char *)str_ptr + sym_ptr[i].n_un.n_strx;
          n = strlen (p);
          if (n > 0 && p[n-1] == '\\')
            {
              memcpy (new, p, n - 1);
              new += n - 1;
            }
          else
            {
              memcpy (new, p, n);
              new += n;
              break;
            }
        }
      *new = 0;
      *index = i;
      return TRUE;
    }
}


/* Parse a stabs type definition and store the internal representation
   of that type.  The sym_ptr index is passed in INDEX.  As we might
   have to concatenate successive symbol table entries, *INDEX is
   updated to refer to the next symbol. */

static void parse_typedef (int *index)
{
  char *name;
  const char *str, *p;
  struct type *t;
  int n, alloc_flag;

  alloc_flag = concat_symbols (index, &str);
  parse_start = str;
  parse_pindex = index;
  p = nextcolon (str);
  if (p != NULL)
    {
      n = p - str;
      name = alloca (n + 1);
      memcpy (name, str, n);
      name[n] = 0;
#if defined (HLL_DEBUG)
      printf ("LSYM/LCSYM/GSYM/PSYM/RSYM/STSYM/FUN %s\n", str);
#endif
      parse_ptr = p + 1;
      switch (*parse_ptr)
        {
        case 'f':
        case 'F':
          ++parse_ptr;
          /* Note: don't call parse_complete_type() as there's a comma
             at the end for nested functions. */
          t = parse_type (NULL);
          break;

        case 't':
        case 'T':
          ++parse_ptr;
          if (*parse_ptr == 't')
            ++parse_ptr;        /* synonymous type */
          t = parse_complete_type (name);
#if defined (HLL_DEBUG)
          printf ("  type: ");
          show_type (t);
          printf ("\n");
#else
          (void)t;
#endif
          break;

        case 'p':
        case 'r':
        case 'G':
        case 'S':
        case 'V':
          ++parse_ptr;
          t = parse_complete_type (name);
#if defined (HLL_DEBUG)
          printf ("  type: ");
          show_type (t);
          printf ("\n");
#else
          (void)t;
#endif
          break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          t = parse_complete_type (name);
#if defined (HLL_DEBUG)
          printf ("  type: ");
          show_type (t);
          printf ("\n");
#else
          (void)t;
#endif
          break;
        }
    }
  if (alloc_flag)
    free ((void *)str);         /* remove const attribute */
}


/* Write a `begin' subrecord to the SST.  The block begins at ADDR in
   the text segment. */

static size_t hll_begin (dword addr)
{
  size_t ptr;
  struct relocation_info r;

  sst_start (SST_begin);
  r.r_address = sst.size;
  ptr = sst.size;
  buffer_dword (&sst, addr);    /* Segment offset */
  buffer_dword (&sst, 0);       /* Length of block -- patched later */
#if 1
  buffer_nstr (&sst, "");       /* Name of block (optional) */
#endif
  sst_end ();

  r.r_extern = 0;
  r.r_length = 2;
  r.r_symbolnum = N_TEXT;
  r.r_pcrel = 0;
  r.r_pad = 0;
  buffer_mem (&sst_reloc, &r, sizeof (r));
  return ptr;
}


/* Start of a block.  Write a `begin' subrecord to the SST.  The block
   begins at ADDR in the text segment.  Push the block onto the block
   stack. */

static void block_begin (dword addr)
{
  size_t ptr;

  ptr = hll_begin (addr);
  grow_by (&block_grow, 1);
  block_stack[block_grow.count].patch_ptr = ptr;
  block_stack[block_grow.count].addr = addr;
  ++block_grow.count;
}


/* Write an `end' subrecord to the SST.  PATCH_PTR is the sst index of
   the `begin' subrecord for this block.  Add ADD_START to the start
   address of the block and set the length of the block to LENGTH, in
   bytes. */

static void hll_end (size_t patch_ptr, int add_start, size_t length)
{
  *(dword *)(sst.buf+patch_ptr+0) += add_start;
  *(dword *)(sst.buf+patch_ptr+4) = length;
  sst_start (SST_end);
  sst_end ();
}


/* Return TRUE iff symbol table entry S defines a function. */

static int is_fun (const struct nlist *s)
{
  const char *p;

  if (s->n_type != N_FUN)
    return FALSE;
  p = nextcolon ((char *)str_ptr + s->n_un.n_strx);
  if (p == NULL)
    return FALSE;
  return (p[1] == 'F' || p[1] == 'f');
}


/* Approximate a function prologue length by examining the code.  The
   function starts at START and ends at END. */

static int get_prologue_length (dword start, dword end)
{
  dword i;

  if (end > text_size)
    end = text_size;
  i = start;
  if (i < end && text_ptr[i] == 0x55) ++i;                  /* PUSH EBP */
  if (i + 1 < end && text_ptr[i+0] == 0x89
      && text_ptr[i+1] == 0xe5)                             /* MOV  EBP, ESP */
    i += 2;
  if (i + 2 < end && text_ptr[i+0] == 0x83
      && text_ptr[i+1] == 0xec)                             /* SUB  ESP, n8 */
    i += 3;
  return i - start;
}


/* End of a function reached.  Try to find the function length.  I is
   the sym_ptr index of the current symbol -- we search the symbol
   table starting at I to find the next function for estimating the
   function length. */

static void fun_end (int i)
{
  int p_len;
  dword addr;

  addr = text_size;
  while (i < sym_count)
    {
      if (is_fun (&sym_ptr[i]))
        {
          addr = sym_ptr[i].n_value;
          break;
        }
      ++i;
    }
  if (prologue_length != -1)
    p_len = prologue_length;
  else
    p_len = get_prologue_length (proc_start_addr, addr);
  *(dword *)(sst.buf+proc_patch_base + 0) = addr - proc_start_addr;
  *(word *)(sst.buf+proc_patch_base + 4) = p_len;
  *(dword *)(sst.buf+proc_patch_base + 6) = addr - proc_start_addr;
  proc_patch_base = 0;
  sst_start (SST_end);
  sst_end ();
}


/* End of a block (N_RBRAC encountered).  I is the sym_ptr index of
   the N_RBRAC symbol.  If we reach the outmost block level, we call
   fun_end(). */

static void block_end (int i)
{
  if (block_grow.count == 0)
    {
      /** @todo fix this - broken after my last change (bird 2003-10-06) */
      /* warning ("Invalid N_RBRAC without N_LBRAC"); */
    }
  else
    {
      --block_grow.count;
      hll_end (block_stack[block_grow.count].patch_ptr, 0,
               sym_ptr[i].n_value - block_stack[block_grow.count].addr);
      if (block_grow.count == 1) /* we make one extra begin */
        {
          --block_grow.count;
          hll_end (block_stack[block_grow.count].patch_ptr, 0,
                   sym_ptr[i].n_value - block_stack[block_grow.count].addr);
        }
      if (block_grow.count == 0 && proc_patch_base != 0)
        fun_end (i);
    }
}


/* Define a function.  SYMBOL points to the sym_ptr entry.  The
   function arguments are taken from the args_list array.  Several
   fields of the SST entry are filled-in later, when reaching the end
   of the function.  proc_patch_base is used for remembering the place
   where to patch the SST entry. */

static void define_fun (const struct nlist *symbol)
{
  struct type t, *t1, *t2, *t3;
  struct relocation_info r;
  const char *str, *p, *p2;
  const char *name, *mnglname;
#ifdef DEMANGLE_PROC_NAMES
  char *pszFree;
#endif
  size_t cchName, cchMnglName;
  int ti, ticlass;

  str = (char *)str_ptr + symbol->n_un.n_strx;
  p = nextcolon (str);
  if (p == NULL)
    abort ();
  cchName = cchMnglName = p - str;
  name = mnglname = strpool_addn (str_pool, str, cchMnglName);
  proc_start_addr = symbol->n_value;

#ifdef DEMANGLE_PROC_NAMES
  /* demangle the name */
  pszFree = cplus_demangle (mnglname, DMGL_ANSI | DMGL_PARAMS);
  if (!pszFree && (*mnglname == '_' || *mnglname == '*'))
    pszFree = cplus_demangle (mnglname + 1, DMGL_ANSI | DMGL_PARAMS);
  if (pszFree)
    {
      cchName = strlen (pszFree);
      name = pszFree;
    }
  else if (*mnglname == '*')
    {
      cchName--;
      name++;
    }
#endif

  /* now let's see if there is a memfunc for this name. */
  for (t1 = type_head; t1; t1 = t1->next)
    if (    t1->tag == ty_memfunc
        &&  t1->d.memfunc.mnglname == mnglname)
      break;

  if (t1)
    { /* C++ member function */
      t2 = t1->d.memfunc.type;
      if (t2 && t2->tag == ty_func && t2->d.func.domain)
        make_type (t2->d.func.domain, &ticlass);
      else
        { /* the hard way - search for the freaking string in suitable .stabs.
           * I think this is rather slow when done for many members.. :/ */
          int i;
          for (i = 0, t3 = NULL, ticlass = -1; !t3 && i < sym_count; ++i)
            switch (sym_ptr[i].n_type)
              {
              case N_LSYM:
                {
                  str = (char *)str_ptr + sym_ptr[i].n_un.n_strx;
                  p = nextcolon (str);
                  if (p && p[1] == 'T' && p[2] == 't')
                    {
                      /* Now, there can be several name which NAME is a
                       * substring in.
                       */
                      /** @todo: the string parsing still is a bit spooky. */
                      for (p2 = p; (p2 = strstr (p2, mnglname)) != NULL; p2 += cchMnglName)
                        if (p2[-1] == ':' && p2[cchMnglName] == ';')
                        {
                            int stab = atoi (&p[3]);
                            t3 = stype_find (stab);
                            break;
                        }
                    }
                  break;
                }
              }
          if (t3)
            make_type (t3, &ticlass);
          if (ticlass < 0)
            {
              warning (" Can't figure out which class method '%s' is a member of!", mnglname);
              ticlass = 0;
            }
        }

      make_type (t1, &ti);
      sst_start (SST_memfunc);
      r.r_address = sst.size;
      buffer_dword (&sst, symbol->n_value); /* Segment offset */
      buffer_word (&sst, ti);       /* Type index */
      proc_patch_base = sst.size;
      buffer_dword (&sst, 0);       /* Length of proc */
      buffer_word (&sst, 0);        /* Length of prologue */
      buffer_dword (&sst, 0);       /* Length of prologue and body */
      buffer_word (&sst, ticlass);  /* Class type (for member functions) */
      buffer_byte (&sst, 8);        /* 32-bit near */
      buffer_enc (&sst, name);      /* Proc name */
      sst_end ();
    }
  else
    {
      if (args_grow.count == 0)
        t1 = NULL;
      else
        {
          t.tag = ty_args;
          t.index = -1;
          t.d.args.count = args_grow.count;
          t.d.args.list = xmalloc (args_grow.count * sizeof (*args_list));
          memcpy (t.d.args.list, args_list, args_grow.count * sizeof (*args_list));
          t1 = type_add (&t);
          free (t.d.args.list);
        }
      last_fun_type.d.func.args = t1;
      last_fun_type.d.func.arg_count = args_grow.count;
      t2 = type_add (&last_fun_type);
      make_type (t2, &ti);

      sst_start (cplusplus_flag || cchName > 255 ? SST_CPPproc : SST_proc);
      r.r_address = sst.size;
      buffer_dword (&sst, symbol->n_value); /* Segment offset */
      buffer_word (&sst, ti);       /* Type index */
      proc_patch_base = sst.size;
      buffer_dword (&sst, 0);       /* Length of proc */
      buffer_word (&sst, 0);        /* Length of prologue */
      buffer_dword (&sst, 0);       /* Length of prologue and body */
      buffer_word (&sst, 0);        /* Class type (for member functions) */
      buffer_byte (&sst, 8);        /* 32-bit near */
      if (cplusplus_flag || cchName > 255)
        buffer_enc (&sst, name);    /* Proc name */
      else
        buffer_nstr (&sst, name);   /* Proc name */
      sst_end ();
    }
  r.r_extern = 0;
  r.r_length = 2;
  r.r_symbolnum = N_TEXT;
  r.r_pcrel = 0;
  r.r_pad = 0;
  buffer_mem (&sst_reloc, &r, sizeof (r));
  set_hll_type (symbol - sym_ptr, ti);

#ifdef DEMANGLE_PROC_NAMES
  /* free the demangled name. */
  free (pszFree);
#endif
}


/* Locate the next N_LBRAC starting at symbol I.  If found, set
   lbrac_index to the index of N_LBRAC and start a new block.  If
   there is no N_LBRAC, set lbrac_index to -1. */

static void next_lbrac (int i)
{
  lbrac_index = -1;
  for (; i < sym_count; ++i)
    if (sym_ptr[i].n_type == N_RBRAC || is_fun (&sym_ptr[i]))
      return;
    else if (sym_ptr[i].n_type == N_LBRAC)
      {
        lbrac_index = i;
        block_begin (sym_ptr[i].n_value);
        return;
      }
}


/* Parse a stabs symbol.  The sym_ptr index is passed in INDEX.  As we
   might have to concatenate successive symbol table entries, *INDEX
   is updated to refer to the next symbol.  WHERE is the segment where
   the symbol is defined (N_DATA, for instance).  If WHERE is -1,
   there is no segment.  MSG is used for warning messages. This
   function is called recursively for static functions (F) There's up
   to one level of recursion.  The arguments of a function are read
   twice: If FLAG is TRUE, we turn the arguments into local symbols;
   if FLAG is FALSE, we put arguments into the args_list array. */

static void parse_symbol (int *index, int where, const char *msg, int flag)
{
  char *name;
  const char *str, *p;
  const struct nlist *symbol;
  struct type *t1;
  int i, n, ti, alloc_flag;

  symbol = &sym_ptr[*index];
  alloc_flag = concat_symbols (index, &str);
  parse_start = str;
  parse_pindex = index;
  p = nextcolon (str);
  if (p != NULL)
    {
      n = p - str;
      name = alloca (n + 1);
      memcpy (name, str, n);
      name[n] = 0;
#if defined (HLL_DEBUG)
      printf ("%s %s\n", msg, str);
#endif

      parse_ptr = p + 1;
      switch (*parse_ptr)
        {
        case 'G':
          /* Static storage, global scope */
          {
          const struct nlist *sym2 = NULL;
          ++parse_ptr;
          ti = hll_type ();
#if defined (HLL_DEBUG)
          printf ("  type=%#x\n", ti);
#endif

          /* Check for a ugly hack in dbxout.c which allow us to resolve
             communal and global variables more accuratly. The n_value is -12357
             and the previous entry is 0xfe. What the previous entry contains
             is the assembly name of the symbol.

             Another part of that hack is that n_value is contains the
             symbol value so we don't need to look up the symbol for that
             reason (but as it turns out we have to look it up to see where
             it is in most cases, bah). */
          if (   symbol->n_value == -12357
              && *index >= 1
              && symbol[-1].n_type == 0xfe)
            {
              sym2 = find_symbol_ex (symbol[-1].n_un.n_strx + (char *)str_ptr, *index - 1, 1);
              if (!sym2)
                {
                  warning ("Cannot find address of communal/external variable %s", name);
                  return;
                }
            }
          else if (where == -1)
            { /* This is not very nice, we have to look up the symbol
                 like we used to do in order to the the segment right.
                 Should probably just use the external/communal hack above
                 for everything or change the n_type of the 'G' stab... However,
                 this will work for most cases and it will be compatible with
                 the unmodified dbxout.c code. */
              if (symbol->n_type == N_FUN) /* in case GCC change... */
                  where = N_TEXT;
              else
                {
                  size_t cch = strlen (name);
                  char *psz = alloca (cch + 2);
                  *psz = '_';
                  memcpy (psz + 1, name, cch + 1);
                  sym2 = find_symbol (psz);
                  if (!sym2)
                    sym2 = find_symbol (psz + 1);
                  where = N_DATA;
                }
            }
          /* if we're lucky we've got a symbol... */
          if (sym2)
            {
              if (sym2->n_type == N_EXT)
                sst_static (name, 0, ti, sym2 - sym_ptr, 1, sym2);
              else
                sst_static (name, sym2->n_value, ti, sym2->n_type, 0, sym2);
            }
          else
            sst_static (name, symbol->n_value, ti, where, 0, NULL);
          break;
          }

        case 'S':
        case 'V':

          /* S = static storage, file scope,
             V = static storage, local scope */

          if (where == -1)
            {
              warning ("Cannot use symbol type %c with N_%s", *parse_ptr, msg);
              return;
            }
          ++parse_ptr;
          ti = hll_type ();
#if defined (HLL_DEBUG)
          printf ("  type=%#x\n", ti);
#endif
          sst_static (name, symbol->n_value, ti, where, 0, NULL);
          break;

        case 'p':

          /* Function argument. */

          if (where != -1)
            {
              warning ("Cannot use argument symbol with N_%s", msg);
              return;
            }
          ++parse_ptr;
          if (flag)
            {
              ti = hll_type ();
#if defined (HLL_DEBUG)
              printf ("  type=%#x\n", ti);
#endif
              sst_start (SST_auto);
              buffer_dword (&sst, symbol->n_value);/* Offset into stk frame */
              buffer_word (&sst, ti);              /* Type index */
              buffer_nstr (&sst, name);            /* Symbol name */
              sst_end ();
            }
          else
            {
              t1 = parse_complete_type (NULL);
              grow_by (&args_grow, 1);
              args_list[args_grow.count++] = t1;
            }
          break;

        case 'r':

          /* Register variable. */

          if (where != -1)
            {
              warning ("Cannot use register symbol with N_%s", msg);
              return;
            }
          ++parse_ptr;
          ti = hll_type ();
          switch (symbol->n_value)
            {
            case 0:             /* EAX */
              i = 0x10;
              break;
            case 1:             /* ECX */
              i = 0x11;
              break;
            case 2:             /* EDX */
              i = 0x12;
              break;
            case 3:             /* EBX */
              i = 0x13;
              break;
            case 4:             /* ESP */
              i = 0x14;
              break;
            case 5:             /* EBP */
              i = 0x15;
              break;
            case 6:             /* ESI */
              i = 0x16;
              break;
            case 7:             /* EDI */
              i = 0x17;
              break;
            case 12:            /* ST(0) */
              i = 0x80;
              break;
            case 13:            /* ST(1) */
              i = 0x81;
              break;
            case 14:            /* ST(2) */
              i = 0x82;
              break;
            case 15:            /* ST(3) */
              i = 0x83;
              break;
            case 16:            /* ST(4) */
              i = 0x84;
              break;
            case 17:            /* ST(5) */
              i = 0x85;
              break;
            case 18:            /* ST(6) */
              i = 0x86;
              break;
            case 19:            /* ST(7) */
              i = 0x87;
              break;
            default:
              warning ("unknown register %lu", symbol->n_value);
              return;
            }
          sst_start (SST_reg);      /* Register variable */
          buffer_word (&sst, ti);   /* Type index */
          buffer_byte (&sst, i);    /* Register number */
          buffer_nstr (&sst, name); /* Symbol name */
          sst_end ();
          break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':

          /* Local symbol. */

          if (where != -1)
            {
              warning ("Cannot use local symbol with N_%s", msg);
              return;
            }
          ti = hll_type ();
#if defined (HLL_DEBUG)
          printf ("  type=%#x\n", ti);
#endif
          sst_start (SST_auto);
          buffer_dword (&sst, symbol->n_value);  /* Offset into stk frame */
          buffer_word (&sst, ti);                /* Type index */
          buffer_nstr (&sst, name);              /* Symbol name */
          sst_end ();
          break;

        case 'F':
        case 'f':

          /* Function. */

          ++parse_ptr;
          if (where != N_TEXT)
            {
              warning ("Cannot use function with N_%s", msg);
              return;
            }
          last_fun_type.tag = ty_func;
          last_fun_type.index = -1;
          last_fun_type.d.func.domain = NULL;
          last_fun_type.d.func.ret = parse_type (NULL);

          /* Don't check for end of string -- parse_ptr points to a
             comma for nested functions! */

          last_fun_type.d.func.args = NULL;
          last_fun_type.d.func.arg_count = 0;
          last_fun_valid = TRUE;
          args_grow.count = 0;
          last_fun_addr = symbol->n_value;
          prologue_length = -1;

          for (i = *index + 1; i < sym_count && sym_ptr[i].n_type == N_PSYM; ++i)
            parse_symbol (&i, -1, "PSYM", FALSE);

          define_fun (symbol);

          /* Now look for N_LBRAC */
          next_lbrac (i);
          if (lbrac_index != -1)
            {
              if (prologue_length == -1)
                prologue_length = sym_ptr[lbrac_index].n_value - last_fun_addr;
            }
          else
            {/* kso #456 2003-06-05: We need a begin to catch the parameters. */
              prologue_length = 0;
              block_begin (proc_start_addr);
            }

          /* Parameters */
          while (*index + 1 < sym_count && sym_ptr[*index+1].n_type == N_PSYM)
            {
              ++(*index);
              parse_symbol (index, -1, "PSYM", TRUE);
            }

          /* Start another block for the local variables. This is to work
             around some missing bracets in stabs and to fix problem with
             C++ bool type.  */
          dword addr = proc_start_addr + prologue_length;
          if (lbrac_index >= 0 && sym_ptr[lbrac_index].n_value > addr)
            addr = sym_ptr[lbrac_index].n_value;
          block_begin (addr);

          /* Local variables  (fix for boolparam testcase too). */
          while (*index + 1 < sym_count && sym_ptr[*index+1].n_type == N_LSYM)
            {
              ++(*index);
              parse_symbol (index, -1, "LSYM", TRUE);
            }

          /* If there's no N_LBRAC, write SST_end now */
          if (lbrac_index == -1)
            {/* kso #456 2003-06-05: We need a begin to catch the parameters. */
              while (block_grow.count > 0)
                block_end (*index+1);
            }
          break;
        }
    }
  if (alloc_flag)
    free ((void *)str);         /* Remove the const attribute */
}


/* Create a type tag for HLL type index INDEX.  TYPE is either SST_tag
   (for structures and unions) or SST_tag2 (for classes).  NAME is
   the name of the structure, union or class. */

static void type_tag (int type, int index, const char *name)
{
  if (name != NULL)
    {
      size_t cch;
      if (type == SST_tag2 || (cch = strlen(name)) > 250)
       {
          sst_start (SST_tag2);
          buffer_word (&sst, index);
          buffer_enc (&sst, name);
        }
      else
        {
          char  achName[256];
          memcpy (achName, name, cch);
          achName[cch] = '\0';

          sst_start (type);
          buffer_word (&sst, index);
          buffer_nstr (&sst, achName);
        }
      sst_end ();
    }
}


/* Write a CuInfo SST entry to tell the debugger what compiler (C or
   C++) has been used. */

static void write_cuinfo (void)
{
  struct timestamp ts;
  time_t now;
  struct tm *tp;
  byte lang;

  if (cplusplus_flag)
    lang = 0x02;                /* C++ */
  else
    lang = 0x01;                /* C */

  time (&now);
  tp = localtime (&now);
  ts.year = tp->tm_year + 1900;
  ts.month = tp->tm_mon + 1;
  ts.day = tp->tm_mday;
  ts.hours = tp->tm_hour;
  ts.minutes = tp->tm_min;
  ts.seconds = tp->tm_sec;
  ts.hundredths = 0;
  sst_start (SST_cuinfo);
  buffer_byte (&sst, lang);
  buffer_nstr (&sst, "    ");       /* Compiler options */
  buffer_nstr (&sst, __DATE__);     /* Compiler date */
  buffer_mem (&sst, &ts, sizeof (ts));
  sst_end ();
}


/* Write a ChangSeg SST entry to define the text segment. */

static void write_changseg (void)
{
  struct relocation_info r;

  sst_start (SST_changseg);
  r.r_address = sst.size;
  buffer_word (&sst, 0);        /* Segment number */
  buffer_word (&sst, 0);        /* Reserved */
  sst_end ();
  r.r_extern = 0;
  r.r_length = 1;
  r.r_symbolnum = N_TEXT;
  r.r_pcrel = 0;
  r.r_pad = 0;
  buffer_mem (&sst_reloc, &r, sizeof (r));
}


/* Convert debug information.  The data is taken from the symbol and
   string tables of the current a.out module.  Output is put into the
   tt, sst etc. buffers. */

void convert_debug (void)
{
  int i;
  struct type *t1, *t2;
#if defined (HLL_DEBUG)
  setvbuf(stdout, NULL, _IONBF, 256);
#endif

  str_pool = strpool_init ();
  grow_init (&stype_grow, &stype_list, sizeof (*stype_list), 32);
  type_head = NULL; hll_type_index = 512;
  grow_init (&block_grow, &block_stack, sizeof (*block_stack), 16);
  grow_init (&args_grow, &args_list, sizeof (*args_list), 16);
  last_fun_valid = FALSE; prologue_length = -1;
  unnamed_struct_number = 1;

  /* Check whether converting a translated C++ program. */
#if 1 /* kso #465 2003-06-04: pretend everything is C++, that symbols is no longer present. */
  cplusplus_flag = 1;
#else
  cplusplus_flag = (find_symbol ("__gnu_compiled_cplusplus") != NULL);
#endif


  /* Parse the typedefs to avoid forward references. */

  for (i = 0; i < sym_count; ++i)
    switch (sym_ptr[i].n_type)
      {
      case N_LSYM:
      case N_LCSYM:
      case N_GSYM:
      case N_PSYM:
      case N_RSYM:
      case N_STSYM:
      case N_FUN:
        parse_typedef (&i);
        break;
      }

  /* Provide information about the compiler. */

  write_cuinfo ();

  /* Change the default segment. */

  write_changseg ();

  /* Parse the other symbol table entries. */

  lbrac_index = -1; block_grow.count = 0; proc_patch_base = 0;
  for (i = 0; i < sym_count; ++i)
    switch (sym_ptr[i].n_type)
      {
      case N_LSYM:
        parse_symbol (&i, -1, "LSYM", TRUE);
        break;

      case N_GSYM:
        parse_symbol (&i, -1, "GSYM", TRUE);
        break;

      case N_LCSYM:
        parse_symbol (&i, N_BSS, "LCSYM", TRUE);
        break;

      case N_STSYM:
        parse_symbol (&i, N_DATA, "STSYM", TRUE);
        break;

      case N_RSYM:
        parse_symbol (&i, -1, "RSYM", TRUE);
        break;

      case N_LBRAC:
        if (lbrac_index == -1)  /* N_LBRAC without local symbols */
          block_begin (sym_ptr[i].n_value);
        else if (i != lbrac_index)
          warning ("Invalid N_LBRAC");
        next_lbrac (i + 1);
        break;

      case N_RBRAC:
        block_end (i);
        next_lbrac (i + 1);
        break;

      case N_FUN:
        parse_symbol (&i, N_TEXT, "FUN", TRUE);
        break;
      }

#ifdef HASH_DEBUG
fprintf(stderr, "\n");
for (i = 0; i < TYPE_HASH_MAX; i++)
  if (i % 7 == 6)
    fprintf(stderr, "%3d: %4d\n", i, ctype_tag[i]);
  else
    fprintf(stderr, "%3d: %4d   ", i, ctype_tag[i]);
fprintf(stderr, "\nctypes=%d\n", ctypes);
for (i = 0; i < TYPE_HASH_MAX; i++)
  if (ctype_tag[i] > 1000)
    {
      int       j;
      unsigned  au[ty_max];
      memset(&au[0], 0, sizeof(au));
      for (t1 = atype_tag[i]; t1 != NULL; t1 = t1->nexthash)
        au[t1->tag]++;

      fprintf(stderr, "\nIndex %d is overpopulated (%d):\n", i, ctype_tag[i]);
      for (j = 0; j < 18; j++)
        if (j % 7 == 6)
          fprintf(stderr, "%2d: %4d\n", j, au[j]);
        else
          fprintf(stderr, "%2d: %4d   ", j, au[j]);
    }
#endif

  /* Create tags for structures, unions and classes. */

  for (t1 = type_head; t1 != NULL; t1 = t1->next)
    if (t1->index != -1)
      switch (t1->tag)
        {
        case ty_struc:
          type_tag (SST_tag, t1->index, t1->d.struc.name);
          break;
        case ty_class:
          type_tag (SST_tag2, t1->index, t1->d.class.name);
          break;
        case ty_enu:
          type_tag (SST_tag, t1->index, t1->d.enu.name);
          break;
        default:
          break;
        }

#ifdef HLL_DEBUG
  printf ("Stabs to HLL type mappings: %d/%d\n", stype_grow.count, stype_grow.alloc);
  for (i = 0; i < stype_grow.alloc; i++)
    if (stype_list[i])
      {
        printf ("  %3d => 0x%03x/%-3d  ", i + 1, stype_list[i]->index, stype_list[i]->index);
        show_type (stype_list[i]);
        printf("\n");
      }
#endif


  /* Deallocate memory. */

  grow_free (&stype_grow);
  grow_free (&args_grow);
  grow_free (&block_grow);
  for (t1 = type_head; t1 != NULL; t1 = t2)
    {
      t2 = t1->next;
      switch (t1->tag)
        {
        case ty_args:
          if (t1->d.args.list != NULL)
            free (t1->d.args.list);
          break;
        case ty_types:
          if (t1->d.types.list != NULL)
            free (t1->d.types.list);
          break;
        case ty_fields:
          if (t1->d.fields.list != NULL)
            free (t1->d.fields.list);
          break;
        case ty_values:
          if (t1->d.values.list != NULL)
            free (t1->d.values.list);
          break;
        default:
          break;
        }
      free (t1);
    }
  type_head = NULL;
  memset (&atype_tag, 0, sizeof(atype_tag));
  strpool_free (str_pool);
  str_pool = NULL;
}
