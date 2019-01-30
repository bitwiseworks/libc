/* emximp.c  -- Manage import libraries
   Copyright (c) 1992-1998 Eberhard Mattes
   Copyright (c) 2004 knut st. osmundsen

This file is part of emximp.

emximp is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emximp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emximp; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wstrict-aliasing" // char[] is packed into bytes on OS/2

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <process.h>
#include <ar.h>
#include <time.h>
#include <sys/moddef.h>
#include <sys/types.h>
#include <a_out.h>
#include "defs.h"
#include <sys/omflib.h>

#define NORETURN2 __attribute__ ((noreturn))

#define MOD_PREDEF   0
#define MOD_DEF      1
#define MOD_REF      2

#define PARMS_REG     (-1)
#define PARMS_FAR16   (-2)

struct lib
{
  struct lib *next;
  char *name;
  int lbl;
};

struct predef
{
  struct predef *next;
  char *name;
};

enum modes
{
  M_NONE,                       /* No mode selected */
  M_LIB_TO_IMP,                 /* .lib -> .imp */
  M_IMP_TO_S,                   /* .imp -> .s or .o */
  M_IMP_TO_DEF,                 /* .imp -> .def */
  M_LIB_TO_A,                   /* .lib -> .a */
  M_IMP_TO_A,                   /* .imp -> .a */
  M_IMP_TO_LIB,                 /* .imp -> .lib */
  M_DEF_TO_IMP,                 /* .def -> .imp */
  M_DEF_TO_A,                   /* .def -> .a */
  M_DEF_TO_LIB,                 /* .def -> .lib */
  M_DLL_TO_IMP,                 /* .dll -> .imp */
  M_DLL_TO_A,                   /* .dll -> .a */
  M_DLL_TO_LIB,                 /* .dll -> .lib */
  M_DLL_TO_DEF                  /* .dll -> .def */
};

static FILE *out_file = NULL;
static char out_fname[128];
static struct lib *libs;
static struct predef *predefs;
static char *out_base;
static int base_len;
static char *as_name;
static int pipe_flag;
static int profile_flag;
static int opt_b;
static int opt_q;
static int opt_s;
static int opt_lazy;
static enum modes mode = M_NONE;
static long mod_lbl;
static long seq_no = 1;
static char *first_module = NULL;
static int warnings = 0;
static struct omflib *out_lib;
static char lib_errmsg[512];
static char *module_name = NULL;
static char *description;


static void error (const char *fmt, ...) NORETURN2;
static void write_error (const char *fname) NORETURN2;
static void lib_error (void) NORETURN2;
static void write_a_import (const char *func_name, const char *mod_name,
    int ordinal, const char *proc_name);


static void usage (void)
{
  puts ("emximp " VERSION INNOTEK_VERSION " -- Copyright (c) 1992-1996 by Eberhard Mattes\n");
  puts ("Usage:");
  puts ("  emximp [-a[<assembler>]] [-b <base_name>|<prefix_length>] "
        "[-p <module>] ...");
  puts ("         [-s] <input_file>.imp");
  puts ("  emximp [-m] -o <output_file>.a <input_file>.def ...");
  puts ("  emximp [-m] -o <output_file>.a <input_file>.imp ...");
  puts ("  emximp [-m] -o <output_file>.a <input_file>.lib ...");
  puts ("  emximp [-m] -o <output_file>.a <input_file>.dll ...");
  puts ("  emximp -o <output_file>.imp <input_file>.def ...");
  puts ("  emximp -o <output_file>.imp <input_file>.lib ...");
  puts ("  emximp -o <output_file>.imp <input_file>.dll ...");
  puts ("  emximp [-p#] -o <output_file>.lib <input_file>.def ...");
  puts ("  emximp [-p#] -o <output_file>.lib <input_file>.imp...");
  puts ("  emximp [-p#] -o <output_file>.lib <input_file>.dll ...");
  puts ("  emximp -o <output_file>.def <input_file>.imp ...");
  puts ("  emximp -o <output_file>.def <input_file>.dll ...");
  puts ("Options:");
  puts ("  -p#  Set page size");
  puts ("  -q   Be quiet");
  puts ("  -m   Call _mcount for profiling");
  puts ("  -l   Lazy loading and resolving");
  exit (1);
}


static void error (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emximp: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
  exit (2);
}


static void warning (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emximp: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
  ++warnings;
}


/* Like warning(), but don't increment `warnings'. */

static void information (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emximp: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
}


static void *xmalloc (size_t n)
{
  void *p;

  p = malloc (n);
  if (p == NULL)
    error ("Out of memory");
  return p;
}


static void *xrealloc (void *p, size_t n)
{
  void *q;

  q = realloc (p, n);
  if (q == NULL)
    error ("Out of memory");
  return q;
}


static char *xstrdup (const char *s)
{
  char *p;

  p = xmalloc (strlen (s) + 1);
  strcpy (p, s);
  return p;
}


static void write_error (const char *fname)
{
  error ("Write error on output file `%s'", fname);
}


static void lib_error (void)
{
  error ("%s", lib_errmsg);
}


static void out_flush (void)
{
  char name[512];
  char *nargv[5];
  int rc;

  if (out_file != NULL)
    {
      if (fflush (out_file) != 0)
        write_error (out_fname);
      if (pipe_flag)
        {
          rc = pclose (out_file);
          if (rc == -1)
            error ("Error while closing pipe");
          if (rc > 0)
            error ("Assembly failed, return code = %d", rc);
        }
      else
        {
          if (fclose (out_file) != 0)
            error ("Cannot close output file `%s'", out_fname);
          if (as_name != NULL)
            {
              _splitpath (out_fname, NULL, NULL, name, NULL);
              strcat (name, ".o");
              nargv[0] = as_name;
              nargv[1] = "-o";
              nargv[2] = name;
              nargv[3] = out_fname;
              nargv[4] = NULL;
              rc = spawnvp (P_WAIT, as_name, nargv);
              if (rc < 0)
                error ("Cannot run `%s'", as_name);
              if (rc > 0)
                error ("Assembly of `%s' failed, return code = %d",
                       out_fname, rc);
              remove (out_fname);
            }
        }
      out_file = NULL;
    }
}


static void out_start (void)
{
  struct lib *lp1, *lp2;
  char name[512], cmd[512];

  if (pipe_flag)
    {
      _splitpath (out_fname, NULL, NULL, name, NULL);
      strcat (name, ".o");
      sprintf (cmd, "%s -o %s", as_name, name);
      out_file = popen (cmd, "wt");
      if (out_file == NULL)
        error ("Cannot open pipe to `%s'", as_name);
    }
  else
    {
      out_file = fopen (out_fname, "wt");
      if (out_file == NULL)
        error ("Cannot open output file `%s'", out_fname);
    }
  fprintf (out_file, "/ %s (emx+gcc)\n\n", out_fname);
  fprintf (out_file, "\t.text\n");
  for (lp1 = libs; lp1 != NULL; lp1 = lp2)
    {
      lp2 = lp1->next;
      free (lp1->name);
      free (lp1);
    }
  libs = NULL; mod_lbl = 1;
}


