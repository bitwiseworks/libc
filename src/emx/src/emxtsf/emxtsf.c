/* emxtsf.c -- Simplify construction of .TSF files
   Copyright (c) 1996-1998 Eberhard Mattes

This file is part of emxtsf.

emxtsf is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxtsf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxtsf; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/moddef.h>


struct sym
{
  struct sym *next;
  int ord;
  char def;
  char *name;
};

#define HASHSIZE        997
static struct sym *sym_hash[HASHSIZE];
static const char *dll_name;
static int warning_level;

static void *xmalloc (size_t n)
{
  void *p;

  p = malloc (n);
  if (p == NULL)
    {
      fprintf (stderr, "Out of memory\n");
      exit (2);
    }
  return p;
}


static void usage (void)
{
  fputs ("emxtsf " VERSION INNOTEK_VERSION " -- Copyright (c) 1996 by Eberhard Mattes\n\n"
         "Usage: emxtsf [-d <dll_name>] [-w <level>] <tss_file> <map_file> <def_file>\n",
         stderr);
  exit (1);
}


static int hash (const char *name, int len)
{
  unsigned h;
  int i;

  h = 0;
  for (i = 0; i < len; ++i)
    h = (h << 2) ^ (unsigned char)name[i];
  return h % HASHSIZE;
}


static struct sym *find_sym (const char *name, int len)
{
  struct sym *sp;

  for (sp = sym_hash[hash (name, len)]; sp != NULL; sp = sp->next)
    if (strlen (sp->name) == len && memcmp (sp->name, name, len) == 0)
      return sp;
  return NULL;
}


static void read_map (const char *fname)
{
  FILE *f;
  char line[512];
  char start;
  unsigned seg, off;
  int pos, i, h, len;
  struct sym *sp;

  f = fopen (fname, "r");
  if (f == NULL)
    {
      perror (fname);
      exit (2);
    }
  start = 0;
  while (!start && fgets (line, sizeof (line), f) != NULL)
    start = strcmp (line, "  Address         Publics by Value\n") == 0;
  if (!start || fgets (line, sizeof (line), f) == NULL || line[0] != '\n')
    {
      fprintf (stderr, "%s: Publics not found\n", fname);
      exit (2);
    }

  while (fgets (line, sizeof (line), f) != NULL && line[0] != '\n')
    {
      if (sscanf (line, " %x:%x %n", &seg, &off, &pos) != 2)
        break;
      if (strncmp (line + pos, "Imp", 3) == 0)
        continue;
      while (line[pos] == ' ')
        ++pos;
      i = pos;
      while (line[i] != ' ' && line[i] != '\n' && line[i] != 0)
        ++i;
      if (i > pos && line[i] == '\n')
        {
          len = i - pos;
          if (find_sym (line + pos, len) != NULL)
            {
              fprintf (stderr, "%s: Symbol `%.*s' multiply defined\n",
                       fname, len, line + pos);
              exit (2);
            }
          h = hash (line + pos, len);
          sp = xmalloc (sizeof (*sp));
          sp->ord = 0;
          sp->def = 0;
          sp->name = xmalloc (len + 1);
          memcpy (sp->name, line + pos, len);
          sp->name[len] = 0;
          sp->next = sym_hash[h];
          sym_hash[h] = sp;
        }
    }

  fclose (f);
}


static int handle_def (struct _md *md, const _md_stmt *stmt,
                       _md_token token, void *arg)
{
  struct sym *sp;
  const char *name;

  switch (token)
    {
    case _MD_EXPORTS:
      if (stmt->export.ordinal != 0)
        {
          name = stmt->export.internalname;
          if (*name == 0)
            name = stmt->export.entryname;
          sp = find_sym (name, strlen (name));
          if (sp != NULL)
            sp->ord = stmt->export.ordinal;
        }
      break;

    case _MD_parseerror:
      fprintf (stderr, "%s:%ld: %s\n", (char *)arg, _md_get_linenumber (md),
               _md_errmsg (stmt->error.code));
      exit (2);

    default:
      break;
    }
  return 0;
}


static void read_def (const char *fname)
{
  struct _md *md;

  md = _md_open (fname);
  if (md == NULL)
    {
      perror (fname);
      exit (2);
    }
  _md_next_token (md);
  _md_parse (md, handle_def, (void *)fname);
  _md_close (md);
}


static const char *tss_fname;
static int tss_lineno;

static void syntax (void)
{
  fprintf (stderr, "%s:%d: Syntax error\n", tss_fname, tss_lineno);
  exit (2);
}


static void read_tss (const char *fname)
{
  FILE *f;
  char line[512], *s, c;
  char fun[512], fun2[520];
  char group[40], type[40], mem[128];
  char *prefix;
  struct sym *sp;
  int i, j, len, off, minor, uminor;

  f = fopen (fname, "r");
  if (f == NULL)
    {
      perror (fname);
      exit (2);
    }

  tss_fname = fname;
  tss_lineno = 0;
  group[0] = 0; type[0] = 0;
  uminor = 0x7fff;
  if (dll_name == NULL)
    prefix = "";
  else
    {
      prefix = xmalloc (strlen (dll_name) + 3);
      sprintf (prefix, "%s: ", dll_name);
    }

  while (fgets (line, sizeof (line), f) != NULL)
    {
      ++tss_lineno;
      s = strchr (line, '\n');
      if (s != NULL) *s = 0;
      if (line[0] == ';' || line[0] == 0)
        continue;
      if (line[1] != ' ' || line[2] == ' ')
        syntax ();
      if (line[0] == 'G')
        {
          _strncpy (group, line + 2, sizeof (group));
          continue;
        }
      if (line[0] == 'T')
        {
          _strncpy (type, line + 2, sizeof (type));
          continue;
        }
      if (group[0] == 0 || type[0] == 0)
        syntax ();
      i = 2;
      while (line[i] != ' ' && line[i] != '(' && line[i] != 0)
        ++i;
      len = i - 2;
      if (len >= sizeof (fun) - 1)
        len = sizeof (fun) - 2;
      memcpy (fun, line + 2, len);
      fun[len] = 0;
      sp = find_sym (fun, len);
      if (sp == NULL)
        {
          fun[0] = '_';
          memcpy (fun + 1, line + 2, len);
          fun[++len] = 0;
          sp = find_sym (fun, len);
          if (sp == NULL)
            {
              fprintf (stderr, "%s:%d: Symbol `%s' not found\n",
                       tss_fname, tss_lineno, fun + 1);
              continue;
            }
        }
      if (sp->def)
        {
          fprintf (stderr, "%s:%d: Symbol `%s' multiply defined\n",
                   tss_fname, tss_lineno, fun);
          continue;
        }
      sp->def = 1;
      if (line[0] == 'I')
        continue;
      if (sp->ord != 0)
        minor = sp->ord;
      else
        minor = uminor--;
      while (line[i] == ' ')
        ++i;
      if (line[i] != '(')
        syntax ();
      ++i;
      printf ("TRACE\n"
              "MINOR=0x%.4x,\n"
              "TP=.%s,\n"
              "TYPE=(PRE,%s),\n"
              "GROUP=%s,\n"
              "DESC=\"%s%s  Pre-Invocation\",\n",
              minor, fun, type, group, prefix, fun);
      while (line[i] == ' ')
        ++i;
      if (line[i] != ')')
        {
          off = 4;
          for (;;)
            {
              c = line[i];
              ++i;
              if (line[i] != ' ')
                syntax ();
              while (line[i] == ' ')
                ++i;
              j = i;
              while (line[i] != ' ' && line[i] != ',' && line[i] != ')'
                     && line[i] != 0)
                ++i;
              if (i == j)
                syntax ();

              printf ("FMT=\"%.*s = ", i - j, line + j);
              switch (c)
                {
                case 'q':
                  fputs ("%P%Q", stdout);
                  sprintf (mem, "MEM32=(FESP+%d,D,8)", off);
                  off += 8;
                  break;
                case 'i':
                  fputs ("%P%D", stdout);
                  sprintf (mem, "MEM32=(FESP+%d,D,4)", off);
                  off += 4;
                  break;
                case 'p':
                  fputs ("%P%F", stdout);
                  sprintf (mem, "MEM32=(FESP+%d,D,4)", off);
                  off += 4;
                  break;
                case 's':
                  fputs ("\\\"%P%S\\\"", stdout);
                  sprintf (mem, "ASCIIZ32=(FESP+%d,I,128)", off);
                  off += 4;
                  break;
                default:
                  syntax ();
                }
              printf ("\",\n%s,\n", mem);
              while (line[i] == ' ')
                ++i;
              if (line[i] == ')')
                break;
              if (line[i] != ',')
                syntax ();
              ++i;
              while (line[i] == ' ')
                ++i;
            }
        }
      ++i;
      while (line[i] == ' ')
        ++i;
      if (line[i] != 0 && line[i] != ';')
        syntax ();
      puts ("FMT=\"Return address = %P%F\",");
      puts ("MEM32=(FESP+0,D,4)\n");
      sprintf (fun2, "__POST$%s", fun);
      sp = find_sym (fun2, strlen (fun2));
      if (sp != NULL)
        {
          printf ("TRACE\n"
                  "MINOR=0x%.4x,\n"
                  "TP=.%s,\n"
                  "TYPE=(POST,%s),\n"
                  "GROUP=%s,\n"
                  "DESC=\"%s%s  Post-Invocation\",\n",
                  minor + 0x8000, fun2, type, group, prefix, fun);
          switch (line[0])
            {
            case 'v':
              break;
            case 'i':
              puts ("FMT=\"Return value = %D\",\n"
                    "REGS=(EAX)");
              break;
            case 'p':
              puts ("FMT=\"Return value = %F\",\n"
                    "REGS=(EAX)");
              break;
            case 'q':
              puts ("FMT=\"Return value = %Q\",\n"
                    "REGS=(EAX,EDX)");
              break;
            default:
              syntax ();
            }
          putchar ('\n');
        }
      else if (warning_level >= 2)
        fprintf (stderr, "%s:%d: Epilogue not found for symbol `%s'\n",
                 tss_fname, tss_lineno, fun);
    }
  fclose (f);
}


static void list_untraced (void)
{
  int h;
  struct sym *sp;

  for (h = 0; h < HASHSIZE; ++h)
    for (sp = sym_hash[h]; sp != NULL; sp = sp->next)
      if (!sp->def && sp->ord != 0)
        fprintf (stderr, "Not traced: %s\n", sp->name);
}


int main (int argc, char *argv[])
{
  int c;
  long n;
  char *end;

  while ((c = getopt (argc, argv, "d:w:")) != -1)
    switch (c)
      {
      case 'd':
        dll_name = optarg;
        break;
      case 'w':
        n = strtol (optarg, &end, 10);
        if (end == optarg || *end != 0 || n < 0 || n > 2)
          usage ();
        warning_level = (int)n;
        break;
      default:
        usage ();
      }

  if (argc - optind != 3)
    usage ();

  read_map (argv[optind+1]);
  read_def (argv[optind+2]);
  read_tss (argv[optind+0]);
  if (warning_level >= 1)
    list_untraced ();
  return 0;
}
