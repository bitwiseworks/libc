/* fnmatch.h (emx+gcc) */

#ifndef _FNMATCH_H
#define _FNMATCH_H

#if defined (__cplusplus)
extern "C" {
#endif

/* POSIX.2 */

#define FNM_NOMATCH        1

#define FNM_NOESCAPE       16
#define FNM_PATHNAME       32
#define FNM_PERIOD         64

int fnmatch (const char *, const char *, int);


#if !defined (_POSIX_SOURCE) || defined (_WITH_UNDERSCORE) || defined(__USE_EMX)

/* emx extensions */

#define _FNM_MATCH         0
#define _FNM_ERR           2

#define _FNM_STYLE_MASK    15

#define _FNM_POSIX         0
#define _FNM_OS2           1
#define _FNM_DOS           2

#define _FNM_IGNORECASE    128
#define _FNM_PATHPREFIX    256

int _fnmatch (const char *, const char *, int);

#endif


#if !defined (_POSIX_SOURCE) || defined (_GnU_SOURCE) || defined(__USE_EMX)
/* GNU liberty compatibility */
# define FNM_FILE_NAME     FNM_PATHNAME
# define FNM_CASEFOLD      128 /* _FNM_IGNORECASE */
/*# define FNM_LEADING_DIR   256 -  _FNM_PATHPREFIX?? */
#endif

#if defined (__cplusplus)
}
#endif

#endif /* not _FNMATCH_H */

