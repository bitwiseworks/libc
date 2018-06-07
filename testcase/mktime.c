/* confdefs.h.  */

#define PACKAGE_NAME "GNU coreutils"
#define PACKAGE_TARNAME "coreutils"
#define PACKAGE_VERSION "5.2.1"
#define PACKAGE_STRING "GNU coreutils 5.2.1"
#define PACKAGE_BUGREPORT "bug-coreutils@gnu.org"
#define PACKAGE "coreutils"
#define VERSION "5.2.1"
#define _GNU_SOURCE 1
#define STDC_HEADERS 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRINGS_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_UNISTD_H 1
#define __EXTENSIONS__ 1
#define HAVE_UNAME 1
#define PROTOTYPES 1
#define STDC_HEADERS 1
#define HAVE_STRING_H 1
#define HAVE_LONG_DOUBLE 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_FLOAT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MEMORY_H 1
#define HAVE_NETDB_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STATFS_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMEB_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_SYSLOG_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_UNISTD_H 1
#define HAVE_UTIME_H 1
#define HAVE_DIRENT_H 1
#define HAVE_STRUCT_STAT_ST_BLOCKS 1
#define HAVE_ST_BLOCKS 1
#define GETGROUPS_T gid_t
#define RETSIGTYPE void
#define HAVE_INTTYPES_H_WITH_UINTMAX 1
#define HAVE_STDINT_H_WITH_UINTMAX 1
#define HAVE_UNSIGNED_LONG_LONG 1
#define HAVE_UINTMAX_T 1
#define HOST_OPERATING_SYSTEM "OS/2"
#define HAVE_SYS_TIME_H 1
#define HAVE_UTIME_H 1
#define TIME_WITH_SYS_TIME 1
#define HAVE_STRUCT_UTIMBUF 1
#define HAVE_STRUCT_DIRENT_D_TYPE 1
#define D_INO_IN_DIRENT 1
#define HAVE_GRP_H 1
#define HAVE_MEMORY_H 1
#define HAVE_PWD_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STDLIB_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_DECL_FREE 1
#define HAVE_DECL_GETENV 1
#define HAVE_DECL_GETEUID 1
#define HAVE_DECL_GETGRGID 1
#define HAVE_DECL_GETLOGIN 1
#define HAVE_DECL_GETPWUID 1
#define HAVE_DECL_GETUID 1
#define HAVE_DECL_GETUTENT 0
#define HAVE_DECL_LSEEK 1
#define HAVE_DECL_MALLOC 1
#define HAVE_DECL_MEMCHR 1
#define HAVE_DECL_MEMRCHR 0
#define HAVE_DECL_NANOSLEEP 1
#define HAVE_DECL_REALLOC 1
#define HAVE_DECL_STPCPY 1
#define HAVE_DECL_STRNDUP 1
#define HAVE_DECL_STRNLEN 1
#define HAVE_DECL_STRSTR 1
#define HAVE_DECL_STRTOUL 1
#define HAVE_DECL_STRTOULL 1
#define HAVE_DECL_TTYNAME 1
#define getline gnu_getline
#define HAVE__BOOL 1
#define HAVE_STDBOOL_H 1
#define HAVE_FCNTL_H 1
#define HAVE_UNISTD_H 1
#define HAVE_DECL_GETENV 1
#define HAVE_MKSTEMP 1
#define mkstemp rpl_mkstemp
#define HAVE_STDINT_H 1
#define HAVE_GETTIMEOFDAY 1
#define FILESYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX 0
#define FILESYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR 0
#define HAVE_LONG_FILE_NAMES 1
#define D_INO_IN_DIRENT 1
#define HAVE_PATHCONF 1
#define HAVE_NETDB_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_DECL_STRERROR_R 1
#define HAVE_STRERROR_R 1
#define HAVE_ISASCII 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_STRUCT_STAT_ST_BLOCKS 1
#define HAVE_ST_BLOCKS 1
#define HAVE_ALLOCA_H 1
#define HAVE_ALLOCA 1
#define HAVE_ATEXIT 1
#define HAVE_DUP2 1
#define HAVE_GETGROUPS 1
#define HAVE_DECL_EUIDACCESS 0
#define mbstate_t int
#define HAVE_DECL_GETENV 1
#define HAVE_MEMPCPY 1
#define fnmatch gnu_fnmatch
#define HAVE_UNAME 1
#define C_GETLOADAVG 1
#define HAVE_SETLOCALE 1
#define HAVE_GETPASS 1
#define HAVE_MEMCHR 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_STDLIB_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_UNISTD_H 1
/* end confdefs.h.  */
/* Test program from Paul Eggert and Tony Leneis.  */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if !HAVE_ALARM
# define alarm(X) /* empty */
#endif

