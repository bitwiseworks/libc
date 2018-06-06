/* sys/heapdump.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include "syscalls.h"
#include <stdio.h>

void _sys_dump_heap_objs (FILE *f); /* debug? */

void _sys_dump_heap_objs (FILE *f)
{
  unsigned i;

  fprintf (f, "base     brk      end\n");
  for (i = 0; i < _sys_heap_obj_count; ++i)
    fprintf (f, "%.8lx %.8lx %.8lx%s\n",
             _sys_heap_objs[i].base,
             _sys_heap_objs[i].brk,
             _sys_heap_objs[i].end,
             _sys_top_heap_obj == &_sys_heap_objs[i] ? " top" : "");
}
