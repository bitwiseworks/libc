/* user.h (emx+gcc) */

#ifndef _SYS_USER_H
#define _SYS_USER_H

#define UMAGIC 0x10f

struct fpstate
{
  unsigned long state[27];
  unsigned long status;
};

struct user
{
  unsigned short u_magic;
  unsigned short u_reserved1;
  unsigned long  u_data_base;
  unsigned long  u_data_end;
  unsigned long  u_data_off;
  unsigned long  u_heap_base;
  unsigned long  u_heap_end;
  unsigned long  u_heap_off;
  unsigned long  u_heap_brk;
  unsigned long  u_stack_base;
  unsigned long  u_stack_end;
  unsigned long  u_stack_off;
  unsigned long  u_stack_low;
  int *          u_ar0;
  char           u_fpvalid;
  char           u_reserved2[3];
  struct fpstate u_fpstate;
  char           u_module[8];
  unsigned long  u_heapobjs_off;
  unsigned long  u_reserved3[20];
  unsigned long  u_regs[19];
};

struct _user_heapobj
{
  unsigned long  u_base;
  unsigned long  u_end;
  unsigned long  u_off;
  unsigned long  u_brk;
};

struct _user_heapobjs
{
  unsigned long u_count;
  struct _user_heapobj u_objs[255];
};

#endif /* not _SYS_USER_H */
