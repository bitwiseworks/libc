/* emxcat.c -- Concatenate source files
   Copyright (c) 1992-1998 Eberhard Mattes

This file is part of emxcat.

emxcat is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxcat is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxcat; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NORETURN volatile

#define FALSE 0
#define TRUE  1

typedef struct list
{
  struct list *next;
  char *name;
} list;

typedef struct edge
{
  struct edge *next;
  struct vertex *v;
} edge;

typedef struct vertex
{
  struct vertex *next;
  edge *desc;
  char *name;
  int mark;
  int number;
  int descs;
} vertex;

static vertex *includes = NULL;
static vertex **includes_add = &includes;
static vertex *prev_include = NULL;
static list *selects = NULL;
static list **selects_add = &selects;
static list *consts = NULL;
static list **consts_add = &consts;
static list *defines;
static list **defines_add;
static list *predefs = NULL;
static list **predefs_add = &predefs;
static char *output_fname;
static FILE *output_file;
static int rc = 0;
static int define_warning;
static int sys_emx_h = FALSE;


/* Prototypes. */

static void NORETURN usage (void);
static void *xmalloc (size_t n);
static char *xstrdup (const char *s);
int main (int argc, char *argv[]);


/* Tell the user how to invoke this program. */

static void NORETURN usage (void)
{
  fprintf (stderr, "emxcat " VERSION INNOTEK_VERSION " -- "
           "Copyright (c) 1992-1995 by Eberhard Mattes\n\n");
  fprintf (stderr, "Usage: emxcat [-D<symbol>]... -o <output_file> <input_file>...\n");
  exit (1);
}


/* Allocate a block of memory.  Abort if out of memory. */

static void *xmalloc (size_t n)
{
  void *p;

  p = malloc (n);
  if (p == NULL)
    {
      fprintf (stderr, "emxcat: out of memory\n");
      exit (2);
    }
  return p;
}


/* Create on the heap a duplicate of the string S.  Abort if out of
   memory. */

static char *xstrdup (const char *s)
{
  char *p;

  p = xmalloc (strlen (s) + 1);
  strcpy (p, s);
  return p;
}


/* Add a string to a linked to list of strings. */

static void add_list (list ***dst, const char *src)
{
  list *p;

  p = xmalloc (sizeof (*p));
  p->next = NULL;
  p->name = (char *)src;
  **dst = p;
  *dst = &p->next;
}


/* Handle an #include directive. */

static int do_include (const char *str, int pass, const char *fname)
{
  char delim;
  char *name;
  int len;
  const char *start, *s;
  vertex *p, *v;
  edge *e;

  if (pass == 1 && defines != NULL && !define_warning)
    {
      fprintf (stderr, "emxcat: #define used before #include in %s\n",
               fname);
      define_warning = TRUE;
      rc = 1;
    }
  s = str;
  if (!isspace ((unsigned char)*s))
    return TRUE;
  while (isspace ((unsigned char)*s))
    ++s;
  if (*s == '"')
    delim = '"';
  else if (*s == '<')
    delim = '>';
  else
    return TRUE;
  start = s;
  ++s;
  while (*s != 0 && *s != delim)
    ++s;
  if (*s != delim)
    return TRUE;
  if (pass == 1)
    return FALSE;
  ++s;
  len = s - start;
  name = alloca (len+1);
  memcpy (name, start, len);
  name[len] = 0;
  if (strcmp (name, "<sys/emx.h>") == 0)
    {
      sys_emx_h = TRUE;
      return FALSE;
    }
  v = NULL;
  for (p = includes; p != NULL; p = p->next)
    if (strcmp (name, p->name) == 0)
      {
        v = p;
        break;
      }
  if (v == NULL)
    {
      v = xmalloc (sizeof (*v));
      v->next = NULL;
      v->name = xstrdup (name);
      v->desc = NULL;
      v->mark = 0;
      v->descs = 0;
      *includes_add = v;
      includes_add = &v->next;
    }
  if (prev_include != NULL)
    {
      for (e = v->desc; e != NULL; e = e->next)
        if (e->v == prev_include)
          break;
      if (e == NULL)
        {
          e = xmalloc (sizeof (*e));
          e->next = v->desc;
          v->desc = e;
          e->v = prev_include;
        }
    }
  prev_include = v;
  return FALSE;
}


/* Handle a #define directive. */

