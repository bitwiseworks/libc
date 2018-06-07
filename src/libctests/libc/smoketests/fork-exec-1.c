/* $Id: fork-exec-1.c 2278 2005-08-14 06:42:50Z bird $ */
/** @file
 *
 * LIBC Testcase - exec + fork + exec in child.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <unistd.h>
#include <string.h>
#include <process.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

volatile int g_Signaled = 0;

static void sig_alarm(int iSignal)
{
    g_Signaled = 1;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "child"))
    {
        printf("fork-exec-1: %d: child of %d\n", getpid(), getppid());
        return 0;
    }
    printf("fork-exec-1: %d: %s, parent %d\n", getpid(), argc >= 2 ? argv[1] : "main", getppid());

    char *argv_child[3];
    argv_child[0] = argv[0];
    argv_child[1] = "child";
    argv_child[2] = NULL;
    if (argc != 2 || strcmp(argv[1], "2nd"))
    {
        int rc = spawnvp(P_WAIT, argv_child[0], argv_child);
        if (rc)
        {
            printf("fork-exec-1: child1 failed! rc=%d\n", rc);
            return 1;
        }
    }

    /*
     * Fork children (three generations).
     */
    pid_t pid = fork();
    if (pid < 0)
    {
        printf("fork failed! errno=%d %m\n", errno);
        return 1;
    }
    if (pid == 0)
    {
        if (argc == 2 && !strcmp(argv[1], "1st"))
        {
            printf("fork-exec-1: %d: 2nd fork, from %d\n", getpid(), getppid());
            argv_child[1] = "2nd";
            execvp(argv_child[0], argv_child);
        }
        else if (argc != 2 || strcmp(argv[1], "2nd"))
        {
            printf("fork-exec-1: %d: 1st fork, from %d\n", getpid(), getppid());
            argv_child[1] = "1st";
            execvp(argv_child[0], argv_child);
        }
        else
        {
            printf("fork-exec-1: %d: 3rd fork, from %d\n", getpid(), getppid());
            execvp(argv_child[0], argv_child);
        }
        printf("fork-exec-1: exec failed! errno=%d %m\n", errno);
        return 1;
    }

    /* parent */
    if (bsd_signal(SIGALRM, sig_alarm) == SIG_ERR)
    {
        printf("fork-exec-1: signal() failed errno=%d %m\n", errno);
        return 1;
    }

    if (alarm(5) < 0)
    {
        printf("fork-exec-1: alarm failed errno=%d %m\n", errno);
        return 1;
    }
    int iStatus = -1;
    pid_t pidRet;
    while ((pidRet = waitpid(pid, &iStatus, 0)) < 0 && errno == EINTR && !g_Signaled)
        /* nothing */;
    if (pidRet != pid)
    {
        printf("fork-exec-1: waitpid failed, pidRet=%d pid=%d errno=%d %m\n", pidRet, pid, errno);
        kill(pid, SIGTERM);
        return 1;
    }
    if (iStatus != 0)
    {
        printf("fork-exec-1: forked child failed, status=%d\n", iStatus);
        return 1;
    }
    return 0;
}
