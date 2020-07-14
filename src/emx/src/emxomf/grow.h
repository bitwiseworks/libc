/* grow.h -- Growing arrays, growing buffers and string pools
   Copyright (c) 1993-1995 Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#define GROW_INIT   {0, 0, 0, 0, NULL}
#define BUFFER_INIT {0, 0, NULL}

struct grow
{
  int count;
  int alloc;
  int size;
  int inc;
  void **ptr;
};

struct buffer
{
  size_t size;
  size_t alloc;
  byte *buf;
};

/* Note: PTR is a pointer to a pointer */
void grow_init (struct grow *g, void *ptr, size_t size, int inc);
void grow_free (struct grow *g);
void grow_to (struct grow *g, int new_count);
void grow_by (struct grow *g, int inc);

void buffer_init (struct buffer *b);
void buffer_free (struct buffer *b);
void buffer_byte (struct buffer *b, byte x);
void buffer_word (struct buffer *b, word x);
void buffer_dword (struct buffer *b, dword x);
void buffer_mem (struct buffer *b, const void *mem, int len);
void buffer_nstr (struct buffer *b, const char *str);
void buffer_enc (struct buffer *b, const char *str);
void buffer_patch_word (struct buffer *b, int index, word x);

struct strpool *strpool_init (void);
void strpool_free (struct strpool *p);
const char *strpool_addn (struct strpool *p, const char *s, int len);
const char *strpool_add (struct strpool *p, const char *s);
const char *strpool_addnu (struct strpool *p, const char *s, int len);
const char *strpool_addu (struct strpool *p, const char *s);
int strpool_len (const char *s);

