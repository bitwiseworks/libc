/* sys/ea.h (emx+gcc) */

#ifndef _SYS_EA_H
#define _SYS_EA_H

#if defined (__cplusplus)
extern "C" {
#endif

struct _ea
{
  int flags;
  int size;
  void *value;
};

void _ea_free (struct _ea *);
int _ea_get (struct _ea *, const char *, int, const char *);
int _ea_put (struct _ea *, const char *, int, const char *);
int _ea_remove (const char *, int, const char *);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_EA_H */
