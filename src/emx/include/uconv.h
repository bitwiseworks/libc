/* uconv.h,v 1.3 2004/09/14 22:27:36 bird Exp */
/** @file
 * IGCC.
 */
/*
 * Legalesy-free Unicode API interface for OS/2
 * Unicode Conversions API
 *
 * Written by Andrew Zabolotny <bit@eltech.ru>
 *
 * This file is put into public domain. You are free to do
 * literally anything you wish with it: modify, print, sell,
 * rent, eat, throw out of window: in all (esp. in later)
 * cases I am not responsible for any damage it causes.
 */

#ifndef __UCONV_H__
#define __UCONV_H__

#include <unidef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

/* Substitution options */
#define UCONV_OPTION_SUBSTITUTE_FROM_UNICODE	1
#define UCONV_OPTION_SUBSTITUTE_TO_UNICODE	2
#define UCONV_OPTION_SUBSTITUTE_BOTH		3

/* Conversion options */
#define CVTTYPE_PATH		0x00000004   /* Treat string as a path   */
#define CVTTYPE_CDRA		0x00000002   /* Use CDRA control mapping */
#define CVTTYPE_CTRL7F		0x00000001   /* Treat 0x7F as a control  */

/* Conversion bit masks */
#define DSPMASK_DATA    0xffffffff
#define DSPMASK_DISPLAY 0x00000000
#define DSPMASK_TAB     0x00000200
#define DSPMASK_LF      0x00000400
#define DSPMASK_CR      0x00002000
#define DSPMASK_CRLF    0x00002400

#define ENDIAN_SYSTEM	0x0000
#define ENDIAN_BIG	0xfeff
#define ENDIAN_LITTLE	0xfffe

typedef struct _conv_endian_t
{
  unsigned short source;
  unsigned short target;
} conv_endian_t;

/* Encoding schemes */
enum uconv_esid
{					/* Process Display  VIO    GPI   */
  ESID_sbcs_data        = 0x2100,	/*    x      x      x       x    */
  ESID_sbcs_pc          = 0x3100,	/*    x      x      x       x    */
  ESID_sbcs_ebcdic      = 0x1100,	/*           x      x       x    */
  ESID_sbcs_iso         = 0x4100,	/*    x      x      x       x    */
  ESID_sbcs_windows     = 0x4105,	/*    x      x      x       x    */
  ESID_sbcs_alt         = 0xF100,	/*           x      x       x    */
  ESID_dbcs_data        = 0x2200,	/*           x              x    */
  ESID_dbcs_pc          = 0x3200,	/*    x      x      x       x    */
  ESID_dbcs_ebcdic      = 0x1200,	/*                          x    */
  ESID_mbcs_data        = 0x2300,	/*           x      x       x    */
  ESID_mbcs_pc          = 0x3300,	/*           x              x    */
  ESID_mbcs_ebcdic      = 0x1301,	/*                               */
  ESID_ucs_2            = 0x7200,	/*                               */
  ESID_ugl              = 0x72FF,	/*                               */
  ESID_utf_8            = 0x7807,	/*           x      x       x    */
  ESID_upf_8            = 0x78FF	/*    x      x      x       x    */
};

typedef struct _uconv_attribute_t
{
  unsigned long  version;		/* R/W Version (must be zero)        */
  char           mb_min_len;		/* R   Minimum char size             */
  char           mb_max_len;		/* R   Maximum char size             */
  char           usc_min_len;		/* R   UCS min size                  */
  char           usc_max_len;		/* R   UCS max size                  */
  unsigned short esid;			/* R   Encoding scheme ID            */
  char           options;		/* R/W Substitution options          */
  char           state;			/* R/W State for stateful convert    */
  conv_endian_t  endian;		/* R/W Source and target endian      */
  unsigned long  displaymask;		/* R/W Display/data mask             */
  unsigned long  converttype;		/* R/W Conversion type               */
  unsigned short subchar_len;		/* R/W MBCS sub len      0=table     */
  unsigned short subuni_len;		/* R/W Unicode sub len   0=table     */
  char           subchar [16];		/* R/W MBCS sub characters           */
  UniChar        subuni [8];		/* R/W Unicode sub characters        */
} uconv_attribute_t;

/* User defined character range */
typedef struct _udcrange_t
{
  unsigned short first;
  unsigned short last;
} udcrange_t;

typedef int uconv_error_t;
typedef void *UconvObject;

int APIENTRY UniCreateUconvObject (UniChar *codepage, UconvObject *uobj);
int APIENTRY UniQueryUconvObject (UconvObject uobj, uconv_attribute_t * attr,
  size_t size, char first [256], char other [256], udcrange_t udcrange[32]);
int APIENTRY UniSetUconvObject (UconvObject uobj, uconv_attribute_t * attr);
int APIENTRY UniUconvToUcs (UconvObject uobj, void **inbuf, size_t *inbytes,
  UniChar **outbuf, size_t *outchars, size_t *subst);
int APIENTRY UniUconvFromUcs (UconvObject uobj, UniChar **inbuf, size_t *inchars,
  void **outbuf, size_t *outbytes, size_t *subst);
int APIENTRY UniFreeUconvObject (UconvObject uobj);
int APIENTRY UniMapCpToUcsCp (unsigned long ulCodePage, UniChar *ucsCodePage, size_t n);
int APIENTRY UniStrFromUcs(UconvObject, char *, UniChar *, int len);
int APIENTRY UniStrToUcs(UconvObject, UniChar *, char *, int);

#define IBM_437         (UniChar *)L"IBM-437"
#define IBM_819         (UniChar *)L"IBM-819"
#define IBM_850         (UniChar *)L"IBM-850"
#define UTF_8           (UniChar *)L"IBM-1208"
#define UCS_2           (UniChar *)L"IBM-1200"
#define ISO8859_1       (UniChar *)L"IBM-819"

/* uconv error aliases. */
#define UCONV_BADATTR           ULS_BADATTR
#define UCONV_E2BIG             ULS_BUFFERFULL
#define UCONV_EBADF             ULS_BADOBJECT
#define UCONV_EILSEQ            ULS_ILLEGALSEQUENCE
#define UCONV_EINVAL            ULS_INVALID
#define UCONV_EMFILE            ULS_MAXFILESPERPROC
#define UCONV_ENFILE            ULS_MAXFILES
#define UCONV_ENOMEM            ULS_NOMEMORY
#define UCONV_EOTHER            ULS_OTHER
#define UCONV_NOTIMPLEMENTED    ULS_NOTIMPLEMENTED

__END_DECLS

#endif /* __UCONV_H__ */