static void write_lib_import (const char *func, const char *module, long ord,
                              const char *name)
{
  byte omfbuf[1024];
  int i, len;
  word page;

  if (omflib_write_module (out_lib, func, &page, lib_errmsg) != 0)
    lib_error ();
  if (omflib_add_pub (out_lib, func, page, lib_errmsg) != 0)
    lib_error ();
  i = 0;
  omfbuf[i++] = 0x00;
  omfbuf[i++] = IMPDEF_CLASS;
  omfbuf[i++] = IMPDEF_SUBTYPE;
  omfbuf[i++] = (ord < 1 ? 0x00 : 0x01);
  len = strlen (func);
  omfbuf[i++] = (byte)len;
  memcpy (omfbuf+i, func, len); i += len;
  len = strlen (module);
  omfbuf[i++] = (byte)len;
  memcpy (omfbuf+i, module, len); i += len;
  if (ord < 1)
    {
      if (strcmp (func, name) == 0)
        len = 0;
      else
        len = strlen (name);
      omfbuf[i++] = (byte)len;
      memcpy (omfbuf+i, name, len); i += len;
    }
  else
    {
      omfbuf[i++] = (byte)ord;
      omfbuf[i++] = (byte)(ord >> 8);
    }
  if (omflib_write_record (out_lib, COMENT, i, omfbuf, TRUE,
                           lib_errmsg) != 0)
    lib_error ();
  omfbuf[0] = 0x00;
  if (omflib_write_record (out_lib, MODEND, 1, omfbuf, TRUE,
                           lib_errmsg) != 0)
    lib_error ();
}


#define DELIM(c) ((c) == 0 || isspace ((unsigned char)c))


static void read_imp (const char *fname)
{
  FILE *inp_file;
  char buf[512], func[256], name[256], module[256], mod_ref[256], *p, *q;
  char tmp[512];
  long line_no, ord, parms, file_no;
  int mod_type;
  struct lib *lp1;
  struct predef *pp1;

  libs = NULL; mod_lbl = 1;
  inp_file = fopen (fname, "rt");
  if (inp_file == NULL)
    error ("Cannot open input file `%s'", fname);
  line_no = 0;
  while (fgets (buf, sizeof (buf), inp_file) != NULL)
    {
      ++line_no;
      p = strchr (buf, '\n');
      if (p != NULL) *p = 0;
      p = buf;
      while (isspace ((unsigned char)*p)) ++p;
      if (*p == '+')
        {
          if (mode == M_IMP_TO_S)
            {
              if (opt_b)
                error ("Output file name in line %ld of %s not allowed "
                       "as -b is used", line_no, fname);
              ++p;
              while (isspace ((unsigned char)*p)) ++p;
              out_flush ();
              q = out_fname;
              while (!DELIM (*p))
                *q++ = *p++;
              *q = 0;
              while (isspace ((unsigned char)*p)) ++p;
              if (*p != 0 && *p != ';')
                error ("Invalid file name in line %ld of %s", line_no, fname);
              out_start ();
            }
        }
      else if (*p == 0 || *p == ';')
        ;           /* empty line */
      else
        {
          if (!opt_b && out_file == NULL && mode != M_IMP_TO_LIB)
            error ("No output file selected in line %ld of %s",
                   line_no, fname);
          if (DELIM (*p))
            error ("Function name expected in line %ld of %s", line_no, fname);
          q = func;
          while (!DELIM (*p))
            *q++ = *p++;
          *q = 0;
          while (isspace ((unsigned char)*p)) ++p;
          if (DELIM (*p))
            error ("Module name expected in line %ld of %s", line_no, fname);
          q = module;
          while (!DELIM (*p))
            *q++ = *p++;
          *q = 0;
          while (isspace ((unsigned char)*p)) ++p;
          if (DELIM (*p))
            error ("External name or ordinal expected in line %ld of %s",
                   line_no, fname);
          if (isdigit ((unsigned char)*p))
            {
              name[0] = 0;
              ord = strtol (p, &p, 10);
              if (ord < 1 || ord > 65535 || !DELIM (*p))
                error ("Invalid ordinal in line %ld of %s", line_no, fname);
            }
          else
            {
              if (opt_b && !opt_s)
                error ("External name in line %ld of %s cannot be used "
                       "as -b is given", line_no, fname);
              ord = -1;
              q = name;
              while (!DELIM (*p))
                *q++ = *p++;
              *q = 0;
            }
          while (isspace ((unsigned char)*p)) ++p;
          if (DELIM (*p))
            error ("Number of arguments expected in line %ld of %s",
                   line_no, fname);
          if (*p == '?')
            {
              ++p;
              parms = 0;
              if (mode == M_IMP_TO_S)
                warning ("Unknown number of arguments in line %ld of %s",
                         line_no, fname);
            }
          else if (*p == 'R')
            {
              ++p;
              parms = PARMS_REG;
            }
          else if (*p == 'F')
            {
              ++p;
              parms = PARMS_FAR16;
              if (mode == M_IMP_TO_S)
                warning ("16-bit function not supported (line %ld of %s)",
                         line_no, fname);
              if (strlen (func) + 4 >= sizeof (func))
                error ("Function name too long in line %ld of %s",
                       line_no, fname);
              strcpy (tmp, func);
              strcpy (func, "_16_");
              strcat (func, tmp);
            }
          else
            {
              parms = strtol (p, &p, 10);
              if (parms < 0 || parms > 255 || !DELIM (*p))
                error ("Invalid number of parameters in line %ld of %s",
                       line_no, fname);
            }
          while (isspace ((unsigned char)*p)) ++p;
          if (*p != 0 && *p != ';')
            error ("Unexpected characters at end of line %ld of %s",
                   line_no, fname);
          switch (mode)
            {
            case M_IMP_TO_DEF:
              if (first_module == NULL)
                {
                  first_module = xstrdup (module);
                  fprintf (out_file, "LIBRARY %s\n", module);
                  fprintf (out_file, "EXPORTS\n");
                }
              else if (strcmp (first_module, module) != 0)
                error ("All functions must be in the same module "
                       "(input file %s)", fname);
              if (ord >= 0)
                fprintf (out_file, "  %-32s @%ld\n", func, ord);
              else if (strcmp (func, name) == 0)
                fprintf (out_file, "  %-32s\n", func);
              else
                fprintf (out_file, "  %-32s = %s\n", name, func);
              break;
            case M_IMP_TO_A:
              if (ord < 1)
                write_a_import (func, module, ord, name);
              else
                write_a_import (func, module, ord, NULL);
              break;
            case M_IMP_TO_LIB:
              write_lib_import (func, module, ord, name);
              break;
            case M_IMP_TO_S:
              if (opt_b)
                {
                  out_flush ();
                  if (opt_s)
                    file_no = seq_no++;
                  else
                    file_no = ord;
                  if (out_base != NULL)
                    sprintf (out_fname, "%s%ld.s", out_base, file_no);
                  else
                    sprintf (out_fname, "%.*s%ld.s",
                             base_len, module, file_no);
                  out_start ();
                }
              for (pp1 = predefs; pp1 != NULL; pp1 = pp1->next)
                if (stricmp (module, pp1->name) == 0)
                  break;
              if (pp1 != NULL)
                {
                  mod_type = MOD_PREDEF;
                  sprintf (mod_ref, "__os2_%s", pp1->name);
                }
              else
                {
                  for (lp1 = libs; lp1 != NULL; lp1 = lp1->next)
                    if (stricmp (module, lp1->name) == 0)
                      break;
                  if (lp1 == NULL)
                    {
                      mod_type = MOD_DEF;
                      lp1 = xmalloc (sizeof (struct lib));
                      lp1->name = xstrdup (module);
                      lp1->lbl = mod_lbl++;
                      lp1->next = libs;
                      libs = lp1;
                    }
                  else
                    mod_type = MOD_REF;
                  sprintf (mod_ref, "L%d", lp1->lbl);
                }
              fprintf (out_file, "\n\t.globl\t_%s\n", func);
              fprintf (out_file, "\t.align\t2, %d\n", 0x90);
              fprintf (out_file, "_%s:\n", func);
              if (parms >= 0)
                fprintf (out_file, "\tmovb\t$%d, %%al\n", (int)parms);
              fprintf (out_file, "1:\tjmp\t__os2_bad\n");
              if (ord >= 0)
                fprintf (out_file, "2:\t.long\t1, 1b+1, %s, %d\n",
                         mod_ref, (int)ord);
              else
                fprintf (out_file, "2:\t.long\t0, 1b+1, %s, 4f\n", mod_ref);
              if (mod_type == MOD_DEF)
                fprintf (out_file, "%s:\t.asciz\t\"%s\"\n", mod_ref, module);
              if (ord < 0)
                fprintf (out_file, "4:\t.asciz\t\"%s\"\n", name);
              fprintf (out_file, "\t.stabs  \"__os2dll\", 23, 0, 0, 2b\n");
              break;
            default:
              abort ();
            }
        }
    }
  if (mode == M_IMP_TO_S)
    out_flush ();
  if (ferror (inp_file))
    error ("Read error on input file `%s'", fname);
  fclose (inp_file);
}


