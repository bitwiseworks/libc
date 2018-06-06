/* emxexp.c -- Create export definitions from .o and .obj files
   Copyright (c) 1993-1998 Eberhard Mattes

This file is part of emxexp.

emxexp is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxexp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxexp; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <ar.h>
#include <sys/omflib.h>
#include <a_out.h>
#include "defs.h"
#include "demangle.h"

struct bss_list
{
  char *name;
  struct bss_list *next;
};

/* if != 0 we're supposed to add ordinals to the symbols. */
static int ordinal = 0;
/* if set the ordinals is to be sequentially assigned with no
   conveniency gaps or anything. */
static int ordinal_seq = FALSE;
static int noname_flag = FALSE;
static int bss_flag = FALSE;
static int weak_flag = FALSE;
/* When clear we'll do sorting and handle make sure there no duplicate weak
   names and such. If set we'll just dump the publics and comdefs as we
   used to do. */
static int legacy_mode = FALSE;

static FILE *out_file;
static const char *inp_fname;
static const char *mod_name;
static int new_mod = FALSE;
static int last_dem = FALSE;
static struct bss_list *bss_list = NULL;

/* OMF globals */
static struct segdef
{
    /* pointer to local name */
    const char *name;
}              *segdefs;
static unsigned num_segdefs;
static char **  lnames;
static unsigned num_lnames;

/* Aout globals */
static char *aout_segnames[] =
{
    NULL,     NULL,     /*  0 N_UNDF */
    NULL,     NULL,     /*  2 N_ABS */
    "TEXT32", "TEXT32", /*  4 N_TEXT */
    "DATA32", "DATA32", /*  6 N_DATA */
    "BSS32",  "BSS32",  /*  8 N_BSS */
    NULL,     NULL,     /*  a */
    NULL,     NULL,     /*  c,  d N_WEAKU */
    NULL,     "TEXT32", /*  d N_WEAKA,  f N_WEAKT */
    "DATA32", "BSS32",  /* 10 N_WEAKD, 11 N_WEAKB */
    NULL, NULL,  NULL, NULL, NULL
};

/* Per segment name sorting
   Symbols are orginized in two constructs. One is a global symbol name
   string pool kind thing. The other is per segment.

   Weak symbols and communial data may be overridden. In those cases we'll
   unlink the symbol and reinstert it to ensure that we don't get it in the
   wrong segment.
   */

/* module in which a symbol is defined. */
struct new_module
{
    char *              name;           /* module name. */
    struct new_module * next;           /* next module in the chain. */
};

/* LIFO of modules. (only so we can free it)
   The head module is the current one. */
struct new_module      *new_modules;


/* one symbol */
struct new_symbol
{
    char *              name;           /* Symbol name from heap. */
    struct new_symbol  *nexthash;       /* Next hash table entry. */
    struct new_symbol  *next, *prev;    /* Per segment chain. */
    struct new_module  *mod;            /* Module in which the symbol is defined. */
    unsigned            fweak:1;        /* Weak symbol. */
    unsigned            fcomm:1;        /* Communial data. */
    unsigned            fdummy:1;       /* Dummy node, the one which is first & last
                                           in the lists to save use from any
                                           unpleasant list head/tail thinking. */
};

/* List of segments. */
static struct new_symgrp
{
    char *              name;           /* segment name */
    struct new_symbol  *head;           /* head of list. Always starts with a dummy node. */
    struct new_symbol  *tail;           /* tail of list. Always starts with a dummy node. */
    struct new_symgrp  *next;           /* next segement. */
}                 * new_symgrps;

/* symbol string hash table */
struct new_symbol *  new_hashtable[211];



static void error (const char *fmt, ...) NORETURN2;
static void usage (void) NORETURN2;
static void bad_omf (void) NORETURN2;


static void error (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emxexp: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
  exit (2);
}


/* Allocate N bytes of memory.  Quit on failure.  This function is
   used like malloc(), but we don't have to check the return value. */

static void *xmalloc (size_t n)
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

static void *xrealloc (void *ptr, size_t n)
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

static char *xstrdup (const char *s)
{
  char *p;

  p = xmalloc (strlen (s) + 1);
  strcpy (p, s);
  return p;
}



/* How to call this program. */

