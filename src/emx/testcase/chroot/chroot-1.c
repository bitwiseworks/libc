/*
 * Exercise /@unixroot-based dir creation and realpath() before and after
 * chroot().
 *
 * AFter chroot(), also exercises /-based dir creation and realpath(), as well
 * as leaving chrooted state.
 */

#include <process.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static char full_exe_name[PATH_MAX];

static char *exe_name = NULL;
static int is_child = 0;

static char *unixroot;
static char *unixroot_chrooted;
static char *rootdir;

static const char *unixroot_test = "/@unixroot/test";
static char unixroot_test_native[PATH_MAX * 2] = "";

static const char *rootdir_test = "/test";
static char rootdir_test_native[PATH_MAX * 2] = "";

static int compare_path(const char *dir, const char *real)
{
    char *resolved;

    resolved = realpath(dir, NULL);
    if (resolved == NULL)
    {
        fprintf(stderr, "realpath(%s): %s\n", dir, strerror(errno));
        return 1;
    }

    printf("realpath(%s) = %s\n", dir, resolved);
    if (strcmp(resolved, real) != 0)
    {
        fprintf(stderr, "realpath expected to be %s\n", real);
        free(resolved);
        return 1;
    }

    free(resolved);
    return 0;
}

static int test_unixroot(const char *dir, const char *real, const char *native)
{
    struct stat st;
    int rc = 0;

    if (mkdir(dir, 0777) != 0)
    {
        fprintf(stderr, "mkdir(%s): %s\n", dir, strerror(errno));
        compare_path(dir, "???");
        return 1;
    }

    if (compare_path(dir, real) != 0)
        return 1;

    if (stat(native, &st) != 0)
    {
        fprintf(stderr, "stat(%s): %s\n", native, strerror(errno));
        rc = 1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        fprintf(stderr, "stat(%s): not a directory\n", native);
        rc = 1;
    }

    if (rmdir(native) != 0)
    {
        fprintf(stderr, "rmdir(%s): %s\n", native, strerror(errno));
        rc = 1;
    }

    return rc;
}

static int test_unixroot_env()
{
    int is_rooted_child = is_child && rootdir;

    if (unixroot && *unixroot)
    {
        if (!(unixroot_chrooted && *unixroot_chrooted) && !is_rooted_child)
        {
            struct stat st;
            if (stat(unixroot, &st) != 0)
            {
                fprintf(stderr, "stat(%s): %s\n", unixroot, strerror(errno));
                return 1;
            }
            if (!S_ISDIR(st.st_mode))
            {
                fprintf(stderr, "stat(%s): not a directory\n", unixroot);
                return 1;
            }

            if (compare_path("/@unixroot", unixroot) != 0)
                return 1;

            if (test_unixroot(unixroot_test, unixroot_test_native, unixroot_test_native) != 0)
                return 1;
        }
        else
        {
            if (compare_path("/", "/") != 0)
                return 1;
            if (compare_path("/@unixroot", "/") != 0)
                return 1;

            if (!is_rooted_child)
            {
                if (test_unixroot(rootdir_test, rootdir_test, unixroot_test_native) != 0)
                    return 1;
                if (test_unixroot(unixroot_test, rootdir_test, unixroot_test_native) != 0)
                    return 1;
            }
            else
            {
                if (test_unixroot(rootdir_test, rootdir_test, rootdir_test_native) != 0)
                    return 1;
                if (test_unixroot(unixroot_test, rootdir_test, rootdir_test_native) != 0)
                    return 1;
            }
        }
    }
    else
    {
        if (!is_rooted_child)
        {
            struct stat st;
            if (stat("/@unixroot", &st) == 0)
            {
                fprintf(stderr, "stat(/@unixroot) succeeded (should fail)\n");
                compare_path("/@unixroot", "???");
                return 1;
            }

            printf("stat(/@unixroot): %s\n", strerror(errno));

            if (compare_path("/@unixroot", "???") == 0)
                return 1;
        }
        else
        {
            if (compare_path("/", "/") != 0)
                return 1;
            if (compare_path("/@unixroot", "/") != 0)
                return 1;
            if (test_unixroot(rootdir_test, rootdir_test, rootdir_test_native) != 0)
                return 1;
            if (test_unixroot(unixroot_test, rootdir_test, rootdir_test_native) != 0)
                return 1;
        }
    }

    return 0;
}