static unsigned hash (const char *name)
{
  unsigned h = 0;
  for (; *name; name++)
    h = (h << 2) ^ (unsigned char)*name;
  return h;
}

static void set_ar (char *dst, const char *src, int size)
{
  while (*src != 0 && size > 0)
    {
      *dst++ = *src++;
      --size;
    }
  while (size > 0)
    {
      *dst++ = ' ';
      --size;
    }
}


static long ar_member_size;

static void write_ar (const char *name, long size)
{
  struct ar_hdr ar;
  char tmp[20];

  ar_member_size = size;
  set_ar (ar.ar_name, name, sizeof (ar.ar_name));
  sprintf (tmp, "%ld", (long)time (NULL));
  set_ar (ar.ar_date, tmp, sizeof (ar.ar_date));
  set_ar (ar.ar_uid, "0", sizeof (ar.ar_uid));
  set_ar (ar.ar_gid, "0", sizeof (ar.ar_gid));
  set_ar (ar.ar_mode, "100666", sizeof (ar.ar_gid));
  sprintf (tmp, "%ld", size);
  set_ar (ar.ar_size, tmp, sizeof (ar.ar_size));
  set_ar (ar.ar_fmag, ARFMAG, sizeof (ar.ar_fmag));
  fwrite (&ar, 1, sizeof (ar), out_file);
}


static void finish_ar (void)
{
  if (ar_member_size & 1)
    fputc (0, out_file);
}


static dword aout_str_size;
static char aout_str_tab[2048];
static int aout_sym_count;
static struct nlist aout_sym_tab[6];

static byte aout_text[300];
static int aout_text_size;

static byte aout_data[300];
static int aout_data_size;

static struct relocation_info aout_treloc_tab[7];
static int aout_treloc_count;

static struct relocation_info aout_dreloc_tab[7];
static int aout_dreloc_count;

static int aout_size;


static void aout_init (void)
{
  aout_str_size = sizeof (dword);
  aout_sym_count = 0;
  aout_text_size = 0;
  aout_data_size = 0;
  aout_treloc_count = 0;
  aout_dreloc_count = 0;
}

static int aout_sym (const char *name, byte type, byte other,
                     word desc, dword value)
{
  int len;

  len = strlen (name);
  if (aout_str_size + len + 1 > sizeof (aout_str_tab))
    error ("a.out string table overflow");
  if (aout_sym_count >= sizeof (aout_sym_tab) / sizeof (aout_sym_tab[0]))
    error ("a.out symbol table overflow");
  aout_sym_tab[aout_sym_count].n_un.n_strx = aout_str_size;
  aout_sym_tab[aout_sym_count].n_type = type;
  aout_sym_tab[aout_sym_count].n_other = other;
  aout_sym_tab[aout_sym_count].n_desc = desc;
  aout_sym_tab[aout_sym_count].n_value = value;
  strcpy (aout_str_tab + aout_str_size, name);
  aout_str_size += len + 1;
  return aout_sym_count++;
}


static void aout_text_byte (byte b)
{
  if (aout_text_size >= sizeof (aout_text))
    error ("a.out text segment overflow");
  aout_text[aout_text_size++] = b;
}


static void aout_text_dword (dword d)
{
  aout_text_byte ((d >> 0) & 0xff);
  aout_text_byte ((d >> 8) & 0xff);
  aout_text_byte ((d >> 16) & 0xff);
  aout_text_byte ((d >> 24) & 0xff);
}


static void aout_treloc (dword address, int symbolnum, int pcrel, int length,
                         int ext)
{
  if (aout_treloc_count >= sizeof (aout_treloc_tab) / sizeof (struct relocation_info))
    error ("a.out text relocation buffer overflow");
  aout_treloc_tab[aout_treloc_count].r_address = address;
  aout_treloc_tab[aout_treloc_count].r_symbolnum = symbolnum;
  aout_treloc_tab[aout_treloc_count].r_pcrel = pcrel;
  aout_treloc_tab[aout_treloc_count].r_length = length;
  aout_treloc_tab[aout_treloc_count].r_extern = ext;
  aout_treloc_tab[aout_treloc_count].r_pad = 0;
  ++aout_treloc_count;
}

static void aout_data_byte (byte b)
{
  if (aout_data_size >= sizeof (aout_data))
    error ("a.out data segment overflow");
  aout_data[aout_data_size++] = b;
}

static void aout_data_dword (dword d)
{
  aout_data_byte ((d >> 0) & 0xff);
  aout_data_byte ((d >> 8) & 0xff);
  aout_data_byte ((d >> 16) & 0xff);
  aout_data_byte ((d >> 24) & 0xff);
}