static void usage (void)
{
  fputs ("emxexp " VERSION INNOTEK_VERSION " -- Copyright (c) 1993-1995 by Eberhard Mattes\n\n"
         "Usage: emxexp [-n] [-u] [-o[<ordinal>] <input_file>...\n\n"
         "Options:\n"
         "  -n          Output NONAME keyword for each exported symbol\n"
         "  -o          Output ordinal numbers, starting at 1\n"
         "  -o<ordinal> Output ordinal numbers, starting at <ordinal>\n"
         "  -u          Also export uninitialized variables - only legacy mode\n"
         "  -w          Allow export of weak symbols - only legacy mode\n"
         "  -l          Legacy mode - disables sorting and weak handling\n",
         stderr);
  exit (1);
}


/* New export function. This will filter out duplicated BSS32 and weak
   symbols and insert the symbols into arrays according to segment
   value. new_print() will be called when all the input files are
   processed and do the actual printing and ordinal assignments. */

static void new_export (const char *name, const char *segname, int fweak, int fcomm)
{
  struct new_symbol  *psym;
  unsigned            uhash;
  const char *        psz;

  /* compute hash */
  for (uhash = 0, psz = name; *psz; *psz++)
    uhash = uhash * 65599 + *psz;
  uhash %= sizeof(new_hashtable) / sizeof(new_hashtable[0]);

  /* look for it */
  for (psym = new_hashtable[uhash]; psym; psym = psym->nexthash)
    if (!strcmp(psym->name, name))
        break;

  /* new */
  if (!psym)
    {
      /* create new symbol node */
      psym = xmalloc(sizeof(*psym));
      psym->name = xstrdup(name);

      /* insert into hash */
      psym->nexthash = new_hashtable[uhash];
      new_hashtable[uhash] = psym;
    }
  else
    {/* existing. look for overrided symbols (weak/comm) and ignore duplicates. */
      if (fweak || fcomm)
        psym = NULL;                  /* just skip it - did I forget something...? */
      else
        {
          if (psym->fcomm || psym->fweak)
            { /* unlink the symbol. */
              psym->prev->next = psym->next;
              psym->next->prev = psym->prev;
            }
          else
            psym = NULL;
        }
    }

  /* insert the correct list */
  if (psym)
    {
      struct new_symgrp *psymgrp;

      /* Set symbol data */
      psym->fcomm = fcomm;
      psym->fweak = fweak;
      psym->fdummy = 0;
      psym->mod = new_modules;


      /* find the segment */
      for (psymgrp = new_symgrps; psymgrp; psymgrp = psymgrp->next)
        if (   psymgrp->name == segname
            || (segname && psymgrp->name && !strcmp(psymgrp->name, segname))
              )
           break;

       /* new or old ? */
       if (!psymgrp)
         {
           psymgrp = xmalloc(sizeof(*psymgrp));
           psymgrp->name = segname ? xstrdup(segname) : NULL;
           psymgrp->head = xmalloc(sizeof(*psymgrp->head));
           psymgrp->tail = xmalloc(sizeof(*psymgrp->tail));
           memset(psymgrp->head, 0, sizeof(*psymgrp->head));
           memset(psymgrp->tail, 0, sizeof(*psymgrp->tail));
           psymgrp->head->fdummy = 1;
           psymgrp->tail->fdummy = 1;
           /* link dummies */
           psymgrp->head->next = psymgrp->tail;
           psymgrp->tail->prev = psymgrp->head;
           psymgrp->head->name = psymgrp->tail->name = "#;$$$$$$dummy$$$$$$$$$$$;#";
           /* link it in */
           psymgrp->next = new_symgrps;
           new_symgrps = psymgrp;
         }

       /* insert at tail. */
       psym->prev = psymgrp->tail->prev;
       psym->next = psymgrp->tail;
       psymgrp->tail->prev->next = psym;
       psymgrp->tail->prev = psym;
    }
}


/* Print the publics. (new method)

   When -o is not specified with number, to leave a round number of unused
   ordinals between the segments out of convenience mostly. */

