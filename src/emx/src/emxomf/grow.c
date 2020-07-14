/* grow.c -- Growing arrays, growing buffers and string pools
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


/* Growing arrays.  Such an array consists of two parts: A struct grow
   which is used in this module for controlling the object, and a
   pointer (the `associated pointer') of the desired type to the
   array.  The associated pointer may change when calling one of the
   functions of these module for the struct grow of that array.*/

/* Growing buffers.  A growing buffer is an array of bytes.  There are
   functions for putting various binary types into the buffer, while
   enlarging the buffer as required. */

/* String pool.  A string pool contains null-terminated strings of
   arbitrary length.  There are no duplicate strings in a string
   pool. */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "defs.h"
#include "grow.h"

/* A program using this module has to define the following two
   functions: */

extern void *xrealloc (void *ptr, size_t n);
extern void *xmalloc (size_t n);

/* Initialize a growing array.  G points to the struct grow used for
   controlling the array, PTR is a pointer to the associated pointer
   of the desired type -- that pointer is used for accessing the
   array.  SIZE is the size of one array element, INC is the number of
   array elements to add when growing the array.  The smaller SIZE is,
   the bigger you should choose INC. */

void grow_init (struct grow *g, void *ptr, size_t size, int inc)
{
  g->count = 0;
  g->alloc = 0;
  g->ptr = ptr;
  g->size = size;
  g->inc = inc;
  *g->ptr = NULL;
}


/* Deallocate a growing array.  Do not use the associated array
   pointer after calling this function. */

void grow_free (struct grow *g)
{
  if (g->ptr != NULL)
    {
      if (*g->ptr != NULL)
        free (*g->ptr);
      *g->ptr = NULL;
    }
  g->ptr = NULL;
  g->count = 0;
  g->alloc = 0;
}


/* Grow the growing array G to NEW_COUNT elements.  If the array is
   already big enough, nothing is done.  Otherwise, the array is
   enlarged by at least INC (of grow_init()) elements.  The associated
   array pointer may change. */

void grow_to (struct grow *g, int new_count)
{
  if (new_count > g->alloc)
    {
      g->alloc += g->inc;
      if (g->alloc < new_count)
        g->alloc = new_count;
      *g->ptr = xrealloc (*g->ptr, g->alloc * g->size);
    }
}


/* Grow the growing array G to make it big enough for INC additional
   elements.  If the array is already big enough, nothing is done.
   Otherwise, the array is enlarged by at least INC (of grow_init())
   elements.  The associated array pointer may change. */

void grow_by (struct grow *g, int inc)
{
  grow_to (g, g->count + inc);
}


/* Initialize a growing buffer.  B is a pointer to the buffer
   descriptor. */

void buffer_init (struct buffer *b)
{
  b->size = 0;
  b->alloc = 0;
  b->buf = NULL;
}


/* Deallocate a growing buffer.  Do not use the buffer after calling
   this function. */

void buffer_free (struct buffer *b)
{
  if (b->buf != NULL)
    {
      free (b->buf);
      b->buf = NULL;
    }
}


/* Grow the buffer B by N bytes.  The buffer size is always an
   integral multiple of 512.  The buffer may move in memory. */

static void buffer_grow_by (struct buffer *b, size_t n)
{
  if (b->size + n > b->alloc)
    {
      n = (n | 0x1ff) + 1;
      b->alloc += n;
      b->buf = xrealloc (b->buf, b->alloc);
    }
}


/* Append the 8-bit byte X to the buffer B. */

void buffer_byte (struct buffer *b, byte x)
{
  buffer_grow_by (b, 1);
  b->buf[b->size++] = x;
}


/* Append the 16-bit word X to the buffer B.  The LSB comes first. */

void buffer_word (struct buffer *b, word x)
{
  buffer_grow_by (b, 2);
  b->buf[b->size++] = x & 0xff;
  b->buf[b->size++] = (x >> 8) & 0xff;
}


/* Append the 32-bit word X to the buffer B.  The LSB comes first. */

void buffer_dword (struct buffer *b, dword x)
{
  buffer_grow_by (b, 4);
  b->buf[b->size++] = x & 0xff;
  b->buf[b->size++] = (x >> 8) & 0xff;
  b->buf[b->size++] = (x >> 16) & 0xff;
  b->buf[b->size++] = (x >> 24) & 0xff;
}


/* Append LEN bytes at MEM to the buffer B. */

void buffer_mem (struct buffer *b, const void *mem, int len)
{
  buffer_grow_by (b, len);
  memcpy (b->buf + b->size, mem, len);
  b->size += len;
}


/* Append the string STR to the buffer B.  The string is preceded by
   its length (one byte).  If the string length exceeds 255
   characters, the string is truncated to 255 characters. */

void buffer_nstr (struct buffer *b, const char *str)
{
  int len;

  if (str == NULL)
    buffer_byte (b, 0);
  else
    {
      len = strlen (str);
      if (len > 255)
        len = 255;
      buffer_byte (b, len);
      buffer_mem (b, str, len);
    }
}


