/* core.c -- Dump core
   Copyright (c) 1994-1996 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


#define INCL_DOSEXCEPTIONS
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#include <os2emx.h>
#include "emxdll.h"
#include <sys/user.h>
#include "reg.h"
#include "clib.h"

/* This is the header of the core dump file. */

static struct user u_hdr;

/* If there are two or more heap objects, this structure will describe
   the heap objects, except for the first one. */

static struct _user_heapobjs u_heap_hdr;

/* Current size of the core dump file. */

static ULONG core_size;


/* Write SIZE bytes from from SRC to the core dump file HANDLE.
   Return the errno code. */

static ULONG core_write (ULONG handle, const void *src, ULONG size)
{
  ULONG rc, cbwritten;

  core_size += size;
  rc = DosWrite (handle, src, size, &cbwritten);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


/* Pad the core dump file HANDLE with zeros, up to offset OFF.  Return
   the errno code. */

static ULONG core_fill (ULONG handle, ULONG off)
{
  ULONG rc, n;
  char buffer[0x1000];

  if (off <= core_size)
    return 0;
  memset (buffer, 0, sizeof (buffer));
  while (off > core_size)
    {
      n = off - core_size;
      if (n > sizeof (buffer))
        n = sizeof (buffer);
      rc = core_write (handle, buffer, n);
      if (rc != 0)
        return rc;
    }
  return 0;
}


/* Dump a region of memory to the core dump file, at offset OFF.  BASE
   and END are the base and end addresses, respectively, of the
   region.  Return the errno code. */

static ULONG core_mem (ULONG handle, ULONG off, ULONG base, ULONG end)
{
  ULONG rc;

  rc = core_fill (handle, off);
  if (rc == 0 && base <= end)
    {
      /* The heap may be loaded from the EXE file by the guard page
         exception handler.  As DosRead doesn't work when invoked from
         inside DosWrite (in the exception handler), we force the
         pages to be read into (virtual) memory before writing them to
         the core dump file. */

      if (exe_heap)
        touch ((void *)base, end - base);
      rc = core_write (handle, (void *)base, end - base);
    }
  return rc;
}


/* Write core dump file.  HANDLE is the file handle of the core dump
   file.  Return an errno code.  Note that this function assumes that
   the file has been rewound.  Call either core_regs_e() or
   core_regs_i() before calling core_main(). */

ULONG core_main (ULONG handle)
{
  ULONG rc;

  /* Reset the size of the file, for core_fill(). */

  core_size = 0;

  /* Write the header. */

  rc = core_write (handle, &u_hdr, sizeof (u_hdr));
  if (rc != 0) return rc;

  /* Dump the data segment. */

  rc = core_mem (handle, u_hdr.u_data_off,
                 u_hdr.u_data_base, u_hdr.u_data_end);
  if (rc != 0) return rc;

  /* Dump the heap. */

  rc = core_mem (handle, u_hdr.u_heap_off,
                 u_hdr.u_heap_base, u_hdr.u_heap_brk);
  if (rc != 0) return rc;

  /* Dump the stack. */

  rc = core_mem (handle, u_hdr.u_stack_off,
                 u_hdr.u_stack_low, u_hdr.u_stack_end);
  if (rc != 0) return rc;

  /* Dump additional heap objects. */

  if (u_hdr.u_heapobjs_off != 0)
    {
      ULONG i;

      rc = core_fill (handle, u_hdr.u_heapobjs_off);
      if (rc != 0) return rc;
      rc = core_write (handle, &u_heap_hdr, sizeof (u_heap_hdr));
      if (rc != 0) return rc;
      for (i = 0; i < u_heap_hdr.u_count; ++i)
        {
          rc = core_mem (handle, u_heap_hdr.u_objs[i].u_off,
                         u_heap_hdr.u_objs[i].u_base,
                         u_heap_hdr.u_objs[i].u_brk);
          if (rc != 0) return rc;
        }
    }

  /* Truncate the file to the current position. */

  rc = DosSetFileSize (handle, core_size);
  return rc;
}


/* Return the size of the "core" image file to be written.  The return
   value need not be exact. */

static ULONG core_file_size (void)
{
  if (u_hdr.u_heapobjs_off == 0)
    return u_hdr.u_stack_off + u_hdr.u_stack_end - u_hdr.u_stack_low;
  else
    {
      int i = (int)u_heap_hdr.u_count - 1;
      return (u_heap_hdr.u_objs[i].u_off
              + u_heap_hdr.u_objs[i].u_brk - u_heap_hdr.u_objs[i].u_base);
    }
}


/* Write "core" image file and say "core dumped".  You should either
   call core_regs_e() or core_regs_i() before calling core_dump(). */

void core_dump (void)
{
  ULONG rc, action;
  HFILE handle;

  if (layout_flags & L_FLAG_LINK386)
    return;

  rc = DosOpen ("core", &handle, &action, core_file_size (), 0,
                OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
                OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE,
                (void *)0);
  if (rc != 0)
    return;
  core_main (handle);
  DosClose (handle);
  otext ("core dumped\r\n");
}


/* Set structure members for core_regs_e() and core_regs_i(). */

static void core_set_header (void)
{
  ULONG rc, sp;
  PTIB ptib;
  PPIB ppib;
  unsigned i;

  u_hdr.u_magic = UMAGIC;
  u_hdr.u_ar0 = (int *)(offsetof (struct user, u_regs) + KERNEL_U_ADDR);
  u_hdr.u_data_base = layout->data_base;
  u_hdr.u_data_end = layout->bss_end;
  if (heap_obj_count == 0)
    {
      u_hdr.u_heap_base = 0;
      u_hdr.u_heap_end = 0;
      u_hdr.u_heap_brk = 0;
    }
  else
    {
      u_hdr.u_heap_base = heap_objs[0].base;
      u_hdr.u_heap_end = heap_objs[0].end;
      u_hdr.u_heap_brk = heap_objs[0].brk;
    }

  /* Set the stack base and end addresses. */

  rc = DosGetInfoBlocks (&ptib, &ppib);
  if (rc != 0)
    {
      u_hdr.u_stack_base = stack_base;
      u_hdr.u_stack_end = stack_end;
    }
  else
    {
      u_hdr.u_stack_base = (ULONG)ptib->tib_pstack;
      u_hdr.u_stack_end = (ULONG)ptib->tib_pstacklimit;
    }

  /* Find the low end of the stack. */

  sp = (ULONG)&sp;
  if (sp >= u_hdr.u_stack_base && sp <= u_hdr.u_stack_end)
    u_hdr.u_stack_low = sp & ~0xfff;
  else
    u_hdr.u_stack_low = u_hdr.u_stack_end;

  /* Set offsets. */

  u_hdr.u_data_off = U_OFFSET;
  u_hdr.u_heap_off = (u_hdr.u_data_off
                      + ROUND_PAGE (u_hdr.u_data_end - u_hdr.u_data_base));
  u_hdr.u_stack_off = (u_hdr.u_heap_off
                       + ROUND_PAGE (u_hdr.u_heap_brk - u_hdr.u_heap_base));
  if (heap_obj_count > 1)
    {
      memset (&u_heap_hdr, 0, sizeof (u_heap_hdr));
      u_hdr.u_heapobjs_off
        = u_hdr.u_stack_off + ROUND_PAGE (u_hdr.u_stack_end
                                          - u_hdr.u_stack_low);
      u_heap_hdr.u_objs[0].u_off
        = u_hdr.u_heapobjs_off + ROUND_PAGE (sizeof (u_heap_hdr));
      u_heap_hdr.u_count = heap_obj_count - 1;
      for (i = 0; i < u_heap_hdr.u_count; ++i)
        {
          u_heap_hdr.u_objs[i].u_base = heap_objs[i+1].base;
          u_heap_hdr.u_objs[i].u_end = heap_objs[i+1].end;
          u_heap_hdr.u_objs[i].u_brk = heap_objs[i+1].brk;
          if (i != 0)
            u_heap_hdr.u_objs[i].u_off
              = (u_heap_hdr.u_objs[i-1].u_off
                 + ROUND_PAGE (u_heap_hdr.u_objs[i-1].u_brk
                               - u_heap_hdr.u_objs[i-1].u_base));
        }
    }
}


/* Copy registers from the stack to the u_hdr structure for
   core_main(). This is used by __raise(), __signal(), and __core().
   F points to the emx_syscall stack frame (for retrieving the values
   of the registers). */

void core_regs_i (syscall_frame *f)
{
  BYTE *ip, **sp;
  USHORT sel;

  /* Zero all bytes of the header structure. */

  memset (&u_hdr, 0, sizeof (u_hdr));

  /* Copy registers from the stack frame to the header. */

  u_hdr.u_regs[R_EFL] = f->e.eflags;
  u_hdr.u_regs[R_EAX] = f->e.eax;
  u_hdr.u_regs[R_EBX] = f->e.ebx;
  u_hdr.u_regs[R_ECX] = f->e.ecx;
  u_hdr.u_regs[R_EDX] = f->e.edx;
  u_hdr.u_regs[R_ESI] = f->e.esi;
  u_hdr.u_regs[R_EDI] = f->e.edi;
  u_hdr.u_regs[R_EBP] = f->e.ebp;

  /* Copy current values of the segment registers to the header. */

  __asm__ ("movw %%cs, %0" : "=g"(sel) : /**/); u_hdr.u_regs[R_CS] = sel;
  __asm__ ("movw %%ds, %0" : "=g"(sel) : /**/); u_hdr.u_regs[R_DS] = sel;
  __asm__ ("movw %%es, %0" : "=g"(sel) : /**/); u_hdr.u_regs[R_ES] = sel;
  __asm__ ("movw %%fs, %0" : "=g"(sel) : /**/); u_hdr.u_regs[R_FS] = sel;
  __asm__ ("movw %%gs, %0" : "=g"(sel) : /**/); u_hdr.u_regs[R_GS] = sel;
  __asm__ ("movw %%ss, %0" : "=g"(sel) : /**/); u_hdr.u_regs[R_SS] = sel;

  /* Fetch the instruction pointer and the stack pointer.  We add 4 to
     the stack pointer to compensate for the PUSHFD of emx_syscall. */

  ip = (BYTE *)f->e.eip;
  sp = (BYTE **)(f->e.esp + 4);

  /* Check whether we have been called from __syscall().  That should
     always be the case, but... We check only the lower 16 bits
     because we don't know the address of __syscall() in a DLL such as
     emxlibcs.dll. */

  if ((f->e.eip & 0xffff) == 12 + 5)
    {

      /* We have been called from __syscall() (see crt0.s for
         details).  Rewind the stack to the function which called
         __syscall(), to get better stack backtraces from the
         debugger. */

      ++sp;                     /* Remove CALL emx_syscall */
      ip = *sp++;               /* Get and remove EIP */

      /* EIP is now in the function which called __syscall().  This is
         one of the following functions:
 
         Name          ³  AL   ³ PUSH
         ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄ
         __core        ³  11H  ³ 1 (EBX)
         __raise       ³  0EH  ³ 0
         __signal      ³  0CH  ³ 0
         __sigprocmask ³  37H  ³ 1 (EBX)
      
         Note that this information has not changed since emx 0.8b.
      
         We look at the instructions to find out what to do:
      
         EIP-9: MOV     AL, syscall_no
         EIP-7: MOV     AH, 7FH
         EIP-5: CALL    __syscall
      
         Perhaps we should consult the layout table first to avoid
         accessing memory outside the text segment. */

      if (ip[-9] == 0xb0          /* MOV AL,          */
          && (ip[-8] == 0x0c      /*         __signal */
              || ip[-8] == 0x0e   /*         __raise  */
              || ip[-8] == 0x11   /*         __core   */
              || ip[-8] == 0x37)  /*         __sigprocmask */
          && ip[-7] == 0xb4       /* MOV AH,          */
          && ip[-6] == 0x7f       /*         7FH      */
          && ip[-5] == 0xe8)      /* CALL             */
        {
          /* Undo the PUSH of EBX for __core() and __sigprocmask(). */

          if (ip[-8] == 0x11 || ip[-8] == 0x37)
            ++sp;

          /* OK, we are in one of the syscall interface functions.
             Rewind the stack to the function which called the
             interface function (after that, EIP is supposed to point
             to a C function; this will keep GDB happy). */

          ip = *sp++;           /* Fetch next EIP */
        }
    }

  /* Put the stack pointer and the instruction pointer into the
     header. */

  u_hdr.u_regs[R_ESP] = (ULONG)sp;
  u_hdr.u_regs[R_UESP] = (ULONG)sp;
  u_hdr.u_regs[R_EIP] = (ULONG)ip;

  /* TODO: 387 state */

  core_set_header ();
}


/* Copy registers from the exception context record to the u_hdr
   structure for core_main().  CONTEXT points to the exception context
   record. */

void core_regs_e (CONTEXTRECORD *context)
{
  ULONG (*p_DosQueryModFromEIP) (HMODULE *, ULONG *, ULONG, PCHAR, ULONG *,
                                 ULONG);
  HMODULE hmod_doscall1, hmod_eip;
  char obj[9];
  ULONG obj_eip, off_eip;

  /* Zero all bytes of the header structure. */

  memset (&u_hdr, 0, sizeof (u_hdr));

  /* If the control registers are available, copy their values to the
     header. */

  if (context->ContextFlags & CONTEXT_CONTROL)
    {
      u_hdr.u_regs[R_CS] = context->ctx_SegCs;
      u_hdr.u_regs[R_SS] = context->ctx_SegSs;
      u_hdr.u_regs[R_EIP] = context->ctx_RegEip;
      u_hdr.u_regs[R_ESP] = context->ctx_RegEsp;
      u_hdr.u_regs[R_EBP] = context->ctx_RegEbp;
      u_hdr.u_regs[R_EFL] = context->ctx_EFlags;
    }

  /* If the segment registers are available, copy their values to the
     header. */

  if (context->ContextFlags & CONTEXT_SEGMENTS)
    {
      u_hdr.u_regs[R_DS] = context->ctx_SegDs;
      u_hdr.u_regs[R_ES] = context->ctx_SegEs;
      u_hdr.u_regs[R_FS] = context->ctx_SegFs;
      u_hdr.u_regs[R_GS] = context->ctx_SegGs;
    }

  /* If the integer registers are available, copy their values to the
     header. */

  if (context->ContextFlags & CONTEXT_INTEGER)
    {
      u_hdr.u_regs[R_EAX] = context->ctx_RegEax;
      u_hdr.u_regs[R_EBX] = context->ctx_RegEbx;
      u_hdr.u_regs[R_ECX] = context->ctx_RegEcx;
      u_hdr.u_regs[R_EDX] = context->ctx_RegEdx;
      u_hdr.u_regs[R_ESI] = context->ctx_RegEsi;
      u_hdr.u_regs[R_EDI] = context->ctx_RegEdi;
    }

  /* Set UESP from ESP. */

  u_hdr.u_regs[R_UESP] = u_hdr.u_regs[R_ESP];

  /* Store the coprocessor state. */

  if (context->ContextFlags & CONTEXT_FLOATING_POINT)
    {
      u_hdr.u_fpvalid = 0xff;
      memcpy (&u_hdr.u_fpstate.state, &context->ctx_env,
              sizeof (u_hdr.u_fpstate.state));
    }

  /* Find the module name for EIP.  Note that DosQueryModFromEIP API
     wasn't available in plain OS/2 2.0. */

  if (DosLoadModule (obj, sizeof (obj), "DOSCALL1", &hmod_doscall1) == 0)
    {
      memset (obj, 0, sizeof (obj));
      if (DosQueryProcAddr (hmod_doscall1, 360, NULL,
                            (PPFN)&p_DosQueryModFromEIP) == 0
          && p_DosQueryModFromEIP (&hmod_eip, &obj_eip, sizeof (obj),
                                   obj, &off_eip, context->ctx_RegEip) == 0)
        memcpy (u_hdr.u_module, obj, sizeof (u_hdr.u_module));
      DosFreeModule (hmod_doscall1);
    }

  core_set_header ();
}