static void new_print(FILE *phfile, struct new_symgrp *psymgrp)
{
  /* enumerate symbol groups (segments) */
  for (; psymgrp; psymgrp = psymgrp->next)
    {
      struct new_symbol  *psym;

      /* print a decent header */
      if (psymgrp->name)
        fprintf (phfile, "  ; segment %s\n", psymgrp->name);
      else
        fprintf (phfile, "  ; segment noname\n");

      /* enumerate symbols */
      for (psym = psymgrp->head->next; psym != psymgrp->tail; psym = psym->next)
        {
          int   cch;

          cch = fprintf (phfile, "  \"%s\"", psym->name);
          /* fancy align */
          if (cch < 69)
              cch = 70 - cch;
          else if (cch < 89)
              cch = 90 - cch;
          else if (cch < 119)
              cch = 120 - cch;
          else
              cch = 40 - (cch % 39);
          fprintf (phfile, "%*s", cch, "");

          /* flags */
          if (ordinal)
            cch += fprintf (phfile, " @%-5d", ordinal++);
          if (noname_flag)
            cch += fprintf (phfile, " NONAME");

          /* comments - the first one might be used by emximp later... */
          fprintf (phfile, "  ; magicseg='%s' len=%d",
                   psymgrp->name ? psymgrp->name : "", strlen(psym->name));
          if (psym->fweak)
              fprintf (phfile, "  weak");
          if (psym->fcomm)
              fprintf (phfile, "  comm");
          if (psym->mod)
              fprintf (phfile, "  mod='%s'", psym->mod->name);

          fputc ('\n', phfile);
        }

      /* skip a line and some ordinals if we're allowed */
      fputc ('\n', phfile);
      if (!ordinal_seq && ordinal)
        ordinal = ((ordinal + 199) / 100) * 100;
    }
}


static void export (const char *name)
{
  char *dem;

  if (new_mod)
    {
      fprintf (out_file, "\n; From %s", inp_fname);
      if (mod_name != NULL)
        fprintf (out_file, "(%s)", mod_name);
      fputc ('\n', out_file);
      new_mod = FALSE;
    }
  dem = cplus_demangle (name, DMGL_ANSI | DMGL_PARAMS);
  if (dem != NULL)
    {
      fprintf (out_file, "\n  ; %s\n", dem);
      free (dem);
      last_dem = TRUE;
    }
  else if (last_dem)
    {
      fputc ('\n', out_file);
      last_dem = FALSE;
    }
  fprintf (out_file, "  \"%s\"", name);
  if (ordinal != 0)
    fprintf (out_file, " @%d", ordinal++);
  if (noname_flag)
    fprintf (out_file, " NONAME");
  fputc ('\n', out_file);
}


static void export_bss (const char *name)
{
  struct bss_list *p;

  if (bss_flag)
    {
      for (p = bss_list; p != NULL; p = p->next)
        if (strcmp (p->name, name) == 0)
          return;
      p = xmalloc (sizeof (*p));
      p->name = xmalloc (strlen (name) + 1);
      strcpy (p->name, name);
      p->next = bss_list;
      bss_list = p;
      export (name);
    }
}


static void process_aout (FILE *inp_file, long size)
{
  byte *inp_buf;
  const struct exec *a_out_h;
  const byte *sym;
  const struct nlist *sym_ptr;
  const byte *str_ptr;
  long str_size;
  int sym_count, i;
  const char *name;

  new_mod = TRUE;

  inp_buf = xmalloc (size);
  size = fread (inp_buf, 1, size, inp_file);

  a_out_h = (struct exec *)inp_buf;
  if (size < sizeof (struct exec) || N_MAGIC (*a_out_h) != OMAGIC)
    error ("Malformed input file `%s'", inp_fname);
  sym = (inp_buf + sizeof (struct exec) + a_out_h->a_text
         + a_out_h->a_data + a_out_h->a_trsize + a_out_h->a_drsize);
  if (!a_out_h->a_syms)
    return;
  str_ptr = sym + a_out_h->a_syms;
  if (str_ptr + 4 - inp_buf > size)
    error ("Malformed input file `%s'", inp_fname);
  str_size = *(long *)str_ptr;
  sym_ptr = (const struct nlist *)sym;
  sym_count = a_out_h->a_syms / sizeof (struct nlist);
  if (str_ptr + str_size - inp_buf > size)
    error ("Malformed input file `%s'", inp_fname);

  for (i = 0; i < sym_count; ++i)
    if (sym_ptr[i].n_type == (N_TEXT|N_EXT) ||
        sym_ptr[i].n_type == (N_DATA|N_EXT) ||
        ((!legacy_mode || weak_flag) &&
         (sym_ptr[i].n_type == N_WEAKT ||
          sym_ptr[i].n_type == N_WEAKD)))
      {
        name = str_ptr + sym_ptr[i].n_un.n_strx;
        if (legacy_mode)
          export (name);
        else
          new_export (name, aout_segnames[sym_ptr[i].n_type],
                      sym_ptr[i].n_type >= N_WEAKU && sym_ptr[i].n_type >= N_WEAKB,
                      FALSE);
      }
    else if ((sym_ptr[i].n_type == N_EXT && sym_ptr[i].n_value != 0) ||
             sym_ptr[i].n_type == (N_BSS|N_EXT) ||
             ((!legacy_mode || weak_flag) && sym_ptr[i].n_type == N_WEAKB))
      {
        name = str_ptr + sym_ptr[i].n_un.n_strx;
        if (legacy_mode)
          export_bss (name);
        else
          new_export (name, aout_segnames[sym_ptr[i].n_type],
                      sym_ptr[i].n_type >= N_WEAKU && sym_ptr[i].n_type >= N_WEAKB,
                      TRUE);
      }

  free (inp_buf);
}