/* Append the string STR to the buffer B.  The string is preceded by
   its length (one byte or two bytes).  If the string length exceeds
   32767 characters, the string is truncated to 32767 characters. */

void buffer_enc (struct buffer *b, const char *str)
{
  size_t len;

  len = strlen (str);
  if (len > 0x7fff)
    len = 0x7fff;
  if (len <= 0x7f)
    buffer_byte (b, len);
  else
    {
      buffer_byte (b, (len >> 8) | 0x80);
      buffer_byte (b, len & 0xff);
    }
  buffer_mem (b, str, len);
}


/* Patch the 16-bit word in the buffer B at offset INDEX to X. */

void buffer_patch_word (struct buffer *b, int index, word x)
{
  b->buf[index+0] = x & 0xff;
  b->buf[index+1] = (x >> 8) & 0xff;
}


/* The size of the string pool hash table.  Should be prime. */

#define STRPOOL_HASH_SIZE 211

/* This structure holds one string.  Note that the first character of
   the string is part of this structure. */

struct string
{
  struct string *next;          /* Pointer to next string in same bucket */
  int len;                      /* The string length. */
  char string[1];               /* The string */
};

/* A string pool consists of its hash table. */

struct strpool
{
  struct string *table[STRPOOL_HASH_SIZE];
};


/* Create and return a new string pool.  Initially, the string pool is
   empty. */

struct strpool *strpool_init (void)
{
  int i;
  struct strpool *p;

  p = xmalloc (sizeof (*p));
  for (i = 0; i < STRPOOL_HASH_SIZE; ++i)
    p->table[i] = NULL;
  return p;
}

/* Destroy the string pool P.  The hash table and the strings are
   deallocated. */

void strpool_free (struct strpool *p)
{
  struct string *v1, *v2;
  int i;

  for (i = 0; i < STRPOOL_HASH_SIZE; ++i)
    for (v1 = p->table[i]; v1 != NULL; v1 = v2)
      {
        v2 = v1->next;
        free (v1);
      }
  free (p);
}


/* Add the string S of LEN characters to the string pool P.  The
   string must not contain null characters. A pointer to a string of
   the string pool is returned.  If a string identical to S already
   exists in the string pool, a pointer to that string is returned.
   Otherwise, a new string entry is added to the string pool. */

const char *strpool_addn (struct strpool *p, const char *s, int len)
{
  unsigned hash;
  int i;
  struct string *v;

  hash = 0;
  for (i = 0; i < len; ++i)
    hash = hash * 65599 + s[i];
  hash %= STRPOOL_HASH_SIZE;
  for (v = p->table[hash]; v != NULL; v = v->next)
    if (v->len == len && memcmp (v->string, s, len) == 0)
      return v->string;
  v = xmalloc (sizeof (*v) + len);
  assert(((uintptr_t)v & (sizeof(int) - 1)) == 0);
  memcpy (v->string, s, len);
  v->string[len] = 0;
  v->len = len;
  v->next = p->table[hash];
  p->table[hash] = v;
  return v->string;
}


/* Add the null-terminated string S to the string pool P.  A pointer
   to a string of the string pool is returned.  If a string identical
   to S already exists in the string pool, a pointer to that string is
   returned.  Otherwise, a new string entry is added to the string
   pool. */

const char *strpool_add (struct strpool *p, const char *s)
{
  if (s == NULL)
    return NULL;
  return strpool_addn (p, s, strlen (s));
}

/* Add the uppercased string S of LEN characters to the string pool P.
   See strpool_addn for more details. */

const char *strpool_addnu (struct strpool *p, const char *s, int len)
{
  unsigned hash;
  int i;
  struct string *v;

  hash = 0;
  for (i = 0; i < len; ++i)
      hash = hash * 65599 + toupper(s[i]);
  hash %= STRPOOL_HASH_SIZE;
  for (v = p->table[hash]; v != NULL; v = v->next)
    if (v->len == len)
      {
        i = len;
        while (--i >= 0)
          if (toupper(s[i]) != v->string[i])
            break;
        if (i < 0)
            return v->string;
      }
  v = xmalloc (sizeof (*v) + len);
  assert(((uintptr_t)v & (sizeof(int) - 1)) == 0);
  memcpy (v->string, s, len);
  v->string[len] = 0;
  v->len = len;
  strupr (v->string);
  v->next = p->table[hash];
  p->table[hash] = v;
  return v->string;
}

/* Add the null-terminated uppercased string S to the string pool P.
   See strpool_add for more details. */

const char *strpool_addu (struct strpool *p, const char *s)
{
  if (s == NULL)
    return NULL;
  return strpool_addnu (p, s, strlen (s));
}

/* Get the length of a string pool string.  This is very quick since we store
   the lenght before the string data.  */
int strpool_len (const char *s)
{
    assert(((uintptr_t)s & (sizeof(int) - 1)) == 0);
    if (!s)
      return 0;
    return ((size_t *)s)[-1];
}

