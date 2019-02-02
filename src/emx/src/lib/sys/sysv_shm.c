/*	$NetBSD: sysv_shm.c,v 1.23 1994/07/04 23:25:12 glass Exp $	*/
/*-
 * Copyright (c) 1994 Adam Glass and Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Adam Glass and Charles
 *	Hannum.
 * 4. The names of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "libc-alias.h"
#include <sys/cdefs.h>
//__FBSDID("$FreeBSD: src/sys/kern/sysv_shm.c,v 1.96.2.3 2005/02/19 19:54:31 csjp Exp $");

#include <sys/types.h>
#define _KERNEL
#include <sys/shm.h>
#undef _KERNEL
#include <sys/mman.h>
#include <sys/stat.h>
#define INCL_DOSSEMAPHORES
#define INCL_DOSEXCEPTIONS
#define INCL_DOSMEMMGR
#define INCL_DOSERRORS
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>
#include "syscalls.h"
#include <string.h>
#include <errno.h>
#include <emx/umalloc.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <386/builtin.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/fork.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>

/* debug printf */
#define DPRINTF(a)	LIBCLOG_MSG a
#include <386/param.h>
#ifndef round_page
#error round_page is missing
#endif



/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/**
 * SysV Shared Memory globals residing in the shared process manager heap.
 */
struct __libc_SysV_Shm
{
    /** Structure version. */
    unsigned            uVersion;
    /** Number of users.
     * This is primarily used for knowing when to re-create the semaphores.
     */
    volatile uint32_t	cUsers;
    /** Mutex protecting everything. */
    __LIBC_SAFESEMMTX   mtx;
    /** Event semaphore which the daemon should sleep on.
     * This is posted whenever a memory object is created or deleted. */
    __LIBC_SAFESEMEV    evDaemon;

    int                 shm_last_free;
    int                 shm_nused;
    int                 shm_committed;
    int                 shmalloced;
    /** Pointer to array of shared memory ids. */
    struct shmid_ds    *shmsegs;

    /** SysV Semaphore info. (Contains all the array sizes and such.)
     * Not changed after initialization. */
    struct shminfo      shminfo;
};

/** Pointer to the globals. */
static struct __libc_SysV_Shm *gpGlobals;

//static int shm_last_free, shm_nused, shm_committed, shmalloced;
//static struct shmid_ds	*shmsegs;

struct shmmap_state {
	vm_offset_t va;
	int shmid;
};

/** Array of shared memory ids attached to this process.
 * On BSD this is the vm_shm of the process vm. */
static struct shmmap_state *g_vm_shm;


static int shmget_allocate_segment(key_t key, size_t size, mode_t mode);
static int shmget_existing(size_t size, mode_t mode, int segnum, int shmflg);

#define	SHMSEG_FREE     	0x0200
#define	SHMSEG_REMOVED  	0x0400
#define	SHMSEG_ALLOCATED	0x0800
//#define	SHMSEG_WANTED		0x1000 - cannot happen with our locking policy.
/** Set when the segment is captured by the server.
 * if cleared it equals the SHMSEG_REMOVED when shm_nattch <= 0. */
#define	SHMSEG_SERVER           0x1000