static int do_define (const char *str, int pass)
{
  char *name;
  int len;
  const char *start, *s;
  list *p;

  s = str;
  if (!isspace ((unsigned char)*s))
    return TRUE;
  while (isspace ((unsigned char)*s))
    ++s;
  start = s;
  while (*s == '_' || isalnum ((unsigned char)*s))
    ++s;
  len = s - start;
  name = alloca (len+1);
  memcpy (name, start, len);
  name[len] = 0;
  while (isspace ((unsigned char)*s))
    ++s;
  if (strncmp (s, "/*", 2) == 0)
    s = strchr (s, 0);
  if (strncmp (name, "INCL_", 5) == 0 && *s == 0)
    {
      if (pass == 1)
        return FALSE;
      for (p = selects; p != NULL; p = p->next)
        if (strcmp (p->name, name) == 0)
          return FALSE;
      add_list (&selects_add, xstrdup (name));
      return FALSE;
    }
  else if (pass == 0)
    return FALSE;
  else
    {
      add_list (&defines_add, xstrdup (name));
      return TRUE;
    }
}


/* Handle a CONST_ definition encountered in an .s file. */

static int do_const (const char *str, int pass)
{
  const char *s;
  char *name;
  int len;
  list *p;

  if (pass == 0)
    return FALSE;
  for (s = str; *s == '_' || isalnum ((unsigned char)*s); ++s)
    ;
  len = s - str;
  name = alloca (len+1);
  memcpy (name, str, len);
  name[len] = 0;
  for (p = consts; p != NULL; p = p->next)
    if (strcmp (name, p->name) == 0)
      return FALSE;
  add_list (&consts_add, xstrdup (name));
  return TRUE;
}


/* Clear all marks of the dag of header files. */

static void unmark (void)
{
  vertex *p;

  for (p = includes; p != NULL; p = p->next)
    p->mark = 0;
}


/* Number the vertices of the dag of header files. */

static int vn;

static void number_2 (vertex *v)
{
  edge *e;

  v->mark = 1;
  v->number = vn++;
  v->descs = 0;
  for (e = v->desc; e != NULL; e = e->next)
    if (!e->v->mark)
      {
        number_2 (e->v);
        v->descs += 1 + e->v->descs;
      }
}

/* Helper function for checking for cycles the dag of header files. */

static void cycles_2 (vertex *v, const char *fname)
{
  edge *e;
  vertex *w;

  v->mark = 1;
  for (e = v->desc; e != NULL; e = e->next)
    {
      w = e->v;
      if (w->mark)
        {
          if (v->number >= w->number && v->number <= w->number + w->descs)
            {
              fprintf (stderr, "emxcat: %s: inconsistent order of %s and %s\n",
                       fname, v->name, w->name);
              rc = 1;
            }
        }
      else
        cycles_2 (w, fname);
    }
}

/* Check for cycles the dag of header files. */

static void cycles (const char *fname)
{
  vertex *p;

  unmark ();
  vn = 1;
  for (p = includes; p != NULL; p = p->next)
    if (!p->mark)
      number_2 (p);
  unmark ();
  for (p = includes; p != NULL; p = p->next)
    if (!p->mark)
      cycles_2 (p, fname);
}


/* Read the input file named FNAME.  If PASS is zero, collect
   information.  If PASS is one, copy the file contents to the output
   file. */

static void read_file (const char *fname, int pass)
{
  FILE *f;
  char *s;
  list *p, *q;
  enum {TYPE_UNKNOWN, TYPE_C, TYPE_S} type;
  char line[1024];
  int copy;

  /* Determine the type of the file (C or assembler) from the file
     name suffix. */

  type = TYPE_UNKNOWN;
  s = _getext (fname);
  if (s != NULL)
    {
      ++s;
      if (stricmp (s, "c") == 0 || stricmp (s, "cc") == 0
          || stricmp (s, "cpp") == 0 || stricmp (s, "cxx") == 0)
        type = TYPE_C;
      else if (stricmp (s, "s") == 0)
        type = TYPE_S;
    }

  /* Abort if the file type is not known. */

  if (type == TYPE_UNKNOWN)
    {
      fprintf (stderr, "emxcat: unknown file type (%s)\n", fname);
      exit (1);
    }

  /* Open the file. */

  f = fopen (fname, "rt");
  if (f == NULL)
    {
      perror (fname);
      exit (2);
    }

  /* Initialize file-local variables. */

  defines = NULL; defines_add = &defines; prev_include = NULL;
  define_warning = FALSE;

  /* Read lines until hitting the end of the file. */

  while (fgets (line, sizeof (line), f) != NULL)
    {
      /* Lines starting with #include and #define are handled
         specially.  In assembler files, lines starting with CONST_
         are handled specially.  In the second pass, all other lines
         are copied verbatim to the output file. */

      if (memcmp (line, "#include", 8) == 0)
        copy = do_include (line+8, pass, fname);
      else if (type == TYPE_S && memcmp (line, "CONST_", 6) == 0)
        copy = do_const (line, pass);
      else if (memcmp (line, "#define", 7) == 0)
        copy = do_define (line+7, pass);
      else
        copy = TRUE;

      /* Write the line to the output file if we are in the second
         pass and the line is to be copied. */

      if (copy && pass == 1)
        {
          if (fputs (line, output_file) == EOF)
            break;
        }
    }

  /* Check the input stream for I/O errors, then close it. */

  if (ferror (f))
    {
      perror (fname);
      exit (2);
    }
  fclose (f);

  /* Undefine all symbols #defined in this file. */

  if (defines != NULL)
    {
      fputc ('\n', output_file);
      for (p = defines; p != NULL; p = q)
        {
          q = p->next;
          fprintf (output_file, "#undef %s\n", p->name);
          free (p->name);
          free (p);
        }
    }

  /* Check for cycles the dag of header files. */

  if (pass == 0)
    cycles (fname);

  /* In the second pass, write an empty line to the output file, and
     check for I/O errors. */

  if (pass == 1)
    {
      fputc ('\n', output_file);
      if (ferror (output_file))
        {
          perror ("emxcat output file");
          exit (2);
        }
    }
}


