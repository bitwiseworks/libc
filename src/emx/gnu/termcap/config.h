/* config.h */

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <gnu/termcap.h>

#define EMX
#define TERMCAP_FILE    "/@unixroot/etc/termcap.dat"
#define HAVE_STRING_H
#define STDC_HEADERS
#define HAVE_UNISTD_H
#define STDC_HEADERS
#if !#machine(i386)
#define NO_ARG_ARRAY
#endif