static byte rec_buf[MAX_REC_SIZE+8];
static int rec_type;
static int rec_len;
static int rec_idx;


static void bad_omf (void)
{
  error ("Malformed OMF file `%s'", inp_fname);
}


static void get_mem (void *dst, int len)
{
  if (rec_idx + len > rec_len)
    bad_omf ();
  memcpy (dst, rec_buf + rec_idx, len);
  rec_idx += len;
}


static int get_byte (void)
{
  if (rec_idx >= rec_len)
    bad_omf ();
  return rec_buf[rec_idx++];
}


static void get_string (byte *dst)
{
  int len;

  len = get_byte ();
  get_mem (dst, len);
  dst[len] = 0;
}


static int get_index (void)
{
  int result;

  result = get_byte ();
  if (result & 0x80)
    {
      if (rec_idx >= rec_len)
        bad_omf ();
      result = ((result & 0x7f) << 8) | rec_buf[rec_idx++];
    }
  return result;
}


static word get_dword (void)
{
  dword result;

  if (rec_idx + 4 > rec_len)
    bad_omf ();
  result = rec_buf[rec_idx++];
  result |= rec_buf[rec_idx++] << 8;
  result |= rec_buf[rec_idx++] << 16;
  result |= rec_buf[rec_idx++] << 24;
  return result;
}


static word get_word (void)
{
  word result;

  if (rec_idx + 2 > rec_len)
    bad_omf ();
  result = rec_buf[rec_idx++];
  result |= rec_buf[rec_idx++] << 8;
  return result;
}


static dword get_word_or_dword (void)
{
  return rec_type & REC32 ? get_dword () : get_word ();
}


static dword get_commlen (void)
{
  dword result;

  result = get_byte ();
  if (result <= 0x80)
    return result;
  switch (result)
    {
    case 0x81:
      return get_word ();
    case 0x84:
      result = get_byte ();
      result |= get_byte () << 8;
      result |= get_byte () << 16;
      return result;
    case 0x88:
      return get_dword ();
    default:
      bad_omf ();
    }
}


static void omf_pubdef (void)
{
  int type, group, seg;
  word frame;
  dword offset;
  byte name[260];

  group = get_index ();
  seg = get_index ();
  if (seg == 0)
    frame = get_word ();

  while (rec_idx < rec_len)
    {
      char *weak;
      get_string (name);
      offset = get_word_or_dword ();
      type = get_index ();
      weak = strstr(name, "$w$");
      if (legacy_mode)
        {
          if (!weak || weak_flag)
            export (name);
        }
      else
        {
          if (weak)
            {
              memmove(weak + 1, weak, strlen(weak) + 1);
              *weak++ = '\0';
            }
          new_export (name, segdefs[seg].name, weak != NULL, FALSE);
        }
    }
}


static void omf_comdef (void)
{
  int type_index, data_type;
  byte name[260];

  while (rec_idx < rec_len)
    {
      char *weak;
      get_string (name);
      type_index = get_index ();
      data_type = get_byte ();
      switch (data_type)
        {
        case 0x61:
          get_commlen ();
          get_commlen ();
          break;
        case 0x62:
          get_commlen ();
          break;
        default:
          bad_omf ();
        }

      weak = strstr(name, "$w$");
      if (legacy_mode)
        {
          if (!weak || weak_flag)
            export_bss (name);
        }
      else
        {
          if (weak)
            {
              memmove(weak + 1, weak, strlen(weak) + 1);
              *weak++ = '\0';
            }
          new_export (name, "BSS32", weak != NULL, TRUE);
        }
    }
}