/* Write #define directives for predefined symbols. */

static void insert_predefs (void)
{
  list *p;

  for (p = predefs; p != NULL; p = p->next)
    fprintf (output_file, "#define %s\n", p->name);
}


/* Insert #define directives which have been used before #include
   directives. */

static void insert_selects (void)
{
  list *p;

  for (p = selects; p != NULL; p = p->next)
    fprintf (output_file, "#define %s\n", p->name);
}


/* Topological sort on the dag of #include directives, writing them to
   the output file. */

static void insert_includes_2 (vertex *v)
{
  edge *e;

  v->mark = 1;
  for (e = v->desc; e != NULL; e = e->next)
    if (!e->v->mark)
      insert_includes_2 (e->v);
  fprintf (output_file, "#include %s\n", v->name);
}


/* Compare two argv[] entries by name.  This function is passed to
   qsort() for sorting the input files by name. */

int cmp_fname (const void *p1, const void *p2)
{
  /* Disregard case when comparing file names, to get same result on
     case-preserving file systems (HPFS) and upper-casing file systems
     (FAT). */

  return strcasecmp (*(const char * const *)p1,
                     *(const char * const *)p2);
}


/* Topologically sort #include directives and write them to the output
   file.  <sys/emx.h> always comes first. */

static void insert_includes (void)
{
  vertex *p;

  if (sys_emx_h)
    fputs ("#include <sys/emx.h>\n", output_file);
  unmark ();
  for (p = includes; p != NULL; p = p->next)
    if (!p->mark)
      insert_includes_2 (p);
}


/* The main code of emxcat. */

int main (int argc, char *argv[])
{
  int i, j, pass;

  /* Expand response files and wildcards. */

  _response (&argc, &argv);
  _wildcard (&argc, &argv);

  /* Handle -D options. */

  i = 1;
  while (i < argc && strncmp (argv[i], "-D", 2) == 0 && strlen (argv[i]) > 2)
    {
      add_list (&predefs_add, xstrdup (argv[i] + 2));
      ++i;
    }

  /* 3 or more arguments must be left: -o <output_file> <input_file> */

  if (argc - i < 3)
    usage ();
  if (strcmp (argv[i], "-o") != 0)
    usage ();
  ++i;
  output_fname = argv[i++];

  /* Open the output file. */

  output_file = fopen (output_fname, "wt");
  if (output_file == NULL)
    {
      perror (output_fname);
      exit (2);
    }

  /* Sort the input files by name.  This is for getting predictable
     results when using wildcards.  Note that _wildcard() yields files
     sorted by name on HPFS partitions.  On FAT partitions, however,
     the files are not in a particular order. */

  qsort (argv + i, argc - i, sizeof (*argv), cmp_fname);

  /* Make two passes over all the files.  The first pass collects
     information, the second pass writes the output file. */

  for (pass = 0; pass < 2; ++pass)
    {
      if (pass == 1)
        {
          /* At the beginning of the second pass, write #define and
             #include directives. */

          insert_predefs ();
          insert_selects ();
          insert_includes ();
        }

      /* Read each input file given on the command line, provided its
         name is not identical to the name of the output file. */

      for (j = i; j < argc; ++j)
        if (strcasecmp (output_fname, argv[j]) != 0)
          read_file (argv[j], pass);
    }

  /* Close the output file. */

  if (fflush (output_file) != 0 || fclose (output_file) != 0)
    {
      perror (output_fname);
      exit (2);
    }

  /* Done. */

  return rc;
}