static void aout_dreloc (dword address, int symbolnum, int pcrel, int length,
                         int ext)
{
  if (aout_dreloc_count >= sizeof (aout_dreloc_tab) / sizeof (struct relocation_info))
    error ("a.out data relocation buffer overflow");
  aout_dreloc_tab[aout_dreloc_count].r_address = address;
  aout_dreloc_tab[aout_dreloc_count].r_symbolnum = symbolnum;
  aout_dreloc_tab[aout_dreloc_count].r_pcrel = pcrel;
  aout_dreloc_tab[aout_dreloc_count].r_length = length;
  aout_dreloc_tab[aout_dreloc_count].r_extern = ext;
  aout_dreloc_tab[aout_dreloc_count].r_pad = 0;
  ++aout_dreloc_count;
}

static void aout_finish (void)
{
  while (aout_text_size & 3)
    aout_text_byte (0x90);
  while (aout_data_size & 3)
    aout_data_byte (0);
  aout_size = (sizeof (struct exec) + aout_text_size + aout_data_size
               + aout_treloc_count * sizeof (struct relocation_info)
               + aout_dreloc_count * sizeof (struct relocation_info)
               + aout_sym_count * sizeof (aout_sym_tab[0])
               + aout_str_size);
}


static void aout_write (void)
{
  struct exec ao;

  N_SET_MAGIC (ao, OMAGIC);
  N_SET_MACHTYPE (ao, 0);
  N_SET_FLAGS (ao, 0);
  ao.a_text = aout_text_size;
  ao.a_data = aout_data_size;
  ao.a_bss = 0;
  ao.a_syms = aout_sym_count * sizeof (aout_sym_tab[0]);
  ao.a_entry = 0;
  ao.a_trsize = aout_treloc_count * sizeof (struct relocation_info);
  ao.a_drsize = aout_dreloc_count * sizeof (struct relocation_info);;
  fwrite (&ao, 1, sizeof (ao), out_file);
  fwrite (aout_text, aout_text_size, 1, out_file);
  fwrite (aout_data, aout_data_size, 1, out_file);
  fwrite (aout_treloc_tab, aout_treloc_count, sizeof (struct relocation_info), out_file);
  fwrite (aout_dreloc_tab, aout_dreloc_count, sizeof (struct relocation_info), out_file);
  fwrite (aout_sym_tab, aout_sym_count, sizeof (aout_sym_tab[0]), out_file);
  *(dword *)aout_str_tab = aout_str_size;
  fwrite (aout_str_tab, 1, aout_str_size, out_file);
}


static void write_a_lazyldr (const char *resolver_sym, const char *mod_name)
{
    char  tmp[257];
    dword *ptr_hmod_fixup, *ptr_name_fixup;
    const char *p;
    int len;
    static char *done = NULL;
    static size_t done_len = 0;
    if (done && strstr (done, resolver_sym))
      return;
    len = strlen (resolver_sym);
    done = xrealloc (done, done_len + len + 2);
    memcpy (done + done_len, resolver_sym, len);
    len += done_len;
    memcpy (done + len, "\b", 2);
    done_len = len + 1;

    /* Write module resolver object
        .text
        .globl ___lazyimp_doscalls_resolver
    ___lazyimp_doscalls_resolver:
        push    name
        push    hmod
        jmp     ___lazyimp_resolver
    name:
        .asciiz "DOSCALLS"
        .byte 0

        .data
    hmod:
        .long   0
    */
    aout_init ();
    aout_sym (resolver_sym, N_TEXT|N_EXT, 0, 0, aout_text_size); /* .globl DosRead */
    aout_text_byte (0x68);              /* push name */
    aout_treloc (aout_text_size, N_TEXT, 0, 2, 0);
    ptr_name_fixup = (dword *)&aout_text[aout_text_size];
    aout_text_dword (0);

    aout_text_byte (0x68);              /* push pfn */
    aout_treloc (aout_text_size, N_DATA, 0, 2, 0);
    ptr_hmod_fixup = (dword *)&aout_text[aout_text_size];
    aout_text_dword (0);

    aout_text_byte (0xe9);            /* jmp ___lazyimp_resolver */
    aout_treloc (aout_text_size, aout_sym ("___lazyimp_resolver", N_EXT, 0, 0, 0), 1, 2, 1);
    aout_text_dword (0 - (aout_text_size + 4));

    *ptr_name_fixup = aout_text_size;   /* name: .ascii */
    strcpy (tmp, mod_name);
    strupr (tmp);
    for (p = tmp; *p; p++)
      aout_text_byte(*p);
    aout_text_byte(0);

    while (aout_text_size & 3)
        aout_text_byte (0xcc);

    *ptr_hmod_fixup = aout_text_size;   /* hmod: .long 0 */
    aout_data_dword (0);


    aout_finish ();
    sprintf (tmp, "LAZYMOD#%ld", seq_no);
    write_ar (tmp, aout_size);
    aout_write ();
    finish_ar ();
    seq_no++;
    if (ferror (out_file))
      write_error (out_fname);
}


