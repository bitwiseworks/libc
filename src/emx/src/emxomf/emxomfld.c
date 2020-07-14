/* emxomfld.c --  Provide an ld-like interface to the IBM and M$ linkers
   Copyright (c) 1992-1998 Eberhard Mattes
   Copyright (c) 2003 InnoTek Systemberatung GmbH
   Copyright (c) 2003-2004 Knut St. Osmundsen

This file is part of emxomld.

emxomfld is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxomfld is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxomfld; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <errno.h>
#include <string.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <sys/moddef.h>
#include <getopt.h>
#include <alloca.h>
#include <sys/omflib.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#include "defs.h"
#include "weakld.h"

#define FALSE 0
#define TRUE  1

/* A member of a linked list of strings such as file names. */
typedef struct name_list
{
  struct name_list *next;
  unsigned flags;
  char *name;
} name_list;


/* Whether or not linker tracing is enabled. */
static int opt_t;

/* Whether or not to include .dll in the shared library searching. */
static int opt_dll_search;

/* The output file name, specified by the -o option.  */
static const char *output_fname = NULL;

/* The map file name (output), set by the -Zmap option. */
static const char *map_fname = NULL;
static int map_flag = FALSE;

/* The sym file output flag, set by the -Zsym option. */
static int sym_flag = FALSE;

/* The module definition file name (input), set if a file matching
   *.def is given on the command line. */
static const char *def_fname = NULL;

/* The binary resource file name (input), set if a file matching *.res
   is given on the command line. */
static const char *res_fname = NULL;

/* Base address of the excecutable file, specified by the -T
   option. */
static const char *base = NULL;

/* List of directories searched for libraries.  Each -L option adds a
   directory to this list.  add_libdirs is used to add another entry
   at the end of the list. */
static name_list *libdirs = NULL;
static name_list **add_libdirs = &libdirs;

/* List of object files.  Each file given on the command line which
   does not match *.def, *.lib and *.res is added to this list.
   add_obj_fnames is used to add another entry at the end of the
   list. */
static name_list *obj_fnames = NULL;
static name_list **add_obj_fnames = &obj_fnames;

/* List of library files.  Each file matching *.lib given on the
   command line is added to this list.  The -l option also adds an
   entry to this list. add_lib_fnames is used to add another entry at
   the end of the list.
   The flags member indicates library search method. If set search for
   static lib, if clear search for shared lib before search for static lib. */
static name_list *lib_fnames = NULL;
static name_list **add_lib_fnames = &lib_fnames;

/* List of linker options.  Linker options can be specified with the
   -O option.  add_options is used to add another entry at the end of
   the list. */
static name_list *options = NULL;
static name_list **add_options = &options;

/* The command line passed to the linker. */
static char command_line[260];

/* The current length of the command line. */
static int line_len;

/* Non-zero if arguments go into the response file instead of
   command_line. */
static int response_flag;

/* The name of the response file. */
static char response_fname[L_tmpnam] = "";

/* The response file. */
static FILE *response_file = NULL;

/* Force the use of a response file from the next put_arg(). */
static int force_response_file = FALSE;

/* Weak alias object file. */
static char weakobj_fname[_MAX_PATH + 1];

/* Weak definition file (modified def_fname). */
static char weakdef_fname[_MAX_PATH + 1];

/* list of converted libraries and objects which must be removed upon exit. */
static name_list *conv_list = NULL;

/* Non-zero if debugging information is to be omitted.  Set by the -s
   and -S options. */
static int strip_symbols = FALSE;

/* Non-zero if emxomfld should create an .exe file and touch the
   output file.  Set by the -Zexe option. */
static int exe_flag = FALSE;

/* Non-zero when creating a dynamic link library.  Set by the -Zdll
   option. */
static int dll_flag = FALSE;

/* The stack size, specified by the -Zstack option, in Kbyte.  If the
   -Zstack option is not used, this variable defaults to 1024 to match
   the defaults of emxbind. */
static long stack_size = 1024;
/* Indicates that we've seen -Zstack. */
static int stack_size_flag = 0;

/* The name of the linker to use.  By default, ilink is used.  This
   can be overridden with the EMXOMFLD_LINKER environment variable. */
static const char *linker_name = "ilink.exe";

/* The type of linker to use. By default we assume it's VAC365 or later
   version of ilink. This can be overridden with the EMXOMFLD_TYPE env.
   var. using any of the values WLINK, VAC365, VAC308 and LINK386. */
static const char *linker_type = "VAC365";

/* The name of the resource compiler to use.  By default, rc is used.
   This can be overridden with the EMXOMFLD_RC environment variable. */
static const char *rc_name = "rc.exe";

/* The type of resource compiler to use. By default we assume it's
   IBM resource compiler. This can be overridden with the EMXOMFLD_RC_TYPE
   env. var. using any of the values RC, WRC. */
static const char *rc_type = "RC";

/* Non-zero if emxomfld should automatically convert a.out objects and
   archives to the OMF equivalents during linking. */
static int autoconvert_flag = 1;


/* Prototypes. */

static void usage (void) NORETURN2;
extern void *xmalloc (size_t n);
extern void *xrealloc (void *ptr, size_t n);
extern char *xstrdup (const char *s);
static void add_name_list (name_list ***add, const char *src, unsigned flags);
static void conv_path (char *name);
static void put_arg (const char *src, int path, int quotable);
static void put_args (const name_list *list, int paths);
static void make_env (void);
static void cleanup (void);
static void arg_init (int rsp);
static void arg_end (void);
int main (int argc, char *argv[]);

/* Profiling macros that enable mllisecond counter in -t -t -t mode */
#define MSCOUNT_INIT() unsigned long msTSStart = 0
#define MSCOUNT_START() do { if (opt_t > 2) msTSStart = fibGetMsCount(); } while(0)
#define MSCOUNT_STOP(msg) do { \
  if (opt_t > 2) { \
    unsigned long msTSEnd = fibGetMsCount(); \
    fprintf(stderr, "*** MSCOUNT: %s: %lu ms\n", msg, msTSEnd - msTSStart); \
    msTSStart = fibGetMsCount(); \
  } \
} while(0)

/* To avoid including os2.h... */
#ifndef _System
#define _System
#endif
extern int _System DosCopy (char *, char *, int);

/* Allocate N bytes of memory.  Quit on failure.  This function is
   used like malloc(), but we don't have to check the return value. */

void *xmalloc (size_t n)
{
  void *p;

  p = malloc (n);
  if (p == NULL && n)
    {
      fprintf (stderr, "emxomfld: out of memory\n");
      exit (2);
    }
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
    {
      fprintf (stderr, "emxomfld: out of memory\n");
      exit (2);
    }
  return p;
}



/* Create a duplicate of the string S on the heap.  Quit on failure.
   This function is used like strdup(), but we don't have to check the
   return value. */

char *xstrdup (const char *s)
{
  char *p;
  int cch = strlen (s) + 1;

  p = xmalloc (cch);
  memcpy (p, s, cch);
  return p;
}


/* Add the name SRC to a list.  ADD is a pointer to the pointer of the
   end of the list.  We duplicate the string before adding it to the
   list. */

static void add_name_list (name_list ***add, const char *src, unsigned flags)
{
  name_list *node;

  node = xmalloc (sizeof (name_list));
  node->next  = NULL;
  node->name  = xstrdup (src);
  node->flags = flags;
  *(*add) = node;
  (*add) = &node->next;
}

/* Opens a response file. */

static void open_response_file(void)
{
    int fd;

    if (response_file)
      return;

    /* Complain if we are not allowed to use a response
       file. */

    if (!response_flag)
      {
        fprintf (stderr, "emxomfld: command line too long\n");
        exit (2);
      }

    /* Choose a unique file name and create the response
       file. */

    strcpy (response_fname, "ldXXXXXX");
    fd = mkstemp (response_fname);
    if (fd < 0)
      {
        perror ("emxomfld");
        exit (2);
      }
    close(fd);
    response_file = fopen (response_fname, "wt");
    if (response_file == NULL)
      {
        perror ("emxomfld");
        exit (2);
      }

    /* Add the name of the response file to the command
       line. */

    command_line[line_len++] = ' ';
    command_line[line_len++] = '@';
    strcpy (command_line+line_len, response_fname);
    if (!stricmp (linker_type, "WLINK"))
      strcat (command_line, ".");

    if (force_response_file)
      force_response_file = FALSE;
}

/* Replace forward slashes `/' in NAME with backslashes `\'.  The linkers
   requires backslashes in path names. */

static void conv_path (char *name)
{
  char *p;

  for (p = name; *p != 0; ++p)
    if (*p == '/')
      *p = '\\';
}


/* Add the argument SRC to the command line or to the response file.
   If PATH is non-zero, SRC is a path name and slashes are to be
   replaced by backslashes.  If the command line gets too long, a
   response file is created.
   If quotable is non-zero SRC will be quoted. This is required for
   supporting files names which includes '+' and spaces. */

