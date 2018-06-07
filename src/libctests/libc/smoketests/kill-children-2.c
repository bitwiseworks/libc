/* $Id: kill-children-2.c 2210 2005-07-06 03:23:46Z bird $ */
/** @file
 *
 * LIBC Testcase - spawn a child which spawn a bunch of blocking children and then exits.
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
#include <time.h>

volatile int g_Signaled = 0;

static void sig_alarm(int iSignal)
{
    g_Signaled = 1;
}

static int spawnit(char *prog, char *arg)
{
    char *argv_child[3];
    argv_child[0] = prog;
    argv_child[1] = arg;
    argv_child[2] = NULL;
    pid_t pid = spawnvp(P_NOWAIT, argv_child[0], argv_child);
    if (pid < 0)
    {
        printf("kill-children-2: Failed to spawn %s, errno=%d %m\n", argv_child[1], errno);
        return -1;
    }
    return pid;
}

static int takefive(void)
{
    time_t t = time(NULL) + 30;
    do sleep(5);
    while (time(NULL) < t);
    printf("kill-children: child %d timed out!\n", getpid());
    return 1;
}

int main(int argc, char **argv)
{
    int iStatus;
    pid_t pidRet;
    if (argc == 2)
    {
        printf("kill-children-2: %s %d\n", argv[1], getpid());
        if (!strcmp(argv[1], "child2"))
        {
            spawnit(argv[0], "child3");
            spawnit(argv[0], "child3");
            spawnit(argv[0], "child3");
            return takefive();
        }
        if (!strcmp(argv[1], "child3"))
            return takefive();
        if (!strcmp(argv[1], "child1"))
        {
            pid_t pid = spawnit(argv[0], "child2");
            if (pid < 0)
                return 1;
            pid = spawnit(argv[0], "child2");
            if (pid < 0)
                return 1;
            pid = spawnit(argv[0], "child2");
            if (pid < 0)
                return 1;
            pid = spawnit(argv[0], "child2");
            if (pid < 0)
                return 1;
            sleep(1); /* give them a chance to kick off */
            pidRet = waitpid(pid, &iStatus, WNOHANG);
            if (pidRet == pid)
            {
                printf("kill-children-2: waitpid failed succeeded! (child1) iStatus=%#x\n", iStatus);
                return 1;
            }
            printf("kill-children-2: child 1 exiting\n");
            return 0;
        }
        printf("kill-children-2: invalid argument %s\n", argv[1]);
        return 1;
    }

    /* parent */
    pid_t pid = spawnit(argv[0], "child1");
    if (pid < 0)
        return 1;

    if (bsd_signal(SIGALRM, sig_alarm) == SIG_ERR)
    {
        printf("kill-children-2: signal() failed errno=%d %m\n", errno);
        return 1;
    }
    if (alarm(5) < 0)
    {
        printf("kill-children-2: alarm failed errno=%d %m\n", errno);
        return 1;
    }

    while ((pidRet = waitpid(pid, &iStatus, 0)) < 0 && errno == EINTR && !g_Signaled)
        /* nothing */;
    if (pidRet != pid)
    {
        printf("kill-children-2: waitpid failed, pidRet=%d pid=%d errno=%d %m\n", pidRet, pid, errno);
        return 1;
    }
    if (iStatus != 0)
    {
        printf("kill-children-2: forked child failed, status=%d\n", iStatus);
        return 1;
    }

    errno = 0;
    pidRet = waitpid(-1, NULL, WNOHANG);
    if (pidRet != -1 || errno != ECHILD)
    {
        printf("kill-children-2: our only child left some other child behind to us. that's not right!\n");
        return 1;
    }
    printf("kill-children-2: parent exiting normally\n");
    return 0;
}