static void write_a_import (const char *func_name, const char *mod_name,
                            int ordinal, const char *proc_name)
{
  char tmp1[256], tmp2[257], tmp3[1024];
  int sym_mcount, sym_entry, sym_import;
  dword fixup_mcount, fixup_import;

  /* #425: Underscore is gone. (was "_%s") */
  sprintf (tmp2, "%s", func_name);
  /*
   * Lazily resolved dynamic import.
   */
  if (opt_lazy && strncmp (func_name, "_16_", 4) != 0)
    {
      int   off_name, off_resolver;
      dword *fixup_ptr1, *fixup_ptr2;
      char *p;

      /* create module resolver object first. */
      sprintf (tmp1, "___lazyldr_%s_resolver", mod_name);
      write_a_lazyldr (tmp1, mod_name);

      /* Write import object
          .data
      pfn:
          .long   resolve
          .text
          .globl DosRead
      DosRead:
          jmp     *(pfn)
      name:
          .asciiz "DosRead"
          .byte 0
      resolve:
          push    name
          push    pfn
          jmp     ___lazyimp_doscalls_resolver
      */
      aout_init ();
      aout_sym (tmp2, N_TEXT|N_EXT, 0, 0, aout_text_size); /* .globl DosRead */
      strlwr (tmp1);
      aout_text_byte (0xff);            /* jmp *(pfn) */
      aout_text_byte (0x25);
      aout_treloc (aout_text_size, N_DATA, 0, 2, 0);
      fixup_ptr1 = (dword *)&aout_text[aout_text_size];
      aout_text_dword (aout_data_size);

      if (proc_name)
        {
          off_name = aout_text_size;        /* name: .ascii */
          for (p = tmp2; *p; p++)
            aout_text_byte(*p);
          aout_text_byte(0);
          aout_text_byte (0xcc);
        }

      off_resolver = aout_text_size;    /* resolve: push name / ordinal */
      aout_text_byte (0x68);
      if (proc_name)
        {
          aout_treloc (aout_text_size, N_TEXT, 0, 2, 0);
          aout_text_dword (off_name);
        }
      else
        aout_text_dword (ordinal);

      aout_text_byte (0x68);            /* resolve: push pfn */
      aout_treloc (aout_text_size, N_DATA, 0, 2, 0);
      fixup_ptr2 = (dword *)&aout_text[aout_text_size];
      aout_text_dword (0);

      aout_text_byte (0xe9);            /* jmp ___lazyimp_????_resolver */
      aout_treloc (aout_text_size, aout_sym (tmp1, N_EXT, 0, 0, 0), 1, 2, 1);
      aout_text_dword (0 - (aout_text_size + 4));
      while (aout_text_size & 3)
          aout_text_byte (0xcc);

      *fixup_ptr1 = aout_text_size;
      *fixup_ptr2 = aout_text_size;
      aout_dreloc (aout_data_size, N_TEXT, 0, 2, 0); /* pfn: .long resolve */
      aout_data_dword (off_resolver);
      sprintf (tmp1, "LAZY#%ld#%x", seq_no, hash (func_name));
    }
  /*
   * Static load time import.
   */
  else
    {
      aout_init ();
      if (profile_flag && strncmp (func_name, "_16_", 4) != 0)
        {
          sym_entry = aout_sym (tmp2, N_TEXT|N_EXT, 0, 0, aout_text_size);
          sym_mcount = aout_sym ("__mcount", N_EXT, 0, 0, 0);

          /* Use, say, "_$U_DosRead" for "DosRead" to import the
             non-profiled function. */
          sprintf (tmp2, "__$U_%s", func_name);
          sym_import = aout_sym (tmp2, N_EXT, 0, 0, 0);

          aout_text_byte (0x55);    /* push ebp */
          aout_text_byte (0x89);    /* mov ebp, esp */
          aout_text_byte (0xe5);
          aout_text_byte (0xe8);    /* call _mcount*/
          fixup_mcount = aout_text_size;
          aout_text_dword (0 - (aout_text_size + 4));
          aout_text_byte (0x5d);    /* pop ebp */
          aout_text_byte (0xe9);    /* jmp _$U_DosRead*/
          fixup_import = aout_text_size;
          aout_text_dword (0 - (aout_text_size + 4));

          aout_treloc (fixup_mcount, sym_mcount, 1, 2, 1);
          aout_treloc (fixup_import, sym_import, 1, 2, 1);
        }
      if (proc_name == NULL)
        sprintf (tmp3, "%s=%s.%d", tmp2, mod_name, ordinal);
      else
        sprintf (tmp3, "%s=%s.%s", tmp2, mod_name, proc_name);
      aout_sym (tmp2, N_IMP1|N_EXT, 0, 0, 0);
      aout_sym (tmp3, N_IMP2|N_EXT, 0, 0, 0);
      sprintf (tmp1, "IMPORT#%ld#%x", seq_no, hash (func_name));
    }

  aout_finish ();
  write_ar (tmp1, aout_size);
  aout_write ();
  finish_ar ();
  seq_no++;
  if (ferror (out_file))
    write_error (out_fname);
}


static void read_lib (const char *fname)
{
  FILE *inp_file;
  int n, i, next, more, impure_warned, ord_flag;
  unsigned char *buf;
#pragma pack(1)
  struct record
    {
      unsigned char type;
      unsigned short length;
    } record, *rec_ptr;
#pragma pack()
  unsigned char func_name[256];
  unsigned char mod_name[256];
  unsigned char proc_name[256];
  unsigned char theadr_name[256];
  int ordinal;
  long pos, size;
  int page_size;

  if (mode == M_LIB_TO_IMP)
    fprintf (out_file, "; -------- %s --------\n", fname);
  if (ferror (out_file))
    write_error (out_fname);
  inp_file = fopen (fname, "rb");
  if (inp_file == NULL)
    error ("Cannot open input file `%s'", fname);
  if (fread (&record, sizeof (record), 1, inp_file) != 1)
    goto read_error;
  if (record.type != LIBHDR || record.length < 5)
    error ("`%s' is not a library file", fname);
  page_size = record.length + 3;
  if (fread (&pos, sizeof (pos), 1, inp_file) != 1)
    goto read_error;
  if (fseek (inp_file, 0L, SEEK_END) != 0)
    goto read_error;
  size = ftell (inp_file);
  if (pos < size)
    size = pos;
  buf = xmalloc (size);
  if (fseek (inp_file, 0L, SEEK_SET) != 0)
    goto read_error;
  size = fread (buf, 1, size, inp_file);
  if (size == 0 || ferror (inp_file))
    goto read_error;
  i = 0; more = TRUE; impure_warned = FALSE; theadr_name[0] = 0;
  while (more)
    {
      rec_ptr = (struct record *)(buf + i);
      i += sizeof (struct record);
      if (i > size) goto bad;
      next = i + rec_ptr->length;
      if (next > size) goto bad;
      switch (rec_ptr->type)
        {
        case MODEND:
        case MODEND|REC32:
          if ((next & (page_size-1)) != 0)
            next = (next | (page_size-1)) + 1;
          break;

        case THEADR:
          n = buf[i++];
          if (i + n > next) goto bad;
          memcpy (theadr_name, buf+i, n);
          theadr_name[n] = 0;
          impure_warned = FALSE;
          break;

        case COMENT:
          if (record.length >= 11 && buf[i+0] == 0x00 && buf[i+1] == 0xa0 &&
              buf[i+2] == 0x01)
            {
              ord_flag = buf[i+3];
              i += 4;
              if (i + 1 > next) goto bad;
              n = buf[i++];
              if (i + n > next) goto bad;
              memcpy (func_name, buf+i, n);
              func_name[n] = 0;
              i += n;
              if (i + 1 > next) goto bad;
              n = buf[i++];
              if (i + n > next) goto bad;
              memcpy (mod_name, buf+i, n);
              mod_name[n] = 0;
              i += n;

              if (ord_flag == 0)
                {
                  ordinal = -1;
                  if (i + 1 > next) goto bad;
                  n = buf[i++];
                  if (i + n > next) goto bad;
                  if (n == 0)
                    strcpy ((char *)proc_name, (char *)func_name);
                  else
                    {
                      memcpy (proc_name, buf+i, n);
                      proc_name[n] = 0;
                      i += n;
                    }
                }
              else
                {
                  if (i + 2 > next) goto bad;
                  ordinal = *(unsigned short *)(buf + i);
                  i += 2;
                  proc_name[0] = 0;
                }
              ++i;              /* Skip checksum */
              if (i != next) goto bad;
              switch (mode)
                {
                case M_LIB_TO_IMP:
                  if (ordinal != -1)
                    sprintf ((char *)proc_name, "%3d", ordinal);
                  if (strncmp ((char *)func_name, "_16_", 4) != 0)
                    fprintf (out_file, "%-23s %-8s %s ?\n",
                             func_name, mod_name, proc_name);
                  else
                    fprintf (out_file, "%-23s %-8s %s F\n",
                             func_name+4, mod_name, proc_name);
                  if (ferror (out_file))
                    write_error (out_fname);
                  break;
                case M_LIB_TO_A:
                  if (ordinal == -1)
                    write_a_import ((char *)func_name, (char *)mod_name, ordinal, (char *)proc_name);
                  else
                    write_a_import ((char *)func_name, (char *)mod_name, ordinal, NULL);
                  break;
                default:
                  abort ();
                }
            }
          break;

        case EXTDEF:
        case PUBDEF:
        case PUBDEF|REC32:
        case SEGDEF:
        case SEGDEF|REC32:
        case COMDEF:
        case COMDAT:
        case COMDAT|REC32:
          if (!opt_q && !impure_warned)
            {
              impure_warned = TRUE;
              information ("%s (%s) is not a pure import library",
                           fname, theadr_name);
            }
          break;

        case LIBEND:
          more = FALSE;
          break;
        }
      i = next;
    }
  free (buf);
  fclose (inp_file);
  return;

read_error:
  error ("Read error on file `%s'", fname);

bad:
  error ("Malformed import library file `%s'", fname);
}