static void put_arg (const char *src, int path, int quotable)
{
  int len, max_len;
  char *tmp;

  if (src != NULL)
    {

      /* Instead of a comma, we write a newline to the response
         file. */

      if (response_file != NULL && strcmp (src, ",") == 0)
        {
          fputc ('\n', response_file);
          line_len = 0;
          return;
        }

      /* Make a local copy of SRC to be able to modify it.  Then,
         translate forward slashes to backslashes if PATH is
         non-zero. */

      len = strlen (src);
      tmp = alloca (len + (quotable ? 3 : 1));
      if (path)
        {
          /* needs quoting? */
          if (quotable)
            {
              *tmp = '"';
              strcpy (tmp+1, src);
              tmp[++len] = '"';
              tmp[++len] = '\0';
            }
          else
            strcpy (tmp, src);
          conv_path (tmp);
        }
      else
        strcpy (tmp, src);


      /* Check if we've reached the maximum line length.  If the
         maximum command line length is exceeded, create a response
         file and write the remaining arguments to that file instead
         of putting them on the command line. */

      max_len = (response_file == NULL ? 110 : 52);
      if (   line_len + len + 1 > max_len
          || (force_response_file && !response_file))
        {

          /* If SRC is a single comma or a single semicolon, copy it
             to the output, ignoring the maximum line length.  This is
             to meet the IBM/M$ linker command syntax. The maximum line
             length allows for enough commas and semicolons added this
             way. */

          if ((*tmp == ',' || *tmp == ';') && tmp[1] == 0)
            {
              if (response_file == NULL)
                {
                  command_line[line_len+0] = *tmp;
                  command_line[line_len+1] = 0;
                }
              else
                fputc (*tmp, response_file);
              ++line_len;
              return;
            }

          /* If a response file has not yet been opened, open it. */

          if (response_file == NULL)
            open_response_file();
          else if (line_len != 0)
            {

              /* Start a new line in the response file. */

              fputs (" +\n", response_file);
            }
          line_len = 0;
        }

      /* Separate command line arguments by spaces (unless the
         argument to be added starts with a delimiter. */

      if (line_len != 0 && *src != ',' && *src != ';')
        {
          if (response_file == NULL)
            command_line[line_len++] = ' ';
          else
            fputc (' ', response_file);
        }

      /* Finally write the argument to the command line or to the
         response file and adjust the current line length. */

      if (response_file == NULL)
        strcpy (command_line + line_len, tmp);
      else
        fputs (tmp, response_file);
      line_len += len;
    }
}


/* Put a list of arguments onto the command line or into the response
   file.  If PATHS is non-zero, the arguments are path names and
   slashes are to be replaced by backslashes. */

static void put_args (const name_list *list, int paths)
{
  while (list != NULL)
    {
      put_arg (list->name, paths, paths);
      list = list->next;
    }
}


/* Build the environment for the IBM/M$ Linkers: define the LIB
   environment variable. */

static void make_env (void)
{
  static char tmp[4096];
  char *p;
  int len;
  const name_list *list;

  /* Create a string for putenv(). */

  strcpy (tmp, "LIB=");
  len = strlen (tmp);

  /* Add the library directories to LIB, using `;' as separator. */

  for (list = libdirs; list != NULL; list = list->next)
    {
      if (list != libdirs && tmp[len-1] != ';')
        tmp[len++] = ';';
      strcpy (tmp+len, list->name);
      conv_path (tmp+len);
      len += strlen (list->name);
    }

  /* Append to the end the previous definition of LIB. */

  p = getenv ("LIB");
  if (p != NULL)
    {
      if (tmp[len-1] != ';')
        tmp[len++] = ';';
      strcpy (tmp+len, p);
    }


  /* Put the new value of LIB into the environment. */

  putenv (tmp);

  if (opt_t)
    fprintf(stderr, "*** %s\n", tmp);
}

/**
 * Checks if the stream phFile is an OMF library.
 *
 * @returns 1 if OMF library.
 * @returns 0 if not OMF library.
 * @param   phFile  Filestream to check.
 */
static int check_omf_library(FILE *phFile)
{
#pragma pack(1)
    struct
    {
        byte rec_type;
        word rec_len;
        dword dict_offset;
        word dict_blocks;
        byte flags;
    } libhdr;
#pragma pack()

    if (    fread(&libhdr, 1, sizeof(libhdr), phFile) == sizeof (libhdr)
        &&  !fseek(phFile, 0, SEEK_SET)
        &&  libhdr.rec_type == LIBHDR
        &&  libhdr.flags <= 1 /* ASSUME only first bit is used... */
       )
    {
        int page_size = libhdr.rec_len + 3;
        if (page_size >= 16
            && page_size <= 32768
            && !(page_size & (page_size - 1)) != 0)
            return 1;
    }
    return 0;
}


/**
 * Checks if the stream phFile is an OMF object or library.
 *
 * @returns 1 if OMF.
 * @returns 0 if not OMF.
 * @param   phFile  Filestream to check.
 */
static int check_omf(FILE *phFile)
{
#pragma pack(1)
    struct
    {
        byte rec_type;
        word rec_len;
    } omfhdr;
#pragma pack()
    if (    fread(&omfhdr, 1, sizeof(omfhdr), phFile) == sizeof (omfhdr)
        &&  omfhdr.rec_type == THEADR
        &&  omfhdr.rec_len >= sizeof(omfhdr)
        &&  !fseek(phFile, 0, SEEK_SET)
       )
        return 1;

    return !fseek(phFile, 0, SEEK_SET)
        && check_omf_library(phFile);
}


/**
 * Checks if the stream phFile is an LX DLL.
 *
 * @returns 1 if LX DLL.
 * @returns 0 if not LX DLL.
 * @param   phFile  File stream to check.
 */
static int check_lx_dll(FILE *phFile)
{
    unsigned long   ul;
    char            achMagic[2];

    if (    fseek(phFile, 0, SEEK_SET)
        ||  fread(&achMagic, 1, 2, phFile) != 2)
        goto thats_not_it;

    if (!memcmp(achMagic, "MZ", 2))
    {
        if (    fseek(phFile, 0x3c, SEEK_SET)
            ||  fread(&ul, 1, 4, phFile) != 4 /* offset of the 'new' header */
            ||  ul < 0x40
            ||  ul >= 0x10000000 /* 512MB stubs sure */
            ||  fseek(phFile, ul, SEEK_SET)
            ||  fread(&achMagic, 1, 2, phFile) != 2)
            goto thats_not_it;
    }

    if (    memcmp(achMagic, "LX", 2)
        ||  fseek(phFile, 14, SEEK_CUR)
        ||  fread(&ul, 1, 4, phFile) != 4) /*e32_mflags*/
        goto thats_not_it;

#define E32MODDLL        0x08000L
#define E32MODPROTDLL    0x18000L
#define E32MODMASK       0x38000L
    if (   (ul & E32MODMASK) != E32MODDLL
        && (ul & E32MODMASK) != E32MODPROTDLL)
        goto thats_not_it;

    /* it's a LX DLL! */
    fseek(phFile, 0, SEEK_SET);
    return 1;


thats_not_it:
    fseek(phFile, 0, SEEK_SET);
    return 0;
}


/**
 * Generates an unique temporary file.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pszFile     Where to put the filename.
 * @param   pszPrefix   Prefix.
 * @param   pszSuffix   Suffix.
 * @param   pszLooklike Filename which name is to be incorporated into the temp filename.
 * @remark  The code is nicked from the weak linker.
 */
static int      make_tempfile(char *pszFile, const char *pszPrefix, const char *pszSuffix, const char *pszLooklike)
{
    struct stat     s;
    unsigned        c = 0;
    char            szLooklike[32];
    pid_t           pid = getpid();
    static char     s_szTmp[_MAX_PATH + 1];

    /* We need to apply _realrealpath to the tmpdir, so resolve that once and for all. */
    if (!s_szTmp[0]) 
    {
        const char *    pszTmp = getenv("TMP");
        if (!pszTmp)    pszTmp = getenv("TMPDIR");
        if (!pszTmp)    pszTmp = getenv("TEMP");
        if (!pszTmp)    pszTmp = ".";
        if (!_realrealpath(pszTmp, s_szTmp, sizeof(s_szTmp)))
        {
            printf("emxomfld: _realrealpath failed on '%s'!!!\n", pszTmp);
            exit(1);
        }
    }

    if (pszLooklike)
    {
        int     cch;
        char   *psz = (char*)pszLooklike; /* we're nice fellows. */
        while ((psz = strpbrk(psz, ":/\\")) != NULL)
            pszLooklike = ++psz;
        cch = strlen(pszLooklike);
        if (cch + 3 > sizeof(szLooklike))
            cch = sizeof(szLooklike) - 3;
        szLooklike[0] = '_';
        memcpy(&szLooklike[1], pszLooklike, cch);
        szLooklike[cch + 1] = '_';
        szLooklike[cch + 2] = '\0';
        pszLooklike = psz = &szLooklike[0];
        while ((psz = strpbrk(psz, ".@%^&#()")) != NULL)
            *psz++ = '_';
    }
    else
        pszLooklike = "";

    do
    {
        struct timeval  tv = {0,0};
        if (c++ >= 200)
            return -1;
        gettimeofday(&tv, NULL);
        sprintf(pszFile, "%s\\%s%s%x%x%d%lx%s", s_szTmp, pszPrefix, pszLooklike, pid, tv.tv_sec, c, tv.tv_usec, pszSuffix);
    } while (!stat(pszFile, &s));

    return 0;
}


/**
 * Converts the file indicated by phFile & pszFilename to omf closing
 * phFile and updating pszFilename with the new (temporary filename).
 *
 * @returns Pointer to an filestream for the converted file and pszFilename
 *          containing the name of the converted file.
 * @returns exit the program
 * @param   phFile      Filestream of the file to convert. (close this)
 * @param   pszFilename Name of the file to convert on entry.
 *                      Name of the converted file on return.
 */