/* Work around redefinition to rpl_putenv by other config tests.  */
#undef putenv

static time_t time_t_max;
static time_t time_t_min;

/* Values we'll use to set the TZ environment variable.  */
static char *tz_strings[] = {
  (char *) 0, "TZ=GMT0", "TZ=JST-9",
  "TZ=EST+3EDT+2,M10.1.0/00:00:00,M2.3.0/00:00:00"
};
#define N_STRINGS (sizeof (tz_strings) / sizeof (tz_strings[0]))

/* Fail if mktime fails to convert a date in the spring-forward gap.
   Based on a problem report from Andreas Jaeger.  */
static void
spring_forward_gap ()
{
  /* glibc (up to about 1998-10-07) failed this test. */
  struct tm tm;

  /* Use the portable POSIX.1 specification "TZ=PST8PDT,M4.1.0,M10.5.0"
     instead of "TZ=America/Vancouver" in order to detect the bug even
     on systems that don't support the Olson extension, or don't have the
     full zoneinfo tables installed.  */
  putenv ("TZ=PST8PDT,M4.1.0,M10.5.0");

  tm.tm_year = 98;
  tm.tm_mon = 3;
  tm.tm_mday = 5;
  tm.tm_hour = 2;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  if (mktime (&tm) == (time_t)-1)
    exit (1);
}

static void
mktime_test1 (now)
     time_t now;
{
  struct tm *lt;
  if ((lt = localtime (&now)) && mktime (lt) != now)
    exit (1);
}

static void
mktime_test (now)
     time_t now;
{
  mktime_test1 (now);
  mktime_test1 ((time_t) (time_t_max - now));
  mktime_test1 ((time_t) (time_t_min + now));
}

static void
irix_6_4_bug ()
{
  /* Based on code from Ariel Faigon.  */
  struct tm tm;
  tm.tm_year = 96;
  tm.tm_mon = 3;
  tm.tm_mday = 0;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  mktime (&tm);
  if (tm.tm_mon != 2 || tm.tm_mday != 31)
    exit (1);
}

static void
bigtime_test (j)
     int j;
{
  struct tm tm;
  time_t now;
  tm.tm_year = tm.tm_mon = tm.tm_mday = tm.tm_hour = tm.tm_min = tm.tm_sec = j;
  now = mktime (&tm);
  if (now != (time_t) -1)
    {
      struct tm *lt = localtime (&now);
      if (! (lt
	     && lt->tm_year == tm.tm_year
	     && lt->tm_mon == tm.tm_mon
	     && lt->tm_mday == tm.tm_mday
	     && lt->tm_hour == tm.tm_hour
	     && lt->tm_min == tm.tm_min
	     && lt->tm_sec == tm.tm_sec
	     && lt->tm_yday == tm.tm_yday
	     && lt->tm_wday == tm.tm_wday
	     && ((lt->tm_isdst < 0 ? -1 : 0 < lt->tm_isdst)
		  == (tm.tm_isdst < 0 ? -1 : 0 < tm.tm_isdst))))
	exit (1);
    }
}

int
main ()
{
  time_t t, delta;
  int i, j;

  /* This test makes some buggy mktime implementations loop.
     Give up after 60 seconds; a mktime slower than that
     isn't worth using anyway.  */
  alarm (60);

  for (time_t_max = 1; 0 < time_t_max; time_t_max *= 2)
    continue;
  time_t_max--;
  if ((time_t) -1 < 0)
    for (time_t_min = -1; (time_t) (time_t_min * 2) < 0; time_t_min *= 2)
      continue;
  delta = time_t_max / 997; /* a suitable prime number */
  for (i = 0; i < N_STRINGS; i++)
    {
      if (tz_strings[i])
	putenv (tz_strings[i]);

      for (t = 0; t <= time_t_max - delta; t += delta)
	mktime_test (t);
      mktime_test ((time_t) 1);
      mktime_test ((time_t) (60 * 60));
      mktime_test ((time_t) (60 * 60 * 24));

      for (j = 1; 0 < j; j *= 2)
	bigtime_test (j);
      bigtime_test (j - 1);
    }
  irix_6_4_bug ();
  spring_forward_gap ();
  exit (0);
}