static void create_output_file (int bin)
{
  out_file = fopen (out_fname, (bin ? "wb" : "wt"));
  if (out_file == NULL)
    error ("Cannot open output file `%s'", out_fname);
  if (!bin)
    fprintf (out_file, ";\n; %s (created by emximp)\n;\n", out_fname);
}


static void close_output_file (void)
{
  if (fflush (out_file) != 0)
    error ("Write error on output file `%s'", out_fname);
  if (fclose (out_file) != 0)
      error ("Cannot close output file `%s'", out_fname);
}


static void init_archive (void)
{
  static char ar_magic[SARMAG+1] = ARMAG;

  fwrite (ar_magic, 1, SARMAG, out_file);
}


static int md_export (struct _md *md, const _md_stmt *stmt, _md_token token,
                      void *arg)
{
  const char *internal;

  switch (token)
    {
    case _MD_LIBRARY:
      module_name = xstrdup (stmt->library.name);
      break;
    case _MD_EXPORTS:
      if (module_name == NULL)
        error ("No module name given in module definition file");
      if (stmt->export.internalname[0] != 0)
        internal = stmt->export.internalname;
      else
        internal = stmt->export.entryname;
      switch (mode)
        {
        case M_DEF_TO_IMP:
          if (stmt->export.flags & _MDEP_ORDINAL)
            fprintf (out_file, "%-23s %-8s %3u ?\n",
                     stmt->export.entryname, module_name,
                     (unsigned)stmt->export.ordinal);
          else
            fprintf (out_file, "%-23s %-8s %-23s ?\n",
                     stmt->export.entryname, module_name, internal);
          if (ferror (out_file))
            write_error (out_fname);
          break;
        case M_DEF_TO_A:
          if (stmt->export.flags & _MDEP_ORDINAL)
            write_a_import (stmt->export.entryname, module_name,
                            stmt->export.ordinal, NULL);
          else
            write_a_import (stmt->export.entryname, module_name,
                            0, internal);
          break;
        case M_DEF_TO_LIB:
          write_lib_import (stmt->export.entryname, module_name,
                            stmt->export.ordinal, internal);
          break;
        default:
          abort ();
        }
      break;
    case _MD_parseerror:
      error ("%s (line %ld of %s)", _md_errmsg (stmt->error.code),
             _md_get_linenumber (md), (const char *)arg);
      break;
    default:
      break;
    }
  return 0;
}


static void read_def (const char *fname)
{
  struct _md *md;

  module_name = NULL;
  md = _md_open (fname);
  if (md == NULL)
    error ("Cannot open input file `%s'", fname);
  if (mode == M_DEF_TO_IMP)
    {
      fprintf (out_file, "; -------- %s --------\n", fname);
      if (ferror (out_file))
        write_error (out_fname);
    }
  _md_next_token (md);
  _md_parse (md, md_export, (void *)fname);
  _md_close (md);
}

/**
 * Reads a LX nametable.
 */
static void dll_read_names(FILE *phFile, off_t off, int cb, int fResident, int fOrdinal0Only, const char *pszFilename)
{
    char            szName[256];
    unsigned        cchName;
    unsigned        iOrd = 0;

    if (fseek(phFile, off, SEEK_SET))
        error("`%s': Bad LX format\n", pszFilename);

    if (fResident)
        module_name = NULL;
    else
        description = NULL;

    /* The names doesn't have to be in order, so we'll have to read
     * until we find the 0 ordinal if that's what we require. */
    while (cb > 0)
    {
        cchName = (unsigned)fgetc(phFile);
        if (!cchName || cb < cchName + 2)
            break;
        iOrd = 0;
        if (    fread(szName, 1, cchName, phFile) != cchName
            ||  fread(&iOrd, 1, 2, phFile) != 2)
            error("`%s': Bad LX format\n", pszFilename);
        szName[cchName] = '\0';
        cb -= 1 + cchName + 2;

        if (iOrd == 0)
        {
            if (fResident)
                module_name = xstrdup(szName);
            else
                description = xstrdup(szName);
            if (fOrdinal0Only)
                return;
        }
        else if (!fOrdinal0Only)
        {
            switch (mode)
            {
                case M_DLL_TO_IMP:
                    if (!fResident)
                        fprintf(out_file, "%-23s %-8s %3u ?\n",   szName, module_name, iOrd);
                    else
                        fprintf(out_file, "%-23s %-8s %-23s ?\n", szName, module_name, szName);
                    if (ferror(out_file))
                        write_error(out_fname);
                    break;
                case M_DLL_TO_A:
                    if (!fResident)
                        write_a_import(szName, module_name, iOrd, NULL);
                    else
                        write_a_import(szName, module_name, 0, szName);
                    break;
                case M_DLL_TO_LIB:
                    if (!fResident)
                        write_lib_import(szName, module_name, iOrd, NULL);
                    else
                        write_lib_import(szName, module_name, 0, szName);
                    break;
                case M_DLL_TO_DEF:
                    if (!fResident)
                        fprintf(out_file, "  \"%s\" @%d\n", szName, iOrd);
                    else
                        fprintf(out_file, "  \"%s\"\n", szName);
                    if (ferror(out_file))
                        write_error(out_fname);
                    break;
                default:
                    abort ();
            }
        }
    } /* read restable loop */
}