static FILE *aout_to_omf(FILE *pf, char *pszFilename, int fLibrary)
{
    int         rc;
    char *      pszNewFile;
    name_list  *pName;

    fclose(pf);                         /* don't need this! */

    if (opt_t)
        fprintf(stderr, "emxomfld: info: converting %s %s to OMF.\n",
                fLibrary ? "lib" : "obj", pszFilename);

    /*
     * Make temporary file.
     */
    pName = xmalloc(sizeof(name_list));
    pName->name = pszNewFile = xmalloc(_MAX_PATH);
    if (make_tempfile(pszNewFile, "ldconv", fLibrary ? ".lib" : ".obj", pszFilename))
    {
        free(pszNewFile);
        return NULL;
    }

    /*
     * Do the conversion.
     */
    rc = spawnlp(P_WAIT, "emxomf.exe", "emxomf.exe", "-o", pszNewFile, pszFilename, NULL);
    if (!rc)
    {
        /* open the file */
        pf = fopen(pszNewFile, "rb");
        if (pf)
        {
            /* add to auto delete list for removal on exit(). */
            pName->next = conv_list;
            conv_list = pName;

            strcpy(pszFilename, pszNewFile);

            if (opt_t)
                fprintf(stderr, "emxomfld: info: convert result '%s'.\n",
                        pszFilename);
            return pf;
        }
        remove(pszNewFile);
    }
    free(pszNewFile);
    free(pName);

    fprintf(stderr, "emxomfld: a.out to omf conversion failed for '%s'.\n",
            pszFilename);
    exit(1);
    return NULL;
}


/**
 * Converts the file indicated by phFile & pszFilename to omf closing
 * phFile and updating pszFilename with the new (temporary filename).
 *
 * @returns Pointer to an filestream for the converted file and pszFilename
 *          containing the name of the converted file.
 * @returns exit the program
 * @param   phFile      Filestream of the file to convert. (close this)
 * @param   pszFilename Name of the file to convert on entry.
 *                      Name of the converted file on return.
 */
static FILE *lx_to_omf(FILE *pf, char *pszFilename)
{
    int         rc;
    char *      pszNewFile;
    name_list  *pName;

    fclose(pf);                         /* don't need this! */

    if (opt_t)
        fprintf(stderr, "emxomfld: info: converting %s %s to an OMF import lib.\n",
                "lib", pszFilename);

    /*
     * Make temporary file.
     */
    pName = xmalloc(sizeof(name_list));
    pName->name = pszNewFile = xmalloc(_MAX_PATH);
    if (make_tempfile(pszNewFile, "ldconv", ".lib", pszFilename))
    {
        free(pszNewFile);
        return NULL;
    }

    /*
     * Do the conversion.
     */
    rc = spawnlp(P_WAIT, "emximp.exe", "emximp.exe", "-o", pszNewFile, pszFilename, NULL);
    if (!rc)
    {
        /* open the file */
        pf = fopen(pszNewFile, "rb");
        if (pf)
        {
            /* add to auto delete list for removal on exit(). */
            pName->next = conv_list;
            conv_list = pName;

            strcpy(pszFilename, pszNewFile);

            if (opt_t)
              fprintf(stderr, "emxomfld: info: convert result '%s'.\n",
                      pszFilename);
            return pf;
        }
        remove(pszNewFile);
    }
    free(pszNewFile);
    free(pName);

    fprintf(stderr, "emxomfld: lx dll to omf conversion failed for '%s'.\n",
            pszFilename);
    exit(2);
    return NULL;
}


/**
 * Finds the full path of a OMF object file and opens the file.
 *
 * This function may perform conversion from a.out to omf if that feature
 * is enabled.
 *
 * We choose to be UNIX compatible her, and not search the LIB env.var.
 * for unqualified objects. Nor will we add any suffixes to the name
 * if it's witout any extension.
 *
 * @returns Pointer to a file stream for the object file to use in the link.
 * @returns NULL on failure with pszFullname containing a copy of
 *          pszName (or something like that).
 * @param   pszFullname Where to store the name of the file to be used
 *                      in the linking (and which stream is returned).
 * @param   pszName     Object name given to on the linker commandline.
 */
static FILE *find_obj(char *pszFullname, const char *pszName)
{
    FILE   *phFile;
    char   *psz;

    /*
     * Make abspath with slashes the desired way and such.
     */
    if (!_realrealpath(pszName, pszFullname, _MAX_PATH + 1))
    {
        printf("emxomfld: _abspath failed on '%s'!!!\n", pszName);
        exit(1);
    }

    psz = pszFullname;
    while ((psz = strchr(psz, '/')) != NULL)
      *psz++ = '\\';

    /*
     * Try open the file.
     */
    phFile = fopen(pszFullname, "rb");
    if (!phFile)
        return NULL;

    /*
     * If autoconversion check if such is needed.
     */
    if (    autoconvert_flag
        &&  !check_omf(phFile))
        phFile = aout_to_omf(phFile, pszFullname, FALSE);

    return phFile;
}



/* Finds the full path of a library file and opens the file.
 *
 * This function may perform conversion from a.out to omf if that feature
 * is enabled.
 *
 * The function assumes that LIB has been updated with all the search paths
 * specified on the commandline.
 *
 * Library names with no extension are given extensions after the rules
 * indicated by the IS_SHARED parameter. If IS_SHARED is set then libraries
 * with suffixes indicating shared libraries will be looked for before
 * libraries with suffixes indicated static libraries. The list is as
 * follows for set IS_SHARED:
 *      1. _dll.lib
 *      2. .lib
 *      3. .dll (optional)
 *      4. _s.lib
 *
 * If IS_SHARED is clear:
 *      1. _s.lib
 *      2. .lib
 *
 * Library names with no path is searched for in the semicolon separated list
 * of paths the env.var. LIB contains. For each directory in LIB we'll start
 * by see if it contains a 'lib' prefixed file, if not found we'll check for
 * the unprefixed filename. If we're appending suffixes too, we'll loop thru
 * all the possible suffixes for each directory before advancing to the next,
 * having the prefixing as the inner most loop.
 *
 * @returns Pointer to a file stream for the library file to use in the link.
 * @returns NULL on failure with pszFullname containing a copy of
 *          pszName (or something like that).
 * @param   pszFullname Where to store the name of the file to be used
 *                      in the linking (and which stream is returned).
 * @param   pszName     Library name given to on the linker commandline.
 */
static FILE *find_lib(char *pszFullname, const char *pszName, int fShared)
{
    /* Suffix list for shared linking. */
    static const char *apszSharedSuff[]     = { "_dll.lib", "_dll.a", ".lib",  ".a", "_s.lib", "_s.a", NULL };
    /* Suffix list for shared linking with .dll. */
    static const char *apszSharedDllSuff[]  = { "_dll.lib", "_dll.a", ".lib",  ".a", ".dll", "_s.lib", "_s.a", NULL };
    /* Suffix list for static linking. */
    static const char *apszStaticSuff[]     = { "_s.lib", "_s.a", ".lib",  ".a", NULL };
    /* Suffix list for names with extension. */
    static const char *apszExtensionSuff[]  = { "", NULL };
    /* Prefix list for names with path. */
    static const char *apszWithPathPref[]   = { "", NULL };
    /* Prefix list for names with no path. */
    static const char *apszWithoutPathPref[]= { "lib", "", NULL };
    int             fPath;              /* set if the library name have a path. */
    int             fExt;               /* set if the library name have an extension. */
    const char    **papszSuffs;         /* Pointer to the suffix list. */
    const char    **papszPrefs;         /* Pointer to the prefix list. */
    const char     *pszLibPath;         /* The path we're searching. */
    size_t          cchCurPath;         /* Size of the current path. */
    size_t          cchName = strlen(pszName);
    const char     *psz;

    /*
     * Check if the file name has a path.
     * (If it has, we won't check the LIB directories.)
     * Choose the prefix list accordingly.
     */
    fPath = (strpbrk(pszName, ":/\\") != NULL);
    papszPrefs = fPath ? apszWithPathPref : apszWithoutPathPref;

    /*
     * Check if the file has a real extension.
     * Real extension means, .lib, .dll or .a. It also implies something
     * before the dot.
     * Choose the suffix list accordingly.
     */
    fExt = (    (cchName > 4 && !stricmp(pszName + cchName - 4, ".lib"))
            ||  (cchName > 4 && !stricmp(pszName + cchName - 4, ".dll"))
            ||  (cchName > 2 && !stricmp(pszName + cchName - 2, ".a")) );

    if (!fExt)
    {
        if (fShared)
            papszSuffs = opt_dll_search ? &apszSharedDllSuff[0] : &apszSharedSuff[0];
        else
            papszSuffs = &apszStaticSuff[0];
    }
    else
        papszSuffs = apszExtensionSuff;

    /*
     * Loop 1: LIB (with a fake .\ as the first iteration)
     * (Looping on pszLibPath, with preinitiated cchCurPath & pszFullname.)
     */
    cchCurPath = 0;
    if (!fPath)
    {
        cchCurPath = 2;
        memcpy(pszFullname, ".\\", 2);
    }
    pszLibPath = getenv("LIB");
    do
    {
        /*
         * Loop2: Suffixes.
         */
        int iSuff;
        for (iSuff = 0; papszSuffs[iSuff]; iSuff++)
        {
            /*
             * Loop3: Prefixes.
             */
            int iPref;
            for (iPref = 0; papszPrefs[iPref]; iPref++)
            {
                FILE   *phFile;
                int     cch = strlen(papszPrefs[iPref]);

                /*
                 * Construct name.
                 */
                memcpy(&pszFullname[cchCurPath], papszPrefs[iPref], cch);
                cch = cchCurPath + cch;
                memcpy(&pszFullname[cch], pszName, cchName);
                cch += cchName;
                strcpy(&pszFullname[cch], papszSuffs[iSuff]);

                /*
                 * Open and if necessary convert it.
                 */
                phFile = fopen(pszFullname, "rb");
                if (phFile)
                {
                    char *pszTmp;
                    if (autoconvert_flag)
                    {
                        if (check_lx_dll(phFile))
                            phFile = lx_to_omf(phFile, pszFullname);
                        else if (!check_omf(phFile))
                            phFile = aout_to_omf(phFile, pszFullname, TRUE);
                    }

                    /* Get the real native path. */
                    pszTmp = _realrealpath(pszFullname, NULL, 0);
                    if (pszTmp)
                    {
                        strcpy(pszFullname, pszTmp);
                        free(pszTmp);
                    }

                    /* Replace forward slashes with backslashes (link386). */
                    while ((pszFullname = strchr(pszFullname, '/')) != NULL)
                      *pszFullname++ = '\\';
                    return phFile;
                }
            } /* next prefix */
        } /* next suffix */

        /*
         * If a path was specified or no LIB we're done now.
         */
        if (fPath || !pszLibPath)
            break;

        /*
         * Next LIB part.
         */
        for (;;)
        {
            psz = strchr(pszLibPath, ';');
            if (!psz)
                psz = strchr(pszLibPath, '\0');
            cchCurPath = psz - pszLibPath;
            if (cchCurPath)
            {
                memcpy(pszFullname, pszLibPath, cchCurPath);
                pszLibPath = psz + (*psz == ';');
                /* Append last slash if it is not there */
                if (   pszFullname[cchCurPath - 1] != '/'
                    && pszFullname[cchCurPath - 1] != '\\')
                    pszFullname[cchCurPath++] = '\\';
                break;
              }
            if (!*psz)
              break;
            pszLibPath = psz + 1;
        }
    } while (cchCurPath);

    /* failure */
    return NULL;
}


