/* _tmp.h (emx+gcc) */

#define IDX_LO 10000000
#define IDX_HI 99999999

extern int _tmpidx;

int _tmpidxnam (char *string);

void _tmpidx_lock (void);
void _tmpidx_unlock (void);

#define TMPIDX_LOCK   _tmpidx_lock ()
#define TMPIDX_UNLOCK _tmpidx_unlock ()