static void omf_lnames (void)
{
  while (rec_idx < rec_len)
    {
      unsigned len = get_byte ();
      if (!(num_lnames % 64))
        lnames = xrealloc(lnames, sizeof(lnames[0]) * (num_lnames + 64));
      lnames[num_lnames] = xmalloc(len + 1);
      get_mem(lnames[num_lnames], len);
      lnames[num_lnames][len] = '\0';
      num_lnames++;
    }
}


static void omf_segdef (void)
{
  byte      flags;
  unsigned  nameidx;
  unsigned  classidx;
  unsigned  ovlidx;

  /* all we want the the segment name */
  flags = get_byte ();                  /* segment attributes */
  if ((flags & 0xE0) == 0)
    {
      get_word ();                      /* frame number */
      get_byte ();                      /* offset */
    }
  if (rec_type & REC32)                 /* segment length */
    get_dword ();
  else
    get_word ();

  nameidx = get_index ();
  classidx = get_index ();
  ovlidx = get_index ();
  if (nameidx == 0)
      nameidx = classidx;               /* paranoia */

  /* add it */
  if (!(num_segdefs % 64))
    segdefs = xrealloc (segdefs, sizeof(segdefs[0]) * (num_segdefs + 64));
  if (nameidx != 0 && nameidx < num_lnames)
    segdefs[num_segdefs].name = lnames[nameidx];
  else
    segdefs[num_segdefs].name = NULL;
  num_segdefs++;
}



static void process_omf (FILE *inp_file)
{
  int               i;
  struct omf_rec    rec;

  /* init */
  new_mod = TRUE;
  lnames = xmalloc(sizeof(lnames[0])* 64);
  lnames[0] = NULL;
  num_lnames = 1;                       /* dummy entry */
  segdefs = xmalloc(sizeof(segdefs[0])* 64);
  memset(&segdefs[0], 0, sizeof(segdefs[0]));
  num_segdefs = 1;

  /* read */
  do
    {
      if (fread (&rec, sizeof (rec), 1, inp_file) != 1)
        error ("Unexpected end of file on input file `%s'", inp_fname);
      rec_type = rec.rec_type;
      rec_len = rec.rec_len;
      rec_idx = 0;
      if (rec_len > sizeof (rec_buf))
        error ("OMF record too long in `%s'", inp_fname);
      if (fread (rec_buf, rec_len, 1, inp_file) != 1)
        error ("Unexpected end of file on input file `%s'", inp_fname);
      --rec_len;                /* Remove checksum */
      switch (rec_type)
        {
        case PUBDEF:
        case PUBDEF|REC32:
          omf_pubdef ();
          break;
        case COMDEF:
          if (!legacy_mode || bss_flag)
            omf_comdef ();
          break;
        case LNAMES:
          omf_lnames ();
          break;
        case SEGDEF:
        case SEGDEF|REC32:
          omf_segdef ();
          break;
        }
    } while (rec.rec_type != MODEND && rec_type != (MODEND|REC32));

   /* cleanup (entry 0 isn't used) */
   for (i = 1; i < num_lnames; i++)
       free(lnames[i]);
   free(lnames);
   free(segdefs);
}


/* Process one input file. */