/* Weak prelinking for Method 2 Weak support. */

static void weak_prelink ()
{
  int           rc = 0;
  name_list *   pOpt;
  PWLD          pwld;
  unsigned      fFlags = 0;

  /* look for ilinker options. */
  if (opt_t)
      fFlags |= WLDC_VERBOSE;
  if (!stricmp(linker_type, "LINK386"))
      fFlags |= WLDC_LINKER_LINK386;
  else if (!stricmp(linker_type, "WLINK"))
      fFlags |= WLDC_LINKER_WLINK;

  for (pOpt = options; pOpt; pOpt = pOpt->next)
    if (    !strnicmp(pOpt->name, "/NOE", 4)
        ||  !strnicmp(pOpt->name, "-NOE", 4))
        fFlags |= WLDC_NO_EXTENDED_DICTIONARY_SEARCH;
    else
    if (    !strnicmp(pOpt->name, "/INC", 4)
        ||  !strnicmp(pOpt->name, "-INC", 4))
        fFlags = fFlags;                /* Ignore for now. */
    else
    if (    !strnicmp(pOpt->name, "/IG", 3)
        ||  !strnicmp(pOpt->name, "-IG", 3))
        fFlags |= WLDC_CASE_INSENSITIVE;
    else
    if (    !strnicmp(pOpt->name, "/I", 2)
        ||  !strnicmp(pOpt->name, "-I", 2))
        fFlags = fFlags;                /* Ignore - require opt_t. */
    else
    if (    !strnicmp(pOpt->name, "/NOIN", 5)
        ||  !strnicmp(pOpt->name, "-NOIN", 5))
        fFlags &= ~WLDC_VERBOSE;
    else
    if (    !strnicmp(pOpt->name, "/NOI", 4)
        ||  !strnicmp(pOpt->name, "/NOI", 4))
        fFlags &= ~WLDC_CASE_INSENSITIVE;

  MSCOUNT_INIT();

  /* create the linker and to the linking. */
  if (opt_t)
    fprintf(stderr, "*** Invoking weak prelinker with flags %x.\n", fFlags);
  pwld = WLDCreate (fFlags);
  if (pwld)
    {
      name_list *   pcur;
      FILE         *phfile;
      char          szname[_MAX_PATH + 1];

      /* definition file if any */
      if (def_fname && def_fname[0])
        {
          phfile = fopen (def_fname, "r");
          rc = WLDAddDefFile (pwld, phfile, def_fname);
        }

      /* objects */
      for (pcur = obj_fnames; !rc && pcur; pcur = pcur->next)
        {
          MSCOUNT_START();
          phfile = find_obj (szname, pcur->name);
          MSCOUNT_STOP("find_obj");
          rc = WLDAddObject (pwld, phfile, szname);
          MSCOUNT_STOP("WLDAddObject");
        }

      /* libraries */
      for (pcur = lib_fnames; !rc && pcur; pcur = pcur->next)
        {
          MSCOUNT_START();
          phfile = find_lib (szname, pcur->name, !pcur->flags);
          MSCOUNT_STOP("find_lib");
          rc = WLDAddLibrary (pwld, phfile, szname);
          MSCOUNT_STOP("WLDAddLibrary");
          free(pcur->name);
          pcur->name = xstrdup(szname);
        }

      /* complete pass 1 */
      if (!rc)
        {
          MSCOUNT_START();
          rc = WLDPass1 (pwld);
          MSCOUNT_STOP("WLDPass1");
          /* ignore unresolved externals for now. */
          if (rc == 42)
            {
              rc = 0;
              fprintf(stderr, "Ignoring unresolved externals reported from weak prelinker.\n");
            }
        }

      /* generate weak aliases. */
      if (!rc)
        {
          MSCOUNT_START();
          rc = WLDGenerateWeakAliases (pwld, weakobj_fname, weakdef_fname);
          MSCOUNT_STOP("WLDGenerateWeakAliases");
        }
      if (!rc && weakobj_fname[0])
        {
          char *pszTmp = _realrealpath(weakobj_fname, NULL, 0);
          if (pszTmp)
          {
              strcpy(weakobj_fname, pszTmp);
              free(pszTmp);
          }
          add_name_list (&add_obj_fnames, weakobj_fname, 0);
        }
      if (!rc && weakdef_fname[0])
        {
          char *pszTmp = _realrealpath(weakdef_fname, NULL, 0);
          if (pszTmp)
          {
              strcpy(weakdef_fname, pszTmp);
              free(pszTmp);
          }
          def_fname = weakdef_fname;
        }

      /* cleanup the linker */
      WLDDestroy (pwld);

      /* last words */
      if (rc)
        {
          fprintf (stderr, "emxomfld: weak prelinker failed. (rc=%d)\n", rc);
          rc = 8;
        }
    }
  else
    {
      fprintf (stderr, "emxomfld: failed to create weak prelinker.\n");
      rc = 8;
    }

  /* die on error. */
  if (rc)
    exit(rc);

  /* verbose */
  if (opt_t)
    fprintf(stderr, "*** Weak prelinker done\n");
}


/* Start a new set of command line arguments.  If RSP is non-zero, we
   are allowed to use a response file. */

static void arg_init (int rsp)
{
  if (response_fname[0] != '\0')
    {
      remove (response_fname);
      response_fname[0] = '\0';
    }
  command_line[0] = '\0';
  line_len = 0;
  response_flag = rsp;
  force_response_file = FALSE;
}


/* Call this after adding all the command line arguments.  If a
   response file has been created, add a newline and close it. */

static void arg_end (void)
{
  if (response_file != NULL)
    {
      fputc ('\n', response_file);
      if (fflush (response_file) != 0 || fclose (response_file) != 0)
        {
          perror ("emxomfld");
          exit (2);
        }
      response_file = NULL;
    }
}

/* Generates a definition file for a dll which doesn't have one. */
static void gen_deffile(void)
{
    char *          psz;
    name_list      *pName;

    /*
     * Make temporary file.
     */
    pName = (name_list *)xmalloc(sizeof(*pName));
    pName->name = psz = xmalloc(_MAX_PATH);
    if (!make_tempfile(psz, "lddef", ".def", NULL))
    {
        FILE *pFile = fopen(psz, "w");
        if (pFile)
        {
            const char *pszName = _getname(output_fname);
            size_t cchName = strlen(pszName);
            if (cchName > 4 && !stricmp(pszName + cchName - 4, ".dll"))
                cchName -= 4;
            fprintf(pFile,
                    ";; Autogenerated by emxomfld\n"
                    "LIBRARY %.*s INITINSTANCE TERMINSTANCE\n"
                    "DATA MULTIPLE\n"
                    "CODE SHARED\n"
                    "\n",
                    cchName, pszName);
            fclose(pFile);
            def_fname = psz;
            if (opt_t)
                fprintf(stderr,
                        "--- Generated def-file %s:\n"
                        ";; Autogenerated by emxomfld\n"
                        "LIBRARY %.*s INITINSTANCE TERMINSTANCE\n"
                        "DATA MULTIPLE NONSHARED\n"
                        "CODE SINGLE SHARED\n"
                        "---- End of generated def-file.\n",
                        psz, cchName, pszName);

            /* add to auto delete list for removal on exit(). */
            pName->next = conv_list;
            conv_list = pName;
            return;
        }
    }
    free(psz);
    free(pName);
}

/* converts a def file statement to watcom responsfile lingo. */

