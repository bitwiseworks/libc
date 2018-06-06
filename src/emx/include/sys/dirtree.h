/* sys/dirtree.h (emx+gcc) */

#ifndef _SYS_DIRTREE_H
#define _SYS_DIRTREE_H

#include <sys/cdefs.h>
#include <sys/_types.h>

#if !defined(_TIME_T_DECLARED) && !defined(_TIME_T)
typedef	__time_t	time_t;
#define	_TIME_T_DECLARED
#define _TIME_T
#endif

#if !defined(_OFF_T_DECLARED) && !defined(_OFF_T)
typedef	__off_t		off_t;		/* file offset */
#define	_OFF_T_DECLARED
#define _OFF_T
#endif

struct _dt_node
{
  struct _dt_node *next;   /* Pointer to next entry of same level */
  struct _dt_node *sub;    /* Pointer to next level (child) */
  char *name;              /* Name */
  off_t size;              /* File size */
  long user;               /* Available for user */
  time_t mtime;            /* Timestamp for last update */
  unsigned char attr;      /* Attributes */
};

struct _dt_tree
{
  struct _dt_node *tree;
  char *strings;
};

#define _DT_TREE     0x4000
#define _DT_NOCPDIR  0x8000

__BEGIN_DECLS
void    _dt_free (struct _dt_tree *dt);
struct _dt_tree *_dt_read (const char *dir, const char *mask,
    unsigned flags);
void    _dt_sort (struct _dt_tree *dt, const char *spec);
int     _dt_split (const char *src, char *dir, char *mask);
__END_DECLS

#endif /* !_SYS_DIRTREE_H */
