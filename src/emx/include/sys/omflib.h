/* sys/omflib.h (emx+gcc) */

/* Public header file for the emx OMFLIB library. */

#ifndef _SYS_OMFLIB_H
#define _SYS_OMFLIB_H

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef _BYTE_WORD_DWORD
#define _BYTE_WORD_DWORD
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
#endif /* _BYTE_WORD_DWORD */

#if !defined (THEADR)

/* OMF record types.  To get the 32-bit variant of a record type, add
   REC32. */
#define THEADR          0x80    /* Translator module header record */
#define COMENT          0x88    /* Comment record */
#define MODEND          0x8a    /* Module end record */
#define EXTDEF          0x8c    /* External names definition record */
#define TYPDEF          0x8e    /* Type definition record */
#define PUBDEF          0x90    /* Public names definition record */
#define LINNUM          0x94    /* Line numbers record */
#define LNAMES          0x96    /* List of names record */
#define SEGDEF          0x98    /* Segment definition record */
#define GRPDEF          0x9a    /* Group definition record */
#define FIXUPP          0x9c    /* Fixup record */
#define LEDATA          0xa0    /* Logical enumerated data record */
#define LIDATA          0xa2    /* Logical iterated data record */
#define COMDEF          0xb0    /* Communal names definition record */
#define COMDEF_TYPEFAR  0x61            /* Commnual name type: far */
#define COMDEF_TYPENEAR 0x62            /* Commnual name type: near */
#define COMDAT          0xc2    /* Common block */
#define ALIAS           0xc6    /* Alias definition record */
#define LIBHDR          0xf0    /* Library header */
#define LIBEND          0xf1    /* Library end */

/* Add this constant (using the | operator) to get the 32-bit variant
   of a record type.  Some fields will contain 32-bit values instead
   of 16-bit values. */
#define REC32           0x01

#endif

/* Comment classes */
#define CLASS_TRANS     0x00            /* Translators - who uses this? */
#define CLASS_INTELC    0x01            /* Intel Copryight */
#define CLASS_MSDOSVER  0x9c            /* MS-DOS version - obsolete */
#define CLASS_MODEL     0x9d            /* Memory model - ignored by linker. */
#define CLASS_DOSSEG    0x9e            /* Linker DOSSEG switch. */
#define CLASS_DEFLIB    0x9f            /* Default library. */
#define CLASS_OMFEXT    0xa0            /* OMF extenstions */
#define OMFEXT_IMPDEF   0x01
#define OMFEXT_EXPDEF   0x02
#define OMFEXT_INCDEF   0x03
#define OMFEXT_PROTLIB  0x04
#define OMFEXT_LNKDIR   0x05
#define CLASS_DBGTYPE   0xa1            /* Debug type */
#define CLASS_PASS      0xa2            /* End of linker pass 1. */
#define CLASS_LIBMOD    0xa3            /* Library module comment. */
#define CLASS_EXESTR    0xa4            /* Executable Module Identification String. */
#define CLASS_INCERR    0xa6            /* Incremental compilation error. */
#define CLASS_NOPAD     0xa7            /* No segment padding. */
#define CLASS_WKEXT     0xa8            /* Weak external. */
#define CLASS_LZEXT     0xa9            /* Lazy external. */
#define CLASS_PHARLAP   0xaa            /* PharLap Format record. */
#define CLASS_IPADATA   0xae            /* Interprocedural Analysis Data record. */
#define CLASS_TIS       0xac            /* TIS standard level? 'TIS' + version word. */
#define CLASS_DBGPACK   0xad            /* /DBGPACK DLL name for ilink. */
#define CLASS_IDMDLL    0xaf            /* Identifier Manipulator Dynamic Link Library. */


#if !defined (IMPDEF_CLASS)
#define IMPDEF_CLASS    CLASS_OMFEXT
#define LIBMOD_CLASS    0xa3
#define IMPDEF_SUBTYPE  OMFEXT_IMPDEF
#endif

struct omflib;

struct omflib *omflib_open (const char *fname, char *error);
struct omflib *omflib_create (const char *fname, int page_size, char *error);
int omflib_close (struct omflib *p, char *error);
int omflib_module_name (char *dst, const char *src);
int omflib_find_module (struct omflib *p, const char *name, char *error);
int omflib_mark_deleted (struct omflib *p, const char *name, char *error);
int omflib_pubdef_walk (struct omflib *p, word page,
    int (*walker)(const char *name, char *error), char *error);
int omflib_extract (struct omflib *p, const char *name, char *error);
int omflib_add_module (struct omflib *p, const char *fname, char *error);
int omflib_module_count (struct omflib *p, char *error);
int omflib_module_info (struct omflib *p, int n, char *name, int *page,
    char *error);
int omflib_copy_lib (struct omflib *dst, struct omflib *src, char *error);
int omflib_finish (struct omflib *p, char *error);
int omflib_write_record (struct omflib *p, byte rec_type, word rec_len,
    const byte *buffer, int chksum, char *error);
int omflib_write_module (struct omflib *p, const char *name, word *pagep,
    char *error);
int omflib_add_pub (struct omflib *p, const char *name, word page,
    char *error);
int omflib_header (struct omflib *p, char *error);
int omflib_find_symbol (struct omflib *p, const char *name, char *error);
long omflib_page_pos (struct omflib *p, int page);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_OMFLIB_H */