static int def_2_watcom(struct _md *md, const _md_stmt *stmt, _md_token token, void *arg)
{
  switch (token)
    {
      case _MD_BASE:
        fprintf (response_file, "OPTION OFFSET=%#lx\n", stmt->base.addr);
        break;

      case _MD_CODE:
        break;

      case _MD_DATA:
        break;

      case _MD_DESCRIPTION:
        fprintf (response_file, "OPTION DESCRIPTION '%s'\n", stmt->descr.string);
        break;

      case _MD_EXETYPE:
        break;

      case _MD_EXPORTS:
        fprintf (response_file, "EXPORT '%s'", stmt->export.entryname);
        if (stmt->export.flags & _MDEP_ORDINAL)
          fprintf (response_file, ".%d", stmt->export.ordinal);
        if (stmt->export.internalname[0])
          fprintf (response_file, "='%s'", stmt->export.internalname);
        if (stmt->export.flags & _MDEP_RESIDENTNAME)
          fprintf (response_file, " RESIDENT");
        /** @todo _MDEP_NONAME */
        fprintf (response_file, "\n");

        /* reference the internal name. */
        if (stmt->export.internalname[0])
          fprintf (response_file, "REFERENCE '%s'\n", stmt->export.internalname);
        break;

      case _MD_HEAPSIZE:
        fprintf (response_file, "OPTION HEAPSIZE=%#lx\n", stmt->heapsize.size);
        break;

      case _MD_IMPORTS:
        fprintf (response_file, "IMPORT '%s' '%s'", stmt->import.internalname,
                 stmt->import.modulename);
        if (stmt->import.flags & _MDEP_ORDINAL)
          fprintf (response_file, ".%d", stmt->import.ordinal);
        else if (stmt->import.internalname[0])
          fprintf (response_file, ".'%s'", stmt->import.entryname);
        fprintf (response_file, "\n");
        break;

      case _MD_LIBRARY:
        if (stmt->library.name[0])
          fprintf (response_file, "OPTION MODNAME='%s'\n", stmt->library.name);
        break;

      case _MD_NAME:
        if (stmt->name.name[0])
          fprintf (response_file, "OPTION MODNAME='%s'\n", stmt->name.name);
        break;

      case _MD_OLD:
        fprintf (response_file, "OPTION OLDLIBRARY='%s'\n", stmt->old.name);
        break;

      case _MD_PROTMODE:
        fprintf (response_file, "OPTION PROTMODE\n");
        break;

      case _MD_REALMODE:
        fprintf (response_file, "OPTION PROTMODE\n");
        break;

      case _MD_SEGMENTS:
        fprintf (stderr, "emxomfld: ignoring SEGMENTS directive in .def-file\n");
        break;

      case _MD_STACKSIZE:
        fprintf (response_file, "OPTION STACK=%#lx\n", stmt->stacksize.size);
        break;

      case _MD_STUB:
        if (!stmt->stub.none)
          fprintf (response_file, "OPTION STUB='%s'\n", stmt->stub.name);
        else
          fprintf (stderr, "emxomfld: warning: \"STUB NONE\" is not supported by wlink. ignoring\n");
        break;

      case _MD_VIRTUAL:
      case _MD_PHYSICAL:
        break;

      case _MD_parseerror:
        fprintf (stderr, "emxomfld: %s (line %ld of %s)",
                 _md_errmsg (stmt->error.code), _md_get_linenumber (md), def_fname);
        exit (2);
        break;

      default:
        abort ();
    }
  return 0;
}

/* -t output. We dump the commandline and responsefile. */
static void  show_spawn(const char *pszwhat)
{
  if (!opt_t)
    return;
  fprintf(stderr, "*** Invoking %s\n %s\n", pszwhat, command_line);
  if (response_fname[0])
    { /* display the responsfile content. */
      char sz[4096];
      FILE *phfile = fopen(response_fname, "r");
      fprintf(stderr, "--- Response file %s:\n", response_fname);
      sz[0] = '\0';
      while (fgets(sz, sizeof(sz), phfile))
          fprintf(stderr, "%s", sz);
      fclose(phfile);
      if (sz[strlen(sz) - 1] != '\n')
          fprintf(stderr, "\n");
      fprintf(stderr, "--- End of Response File\n");
    }
}


/* Execute commandline and returns the result.
   pszwhat is used for opt_t trace information. */

static int emxomfld_spawn(char *pszcmd, const char *pszwhat)
{
  int       argi;
  char **   argv;
  char *    psz;
  int       rc;

  if (opt_t)
    show_spawn(pszwhat);

  /* construct spawnvp() argument array */
  argi = 0;
  argv = NULL;
  psz = pszcmd;
  while (psz && *psz)
    {
      char *psz2 = psz;

      /* skip blanks. */
      while (*psz2 == '\t' || *psz2 == ' ')
          psz2++;

      /* find end of argument taking in account in arg quoting. */
      while (*psz2 && *psz2 != '\t' && *psz2 != ' ')
        {
          if (*psz2 == '"' || *psz2 == '\'')
            {
              char chQuote = *psz2++;
              while (*psz2 && *psz2 != chQuote)
                psz2++;
            }
          psz2++;
        }

      /* terminate and set psz2 to point to next */
      if (*psz2)
        *psz2++ = '\0';

      /* add argument to argument vector. */
      if (!(argi % 32))
        argv = xrealloc(argv, sizeof(argv[0]) * (argi + 32 + 1));
      argv[argi++] = psz;

      /* next */
      psz = psz2;
    }
  argv[argi] = NULL;

  /* Spawn process. */
  rc = spawnvp(P_WAIT, argv[0], argv);
  if (opt_t)
    fprintf(stderr, "*** Return from %s is %d\n", pszwhat, rc);

  free(argv);
  return rc;
}


/* Cleanup by closing (if open) and deleting (if pressent) the
   response file.  This function is used with atexit(). */

static void cleanup (void)
{
  if (response_file != NULL)
    {
      fclose (response_file);
      response_file = NULL;
    }
  if (opt_t <= 1)
    {
      if (response_fname[0] != '\0')
        {
          remove (response_fname);
          response_fname[0] = '\0';
        }
      if (weakobj_fname[0] != '\0')
        {
          remove (weakobj_fname);
          weakobj_fname[0] = '\0';
        }
      if (weakdef_fname[0] != '\0')
        {
          remove (weakdef_fname);
          weakdef_fname[0] = '\0';
        }
      for (; conv_list; conv_list = conv_list->next)
        remove (conv_list->name);
    }
}

/* Tell the user how to run this program. */

static void usage (void)
{
  fputs ("emxomfld " VERSION VERSION_DETAILS "\n"
         "Copyright (c) 1992-1996 by Eberhard Mattes\n"
         "Copyright (c) 2003 by InnoTek Systemberatung GmbH\n"
         "Copyright (c) 2003-2006 by Knut St. Osmundsen\n"
         VERSION_COPYRIGHT "\n"
         "\n", stderr);
  fputs ("Usage: emxomfld -o <file> [-l <lib>] [-L <libdir>] [-T <base>] [-igtsS]\n"
         "           [-Zexe] [-Zdll] [-Zsym] [-Zstack <size>] [-Zmap[=<map_file>]]\n"
         "           [-Z[no-]autoconv] [-Zdll-search] [-O <option>] [-static]\n"
         "           [-non_shared] [-Bstatic] [-dn] [call_shared] [-Bshared]\n"
         "           [-dy] <file>...\n"
         "\n", stderr);
  fputs ("Options:\n"
         " -Zno-autoconv / -Zautoconv:\n"
         "    Turns off/on the automatic conversion of a.out libs and objs.\n"
         "    default: -Zautoconv\n"
         " -Bstatic, -non_shared, -dn, -static:\n"
         "    Link with static libraries.\n"
         "    The search order is then changed to: lib<name>_s.lib, <name>_s.lib,\n"
         "    lib<name>.lib, <name>.lib\n", stderr);
  fputs (" -Bshared, -call_shared, -dy:\n"
         "    Link with shared libraries. This is default.\n"
         "    The search order is then changed to: lib<name>_dll.lib, <name>_dll.lib,\n"
         "    lib<name>.lib, <name>.lib, <name>.dll, lib<name>_s.lib, <name>_s.lib.\n"
         " -Zdll-search:\n"
         "    Enables dlls as valid libraries from shared linking. (default disabled)\n",
         stderr);
  fputs (" -Zsym:"
         "    Invoke mapsym.cmd on the mapfile to produce a .sym file. Requires -Zmap.\n"
         "\n", stderr);
  fputs ("Environment variables:\n"
         "  EMXOMFLD_TYPE:\n"
         "    The type of linker we're using. Values: WLINK, VAC365, VAC308, LINK386.\n"
         "        WLINK    wlink.exe from Open Watcom v1.5 or later.\n"
         "        VAC365   ilink.exe from IBM C and C++ Compilers for OS/2 v3.6 or later.\n"
         "        VAC308   ilink.exe from Visual Age for C++ v3.08.\n"
         "        LINK386  link386 form OS/2 install or DDK.\n", stderr);
  fputs ("  EMXOMFLD_LINKER:\n"
         "    Name of the linker to use and optionally extra parameters. Spaces in the\n"
         "    linker name or path is not supported. Quotes are not supported either.\n"
         "The default values for these two variables are VAC365 and ilink.exe.\n", stderr);
  fputs ("  EMXOMFLD_RC_TYPE:\n"
         "    The type of resource compiler we're using. Values: RC,WRC.\n"
         "        RC       rc.exe as shipped with OS/2 or found in the Toolkit\n"
         "        WRC      wrc.exe from Open Watcom v1.6 or later.\n", stderr);
  fputs ("  EMXOMFLD_RC:\n"
         "    Name of the resource compiler to use and optionally extra parameters.\n"
         "    Spaces or quotes in the name or path are not supported.\n"
         "The default values for these two variables are RC and rc.exe.\n", stderr);
  exit (1);
}



