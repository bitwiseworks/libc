/* sys/moddef.h (emx+gcc) */

#ifndef _SYS_MODDEF_H
#define _SYS_MODDEF_H

#if defined (__cplusplus)
extern "C" {
#endif

struct _md;

#define _MDT_DEFAULT            0
#define _MDT_WINDOWAPI          1
#define _MDT_WINDOWCOMPAT       2
#define _MDT_NOTWINDOWCOMPAT    3

#define _MDIT_DEFAULT           0
#define _MDIT_INSTANCE          1
#define _MDIT_GLOBAL            2

#define _MDS_ALIAS              0x000001
#define _MDS_CONFORMING         0x000002
#define _MDS_DISCARDABLE        0x000004
#define _MDS_EXECUTEONLY        0x000008
#define _MDS_EXECUTEREAD        0x000010
#define _MDS_FIXED              0x000020
#define _MDS_INVALID            0x000040
#define _MDS_IOPL               0x000080
#define _MDS_LOADONCALL         0x000100
#define _MDS_MIXED1632          0x000200
#define _MDS_MOVEABLE           0x000400
#define _MDS_MULTIPLE           0x000800
#define _MDS_NOIOPL             0x001000
#define _MDS_NONCONFORMING      0x002000
#define _MDS_NONDISCARDABLE     0x004000
#define _MDS_NONE               0x008000
#define _MDS_NONSHARED          0x010000
#define _MDS_PRELOAD            0x020000
#define _MDS_READONLY           0x040000
#define _MDS_READWRITE          0x080000
#define _MDS_SHARED             0x100000
#define _MDS_SINGLE             0x200000

#define _MDX_DEFAULT            0
#define _MDX_UNKNOWN            1
#define _MDX_OS2                2
#define _MDX_WINDOWS            3

#define _MDEP_ORDINAL           0x0001
#define _MDEP_RESIDENTNAME      0x0002
#define _MDEP_NONAME            0x0004
#define _MDEP_NODATA            0x0008
#define _MDEP_PWORDS            0x0010

#define _MDIP_ORDINAL           0x0001

typedef enum
{
  _MD_ALIAS, _MD_BASE, _MD_CLASS, _MD_CODE, _MD_CONFORMING,
  _MD_CONTIGUOUS, _MD_DATA, _MD_DESCRIPTION, _MD_DEV386, _MD_DEVICE,
  _MD_DISCARDABLE, _MD_DOS4, _MD_DYNAMIC, _MD_EXECUTEONLY,
  _MD_EXECUTEREAD, _MD_EXETYPE, _MD_EXPANDDOWN, _MD_EXPORTS,
  _MD_FIXED, _MD_HEAPSIZE, _MD_HUGE, _MD_IMPORTS, _MD_IMPURE,
  _MD_INCLUDE, _MD_INITGLOBAL, _MD_INITINSTANCE, _MD_INVALID,
  _MD_IOPL, _MD_LIBRARY, _MD_LOADONCALL, _MD_MAXVAL, _MD_MIXED1632,
  _MD_MOVEABLE, _MD_MULTIPLE, _MD_NAME, _MD_NEWFILES, _MD_NODATA,
  _MD_NOEXPANDDOWN, _MD_NOIOPL, _MD_NONAME, _MD_NONCONFORMING,
  _MD_NONDISCARDABLE, _MD_NONE, _MD_NONPERMANENT, _MD_NONSHARED,
  _MD_NOTWINDOWCOMPAT, _MD_OBJECTS, _MD_OLD, _MD_ORDER, _MD_OS2,
  _MD_PERMANENT, _MD_PHYSICAL, _MD_PRELOAD, _MD_PRIVATE,
  _MD_PRIVATELIB, _MD_PROTECT, _MD_PROTMODE, _MD_PURE, _MD_READONLY,
  _MD_READWRITE, _MD_REALMODE, _MD_RESIDENT, _MD_RESIDENTNAME,
  _MD_SEGMENTS, _MD_SHARED, _MD_SINGLE, _MD_STACKSIZE, _MD_STUB,
  _MD_SWAPPABLE, _MD_TERMGLOBAL, _MD_TERMINSTANCE, _MD_UNKNOWN,
  _MD_VIRTUAL, _MD_WINDOWAPI, _MD_WINDOWCOMPAT, _MD_WINDOWS,
  /* The following tokens are no keywords: */
  _MD_dot, _MD_at, _MD_equal, _MD_quote, _MD_number, _MD_word,
  _MD_eof, _MD_ioerror, _MD_missingquote,
  /* Special `token' for _md_parse() callback: */
  _MD_parseerror
} _md_token;

typedef enum
{
  _MDE_IO_ERROR,
  _MDE_MISSING_QUOTE,
  _MDE_EMPTY,
  _MDE_NAME_EXPECTED,
  _MDE_STRING_EXPECTED,
  _MDE_NUMBER_EXPECTED,
  _MDE_DEVICE_EXPECTED,
  _MDE_EQUAL_EXPECTED,
  _MDE_DOT_EXPECTED,
  _MDE_STRING_TOO_LONG,
  _MDE_INVALID_ORDINAL,
  _MDE_INVALID_STMT
} _md_error;

/** The maximum symbol name size (terminator included). */ 
#define _MD_MAX_SYMBOL_SIZE     1536

typedef struct
{
  unsigned  line_number;                /* Linenumber of the statement. */
  union 
  {
    struct
      {
        _md_error code;
        _md_token stmt;
      } error;                    /* Error */
    struct
      {
        unsigned long addr;
      } base;                     /* BASE */
    struct
      {
        char string[256];
      } descr;                    /* DESCRIPTION */
    struct
      {
        char name[256];
      } device;                   /* PHYSICAL DEVICE and VIRTUAL DEVICE */
    struct
      {
        int type;
        int minor_version;
        int major_version;
      } exetype;                  /* EXETYPE */
    struct
      {
        char entryname[_MD_MAX_SYMBOL_SIZE];
        char internalname[_MD_MAX_SYMBOL_SIZE];
        int ordinal;
        int pwords;
        unsigned flags;
      } export;                   /* EXPORTS */
    struct
      {
        char entryname[_MD_MAX_SYMBOL_SIZE];
        char internalname[_MD_MAX_SYMBOL_SIZE];
        char modulename[256];
        int ordinal;
        unsigned flags;
      } import;                   /* IMPORTS */
    struct
      {
        unsigned long size;
        int maxval;
      } heapsize;                 /* HEAPSIZE */
    struct
      {
        char name[256];
        int init;
        int term;
      } library;                  /* LIBRARY */
    struct
      {
        char name[256];
        int pmtype;
        int newfiles;
      } name;                     /* NAME */
    struct
      {
        char name[256];
      } old;                      /* OLD */
    struct
      {
        char segname[256];
        char classname[256];
        unsigned attr;
      } segment;                  /* SEGMENTS, CODE, DATA */
    struct
      {
        unsigned long size;
      } stacksize;                /* STACKSIZE */
    struct
      {
        char name[256];
        int none;
      } stub;                     /* STUB */
  }; /* nameless */
} _md_stmt;


long _md_get_linenumber (const struct _md *md);
long _md_get_number (const struct _md *md);
const char *_md_get_string (const struct _md *md);
_md_token _md_get_token (const struct _md *md);

_md_token _md_next_token (struct _md *md);

int _md_close (struct _md *md);
struct _md *_md_open (const char *fname);
struct _md *_md_use_file (FILE *f);

int _md_parse (struct _md *md,
               int (*callback)(struct _md *md, const _md_stmt *stmt,
                               _md_token token, void *arg),
               void *arg);

const char *_md_errmsg (_md_error code);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_MODDEF_H */
