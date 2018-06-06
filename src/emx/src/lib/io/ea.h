/* ea.h (emx+gcc) */

struct _ead_data
{
  int count;                    /* Number of EAs */
  int max_count;                /* Number of pointers allocated for INDEX */
  int total_value_size;         /* Total size of values */
  int total_name_len;           /* Total length of names w/o null characters */
  size_t buffer_size;           /* Number of bytes allocated for BUFFER */
  PFEA2LIST buffer;             /* Buffer holding FEA2LIST */
  PFEA2 *index;                 /* Index for BUFFER */
};

void _ea_set_errno (ULONG rc);
int _ea_write (const char *path, int handle, PFEA2LIST src);

int _ead_make_index (struct _ead_data *ead, int new_count);
int _ead_size_buffer (struct _ead_data *ead, int new_size);
int _ead_enum (struct _ead_data *ead, const char *path, int handle,
    int (*function)(struct _ead_data *ead, PDENA2 pdena, void *arg),
    void *arg);

#define _EA_ALIGN(i)     (((i) + 3) & -4)
#define _EA_SIZE1(n, v)  _EA_ALIGN (sizeof (FEA2) + (n) + (v))
#define _EA_SIZE2(p)     _EA_SIZE1 ((p)->cbName, (p)->cbValue)