/** reads a dll */
static void read_dll(const char *pszFilename)
{
#pragma pack(1)
#define EMAGIC 0x5A4D
    struct
    {
        unsigned short e_magic;
        unsigned short e_cblp;
        unsigned short e_cp;
        unsigned short e_crlc;
        unsigned short e_cparhdr;
        unsigned short e_minalloc;
        unsigned short e_maxalloc;
        unsigned short e_ss;
        unsigned short e_sp;
        unsigned short e_csum;
        unsigned short e_ip;
        unsigned short e_cs;
        unsigned short e_lfarlc;
        unsigned short e_ovno;
        unsigned long  e_sym_tab;
        unsigned short e_flags;
        unsigned short e_res;
        unsigned short e_oemid;
        unsigned short e_oeminfo;
        unsigned short e_res2[10];
        unsigned long  e_lfanew;
    }           doshdr;

#define E32MAGIC    0x584c /* LX */
#define E32MODDLL        0x08000L
#define E32MODPROTDLL    0x18000L
#define E32MODMASK       0x38000L
#define E32LIBINIT       0x0004L
#define E32LIBTERM       0x40000000L
    struct
    {
        unsigned char   e32_magic[2];
        unsigned char   e32_border;
        unsigned char   e32_worder;
        unsigned long   e32_level;
        unsigned short  e32_cpu;
        unsigned short  e32_os;
        unsigned long   e32_ver;
        unsigned long   e32_mflags;
        unsigned long   e32_mpages;
        unsigned long   e32_startobj;
        unsigned long   e32_eip;
        unsigned long   e32_stackobj;
        unsigned long   e32_esp;
        unsigned long   e32_pagesize;
        unsigned long   e32_pageshift;
        unsigned long   e32_fixupsize;
        unsigned long   e32_fixupsum;
        unsigned long   e32_ldrsize;
        unsigned long   e32_ldrsum;
        unsigned long   e32_objtab;
        unsigned long   e32_objcnt;
        unsigned long   e32_objmap;
        unsigned long   e32_itermap;
        unsigned long   e32_rsrctab;
        unsigned long   e32_rsrccnt;
        unsigned long   e32_restab;
        unsigned long   e32_enttab;
        unsigned long   e32_dirtab;
        unsigned long   e32_dircnt;
        unsigned long   e32_fpagetab;
        unsigned long   e32_frectab;
        unsigned long   e32_impmod;
        unsigned long   e32_impmodcnt;
        unsigned long   e32_impproc;
        unsigned long   e32_pagesum;
        unsigned long   e32_datapage;
        unsigned long   e32_preload;
        unsigned long   e32_nrestab;
        unsigned long   e32_cbnrestab;
        unsigned long   e32_nressum;
        unsigned long   e32_autodata;
        unsigned long   e32_debuginfo;
        unsigned long   e32_debuglen;
        unsigned long   e32_instpreload;
        unsigned long   e32_instdemand;
        unsigned long   e32_heapsize;
        unsigned long   e32_stacksize;
        unsigned char   e32_res3[20];
    }           os2hdr;
#pragma pack()
    off_t       offLX;
    FILE       *phFile = fopen(pszFilename, "rb");

    if (!phFile)
        error("Cannot open input file `%s'", pszFilename);

    /* stub header */
    if (fread(&doshdr, 1, sizeof(doshdr), phFile) != sizeof(doshdr))
        error("Error read error while reading `%s'\n", pszFilename);
    if (doshdr.e_magic != EMAGIC && doshdr.e_magic != E32MAGIC)
        error("`%s' is not a LX DLL\n", pszFilename);
    offLX = doshdr.e_magic == EMAGIC ? doshdr.e_lfanew : 0;

    /* LX header */
    if (fseek(phFile, offLX, SEEK_SET))
        error("Bad LX .DLL `%s'\n", pszFilename);
    if (fread(&os2hdr, 1, sizeof(os2hdr), phFile) != sizeof(os2hdr))
        error("Error read error while reading `%s'\n", pszFilename);
    if (*(unsigned short*)os2hdr.e32_magic != E32MAGIC || !os2hdr.e32_restab)
        error("`%s': Bad LX format\n", pszFilename);
    if (   (os2hdr.e32_mflags & E32MODMASK) != E32MODDLL
        && (os2hdr.e32_mflags & E32MODMASK) != E32MODPROTDLL)
        error("`%s': Not a LX DLL\n", pszFilename);

    /* Read the tables. */
    if (mode == M_DLL_TO_DEF && !first_module)
    {
        dll_read_names(phFile, os2hdr.e32_restab + offLX, 0x7ffffff, 1, 1, pszFilename);
        dll_read_names(phFile, os2hdr.e32_nrestab, os2hdr.e32_cbnrestab, 0, 1, pszFilename);

        first_module = xstrdup(module_name);
        fprintf(out_file, "LIBRARY %s", first_module);
        fprintf(out_file, (os2hdr.e32_mflags & E32LIBINIT) ? " INITINSTANCE" : " INITGLOBAL");
        fprintf(out_file, (os2hdr.e32_mflags & E32LIBTERM) ? " TERMINSTANCE" : " TERMGLOBAL");
        fprintf(out_file, "\n");
        if (description)
            fprintf(out_file, "DESCRIPTION '%s'\n", description);
        fprintf(out_file, "EXPORTS\n");
        if (ferror(out_file))
          write_error(out_fname);
    }
    dll_read_names(phFile, os2hdr.e32_restab + offLX, 0x7ffffff, 1, 0, pszFilename);
    if (os2hdr.e32_nrestab)
        dll_read_names(phFile, os2hdr.e32_nrestab, os2hdr.e32_cbnrestab, 0, 0, pszFilename);

    fclose(phFile);
}


