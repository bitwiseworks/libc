/* sys/ead.h (emx+gcc) */

#ifndef _SYS_EAD_H
#define _SYS_EAD_H

#if defined (__cplusplus)
extern "C" {
#endif

#define _EAD_MERGE       0x0001

typedef struct _ead_data *_ead;

int _ead_add (_ead, const char *, int, const void *, int);
void _ead_clear (_ead);
int _ead_copy (_ead, _ead, int);
int _ead_count (_ead);
_ead _ead_create (void);
int _ead_delete (_ead, int);
void _ead_destroy (_ead);
int _ead_fea2list_size (_ead);
void *_ead_fea2list_to_fealist (const void *);
void *_ead_fealist_to_fea2list (const void *);
int _ead_find (_ead, const char *);
const void *_ead_get_fea2list (_ead);
int _ead_get_flags (_ead, int);
const char *_ead_get_name (_ead, int);
const void *_ead_get_value (_ead, int);
int _ead_name_len (_ead, int);
int _ead_read (_ead, const char *, int, int);
int _ead_replace (_ead, int, int, const void *, int);
void _ead_sort (_ead);
int _ead_use_fea2list (_ead, const void *);
int _ead_value_size (_ead, int);
int _ead_write (_ead, const char *, int, int);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_EAD_H */