static void shm_deallocate_segment(struct shmid_ds *);
static int shm_find_segment_by_key(key_t);
static struct shmid_ds *shm_find_segment_by_shmid(int);
static struct shmid_ds *shm_find_segment_by_shmidx(int);
static int shm_delete_mapping(struct shmmap_state *, int);
static int shminit(void);
static void shmexit_myhook(void);
static int shmfork_myhook(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
//static int sysctl_shmsegs(SYSCTL_HANDLER_ARGS);

/*
 * Tuneable values.
 */
#ifndef SHMMAXPGS
#define	SHMMAXPGS	8192	/* Note: sysv shared memory is swap backed. */
#endif
#ifndef SHMMAX
#define	SHMMAX	(SHMMAXPGS*PAGE_SIZE)
#endif
#ifndef SHMMIN
#define	SHMMIN	1
#endif
#ifndef SHMMNI
#define	SHMMNI	192
#endif
#ifndef SHMSEG
#define	SHMSEG	128
#endif
#ifndef SHMALL
#define	SHMALL	(SHMMAXPGS)
#endif

static struct	shminfo shminfo = {
	SHMMAX,                 /* max shared memory segment size (bytes) */
	SHMMIN,                 /* min shared memory segment size (bytes) */
	SHMMNI,                 /* max number of shared memory identifiers */
	SHMSEG,                 /* max shared memory segments per process */
	SHMALL                  /* max amount of shared memory (pages) */
};

//static int shm_use_phys;
#define shm_use_phys 0
//static int shm_allow_removed;
#define shm_allow_removed 0

#if 0
SYSCTL_DECL(_kern_ipc);
SYSCTL_INT(_kern_ipc, OID_AUTO, shmmax, CTLFLAG_RW, &shminfo.shmmax, 0,
    "Maximum shared memory segment size");
SYSCTL_INT(_kern_ipc, OID_AUTO, shmmin, CTLFLAG_RW, &shminfo.shmmin, 0,
    "Minimum shared memory segment size");
SYSCTL_INT(_kern_ipc, OID_AUTO, shmmni, CTLFLAG_RDTUN, &shminfo.shmmni, 0,
    "Number of shared memory identifiers");
SYSCTL_INT(_kern_ipc, OID_AUTO, shmseg, CTLFLAG_RDTUN, &shminfo.shmseg, 0,
    "Number of segments per process");
SYSCTL_INT(_kern_ipc, OID_AUTO, shmall, CTLFLAG_RW, &shminfo.shmall, 0,
    "Maximum number of pages available for shared memory");
//SYSCTL_INT(_kern_ipc, OID_AUTO, shm_use_phys, CTLFLAG_RW,
//    &shm_use_phys, 0, "Enable/Disable locking of shared memory pages in core");
//SYSCTL_INT(_kern_ipc, OID_AUTO, shm_allow_removed, CTLFLAG_RW,
//    &shm_allow_removed, 0,
//    "Enable/Disable attachment to attached segments marked for removal");
SYSCTL_PROC(_kern_ipc, OID_AUTO, shmsegs, CTLFLAG_RD,
    NULL, 0, sysctl_shmsegs, "",
    "Current number of shared memory segments allocated");
#endif

/* wrappers */
/* wrappers */
/* wrappers */
/* wrappers */
/* wrappers */

#define panic LIBC_ASSERTM_FAILED
#undef copyout
#define copyout(x,y,z)  !memcpy((y),(x),(z))
#undef copyin
#define copyin(x,y,z)   !memcpy((y),(x),(z))
#define TUNABLE_INT_FETCH(path, var) _getenv_int("LIBC_" path, (var))
#define GIANT_REQUIRED do {} while (0)

/** @todo add these to param.h */
/* clicks to bytes */
#ifndef ctob
#define ctob(x)	((x)<<PAGE_SHIFT)
#endif

/* bytes to clicks */
#ifndef btoc
#define btoc(x)	(((vm_offset_t)(x)+PAGE_MASK)>>PAGE_SHIFT)
#endif



static int
shm_find_segment_by_key(key)
	key_t key;
{
	int i;
	const unsigned shmalloced = gpGlobals->shmalloced;
        struct shmid_ds *shmsegs = gpGlobals->shmsegs;

	for (i = 0; i < shmalloced; i++)
		if ((shmsegs[i].shm_perm.mode & SHMSEG_ALLOCATED) &&
		    shmsegs[i].shm_perm.key == key)
			return (i);
	return (-1);
}

static struct shmid_ds *
shm_find_segment_by_shmid(int shmid)
{
	int segnum;
	struct shmid_ds *shmseg;
	const unsigned shmalloced = gpGlobals->shmalloced;
        struct shmid_ds *shmsegs = gpGlobals->shmsegs;

	segnum = IPCID_TO_IX(shmid);
	if (segnum < 0 || segnum >= shmalloced)
		return (NULL);
	shmseg = &shmsegs[segnum];
	if ((shmseg->shm_perm.mode & SHMSEG_ALLOCATED) == 0 ||
	    (!shm_allow_removed &&
	     (shmseg->shm_perm.mode & SHMSEG_REMOVED) != 0) ||
	    shmseg->shm_perm.seq != IPCID_TO_SEQ(shmid))
		return (NULL);
	return (shmseg);
}

static struct shmid_ds *
shm_find_segment_by_shmidx(int segnum)
{
	struct shmid_ds *shmseg;
	const unsigned shmalloced = gpGlobals->shmalloced;
        struct shmid_ds *shmsegs = gpGlobals->shmsegs;

	if (segnum < 0 || segnum >= shmalloced)
		return (NULL);
	shmseg = &shmsegs[segnum];
	if ((shmseg->shm_perm.mode & SHMSEG_ALLOCATED) == 0 ||
	    (!shm_allow_removed &&
	     (shmseg->shm_perm.mode & SHMSEG_REMOVED) != 0))
		return (NULL);
	return (shmseg);
}

static void
shm_deallocate_segment(shmseg)
	struct shmid_ds *shmseg;
{
	struct __libc_SysV_Shm *pGlobals = gpGlobals;
	size_t size;

	GIANT_REQUIRED;

        shmseg->shm_internal = NULL;
	size = round_page(shmseg->shm_segsz);
	pGlobals->shm_committed -= btoc(size);
	pGlobals->shm_nused--;
	shmseg->shm_perm.mode = SHMSEG_FREE;
	__libc_Back_safesemEvWakeup(&pGlobals->evDaemon);
}

static int
shm_delete_mapping(struct shmmap_state *shmmap_s, int fExitting)
{
	struct __libc_SysV_Shm *pGlobals = gpGlobals;
        struct shmid_ds *shmsegs = pGlobals->shmsegs;
	struct shmid_ds *shmseg;
	int segnum, result;

	GIANT_REQUIRED;

	segnum = IPCID_TO_IX(shmmap_s->shmid);
	shmseg = &shmsegs[segnum];
	FS_VAR_SAVE_LOAD();
	result = DosFreeMem((void *)shmmap_s->va);
	FS_RESTORE();
	if (result)
		return (EINVAL);
	shmmap_s->shmid = -1;
	shmseg->shm_dtime = fibGetUnixSeconds();
	if ((--shmseg->shm_nattch <= 0) &&
	    (   (shmseg->shm_perm.mode & SHMSEG_REMOVED)
             || (fExitting && !(shmseg->shm_perm.mode & SHMSEG_SERVER)))) {
		shm_deallocate_segment(shmseg);
		pGlobals->shm_last_free = segnum;
	}
        __libc_Back_safesemEvWakeup(&pGlobals->evDaemon);
	return (0);
}

#ifndef _SYS_SYSPROTO_H_
struct shmdt_args {
	const void *shmaddr;
};
#endif

/**
 * shmdt.
 */
int __libc_Back_sysvShmDt(const void *shmaddr)
{
	LIBCLOG_ENTER("shmaddr=%p\n", shmaddr);
	struct shmmap_state *shmmap_s;
	int i;
	int error = 0;

	/*
	 * Lazy init.
	 */
	if (!gpGlobals)
	{
	    error = shminit();
	    if (error)
		LIBCLOG_ERROR_RETURN_INT(error);
	}
	
	error = __libc_Back_safesemMtxLock(&gpGlobals->mtx);
	if (error)
	    LIBCLOG_ERROR_RETURN_INT(error);

	shmmap_s = g_vm_shm;
 	if (shmmap_s == NULL) {
		error = EINVAL;
		goto done2;
	}
	for (i = 0; i < shminfo.shmseg; i++, shmmap_s++) {
		if (shmmap_s->shmid != -1 &&
		    shmmap_s->va == (vm_offset_t)shmaddr) {
			break;
		}
	}
	if (i == shminfo.shmseg) {
		error = EINVAL;
		goto done2;
	}
	error = shm_delete_mapping(shmmap_s, 0);
done2:
	__libc_Back_safesemMtxUnlock(&gpGlobals->mtx);
	return (-error);
}


/**
 * shmat.
 */
int __libc_Back_sysvShmAt(int shmid, const void *shmaddr, int shmflg, void **ppvActual)
{
    	LIBCLOG_ENTER("shmid=%#x shmaddr=%p shmflg=%#x ppvActual=%p\n", shmid, shmaddr, shmflg, (void *)ppvActual);
	int i;
	struct shmid_ds *shmseg;
	struct shmmap_state *shmmap_s = NULL;
	vm_offset_t attach_va = 0;
	vm_size_t size;
	int error = 0;

	/*
	 * Lazy init.
	 */
	if (!gpGlobals)
	{
	    error = shminit();
	    if (error)
		LIBCLOG_ERROR_RETURN_INT(error);
	}
	struct __libc_SysV_Shm *pGlobals = gpGlobals;
	
	error = __libc_Back_safesemMtxLock(&pGlobals->mtx);
	if (error)
	    LIBCLOG_ERROR_RETURN_INT(error);

	/*
	 * Make sure we've got a g_vm_shm.
	 */
	shmmap_s = g_vm_shm;
	if (shmmap_s == NULL) {
		__libc_Back_safesemMtxUnlock(&pGlobals->mtx);
		size = shminfo.shmseg * sizeof(struct shmmap_state);
		shmmap_s = _hmalloc(size);
		if (!shmmap_s)
		    LIBCLOG_ERROR_RETURN_INT(-ENOMEM);
		for (i = 0; i < shminfo.shmseg; i++)
			shmmap_s[i].shmid = -1;
		__libc_Back_safesemMtxLock(&pGlobals->mtx);
		if (!g_vm_shm)
		    g_vm_shm = shmmap_s;
		else
		{
		    free(shmmap_s);
                    g_vm_shm = NULL;
		}
	}
        shmseg = shm_find_segment_by_shmid(shmid);
	if (shmseg == NULL) {
		error = EINVAL;
		goto done2;
	}
	error = -__libc_spmCanIPC(&shmseg->shm_perm,
	    (shmflg & SHM_RDONLY) ? IPC_R : IPC_R|IPC_W);
	if (error)
		goto done2;
	
	/* Find free usage struct. */
	for (i = 0; i < shminfo.shmseg; i++) {
		if (shmmap_s->shmid == -1)
			break;
		shmmap_s++;
	}
	if (i >= shminfo.shmseg) {
		error = EMFILE;
		goto done2;
	}

	/*
	 * Do the attaching.
	 */
	size = round_page(shmseg->shm_segsz);
	ULONG flFlags = PAG_READ | PAG_EXECUTE;
	if ((shmflg & SHM_RDONLY) == 0)
		flFlags |= PAG_WRITE;
	if (shmaddr) {
                if (shmflg & SHM_RND) {
			attach_va = (vm_offset_t)shmaddr & ~(SHMLBA-1);
		} else if (((vm_offset_t)shmaddr & (SHMLBA-1)) == 0) {
			attach_va = (vm_offset_t)shmaddr;
		} else {
			error = EINVAL;
			goto done2;
		}
		if ((void *)attach_va != shmseg->shm_internal) {
			error = EOPNOTSUPP;
			goto done2;
		}
	} else {
            attach_va = (vm_offset_t)shmseg->shm_internal;
	}

	FS_VAR_SAVE_LOAD();
	error = DosGetSharedMem((PVOID)attach_va, flFlags);
	FS_RESTORE();
	if (error) {
		error = __libc_native2errno(error);
		goto done2;
	}

	shmmap_s->va = attach_va;
	shmmap_s->shmid = shmid;
	shmseg->shm_lpid = fibGetPid();
	shmseg->shm_atime = fibGetUnixSeconds();
	shmseg->shm_nattch++;
	
done2:
	__libc_Back_safesemMtxUnlock(&pGlobals->mtx);
        *ppvActual = (void *)attach_va;
	if (!error)
	    LIBCLOG_RETURN_INT(-error);
	LIBCLOG_ERROR_RETURN_INT(-error);
}



#ifndef _SYS_SYSPROTO_H_
struct shmctl_args {
	int shmid;
	int cmd;
	struct shmid_ds *buf;
};
#endif

/*
 * MPSAFE
 */
static int kern_shmctl(int shmid, int cmd, void *buf, size_t *bufsz)
{
	int error = 0;
	int rval = 0;
	struct shmid_ds *shmseg;

	/*
	 * Lazy init.
	 */
	if (!gpGlobals)
	{
	    error = shminit();
	    if (error)
		return (error);
	}
	struct __libc_SysV_Shm *pGlobals = gpGlobals;
	
	error = __libc_Back_safesemMtxLock(&pGlobals->mtx);
	if (error)
	    return error;
        const int shmalloced = pGlobals->shmalloced;
        const int shm_nused = pGlobals->shm_nused;

	/*
	 * Execute the command.
	 */
	switch (cmd) {
	case IPC_INFO:
		memcpy(buf, &shminfo, sizeof(shminfo));
		if (bufsz)
			*bufsz = sizeof(shminfo);
		rval = shmalloced;
		goto done2;
	case SHM_INFO: {
		struct shm_info shm_info;
		shm_info.used_ids = shm_nused;
		shm_info.shm_rss = 0;	/*XXX where to get from ? */
		shm_info.shm_tot = 0;	/*XXX where to get from ? */
		shm_info.shm_swp = 0;	/*XXX where to get from ? */
		shm_info.swap_attempts = 0;	/*XXX where to get from ? */
		shm_info.swap_successes = 0;	/*XXX where to get from ? */
		memcpy(buf, &shm_info, sizeof(shm_info));
		if (bufsz)
			*bufsz = sizeof(shm_info);
		rval = shmalloced;
		goto done2;
	}
	}
	if (cmd == SHM_STAT)
		shmseg = shm_find_segment_by_shmidx(shmid);
	else
		shmseg = shm_find_segment_by_shmid(shmid);
	if (shmseg == NULL) {
		error = EINVAL;
		goto done2;
	}
	switch (cmd) {
	case SHM_STAT:
	case IPC_STAT:
		error = -__libc_spmCanIPC(&shmseg->shm_perm, IPC_R);
		if (error)
			goto done2;
		memcpy(buf, shmseg, sizeof(struct shmid_ds));
		if (bufsz)
			*bufsz = sizeof(struct shmid_ds);
		if (cmd == SHM_STAT)
			rval = IXSEQ_TO_IPCID(shmid, shmseg->shm_perm);
		break;
	case IPC_SET: {
		struct shmid_ds *shmid;

		shmid = (struct shmid_ds *)buf;
		error = -__libc_spmCanIPC(&shmseg->shm_perm, IPC_M);
		if (error)
			goto done2;
		shmseg->shm_perm.uid = shmid->shm_perm.uid;
		shmseg->shm_perm.gid = shmid->shm_perm.gid;
		shmseg->shm_perm.mode =
		    (shmseg->shm_perm.mode & ~ACCESSPERMS) |
		    (shmid->shm_perm.mode & ACCESSPERMS);
		shmseg->shm_ctime = fibGetUnixSeconds();
		break;
	}
	case IPC_RMID:
		error = -__libc_spmCanIPC(&shmseg->shm_perm, IPC_M);
		if (error)
			goto done2;
		shmseg->shm_perm.key = IPC_PRIVATE;
		shmseg->shm_perm.mode |= SHMSEG_REMOVED;
		if (shmseg->shm_nattch <= 0) {
			shm_deallocate_segment(shmseg);
			pGlobals->shm_last_free = IPCID_TO_IX(shmid);
		}
		break;
#if 0
	case SHM_LOCK:
	case SHM_UNLOCK:
#endif
	default:
		error = EINVAL;
		break;
	}
done2:
	__libc_Back_safesemMtxUnlock(&pGlobals->mtx);
	if (error)
	    rval = -error;
	return (rval);
}


/**
 * shmctl.
 */
int __libc_Back_sysvShmCtl(int shmid, int cmd, struct shmid_ds *bufptr)
{
    	LIBCLOG_ENTER("shmid=%#x cmd=%d bufptr=%p\n", shmid, cmd, (void *)bufptr);
	int error = 0;
	int rval = 0;
	struct shmid_ds buf;
	size_t bufsz = 0;
	
	/* IPC_SET needs to copyin the buffer before calling kern_shmctl */
	if (cmd == IPC_SET) {
		if ((error = copyin(bufptr, &buf, sizeof(struct shmid_ds))))
			goto done;
	}
	
	rval = kern_shmctl(shmid, cmd, (void *)&buf, &bufsz);
	if (rval < 0)
		goto done;
	
	/* Cases in which we need to copyout */
	switch (cmd) {
	case IPC_INFO:
	case SHM_INFO:
	case SHM_STAT:
	case IPC_STAT:
		error = copyout(&buf, bufptr, bufsz);
		break;
	}

done:
	if (error)
	    rval = -error;
	if (rval >= 0)
	    LIBCLOG_RETURN_INT(rval);
	LIBCLOG_ERROR_RETURN_INT(rval);
}



static int
shmget_existing(size_t size, mode_t mode, int segnum, int shmflg)
{
	struct shmid_ds *shmseg;
	int error;

	shmseg = &gpGlobals->shmsegs[segnum];
	if (shmseg->shm_perm.mode & SHMSEG_REMOVED) {
		return (-EAGAIN);
	}
	if ((shmflg & (IPC_CREAT | IPC_EXCL)) == (IPC_CREAT | IPC_EXCL))
		return (-EEXIST);
	error = __libc_spmCanIPC(&shmseg->shm_perm, mode);
	if (error)
		return (error);
	if (size && size > shmseg->shm_segsz)
		return (-EINVAL);
	return IXSEQ_TO_IPCID(segnum, shmseg->shm_perm);
}

static int
shmget_allocate_segment(key_t key, size_t size_in, mode_t mode)
{
	int i, segnum, shmid, size;
	struct shmid_ds *shmseg;
	struct __libc_SysV_Shm *pGlobals = gpGlobals;
        const int shmalloced = pGlobals->shmalloced;
	struct shmid_ds *shmsegs = pGlobals->shmsegs;
	void *shm_object;

	GIANT_REQUIRED;

	if (size_in < shminfo.shmmin || size_in > shminfo.shmmax)
		return (-EINVAL);
	if (pGlobals->shm_nused >= shminfo.shmmni) /* Any shmids left? */
		return (-ENOSPC);
	size = round_page(size_in);
	if (pGlobals->shm_committed + btoc(size) > shminfo.shmall)
		return (-ENOMEM);
	if (pGlobals->shm_last_free < 0) {
		for (i = 0; i < shmalloced; i++)
			if (shmsegs[i].shm_perm.mode & SHMSEG_FREE)
				break;
		if (i == shmalloced)
			return (-ENOSPC);
		segnum = i;
	} else  {
		segnum = pGlobals->shm_last_free;
		pGlobals->shm_last_free = -1;
	}
	shmseg = &shmsegs[segnum];

	/*
	 * Allocate shared memory object.
	 * Unfortunately the creator will get it mapped in with full access.
	 */
	FS_VAR_SAVE_LOAD();
	int error = DosAllocSharedMemEx(&shm_object, NULL, size, PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_COMMIT | OBJ_FORK | OBJ_GETTABLE | OBJ_ANY);
	if (error)
	    error = DosAllocSharedMemEx(&shm_object, NULL, size, PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_COMMIT | OBJ_FORK | OBJ_GETTABLE);
	FS_RESTORE();
	if (error)
	    return -__libc_native2errno(error);

	/*
	 * Set up the segment.
	 */
	shmseg->shm_internal = shm_object;
	shmseg->shm_perm.key = key;
	shmseg->shm_perm.seq = (shmseg->shm_perm.seq + 1) & 0x7fff;
	shmid = IXSEQ_TO_IPCID(segnum, shmseg->shm_perm);
	shmseg->shm_perm.cuid = shmseg->shm_perm.uid = __libc_spmGetId(__LIBC_SPMID_EUID);
	shmseg->shm_perm.cgid = shmseg->shm_perm.gid = __libc_spmGetId(__LIBC_SPMID_EGID);
	shmseg->shm_perm.mode = (mode & ACCESSPERMS) | SHMSEG_ALLOCATED;
	shmseg->shm_segsz = size;
	shmseg->shm_cpid = fibGetPid();
	shmseg->shm_lpid = shmseg->shm_nattch = 0;
	shmseg->shm_atime = shmseg->shm_dtime = 0;
	shmseg->shm_ctime = fibGetUnixSeconds();
	pGlobals->shm_committed += btoc(size);
	pGlobals->shm_nused++;
	return shmid;
}


/**
 * shmget.
 */
int __libc_Back_sysvShmGet(key_t key, size_t size, int shmflg)
{
    	LIBCLOG_ENTER("key=%#lx size=%#x shmflg=%#x\n", key, size, shmflg);
	int segnum, mode;
	int rval;

	/*
	 * Lazy init.
	 */
	if (!gpGlobals)
	{
	    rval = shminit();
	    if (rval)
		LIBCLOG_ERROR_RETURN_INT(rval);
	}
	struct __libc_SysV_Shm *pGlobals = gpGlobals;
	
	rval = __libc_Back_safesemMtxLock(&pGlobals->mtx);
	if (rval)
	    LIBCLOG_ERROR_RETURN_INT(rval);

	/* mark that all subfunctions here return negative errno or return value. */
	mode = shmflg & ACCESSPERMS;
	if (key != IPC_PRIVATE) {
	//again:
		segnum = shm_find_segment_by_key(key);
		if (segnum >= 0) {
			rval = shmget_existing(size, mode, segnum, shmflg);
			//I don't think this is right for us...
			//if (rval == -EAGAIN)
			//	goto again;
			goto done2;
		}
		if ((shmflg & IPC_CREAT) == 0) {
			rval = -ENOENT;
			goto done2;
		}
	}
	rval = shmget_allocate_segment(key, size, mode);
done2:
	__libc_Back_safesemMtxUnlock(&pGlobals->mtx);
	if (rval >= 0)
	    LIBCLOG_RETURN_INT(rval);
	LIBCLOG_ERROR_RETURN_INT(rval);
}


_FORK_CHILD1(0xf0010000, shmfork_myhook)

/**
 * Callback function for registration using _FORK_PARENT1()
 * or _FORK_CHILD1().
 *
 * @returns 0 on success.
 * @returns positive errno on warning.
 * @returns negative errno on failure. Fork will be aborted.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Callback operation.
 */
static int shmfork_myhook(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    switch (enmOperation)
    {
	/*
	 * We'll have to increment the shm_nattch counts of any attached segments.
	 */
	case __LIBC_FORK_OP_EXEC_CHILD:
	{
	    struct __libc_SysV_Shm *pGlobals = gpGlobals;
	    if (pGlobals)
	    {
		__atomic_increment_u32(&pGlobals->cUsers);
		struct shmmap_state *shm = g_vm_shm;
		if (shm) {
		    __libc_Back_safesemMtxLock(&pGlobals->mtx);
		    int c = shminfo.shmseg;
		    while (c-- > 0)
		    {
			if (shm->shmid != -1)
			    pGlobals->shmsegs[IPCID_TO_IX(shm->shmid)].shm_nattch++;
		    }
		    __libc_Back_safesemMtxUnlock(&pGlobals->mtx);
		}
	    }
	    break;
	}

	default:
	    break;
    }
    return 0;
}


static void
shmexit_myhook(void)
{
	static volatile int fDone = 0;
	struct __libc_SysV_Shm *pGlobals = gpGlobals;
	if (pGlobals && !fDone) {
                fDone = 1;
		__libc_Back_safesemMtxLock(&pGlobals->mtx);
		struct shmmap_state *shm = g_vm_shm;
		if (shm) {
    			g_vm_shm = NULL;
			int c = shminfo.shmseg;
			while (c-- > 0) {
				if (shm->shmid != -1) {
					shm_delete_mapping(shm, 1);
                                }
				shm++;
			}
		}

                /* Clean out non-referenced objects when the server isn't around for them. */
                int c = pGlobals->shm_nused;
                if (c > 0)
                {
                    int i = pGlobals->shmalloced;
                    struct shmid_ds *shmseg = pGlobals->shmsegs;
                    while (i-- > 0)
                    {
                        if (shmseg->shm_perm.mode & SHMSEG_ALLOCATED)
                        {
                            if (    shmseg->shm_nattch <= 0
                                &&  (   !(shmseg->shm_perm.mode & SHMSEG_SERVER)
                                     ||  (shmseg->shm_perm.mode & SHMSEG_REMOVED)))
                                shm_deallocate_segment(shmseg);
                            if (c-- <= 0)
                                break;
                        }

                        /* next */
                        shmseg++;
                    }
                }
		__libc_Back_safesemMtxUnlock(&pGlobals->mtx);
		__atomic_decrement_u32(&pGlobals->cUsers);
                gpGlobals = NULL;
	}
}

static int shminit(void)
{
    LIBCLOG_ENTER("\n");

    /*
     * Try open it first.
     */
    __LIBC_SPMXCPTREGREC    RegRec;
    __LIBC_PSPMHEADER       pSPMHdr;
    int rc = __libc_spmLock(&RegRec, &pSPMHdr);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);
    if (gpGlobals)
    {
        __libc_spmUnlock(&RegRec);
        LIBCLOG_RETURN_INT(0);
    }
    struct __libc_SysV_Shm *pGlobals = gpGlobals = pSPMHdr->pSysVShm;
    if (!pGlobals)
    {
        /*
         * We will have to initialize the shared memory stuff.
         * Since we're gonna parse environment first, release the SPM lock.
         */
        __libc_spmUnlock(&RegRec);

	struct shminfo si = shminfo;
	TUNABLE_INT_FETCH("kern.ipc.shmmin", &si.shmmin);
	TUNABLE_INT_FETCH("kern.ipc.shmmni", &si.shmmni);
	TUNABLE_INT_FETCH("kern.ipc.shmseg", &si.shmseg);
	TUNABLE_INT_FETCH("kern.ipc.shmmaxpgs", &si.shmall);
	si.shmmax = si.shmall * PAGE_SIZE;
	if (si.shmmax < si.shmall)
            si.shmmax = 0xc0000000;
	//TUNABLE_INT_FETCH("kern.ipc.shm_use_phys", &shm_use_phys);

	/* calc size. */
	size_t cb = sizeof(*pGlobals);
	cb += si.shmmni * sizeof(struct shmid_ds);

        /*
         * Retake the sem and allocate the memory.
         */
        rc = __libc_spmLock(&RegRec, &pSPMHdr);
        if (rc)
            LIBCLOG_ERROR_RETURN_INT(rc);
        if (!pSPMHdr->pSysVShm)
        {
            pGlobals = (struct __libc_SysV_Shm *)__libc_spmAllocLocked(cb);
            if (!pGlobals)
            {
                __libc_spmUnlock(&RegRec);
                LIBCLOG_ERROR_RETURN_INT(-ENOMEM);
            }
	
            /* init globals */
	    bzero(pGlobals, cb);
	    pGlobals->uVersion = 0x00020000;
	    pGlobals->cUsers = 0;
	    //pGlobals->shm_last_free = 0;
	    //pGlobals->shm_nused = 0;
	    //pGlobals->shm_committed = 0;
            pGlobals->shmalloced = si.shmmni;
	    struct shmid_ds *shmsegs = pGlobals->shmsegs = (struct shmid_ds *)(pGlobals + 1);
	    int i;
	    for (i = 0; i < si.shmmni; i++) {
		    shmsegs[i].shm_perm.mode = SHMSEG_FREE;
		    //shmsegs[i].shm_perm.seq = 0;
	    }
	    pGlobals->shminfo = si;
	
	    /* Set globals. */
            pSPMHdr->pSysVShm = pGlobals;
            gpGlobals = pGlobals;
	}
    }
    else
    {
        if ((pGlobals->uVersion >> 16) != (0x00020000 >> 16))
        {
            __libc_spmUnlock(&RegRec);
            __libc_Back_panic(0, NULL, "sysv_shm - pGlobal->uVersion %#x is not compatible with this LIBC. Don't mix incompatible LIBCs!\n",
                              pGlobals->uVersion);
        }
    }

    /*
     * Open or create the semaphores.
     */
    if (!pGlobals->cUsers)
    {
        /* create */
        rc = __libc_Back_safesemMtxCreate(&pGlobals->mtx, 1);
	if (!rc)
            rc = __libc_Back_safesemEvCreate(&pGlobals->evDaemon, &pGlobals->mtx, 1);
    }
    else
    {
        /* open */
        rc = __libc_Back_safesemMtxOpen(&pGlobals->mtx);
	if (!rc)
            rc = __libc_Back_safesemEvOpen(&pGlobals->evDaemon);
    }
    __atomic_increment_u32(&pGlobals->cUsers);

    /*
     * Register exit list handler and release the SPM lock.
     */
    __libc_spmRegTerm(shmexit_myhook);
    __libc_spmUnlock(&RegRec);

    /*
     * Update seminfo and return.
     */
    shminfo = pGlobals->shminfo;
    if (!rc)
	LIBCLOG_ERROR_RETURN_INT(rc);
    rc = -__libc_native2errno(rc);
    LIBCLOG_RETURN_INT(rc);
}

#if 0
static int
sysctl_shmsegs(SYSCTL_HANDLER_ARGS)
{

	return (SYSCTL_OUT(req, shmsegs, shmalloced * sizeof(shmsegs[0])));
}
#endif