int main (int argc, char *argv[])
{
  int i, c;
  int imp_count, lib_count, def_count, dll_count, page_size;
  struct predef *pp1;
  char *q, *ext, *opt_o;

  _response (&argc, &argv);
  predefs = NULL; out_base = NULL; as_name = NULL; pipe_flag = FALSE;
  profile_flag = FALSE; page_size = 16;
  opt_b = FALSE; opt_q = FALSE; opt_s = FALSE; base_len = 0; opt_o = NULL;
  opterr = 0;
  optind = 1;
  while ((c = getopt (argc, argv, "a::b:lmo:p:qsP:")) != EOF)
    {
      switch (c)
        {
        case 'a':
          as_name = (optarg != NULL ? optarg : "as");
          pipe_flag = (_osmode != DOS_MODE);
          break;
        case 'b':
          opt_b = TRUE;
          base_len = strtol (optarg, &q, 10);
          if (base_len > 1 && *q == 0)
            out_base = NULL;
          else
            {
              out_base = optarg;
              base_len = 0;
            }
          break;
        case 'l':
          opt_lazy = TRUE;
          break;
        case 'm':
          profile_flag = TRUE;
          break;
        case 'o':
          if (opt_o != NULL)
            usage ();
          opt_o = optarg;
          break;
        case 'p':
          pp1 = xmalloc (sizeof (struct predef));
          pp1->name = xstrdup (optarg);
          pp1->next = predefs;
          predefs = pp1;
          break;
        case 'q':
          opt_q = TRUE;
          break;
        case 's':
          opt_s = TRUE;
          break;
        default:
          error ("Invalid option");
        }
    }
  imp_count = 0; lib_count = 0; def_count = 0; dll_count = 0;
  for (i = optind; i < argc; ++i)
    {
      ext = _getext (argv[i]);
      if (ext != NULL && stricmp (ext, ".lib") == 0)
        ++lib_count;
      else if (ext != NULL && stricmp (ext, ".imp") == 0)
        ++imp_count;
      else if (ext != NULL && stricmp (ext, ".def") == 0)
        ++def_count;
      else if (ext != NULL && stricmp (ext, ".dll") == 0)
        ++dll_count;
      else
        error ("Input file `%s' has unknown file name extension", argv[i]);
    }
  if (imp_count == 0 && lib_count == 0 && def_count == 0 && dll_count == 0)
    usage ();
  if ((imp_count != 0) + (lib_count != 0) + (def_count != 0) + (dll_count != 0) > 1)
    error ("More than one type of input files");
  if (opt_o == NULL)
    {
      if (lib_count != 0)
        error ("Cannot convert .lib files to %s files",
               (as_name == NULL ? ".s" : ".o"));
      if (def_count != 0)
        error ("Cannot convert .def files to %s files",
               (as_name == NULL ? ".s" : ".o"));
      if (dll_count != 0)
        error ("Cannot convert .dll files to %s files",
               (as_name == NULL ? ".s" : ".o"));
      mode = M_IMP_TO_S;
    }
  else
    {
      ext = _getext (opt_o);
      if (ext != NULL && stricmp (ext, ".imp") == 0)
        {
          if (imp_count != 0)
            error ("Cannot convert .imp files to .imp file");
          if (dll_count != 0)
            mode = M_DLL_TO_IMP;
          else if (lib_count != 0)
            mode = M_LIB_TO_IMP;
          else
            mode = M_DEF_TO_IMP;
        }
      else if (ext != NULL && stricmp (ext, ".a") == 0)
        {
          if (dll_count != 0)
            mode = M_DLL_TO_A;
          else if (def_count != 0)
            mode = M_DEF_TO_A;
          else if (imp_count != 0)
            mode = M_IMP_TO_A;
          else
            mode = M_LIB_TO_A;
        }
      else if (ext != NULL && stricmp (ext, ".def") == 0)
        {
          if (def_count != 0)
            error ("Cannot convert .def files to .def file");
          if (lib_count != 0)
            error ("Cannot convert .lib files to .def file");
          if (dll_count != 0)
            mode = M_DLL_TO_DEF;
          else
            mode = M_IMP_TO_DEF;
        }
      else if (ext != NULL && stricmp (ext, ".lib") == 0)
        {
          if (lib_count != 0)
            error ("Cannot convert .lib files to .lib file");
          if (dll_count != 0)
            mode = M_DLL_TO_LIB;
          else if (def_count != 0)
            mode = M_DEF_TO_LIB;
          else
            mode = M_IMP_TO_LIB;
          if (predefs != NULL)
            {
              if (predefs->next != NULL)
                usage ();
              page_size = strtol (predefs->name, &q, 10);
              if (page_size < 1 || *q != 0)
                usage ();
              predefs = NULL;   /* See below */
            }
        }
      else
        error ("File name extension of output file not supported");
      _strncpy (out_fname, opt_o, sizeof (out_fname));
    }
  if (mode != M_IMP_TO_S)
    if (as_name != NULL || opt_b || opt_s || predefs != NULL)
      usage ();
  if (profile_flag && mode != M_DEF_TO_A && mode != M_IMP_TO_A
      && mode != M_LIB_TO_A && mode != M_DEF_TO_A)
    usage ();
  switch (mode)
    {
    case M_LIB_TO_IMP:
      create_output_file (FALSE);
      for (i = optind; i < argc; ++i)
        read_lib (argv[i]);
      close_output_file ();
      break;
    case M_LIB_TO_A:
      create_output_file (TRUE);
      init_archive ();
      for (i = optind; i < argc; ++i)
        read_lib (argv[i]);
      close_output_file ();
      break;
    case M_IMP_TO_S:
      for (i = optind; i < argc; ++i)
        read_imp (argv[i]);
      break;
    case M_IMP_TO_DEF:
      create_output_file (FALSE);
      for (i = optind; i < argc; ++i)
        read_imp (argv[i]);
      close_output_file ();
      break;
    case M_IMP_TO_LIB:
      out_lib = omflib_create (out_fname, page_size, lib_errmsg);
      if (out_lib == NULL)
        lib_error ();
      if (omflib_header (out_lib, lib_errmsg) != 0)
        lib_error ();
      for (i = optind; i < argc; ++i)
        read_imp (argv[i]);
      if (omflib_finish (out_lib, lib_errmsg) != 0
          || omflib_close (out_lib, lib_errmsg) != 0)
        lib_error ();
      break;
    case M_IMP_TO_A:
      create_output_file (TRUE);
      init_archive ();
      for (i = optind; i < argc; ++i)
        read_imp (argv[i]);
      close_output_file ();
      break;
    case M_DEF_TO_A:
      create_output_file (TRUE);
      init_archive ();
      for (i = optind; i < argc; ++i)
        read_def (argv[i]);
      close_output_file ();
      break;
    case M_DEF_TO_IMP:
      create_output_file (FALSE);
      for (i = optind; i < argc; ++i)
        read_def (argv[i]);
      close_output_file ();
      break;
    case M_DEF_TO_LIB:
      out_lib = omflib_create (out_fname, page_size, lib_errmsg);
      if (out_lib == NULL)
        lib_error ();
      if (omflib_header (out_lib, lib_errmsg) != 0)
        lib_error ();
      for (i = optind; i < argc; ++i)
        read_def (argv[i]);
      if (omflib_finish (out_lib, lib_errmsg) != 0
          || omflib_close (out_lib, lib_errmsg) != 0)
        lib_error ();
      break;
    case M_DLL_TO_A:
      create_output_file (TRUE);
      init_archive ();
      for (i = optind; i < argc; ++i)
        read_dll (argv[i]);
      close_output_file ();
      break;
    case M_DLL_TO_IMP:
      create_output_file (FALSE);
      for (i = optind; i < argc; ++i)
        read_dll (argv[i]);
      close_output_file ();
      break;
    case M_DLL_TO_LIB:
      out_lib = omflib_create (out_fname, page_size, lib_errmsg);
      if (out_lib == NULL)
        lib_error ();
      if (omflib_header (out_lib, lib_errmsg) != 0)
        lib_error ();
      for (i = optind; i < argc; ++i)
        read_dll (argv[i]);
      if (omflib_finish (out_lib, lib_errmsg) != 0
          || omflib_close (out_lib, lib_errmsg) != 0)
        lib_error ();
      break;

    case M_DLL_TO_DEF:
      create_output_file (FALSE);
      for (i = optind; i < argc; ++i)
        read_dll (argv[i]);
      close_output_file ();
      break;

    default:
      usage ();
    }
  return (warnings == 0 ? 0 : 1);
}
