/* $FreeBSD: src/sys/sys/shm.h,v 1.19 2003/01/25 21:33:05 alfred Exp $ */
/*	$NetBSD: shm.h,v 1.15 1994/06/29 06:45:17 cgd Exp $	*/

/*
 * Copyright (c) 1994 Adam Glass
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Adam Glass.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * As defined+described in "X/Open System Interfaces and Headers"
 *                         Issue 4, p. XXX
 */

/** @file
 * FreeBSD 5.3
 * @changed     bird: the SHMLBA to 64KB.
 * @changed     bird: added pid_t, time_t and size_t (POSIX).
 */

#ifndef _SYS_SHM_H_
#define _SYS_SHM_H_

#include <sys/ipc.h>

#include <sys/_types.h>

#if !defined(_TIME_T_DECLARED) && !defined(_TIME_T)
typedef	__time_t	time_t;
#define	_TIME_T_DECLARED
#define _TIME_T
#endif

#include <sys/_types.h>
#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T)
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#define _SIZE_T
#endif

#if !defined(_PID_T_DECLARED) && !defined(_PID_T)
typedef	__pid_t		pid_t;		
#define	_PID_T_DECLARED
#define _PID_T
#endif

#define SHM_RDONLY  010000  /* Attach read-only (else read-write) */
#define SHM_RND     020000  /* Round attach address to SHMLBA */
#define SHMLBA      (0x10000) /* Segment low boundary address multiple */

/* "official" access mode definitions; somewhat braindead since you have
   to specify (SHM_* >> 3) for group and (SHM_* >> 6) for world permissions */
#define SHM_R       (IPC_R)
#define SHM_W       (IPC_W)

/* predefine tbd *LOCK shmctl commands */
#define	SHM_LOCK	11
#define	SHM_UNLOCK	12

/* ipcs shmctl commands */
#define	SHM_STAT	13
#define	SHM_INFO	14

#ifdef __EMX__
typedef __uint32_t shmatt_t;
#endif

struct shmid_ds {
	struct ipc_perm shm_perm;	/* operation permission structure */
#ifdef __EMX__
	size_t          shm_segsz;	/* size of segment in bytes */
#else
	int             shm_segsz;	/* size of segment in bytes */
#endif
	pid_t           shm_lpid;   /* process ID of last shared memory op */
	pid_t           shm_cpid;	/* process ID of creator */
#ifdef __EMX__
	shmatt_t	shm_nattch;	/* number of current attaches */
#else
	short   	shm_nattch;	/* number of current attaches */
#endif
	time_t          shm_atime;	/* time of last shmat() */
	time_t          shm_dtime;	/* time of last shmdt() */
	time_t          shm_ctime;	/* time of last change by shmctl() */
	void           *shm_internal;   /* sysv stupidity */
};

#ifdef _KERNEL

/*
 * System 5 style catch-all structure for shared memory constants that
 * might be of interest to user programs.  Do we really want/need this?
 */
struct shminfo {
	int	shmmax,		/* max shared memory segment size (bytes) */
		shmmin,		/* min shared memory segment size (bytes) */
		shmmni,		/* max number of shared memory identifiers */
		shmseg,		/* max shared memory segments per process */
		shmall;		/* max amount of shared memory (pages) */
};
#ifndef __EMX__
extern struct shminfo	shminfo;
extern struct shmid_ds	*shmsegs;
#endif

struct shm_info {
	int used_ids;
	unsigned long shm_tot;
	unsigned long shm_rss;
	unsigned long shm_swp;
	unsigned long swap_attempts;
	unsigned long swap_successes;
};

#ifndef __EMX__
struct thread;
struct proc;
struct vmspace;

void	shmexit(struct vmspace *);
void	shmfork(struct proc *, struct proc *);
#endif /* !__EMX__ */
#else /* !_KERNEL */

#include <sys/cdefs.h>

#ifndef _SIZE_T_DECLARED
typedef __size_t        size_t;
#define _SIZE_T_DECLARED
#endif

__BEGIN_DECLS
#ifndef __EMX__
int shmsys(int, ...);
#endif
void *shmat(int, const void *, int);
int shmget(key_t, size_t, int);
int shmctl(int, int, struct shmid_ds *);
int shmdt(const void *);
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_SYS_SHM_H_ */
