/** @file
 * UNKNOWN.
 *
 * For compatibilty with some unknown system, but chiefly for fixing
 * trouble caused by configure of GNU coreutils which doesn't include
 * sys/mount.h if sys/statvfs.h is found.
 */
#include <sys/types.h>
#include <sys/mount.h>