static void process (void)
{
  static char ar_magic[SARMAG+1] = ARMAG;
  char ar_test[SARMAG], *p, *end;
  struct ar_hdr ar;
  long size, ar_pos, index;
  int i, n;
  FILE *inp_file;
  char *long_names = NULL;
  size_t long_names_size = 0;

  mod_name = NULL;
  inp_file = fopen (inp_fname, "rb");
  if (inp_file == NULL)
    error ("Cannot open input file `%s'", inp_fname);

  /* Read some bytes from the start of the file to find out whether
     this is an archive (.a) file or not. */

  if (fread (ar_test, sizeof (ar_test), 1, inp_file) != 1)
    error ("Cannot read input file `%s'", inp_fname);

  if (memcmp (ar_test, ar_magic, SARMAG) == 0)
    {

      /* The input file is an archive. Loop over all the members of
         the archive. */

      ar_pos = SARMAG;
      for (;;)
        {

          /* Read the header of the member. */

          fseek (inp_file, ar_pos, SEEK_SET);
          size = fread  (&ar, 1, sizeof (ar), inp_file);
          if (size == 0)
            break;
          else if (size != sizeof (ar))
            error ("Malformed archive `%s'", inp_fname);

          /* Decode the header. */

          errno = 0;
          size = strtol (ar.ar_size, &p, 10);
          if (p == ar.ar_size || errno != 0 || size <= 0 || *p != ' ')
            error ("Malformed archive header in `%s'", inp_fname);
          ar_pos += (sizeof (ar) + size + 1) & -2;

          /* Remove trailing blanks from the member name. */

          i = sizeof (ar.ar_name) - 1;
          while (i > 0 && ar.ar_name[i-1] == ' ')
            --i;
          ar.ar_name[i] = 0;

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

          /* Ignore the __.SYMDEF and __.IMPORT members. */

          else if (strcmp (ar.ar_name, "__.SYMDEF") != 0
              && strcmp (ar.ar_name, "__.IMPORT") != 0)
            {
              /* Process the current member.  First, fetch the name of
                 the member.  If the ar_name starts with a space, the
                 decimal number following that space is an offset into
                 the "ARFILENAMES/" member.  The number may be
                 followed by a space and a substring of the long file
                 name. */

              mod_name = ar.ar_name;
              if (mod_name[0] == ' ' && long_names != NULL
                  && (index = strtol (mod_name + 1, &end, 10)) >= 0
                  && index < long_names_size - 1
                  && (*end == 0 || *end == ' ')
                  && (index == 0 || long_names[index-1] == 0))
                mod_name = long_names + index;

              process_aout (inp_file, size);
            }
        }
    }
  else
    {
      if (*(word *)ar_test == 0407)
        {
          if (fseek (inp_file, 0L, SEEK_END) != 0)
            error ("Input file `%s' is not seekable", inp_fname);
          size = ftell (inp_file);
          fseek (inp_file, 0L, SEEK_SET);
          process_aout (inp_file, size);
        }
      else if (*(byte *)ar_test == LIBHDR)
        {
          struct omflib *lib;
          char errmsg[512], name[257];
          int page;

          lib = omflib_open (inp_fname, errmsg);
          if (lib == NULL)
            error ("%s: %s", inp_fname, errmsg);

          n = omflib_module_count (lib, errmsg);
          if (n == -1)
            error ("%s: %s", inp_fname, errmsg);
          for (i = 0; i < n; ++i)
            {
              if (omflib_module_info (lib, i, name, &page, errmsg) != 0)
                error ("%s: %s", inp_fname, errmsg);
              else
                {
                  fseek (inp_file, omflib_page_pos (lib, page), SEEK_SET);
                  mod_name = name;
                  process_omf (inp_file);
                }
            }
          if (omflib_close (lib, errmsg) != 0)
            error ("%s: %s", inp_fname, errmsg);
        }
      else
        {
          fseek (inp_file, 0L, SEEK_SET);
          process_omf (inp_file);
        }
    }
  fclose (inp_file);
}


/* Main line. */

int main (int argc, char **argv)
{
  int c, i;
  char *p;

  _response (&argc, &argv);
  _wildcard (&argc, &argv);
  opterr = 0;
  optind = 0;
  while ((c = getopt (argc, argv, "no::uw")) != EOF)
    {
      switch (c)
        {
        case 'n':
          noname_flag = TRUE;
          break;
        case 'o':
          if (optarg == NULL)
              ordinal = 1;
          else
            {
              errno = 0;
              ordinal = (int)strtol (optarg, &p, 0);
              if (p == optarg || errno != 0 || *p != 0
                  || ordinal < 1 || ordinal > 65535)
                usage ();
              ordinal_seq = 1;
            }
          break;
        case 'u':
          bss_flag = TRUE;
          break;
        case 'w':
          weak_flag = TRUE;
          break;
        case 'l':
          legacy_mode = TRUE;
          break;
        default:
          error ("Invalid option");
        }
    }

  out_file = stdout;

  if (optind >= argc)
    usage ();

  for (i = optind; i < argc; ++i)
    {
      inp_fname = argv[i];
      process ();
    }

  if (!legacy_mode)
    new_print(out_file, new_symgrps);

  if (fflush (out_file) != 0 || (out_file != stdout && fclose (out_file) != 0))
    error ("Write error");

  return 0;
}
