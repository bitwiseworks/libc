/* defs.h -- Various definitions for emx development utilities
   Copyright (c) 1992-1998 Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#define FALSE 0
#define TRUE  1

#ifdef __GNUC__
#define NORETURN2        __attribute__ ((noreturn))
#define ATTR_PRINTF(s,f) __attribute__ ((__format__ (__printf__, s, f)))
#else
#define NORETURN2
#define ATTR_PRINTF(s,f)
#endif

/* The magic number that is always the first DWORD in .data,
   followed by a offset to the __os2dll set. */
#define DATASEG_MAGIC	0xba0bab

/* The maximum OMF record size supported by OMF linkers.  This value
   includes the record type, length and checksum fields. */
#define MAX_REC_SIZE    1024

#ifndef _BYTE_WORD_DWORD
#define _BYTE_WORD_DWORD
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
#endif /* _BYTE_WORD_DWORD */

/* This is the header of a DOS executable file. */
struct exe1_header
{
  word magic;                   /* Magic number, "MZ" (0x5a4d) */
  word last_page;               /* Number of bytes in last page */
  word pages;                   /* Size of the file in 512-byte pages */
  word reloc_size;              /* Number of relocation entries */
  word hdr_size;                /* Size of header in 16-byte paragraphs */
  word min_alloc;               /* Minimum allocation (paragraphs) */
  word max_alloc;               /* Maximum allocation (paragraphs) */
  word ss, sp;                  /* Initial stack pointer */
  word chksum;                  /* Checksum */
  word ip, cs;                  /* Entry point */
  word reloc_ptr;               /* Location of the relocation table */
  word ovl;                     /* Overlay number */
};

/* This is an additional header of a DOS executable file.  It contains
   a pointer to the new EXE header. */
struct exe2_header
{
  word res1[16];                /* Reserved */
  word new_lo;                  /* Low word of the location of the header */
  word new_hi;                  /* High word of the location of the header */
};


/* exe1 + exe2, with more canonical member names. */
struct exe1p2_header
{
  word e_magic;
  word e_cblp;
  word e_cp;
  word e_crlc;
  word e_cparhdr;
  word e_minalloc;
  word e_maxalloc;
  word e_ss;
  word e_sp;
  word e_csum;
  word e_ip;
  word e_cs;
  word e_lfarlc;
  word e_ovno;
  word e_res[4];
  word e_oemid;
  word e_oeminfo;
  word e_res2[10];
  dword e_lfanew;
};

#define EXE_MAGIC_MZ 0x5a4d


/* This is the layout of the OS/2 LX header. */
struct os2_header
{
  word magic;                   /* Magic number, "LX" (0x584c) */
  byte byte_order;              /* Byte order */
  byte word_order;              /* Word order */
  dword level;                  /* Format level */
  word cpu;                     /* CPU type */
  word os;                      /* Operating system type */
  dword ver;                    /* Module version */
  dword mod_flags;              /* Module flags */
  dword mod_pages;              /* Number of pages in the EXE file */
  dword entry_obj;              /* Object number for EIP */
  dword entry_eip;              /* Entry point */
  dword stack_obj;              /* Object number for ESP */
  dword stack_esp;              /* Stack */
  dword pagesize;               /* System page size */
  dword pageshift;              /* Page offset shift */
  dword fixup_size;             /* Fixup section size */
  dword fixup_checksum;         /* Fixup section checksum */
  dword loader_size;            /* Loader section size */
  dword loader_checksum;        /* Loader section checksum */
  dword obj_offset;             /* Object table offset */
  dword obj_count;              /* Number of objects in module */
  dword pagemap_offset;         /* Object page table offset */
  dword itermap_offset;         /* Object iterated pages offset */
  dword rsctab_offset;          /* Resource table offset */
  dword rsctab_count;           /* Number of resource table entries */
  dword resname_offset;         /* Resident name table offset */
  dword entry_offset;           /* Entry table offset */
  dword moddir_offset;          /* Module directives offset */
  dword moddir_count;           /* Number of module directives */
  dword fixpage_offset;         /* Fixup page table offset */
  dword fixrecord_offset;       /* Fixup record table offset */
  dword impmod_offset;          /* Import module table offset */
  dword impmod_count;           /* Number of import module table entries */
  dword impprocname_offset;     /* Import procedure table offset */
  dword page_checksum_offset;   /* Per page checksum table offset */
  dword enum_offset;            /* Data pages offset */
  dword preload_count;          /* Number of preload pages */
  dword nonresname_offset;      /* Non-resident name table offset */
  dword nonresname_size;        /* Non-resident name table size */
  dword nonresname_checksum;    /* Non-resident name table checksum */
  dword auto_obj;               /* Auto data segment object number */
  dword debug_offset;           /* Debug information offset */
  dword debug_size;             /* Debug information size */
  dword instance_preload;       /* Number of instance preload pages */
  dword instance_demand;        /* Number of instance load-on-demand pages */
  dword heap_size;              /* Heap size */
  dword stack_size;             /* Stack size */
  dword reserved[5];            /* Reserved */
};

#define EXE_MAGIC_LX 0x584c


/* This is the layout of an object table entry. */
struct object
{
  dword virt_size;              /* Virtual size */
  dword virt_base;              /* Relocation base address */
  dword attr_flags;             /* Object attributes and flags */
  dword map_first;              /* Page table index */
  dword map_count;              /* Number of page table entries */
  dword reserved;               /* Reserved */
};

#pragma pack(1)

struct omf_rec
{
  byte rec_type;
  word rec_len;
};

#pragma pack()
