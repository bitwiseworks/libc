/* omflib0.h (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

/* Private header file for the emx OMFLIB library. */

#define FALSE 0
#define TRUE  1

#ifndef _BYTE_WORD_DWORD
#define _BYTE_WORD_DWORD
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
#endif /* _BYTE_WORD_DWORD */

#define FLAG_DELETED  0x0001

enum omf_state
{
  OS_EMPTY,                     /* Empty module */
  OS_SIMPLE,                    /* Alias or import definition only */
  OS_OTHER,                     /* Any other records present */
};

struct omfmod
{
  char *name;
  word page;
  word flags;
};

struct pubsym
{
  word page;
  char *name;
};

struct omflib
{
  FILE *f;
  long dict_offset;
  int page_size;
  int dict_blocks;
  int flags;
  struct omfmod *mod_tab;
  int mod_alloc;
  int mod_count;
  byte *dict;
  int block_index;
  int block_index_delta;
  int bucket_index;
  int bucket_index_delta;
  struct pubsym *pub_tab;
  int pub_alloc;
  int pub_count;
  char output;
  word mod_page;
  enum omf_state state;
  char mod_name[256+1];
};

struct ptr
{
  byte *ptr;
  int len;
};

#pragma pack(1)

struct omf_rec
{
  byte rec_type;
  word rec_len;
};

struct lib_header
{
  byte rec_type;
  word rec_len;
  dword dict_offset;
  word dict_blocks;
  byte flags;
};

#pragma pack()

int omflib_set_error (char *error);
int omflib_read_dictionary (struct omflib *p, char *error);
void omflib_hash (struct omflib *p, const byte *name);
int omflib_pad (FILE *f, int size, int force, char *error);
int omflib_copy_module (struct omflib *dst_lib, FILE *dst_file,
    struct omflib *src_lib, FILE *src_file, const char *mod_name, char *error);
int omflib_make_mod_tab (struct omflib *p, char *error);
int omflib_pubdef (struct omf_rec *rec, byte *buf, word page,
    int (*walker)(const char *name, char *error), char *error);
int omflib_impdef (struct omf_rec *rec, byte *buf, word page,
    int (*walker)(const char *name, char *error), char *error);
int omflib_alias (struct omf_rec *rec, byte *buf, word page,
    int (*walker)(const char *name, char *error), char *error);