static struct option longopts[] =
{
#define OPT_LIBS_STATIC     0x1000
  {"Bstatic", 0, 0, OPT_LIBS_STATIC},
  {"non_shared", 0, 0, OPT_LIBS_STATIC},
  {"dn", 0, 0, OPT_LIBS_STATIC},
  {"static", 0, 0, OPT_LIBS_STATIC},
#define OPT_LIBS_SHARED     0x1001
  {"Bshared", 0, 0, OPT_LIBS_SHARED},
  {"call_shared", 0, 0, OPT_LIBS_SHARED},
  {"dy", 0, 0, OPT_LIBS_SHARED},
#define OPT_ZEXE            0x1002
  {"Zexe", 0, 0, OPT_ZEXE},     /* Create .exe file, touch `output file' */
#define OPT_ZDLL            0x1003
  {"Zdll", 0, 0, OPT_ZDLL},     /* Create .dll file, touch `output file' */
#define OPT_ZSTACK          0x1004
  {"Zstack", 1, 0, OPT_ZSTACK}, /* Set stack size */
#define OPT_ZMAP            0x1005
  {"Zmap", 2, 0, OPT_ZMAP},     /* Create .map file */
  {"Zmap=", 1, 0, OPT_ZMAP},
#define OPT_ZAUTOCONV       0x1006
  {"Zautoconv",0, 0, OPT_ZAUTOCONV},
#define OPT_ZNO_AUTOCONV    0x1007
  {"Zno-autoconv",0, 0, OPT_ZNO_AUTOCONV},
#define OPT_ZDLL_SEARCH     0x1008
  {"Zdll-search",0, 0, OPT_ZDLL_SEARCH},
#define OPT_ZSYM            0x1009
  {"Zsym",0, 0, OPT_ZSYM},
/*  {"e", 1, 0, 'e'}, entry point */
  {"i", 0, 0, 'i'},
  {"o", 1, 0, 'o'},
  {"O", 1, 0, 'O'},
/*  {"u", 1, 0, 'u'}, reference symbol */
  {"s", 0, 0, 's'},
  {"S", 0, 0, 'S'},
  {"t", 0, 0, 't'},
  {"T", 1, 0, 'T'},
  {"v", 0, 0, 'v'},
  {"x", 0, 0, 'x'},
  {"X", 0, 0, 'X'},
  {NULL, 0, 0, 0}
};

/* Main function of emxomf.  Parse the command line and call the IBM/M$
   linker (and optionally RC). */

