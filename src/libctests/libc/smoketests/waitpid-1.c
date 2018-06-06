/* $Id: waitpid-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * LIBC - waitpid testcase no.1.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <process.h>
#include <signal.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
volatile pid_t g_pid0 = -1;
volatile pid_t g_pid1 = -1;
volatile pid_t g_pid64 = -1;
volatile pid_t g_pid128 = -1;
volatile pid_t g_pidKill = -1;
volatile int   g_cErrors = 0;
volatile int   g_cReaped = 0;

static void sigchild(int iSignal)
{
    int     rc;
    pid_t   pid;
    while ((pid = waitpid(-1, &rc, WNOHANG)) > 0)
    {
        g_cReaped++;
        printf("waitpid-1: info - reaped %#x rc=%d\n", pid, rc);
        int rcWanted = -1;
        if (pid == g_pid0)
            rcWanted = W_EXITCODE(0,0);
        else if (pid == g_pid1)
            rcWanted = W_EXITCODE(1, 0);
        else if (pid == g_pid64)
            rcWanted = W_EXITCODE(64, 0);
        else if (pid == g_pid128)
            rcWanted = W_EXITCODE(128, 0);
        else if (pid == g_pidKill)
            rcWanted = W_EXITCODE(0, SIGKILL);
        else
        {
            printf("waitpid-1: WARNING - interrupted before spawn returned!?!\n");
            continue;
        }
        if (rc != rcWanted)
        {
            printf("waitpid-1: ERROR - pid=%#x exitted with rc=%#x expected %#x\n", pid, rc, rcWanted);
            g_cErrors++;
        }
    }
}

int main(int argc, char **argv)
{
    /*
     * Child for child.
     */
    if (argc == 2)
    {
        usleep(1000);
        if (!strcmp(argv[1], "child-1"))
            return 1;
        if (!strcmp(argv[1], "child-64"))
            return 64;
        if (!strcmp(argv[1], "child-128"))
            return 128;
        if (!strcmp(argv[1], "child-kill"))
            sleep(24*60*60);
        return 0;
    }

    struct sigaction sigact;
    sigact.sa_handler = sigchild;
    sigact.sa_flags = 0;
    sigemptyset(&sigact.sa_mask);
    if (sigaction(SIGCHLD, &sigact, NULL))
    {
        printf("waitpid-1: ERROR - sigaction failed. errno=%d\n", errno);
        return 1;
    }

    /*
     * spawn children.
     */
    sighold(SIGCHLD);
    g_pid0 = spawnl(P_NOWAIT, argv[0], argv[0], "child", NULL);
    g_pid1 = spawnl(P_NOWAIT, argv[0], argv[0], "child-1", NULL);
    g_pid64 = spawnl(P_NOWAIT, argv[0], argv[0], "child-64", NULL);
    g_pid128 = spawnl(P_NOWAIT, argv[0], argv[0], "child-128", NULL);
    g_pidKill = spawnl(P_NOWAIT, argv[0], argv[0], "child-kill", NULL);
    int cSpawned = (g_pid0 > 0)
                 + (g_pid1 > 0)
                 + (g_pid64 > 0)
                 + (g_pid128 > 0)
                 + (g_pidKill > 0);
    printf("waitpid-1: info - pid0=%#x pid1=%#x pid64=%#x pid128=%#x pidKill=%#x\n", g_pid0, g_pid1, g_pid64, g_pid128, g_pidKill);
    if (cSpawned != 5)
    {
        printf("waitpid-1: ERROR - one or more spawns failed. cSpawned=%d should be 5!\n", cSpawned);
        g_cErrors++;
    }
    sigrelse(SIGCHLD);

    int cWaits = 0;
    while (g_cReaped < cSpawned && cWaits++ < 30)
    {
        if (cWaits == 1 && g_cReaped + 1 == cSpawned)
            kill(g_pidKill, SIGKILL);
        else
        {
            sleep(1);
            kill(g_pidKill, SIGKILL);
        }
    }

    if (g_cReaped != cSpawned)
    {
        printf("waitpid-1: ERROR - reaped %d spawned %d!\n", g_cReaped, cSpawned);
        g_cErrors++;
    }

    if (!g_cErrors)
        printf("waitpid-1: SUCCESS\n");
    else
        printf("waitpid-1: FAILED - %d errors\n", g_cErrors);

    return !!g_cErrors;
}