static int test_child(char *rootdir)
{
    char *args[] = { exe_name, "child", rootdir, NULL };

    printf("Calling child:\n");
    int rc = spawnv(P_WAIT, full_exe_name, args);
    if (rc < 0)
    {
        fprintf(stderr, "spawnv(%s): %s\n", full_exe_name, strerror(errno));
        return 1;
    }

    printf("Child returned %d\n\n", rc);
    return rc;
}

static int test_chroot()
{
    int fcwd = -1;

    const char *test_chroot_null = getenv("TEST_CHROOT_NULL");
    int escape_with_null = !(unixroot && *unixroot) || (test_chroot_null && *test_chroot_null);

    if (!escape_with_null)
    {
        if ((fcwd = open(".", O_RDONLY)) == -1)
        {
            fprintf(stderr, "open(.): %s\n", strerror(errno));
            return 1;
        }

        if (chdir("/@unixroot"))
        {
            fprintf(stderr, "chdir(/@unixroot): %s\n", strerror(errno));
            return 1;
        }
    }

    if (chroot(rootdir) != 0)
    {
        fprintf(stderr, "chroot(%s): %s\n", rootdir, strerror(errno));
        return 1;
    }

    printf("After chroot(%s):\n", rootdir);

    if (compare_path("/", "/") != 0)
        return 1;
    if (compare_path("/@unixroot", "/") != 0)
        return 1;

    if (test_unixroot(rootdir_test, rootdir_test, rootdir_test_native) != 0)
        return 1;
    if (test_unixroot(unixroot_test, rootdir_test, rootdir_test_native) != 0)
        return 1;

    if (!is_child && test_child(rootdir))
        return 1;

    const char *chroot_out = escape_with_null ?
        NULL : // Leave chroot the OS/2 way (doesn't require chdir("/@unixroot"))
        "."; // Leave chroot the Unix way (requires chdir("/@unixroot"))

    if (chroot(chroot_out) != 0)
    {
        fprintf(stderr, "chroot(%s): %s\n", chroot_out, strerror(errno));
        return 1;
    }

    if (fcwd >= 0)
    {
        if (fchdir(fcwd))
        {
            fprintf(stderr, "fchdir(%d): %s\n", fcwd, strerror(errno));
            return 1;
        }
        close(fcwd);
    }

    printf("After chroot(%s):\n", chroot_out);
    return 0;
}

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    _execname(full_exe_name, PATH_MAX);

    exe_name = argv[0];

    int a = 1;
    if (argc > a && (is_child = !strcmp(argv[a], "child")))
        ++a;
    rootdir = argc > a ? argv[a] : NULL;

    if ((!rootdir || !*rootdir) && !is_child)
    {
        fprintf(stderr, "Don't run me directly, run test-chroot-1.sh\n");
        return 1;
    }

    printf("\n%s: is_child=%d rootdir=%s\n", argv[0], is_child, rootdir);

    unixroot = getenv("UNIXROOT");
    unixroot_chrooted = getenv("UNIXROOT_CHROOTED");

    printf("UNIXROOT=%s\n", unixroot != NULL ? unixroot : "(unset)");
    printf("UNIXROOT_CHROOTED=%s\n", unixroot_chrooted != NULL ? unixroot_chrooted : "(unset)");

    if (unixroot && *unixroot)
    {
        strcpy(unixroot_test_native, unixroot);
        strcat(unixroot_test_native, "/test");
    }

    if (rootdir && *rootdir)
    {
        strcpy(rootdir_test_native, rootdir);
        strcat(rootdir_test_native, "/test");
    }

    if (test_unixroot_env() != 0)
        return 1;

    if (!is_child)
    {
        if (test_child(NULL) != 0)
            return 1;

        if (test_chroot() != 0)
            return 1;

        if (test_unixroot_env() != 0)
            return 1;

        if (test_child(NULL) != 0)
            return 1;
    }

    return 0;
}