int main (int argc, char *argv[])
{
  struct stat s;
  int c, rc, files;
  const char *ext;
  char tmp[512 + 16], *t;
  char execname[512];
  name_list *pcur;
  int   opt_libs_static = 0;
  int   longind;

  /* Get options from response files (@filename) and wildcard (*.o) on the command. */

  _response (&argc, &argv);
  _wildcard (&argc, &argv);

  /* Close and delete the response file on exit. */

  atexit (cleanup);

  /* Prepare parsing of the command line. */

  files = 0;
  opterr = FALSE;
  /*optmode = GETOPT_KEEP; */
  if (argc < 2)
    usage ();

  /* Parse the command line options and other arguments. */
  while ((c = getopt_long_only (argc, argv, "-l:y:L:", longopts, &longind)) != EOF)
    {
      if (c == 0)
        c = longopts[longind].val;
      switch (c)
        {
        case 1:                   /* Non-option argument */

          /* Extract the extension to see what to do with this
             argument. */

          ext = _getext (optarg);

          if (ext == NULL)
            {
              /* GCC's temporary files don't have an extension.  Add a
                 dot to the end of the name to prevent the linker from
                 adding `.obj'. */

              sprintf (tmp, "%s.", optarg);
              add_name_list (&add_obj_fnames, tmp, 0);
            }

          /* If it's a .def file, use it as module definition file
             (input). */

          else if (stricmp (ext, ".def") == 0)
            {
              if (def_fname != NULL)
                {
                  fprintf (stderr,
                           "emxomfld: multiple module definition files\n");
                  return 1;
                }
              def_fname = _realrealpath(optarg, NULL, 0);
              if (!def_fname)
                def_fname = optarg;
            }

          /* If it's a .res file, use it as binary resource file
             (input). */

          else if (stricmp (ext, ".res") == 0)
            {
              if (res_fname != NULL)
                {
                  fprintf (stderr,
                           "emxomfld: multiple binary resource files\n");
                  return 1;
                }
              res_fname = _realrealpath(optarg, NULL, 0);
              if (!res_fname)
                res_fname = optarg;
            }

          /* If it's a .lib file, use it as library file.  We also
             accept .a files for those who use OMF files disguised as
             a.out files (to simplify their make files). */

          else if (stricmp (ext, ".lib") == 0 || stricmp (ext, ".a") == 0 || stricmp (ext, ".dll") == 0)
            add_name_list (&add_lib_fnames, optarg, opt_libs_static);

          /* Otherwise, assume it's an object file. */

          else
            add_name_list (&add_obj_fnames, optarg, 0);
          ++files;
          break;

        case 't':
        case 'i':                 /* Trace the linking process, sending /INFO to the IBM/M$ linker. */
          opt_t++;
          break;

        case 'l':                 /* Add library */
          add_name_list (&add_lib_fnames, optarg, opt_libs_static);
          break;

        case 'o':                 /* Set output file name */
          output_fname = optarg;
          break;

        case 'L':                 /* Add library directory */
          add_name_list (&add_libdirs, optarg, 0);
          break;

        case 'T':                 /* Set base address */
          base = optarg;
          break;

        case 's':                 /* Strip all symbols */
        case 'S':                 /* Strip debugging symbols */
          strip_symbols = TRUE;
          break;

        case 'x':                 /* Discard all local symbols */
        case 'X':                 /* Discard local symbols starting with L */
          break;

        case 'v':                 /* For compatibility */
          break;

        case 'O':                 /* Specify Linker option */
          add_name_list (&add_options, optarg, 0);
          break;

        case OPT_ZDLL:
          dll_flag = TRUE;
          break;

        case OPT_ZEXE:
          exe_flag = TRUE;
          break;

        case OPT_ZMAP:
          map_flag = TRUE;
          if (optarg)
            {
              if (map_fname != NULL)
                {
                  fprintf (stderr, "emxomfld: multiple map files files\n");
                  return 1;
                }
            map_fname = optarg;
            }
          break;

        case OPT_ZSTACK:
          if (!optarg)
            return 1;
          errno = 0;
          stack_size = strtol (optarg, &t, 0);
          if (errno != 0 || *t != 0 || t == optarg)
            return 1;
          stack_size_flag = 1;
          break;

        case OPT_ZAUTOCONV:
          autoconvert_flag = 1;
          break;
        case OPT_ZNO_AUTOCONV:
          autoconvert_flag = 0;
          break;

        case OPT_ZDLL_SEARCH:
          opt_dll_search = 1;
          break;

        case OPT_ZSYM:
          sym_flag = TRUE;
          break;

        case OPT_LIBS_STATIC:
          opt_libs_static = 1;
          break;
        case OPT_LIBS_SHARED:
          opt_libs_static = 0;
          break;

        case '?':
        default:
          if (optind > 1)
            fprintf (stderr, "emxomfld: invalid option (%s)\n", argv[optind - 1]);
          else
            usage ();
          return 1;
        }
    }
  /* Set default value for output file. */

  if (output_fname == NULL)
    {
      fprintf (stderr,
               "emxomfld: no output file, creating $$$.exe or $$$.dll\n");
      output_fname = "$$$";
    }

  /* Check if there are any input files. */

  if (files == 0)
    {
      fprintf (stderr, "emxomfld: no input files\n");
      return 1;
    }

  /* Check that -Zmap was specified with -Zsym. */

  if (sym_flag && !map_flag)
    {
      fprintf (stderr, "emxomfld: -Zsym without -Zmap.\n");
      return 1;
    }

  /* Remove the output file if -Zexe is given. */

  if (exe_flag)
    remove (output_fname);

  /* If neither -Zmap nor -Zmap=file is used, pass "nul" to the linker in
     the map file field.  If -Zmap is used, construct the name of the
     .map file.  If -Zmap=file is used, use `file' as the name of the
     .map file. */

  if (!map_flag)
    map_fname = "nul";
  else if (map_fname == NULL)
    {
      int cch = strlen (output_fname) + 1;
      t = xmalloc (cch + 4);
      memcpy (t, output_fname, cch);
      _remext (t);
      strcat (t, ".map");
      map_fname = t;
    }

  /* Build the environment for the linker. */

  make_env ();

  /* EMXOMFLD_TYPE contains VAC365, VAC308 or LINK386 if set. If non of these
     we assume VAC365.
     EMXOMFLD_LINKER contains the linker name and perhaps extra arguments. If
     not set we'll use the default linker, ilink.  */

  t = getenv ("EMXOMFLD_TYPE");
  if (    t
      &&  stricmp (t, "WLINK")
      &&  stricmp (t, "VAC365")
      &&  stricmp (t, "VAC308")
      &&  stricmp (t, "LINK386")
      )
    fprintf (stderr, "emxomfld: warning: '%s' is an invalid value for EMXOMFLD_TYPE.\n", t);
  else if (t)
    linker_type = t;

  t = getenv ("EMXOMFLD_LINKER");
  if (t)
    linker_name = t;
  if (opt_t)
    fprintf (stderr, "*** Linker     : %s\n"
                     "*** Linker type: %s\n", linker_name, linker_type);

  /* apply object & library hacks */
  for (pcur = obj_fnames, rc = 0; !rc && pcur; pcur = pcur->next)
    {
      char szname[_MAX_PATH + 1];
      FILE *phfile = find_obj (szname, pcur->name);
      if (!phfile)
        continue;
      free (pcur->name);
      pcur->name = xstrdup(szname);
      fclose (phfile);
    }

  for (pcur = lib_fnames, rc = 0; !rc && pcur; pcur = pcur->next)
    {
      char szname[_MAX_PATH + 1];
      FILE *phfile = find_lib (szname, pcur->name, !pcur->flags);
      if (!phfile)
        continue;
      free (pcur->name);
      pcur->name = xstrdup(szname);
      fclose (phfile);
    }

  /* generate .def-file for dlls. */

  if (!def_fname && dll_flag)
    gen_deffile ();

  /* Do the weak prelinking. Important that this is done after make_env(). */

  weak_prelink ();

  /* Start building the linker command line.  We can use a response
     file if the command line gets too long. */

  arg_init (TRUE);

  /* issue commandline */
  put_arg (linker_name, TRUE, FALSE);

  if (stricmp (linker_type, "WLINK"))
    {
      /*
         For VAC365 and VAC308 the default options are:

         /NOFR[EEFORMAT]    Use /NOFREEFORMAT to allow a LINK386-compatible
                            command line syntax, in which different types of file
                            are grouped and separated by commas.

         /DBGPACK           If !strip_symbols then we'll add this option, which
                            will cause type tables to be merged into one global
                            table and so eliminating a lot of duplicate info.

         For VAC365 additional default option is:

         /STUB:<emxomfld-path>\os2stub.bin
                            Causes this MZ stub to be used when linking the
                            executables instead of the default on for the linker.

         For LINK386 the default options are:

         /BATCH             Run in batch mode (disable prompting, don't
                            echo response file)

         The default options for all linkers are:

         /NOLOGO            Don't display sign-on banner

         /NOEXTDICTIONARY   Don't use extended dictionary (redefining
                            library symbols is quite common)

         /NOIGNORECASE      Make symbols case-sensitive

         /PACKCODE          Group neighboring code segments (this is the
                            default unless the SEGMENTS module definition
                            statement is used for a segment of class
                            'CODE').  Not grouping neighboring code
                            segments would break sets

         For non DLLs targets:

         /BASE:0x10000      Base the executable an so removing extra fixups.

      */

      /* the next part depends on the linker type. */
      if (!stricmp (linker_type, "LINK386"))
          put_arg ("/bat", FALSE, FALSE);
      else /* vac3xx: */
        {
          put_arg ("/nofree", FALSE, FALSE);
          if (!strip_symbols)
            put_arg ("/db", FALSE, FALSE);
          if (map_flag)
            put_arg ("/map", FALSE, FALSE);
        }
      put_arg ("/nol", FALSE, FALSE);
      put_arg ("/noe", FALSE, FALSE);
      put_arg ("/noi", FALSE, FALSE);
      put_arg ("/packc", FALSE, FALSE);


      /* VAC365: check if we have os2stub.bin.
         We must to this after the above stuff else /nol might end up in the
         response file and we'll get the component output. */

      if (!stricmp (linker_type, "VAC365"))
        {
          /* gklayout show that the linker isn't capable of determining a
             decent value for this parameter. 32MB makes gklayout link. */
          put_arg ("/ocache:0x02000000", FALSE, FALSE);

          _execname (&execname[0], sizeof(execname));
          strcpy (_getname (&execname[0]), "os2stub.bin");
          if (!stat (execname, &s))
            {
              sprintf (tmp, "/STUB:%s", &execname[0]);
              put_arg (tmp, FALSE, FALSE);
            }
        }

      /* Add the /INFORMATION option if the -i or -t option was given.  This is
         for debugging. */

      if (opt_t)
        put_arg ("/i", FALSE, FALSE);

      /* Add the /DEBUG option if the -s option was not given.  Without
         this, the linker throws away debugging information. */

      if (!strip_symbols)
        put_arg ("/de", FALSE, FALSE);

      /* Add the /BASE:n option to set the base address.  This specifies
         the preferred load address of object 1.  The base address being
         used is 0x10000 unless a DLL is generated or the -T option was
         given.  -Tno can be used to suppress the /BASE:n option. */

      if (base == NULL && !dll_flag)
        {
          struct _md *md;

          if (def_fname != NULL)
            {
              int token;
              md = _md_open (def_fname);
              if (md == NULL)
                {
                  fprintf (stderr, "emxomfld: cannot open `%s'\n", def_fname);
                  exit (2);
                }
              token = _md_next_token (md);
              if (token == _MD_LIBRARY || token == _MD_PHYSICAL || token == _MD_VIRTUAL)
                dll_flag = TRUE;
              _md_close (md);
            }
        }
      if (base == NULL && !dll_flag)
        base = "0x10000";
      if (base != NULL && stricmp (base, "no") != 0)
        {
          sprintf (tmp, "/bas:%s", base);
          put_arg (tmp, FALSE, FALSE);
        }

      /* Add the /STACK:n option if the -Zstack option was given. */

      if (!dll_flag)
        {
          sprintf (tmp, "/st:0x%lx", stack_size * 1024);
          put_arg (tmp, FALSE, FALSE);
        }

      /* Add the linker options specified with -O. */

      put_args (options, FALSE);

      /* Put the object file names onto the command line. */

      force_response_file = TRUE;           /* link386 workaround. */
      put_args (obj_fnames, TRUE);
      put_arg (",", FALSE, FALSE);

      /* Put the output file name onto the command line. */

      put_arg (output_fname, TRUE, TRUE);
      put_arg (",", FALSE, FALSE);

      /* Put the map file name onto the command line. */

      put_arg (map_fname, TRUE, TRUE);
      put_arg (",", FALSE, FALSE);

      /* Put the library file names onto the command line. */

      put_args (lib_fnames, TRUE);
      put_arg (",", FALSE, FALSE);

      /* Put the name of the module definition file onto the command line. */

      put_arg (def_fname, TRUE, TRUE);
      put_arg (";", FALSE, FALSE);

      /* Call Linker and abort on failure. */
    }
  else /* wlink */
    {
      unsigned uPMType = 0;

      open_response_file ();

      /* convert common ilink/link386 command line arguments. */

      for (pcur = options; pcur; pcur = pcur->next)
        {
          size_t cchOpt;
          const char *pszVal;
          size_t cchVal;

          if (pcur->name [0] != '-' && pcur->name [0] != '/')
            continue;
          if (strchr (&pcur->name[1], '='))
            continue;
          pszVal = strchr (&pcur->name[1], ':');
          cchOpt = pszVal ? pszVal - &pcur->name[1] : strlen (&pcur->name[1]);
          if (pszVal && pszVal[1])
            cchVal = strlen (++pszVal);
          else
            pszVal = NULL, cchVal = 0;
#define MAX(a,b) ((a) <= (b) ? (a) : (b))
          if (!strnicmp (&pcur->name[1], "PMtype", cchOpt)
           && cchVal)
            {
              if (!strnicmp (pszVal, "PM", cchVal))
                uPMType = _MD_WINDOWAPI;
              else if (!strnicmp (pszVal, "VIO", cchVal))
                uPMType = _MD_WINDOWCOMPAT;
              else if (!strnicmp (pszVal, "NOVIO", MAX (cchVal, 3)))
                uPMType = _MD_NOTWINDOWCOMPAT;
              else
                continue;
            }
          else if (!strnicmp (&pcur->name[1], "PDD", MAX (cchOpt, 3)))
            uPMType = _MD_PHYSICAL;
          else if (!strnicmp (&pcur->name[1], "VDD", MAX (cchOpt, 3)))
            uPMType = _MD_VIRTUAL;
          else if (!strnicmp (&pcur->name[1], "STACK", MAX (cchOpt, 2))
                && cchVal && !stack_size_flag)
            {
              errno = 0;
              stack_size = strtol (pszVal, &t, 0);
              if (errno || *t)
                {
                  fprintf (stderr, "emxomfld: Number conversion failed: '%s'\n", pcur->name);
                  return 1;
                }
              stack_size = (stack_size + 1023) / 1024;
            }
          /* ignore these */
          else if (!strnicmp (&pcur->name[1], "PACKCODE", MAX (cchOpt, 5))
                || !strnicmp (&pcur->name[1], "NOPACKCODE", MAX (cchOpt, 3))
                || !strnicmp (&pcur->name[1], "PACKDATA", MAX (cchOpt, 5))
                || !strnicmp (&pcur->name[1], "NOPACKDATA", MAX (cchOpt, 7))
                || !strnicmp (&pcur->name[1], "EXEPACK", MAX (cchOpt, 1))
                || !strnicmp (&pcur->name[1], "NOEXEPACK", MAX (cchOpt, 5))
                || !strnicmp (&pcur->name[1], "DBGPACK", MAX (cchOpt, 2))
                || !strnicmp (&pcur->name[1], "NODBGPACK", MAX (cchOpt, 4))
                )
            fprintf (stderr, "emxomfld: warning: ignoring ilink option '%s'\n", pcur->name);
          else
            continue;
          pcur->name = NULL;
#undef MAX
        }

      /* figure out what format options we're gonna use */

      if (!def_fname && !dll_flag)
        {
          switch (uPMType)
            {
              case 0:
              case _MD_WINDOWCOMPAT:
              default:
                fprintf (response_file, "FORMAT OS2 LX PMCompatible\n");
                break;
              case _MD_WINDOWAPI:
                fprintf (response_file, "FORMAT OS2 LX PM\n");
                break;
              case _MD_NOTWINDOWCOMPAT:
                fprintf (response_file, "FORMAT OS2 LX FULLscreen\n");
                break;
              case _MD_PHYSICAL:
                dll_flag = TRUE;
                fprintf (response_file, "FORMAT OS2 LX PHYSdevice\n");
                break;
              case _MD_VIRTUAL:
                dll_flag = TRUE;
                fprintf (response_file, "FORMAT OS2 LX VIRTdevice\n");
                break;
            }
        }
      else if (!def_fname && dll_flag)
        fprintf (response_file, "FORMAT OS2 LX DLL INITINSTANCE TERMINSTANCE\n");
      else
        {
          int token;
          struct _md *pMd = _md_open (def_fname);
          if (!pMd)
            {
              fprintf (stderr, "emxomfld: cannot open `%s'\n", def_fname);
              exit (2);
            }
          token = _md_next_token (pMd);
          if (token == _MD_PHYSICAL)
            {
              dll_flag = TRUE;
              fprintf (response_file, "FORMAT OS2 LX PHYSdevice\n");
            }
          else if (token == _MD_VIRTUAL)
            {
              dll_flag = TRUE;
              fprintf (response_file, "FORMAT OS2 LX VIRTdevice\n");
            }
          else if (token == _MD_LIBRARY || dll_flag)
            {
              int fInitInstance = 1;
              int fTermInstance = 1;
              for (;;)
                {
                  switch (_md_next_token (pMd))
                    {
                      case _MD_INITINSTANCE:    fInitInstance = 1; continue;
                      case _MD_INITGLOBAL:      fInitInstance = 0; continue;
                      case _MD_TERMINSTANCE:    fTermInstance = 1; continue;
                      case _MD_TERMGLOBAL:      fTermInstance = 0; continue;
                      case _MD_quote:           continue;
                      case _MD_word:            continue;
                      default:                  break;
                    }
                  break;
                }
              dll_flag = TRUE;
              fprintf (response_file, "FORMAT OS2 LX DLL %s %s\n",
                       fInitInstance ? "INITINSTANCE" : "INITGLOBAL",
                       fTermInstance ? "TERMINSTANCE" : "TERMGLOBAL");
            }
          else
            {
              if (token == _MD_NAME)
                {
                  /* (ignores uPMType and uses the .def-file) */
                  token = _md_next_token (pMd);
                  if (token == _MD_quote || token == _MD_word)
                    token = _md_next_token (pMd);
                }
              else
                token = uPMType;
              switch (token)
                {
                  case _MD_WINDOWAPI:
                    fprintf (response_file, "FORMAT OS2 LX PM\n");
                    break;
                  default:
                  case _MD_WINDOWCOMPAT:
                    fprintf (response_file, "FORMAT OS2 LX PMCompatible\n");
                    break;
                  case _MD_NOTWINDOWCOMPAT:
                    fprintf (response_file, "FORMAT OS2 LX FullScreen\n");
                    break;
                }
            }
          _md_close (pMd);
        }

      /* output files */

      fprintf (response_file, "NAME '%s'\n", output_fname);

      if (map_flag && map_fname)
        fprintf (response_file, "OPTION MAP='%s'\n", map_fname);
      else if (map_flag)
        fprintf (response_file, "OPTION MAP\n");

      /* standard stuff */

      if (!strip_symbols)
        fprintf (response_file, "DEBUG HLL\n");
      fprintf (response_file, "OPTION QUIET\n");
      fprintf (response_file, "OPTION OSNAME='OS/2 EMX'\n");
      fprintf (response_file, "OPTION CASEEXACT\n");
      if (!dll_flag)
        fprintf (response_file, "OPTION STACK=%#lx\n", stack_size * 1024);
      if (!dll_flag && !base)
        base = "0x10000";
      if (base)
        fprintf (response_file, "OPTION OFFSET=%s\n", base);

      /* the stub */

      _execname(&execname[0], sizeof(execname));
      strcpy (_getname (&execname[0]), "os2stub.bin");
      if (!stat (execname, &s))
        fprintf (response_file, "OPTION STUB='%s'\n", execname);

      /* Add the /INFORMATION option if the -i or -t option was given.  This is
         for debugging. */

//      if (opt_t)
//        put_arg ("/i", FALSE, FALSE);

      /* Add the linker options specified with -O. */

      for (pcur = options; pcur; pcur = pcur->next)
        if (pcur->name)
          fprintf (response_file, "%s\n", pcur->name);

      /* Put the object file names onto the command line. */

      for (pcur = obj_fnames; pcur; pcur = pcur->next)
        fprintf (response_file, "FILE '%s'\n", pcur->name);

      /* Put the library file names onto the command line. */

      for (pcur = lib_fnames; pcur; pcur = pcur->next)
        fprintf (response_file, "LIBRARY '%s'\n", pcur->name);

      /* Translate the essentials of the module definition file into wlink lingo. */
      if (def_fname)
        {
          struct _md *pMd = _md_open (def_fname);
          if (!pMd)
            {
              fprintf (stderr, "emxomfld: cannot open `%s'\n", def_fname);
              exit (2);
            }
          _md_next_token (pMd);
          _md_parse (pMd, def_2_watcom, NULL);
          _md_close (pMd);
        }
    }

  /* End the arguments and run the linker. */

  arg_end ();

  rc = emxomfld_spawn (command_line, "Linker");
  if (rc == 4 && !strnicmp(linker_type, "VAC3", 4)) /* Ignore iLink warnings. */
      rc = 0;
  if (rc < 0)
    {
      perror (linker_name);
      exit (2);
    }

  /* Run RC if Linker completed successfully and a binary resource
     file was given on the command line. */

  if (rc == 0 && res_fname != NULL)
    {
      /* EMXOMFLD_RC_TYPE contains RC or WRC if set. If non of these
         we assume RC.
         EMXOMFLD_RC contains the compiler name and perhaps extra
         arguments. If not set we'll use the default compiler, rc.exe. */

      t = getenv ("EMXOMFLD_RC_TYPE");
      if (   t
          && stricmp (t, "RC")
          && stricmp (t, "WRC"))
        fprintf (stderr, "emxomfld: warning: '%s' is an invalid value for EMXOMFLD_RC_TYPE.\n", t);
      else if (t)
        rc_type = t;

      t = getenv ("EMXOMFLD_RC");
      if (t)
        rc_name = t;
      if (opt_t)
        fprintf (stderr, "*** Resource compiler     : %s\n"
                         "*** Resource compiler type: %s\n", rc_name, rc_type);

      if (!stricmp (rc_type, "RC"))
        {
          arg_init (TRUE);
          put_arg (rc_name, TRUE, FALSE);
          put_arg ("-n", FALSE, FALSE);
          put_arg (res_fname, TRUE, FALSE);
          put_arg (output_fname, TRUE, FALSE);
          arg_end ();
          rc = emxomfld_spawn (command_line, "Resource Linker");
          if (rc < 0)
            {
              perror ("emxomfld: rc");
              exit (2);
            }
        }
      else
        {
          /* wrc doesn't understand response files and put_arg handles max 110 chars */
          rc = spawnlp (P_WAIT, rc_name, rc_name, "-q", res_fname, output_fname, NULL);
          if (rc < 0)
            {
              perror ("emxomfld: wrc");
              exit (2);
            }
        }
    }

  /* If both Linker and RC completed successfully and the -Zexe option
     was given, touch the output file (without .exe) to keep `make'
     happy. */

  if (rc == 0 && exe_flag)
    {
      /* find target and source filenames. */
      t = xstrdup (output_fname);
      _remext (t);
      _execname (&execname[0], sizeof(execname));
      strcpy (_getname(&execname[0]), "ldstub.bin");

      /* Copy stub into file */
      if (opt_t)
        fprintf (stderr, "*** copy %s to %s (-Zexe)", execname, t);
      DosCopy (&execname[0], t, 4);

      /* Now touch it */
      if (utime (t, NULL))
        {
          perror ("emxomfld");
          exit (2);
        }
      free (t);
    }

  /* Run mapsym if requested and linking succeeded. */
  if (rc == 0 && sym_flag)
    {
      char* cwd = getcwd (NULL, 0);
      char  map_fname_fullpath[_MAX_PATH];
      char  drive[_MAX_PATH];
      char  dir[_MAX_DIR];

      /* get absolute path of map file and change CWD to map file directory. */
      _fullpath (map_fname_fullpath, map_fname, sizeof (map_fname_fullpath));
      _splitpath (map_fname_fullpath, drive, dir, NULL, NULL);
      strcat (drive, dir);
      if (chdir (drive))
        {
          perror ("chdir failed");
          exit (2);
        }

      /* Invoke mapsym.cmd writing the .sym file to current directory. */
      arg_init (TRUE);
      if (getenv ("COMSPEC"))
        put_arg (getenv ("COMSPEC"), TRUE, FALSE);
      else
        put_arg ("cmd.exe", TRUE, FALSE);
      put_arg ("/c", FALSE, FALSE);
      put_arg ("mapsym.cmd", TRUE, FALSE);
      if (!stricmp (linker_type, "WLINK"))
         put_arg ("watcom", TRUE, FALSE);
      else if (!stricmp (linker_type, "LINK386"))
         put_arg ("link386", TRUE, FALSE);
      else
         put_arg ("vac3xx", TRUE, FALSE);
      put_arg (map_fname_fullpath, TRUE, FALSE);
      arg_end ();
      rc = emxomfld_spawn (command_line, "Mapsym");
      if (rc < 0)
        {
          perror ("emxomfld: mapsym");
          exit (2);
        }

      /* Restore the working directory */
      if (chdir (cwd))
        {
          perror ("chdir failed");
          exit (2);
        }
    }

  /* Return the return code of Linker or RC. */

  return rc;
}

