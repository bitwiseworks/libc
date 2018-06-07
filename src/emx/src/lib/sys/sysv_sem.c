/*-
 * Implementation of SVID semaphores
 *
 * Author:  Daniel Boulet
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */

#include "libc-alias.h"
#include <sys/cdefs.h>
//__FBSDID("$FreeBSD: src/sys/kern/sysv_sem.c,v 1.70.2.3 2005/03/22 17:20:07 sam Exp $");

#include <sys/types.h>
#define _KERNEL
#include <sys/sem.h>
#undef _KERNEL
#define INCL_DOSSEMAPHORES
#define INCL_DOSEXCEPTIONS
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
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_IPC
#include <InnoTekLIBC/logstrict.h>

/* debug printf */
#define DPRINTF(a)	LIBCLOG_MSG a


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/**
 * Representation of a semaphore id.
 */
struct semid_kernel
{
    /** Semaphore id. */
    struct semid_ds u;
    /** Event semaphore to wait on. */
    __LIBC_SAFESEMEV    ev;
    /** Mutex protecting the semid_ds and ev members. */
    __LIBC_SAFESEMMTX   mtx;
};

/**
 * SysV Sempahore globals residing in the shared process manager heap.
 */
struct __libc_SysV_Sem
{
    /** Structure version. */
    unsigned            uVersion;
    /** Number of users.
     * This is primarily used for knowing when to re-create the semaphores.
     */
    volatile uint32_t	cUsers;
    /** Number of active semaphores. */
    int                 semtot;
    /** Sempahores id pool. */
    struct semid_kernel *sema;
    /** Semaphore pool. */
    struct sem         *sem;
    /** Mutex protecting the undo list and other global sem stuff. */
    __LIBC_SAFESEMMTX   mtx;
    /** list of active undo structures. */
    SLIST_HEAD(, sem_undo) semu_list;
    /** undo structure pool. */
    int	               *semu;

    /** SysV Semaphore info. (Contains all the array sizes and such.) */
    struct seminfo      seminfo;
};

/** Pointer to the globals. */
static struct __libc_SysV_Sem *gpGlobals;

//static struct mtx	sem_mtx;	/* semaphore global lock */ == spm mutex
//static int	semtot = 0;
//static struct semid_ds *sema;	/* semaphore id pool */
//static struct mtx *sema_mtx;	/* semaphore id pool mutexes*/
//static struct sem *sem;		/* semaphore pool */
//SLIST_HEAD(, sem_undo) semu_list;	/* list of active undo structures */
//static int	*semu;		/* undo structure pool */

//#define SEMUNDO_MTX		gpGlobals->mtx
#define SEMUNDO_LOCK()		__libc_Back_safesemMtxLock(&gpGlobals->mtx);
#define SEMUNDO_UNLOCK()	__libc_Back_safesemMtxUnlock(&gpGlobals->mtx);
#define SEMUNDO_LOCKASSERT(how)	do {} while (0) //mtx_assert(&gpGlobals->mtx, (how));

struct sem {
	u_short	semval;		/* semaphore value */
	pid_t	sempid;		/* pid of last operation */
	u_short	volatile semncnt;	/* # awaiting semval > cval */
	u_short	volatile semzcnt;	/* # awaiting semval = 0 */
};

/*
 * Undo structure (one per process)
 */
struct sem_undo {
	SLIST_ENTRY(sem_undo) un_next;	/* ptr to next active undo structure */
	pid_t   un_proc;		/* owner of this structure */
	short	un_cnt;			/* # of active entries */
	struct undo {
		short	un_adjval;	/* adjust on exit values */
		short	un_num;		/* semaphore # */
		int	un_id;		/* semid */
	} un_ent[1];			/* undo entries */
};


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int seminit(void);
static void semexit_myhook(void);
//later static int sysctl_sema(SYSCTL_HANDLER_ARGS);
static int semvalid(int semid, struct semid_kernel *semaptr);
static struct sem_undo *semu_alloc(void);
static int semundo_adjust(struct sem_undo **supptr,
		int semid, int semnum, int adjval);
static void semundo_clear(int semid, int semnum);


/*
 * Configuration parameters
 */
#ifndef SEMMNI
#define SEMMNI	10		/* # of semaphore identifiers */
#endif
#ifndef SEMMNS
#define SEMMNS	60		/* # of semaphores in system */
#endif
#ifndef SEMUME
#define SEMUME	10		/* max # of undo entries per process */
#endif
#ifndef SEMMNU
#define SEMMNU	30		/* # of undo structures in system */
#endif

/* shouldn't need tuning */
#ifndef SEMMAP
#define SEMMAP	30		/* # of entries in semaphore map */
#endif
#ifndef SEMMSL
#define SEMMSL	SEMMNS		/* max # of semaphores per id */
#endif
#ifndef SEMOPM
#define SEMOPM	100		/* max # of operations per semop call */
#endif

#define SEMVMX	32767		/* semaphore maximum value */
#define SEMAEM	16384		/* adjust on exit max value */

/*
 * Due to the way semaphore memory is allocated, we have to ensure that
 * SEMUSZ is properly aligned.
 */

#define SEM_ALIGN(bytes) (((bytes) + (sizeof(long) - 1)) & ~(sizeof(long) - 1))

/* actual size of an undo structure */
#define SEMUSZ	SEM_ALIGN(offsetof(struct sem_undo, un_ent[SEMUME]))

/*
 * Macro to find a particular sem_undo vector
 */
#define SEMU(ix) \
	((struct sem_undo *)(((intptr_t)gpGlobals->semu)+ix * seminfo.semusz))

/*
 * semaphore info struct
 */
static struct seminfo seminfo = {
                SEMMAP,         /* # of entries in semaphore map */
                SEMMNI,         /* # of semaphore identifiers */
                SEMMNS,         /* # of semaphores in system */
                SEMMNU,         /* # of undo structures in system */
                SEMMSL,         /* max # of semaphores per id */
                SEMOPM,         /* max # of operations per semop call */
                SEMUME,         /* max # of undo entries per process */
                SEMUSZ,         /* size in bytes of undo structure */
                SEMVMX,         /* semaphore maximum value */
                SEMAEM          /* adjust on exit max value */
};

#if 0
SYSCTL_DECL(_kern_ipc);
SYSCTL_INT(_kern_ipc, OID_AUTO, semmap, CTLFLAG_RD/*W*/, &seminfo.semmap, 0,
    "Number of entries in the semaphore map");
SYSCTL_INT(_kern_ipc, OID_AUTO, semmni, CTLFLAG_RDTUN, &seminfo.semmni, 0,
    "Number of semaphore identifiers");
SYSCTL_INT(_kern_ipc, OID_AUTO, semmns, CTLFLAG_RDTUN, &seminfo.semmns, 0,
    "Maximum number of semaphores in the system");
SYSCTL_INT(_kern_ipc, OID_AUTO, semmnu, CTLFLAG_RDTUN, &seminfo.semmnu, 0,
    "Maximum number of undo structures in the system");
SYSCTL_INT(_kern_ipc, OID_AUTO, semmsl, CTLFLAG_RD/*W*/, &seminfo.semmsl, 0,
    "Max semaphores per id");
SYSCTL_INT(_kern_ipc, OID_AUTO, semopm, CTLFLAG_RDTUN, &seminfo.semopm, 0,
    "Max operations per semop call");
SYSCTL_INT(_kern_ipc, OID_AUTO, semume, CTLFLAG_RDTUN, &seminfo.semume, 0,
    "Max undo entries per process");
SYSCTL_INT(_kern_ipc, OID_AUTO, semusz, CTLFLAG_RDTUN, &seminfo.semusz, 0,
    "Size in bytes of undo structure");
SYSCTL_INT(_kern_ipc, OID_AUTO, semvmx, CTLFLAG_RD/*W*/, &seminfo.semvmx, 0,
    "Semaphore maximum value");
SYSCTL_INT(_kern_ipc, OID_AUTO, semaem, CTLFLAG_RD/*W*/, &seminfo.semaem, 0,
    "Adjust on exit max value");
SYSCTL_PROC(_kern_ipc, OID_AUTO, sema, CTLFLAG_RD,
    NULL, 0, sysctl_sema, "", "");
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


static int seminit(void)
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
    struct __libc_SysV_Sem *pGlobals = gpGlobals = pSPMHdr->pSysVSem;
    if (!pGlobals)
    {
        /*
         * We will have to initialize the semaphore stuff.
         * Since we're gonna parse environment first, release the SPM lock.
         */
        __libc_spmUnlock(&RegRec);

        /*
         * Get configuration options.
         */
        struct seminfo si = seminfo;
        TUNABLE_INT_FETCH("kern.ipc.semmap", &si.semmap);
        TUNABLE_INT_FETCH("kern.ipc.semmni", &si.semmni);
        TUNABLE_INT_FETCH("kern.ipc.semmns", &si.semmns);
        TUNABLE_INT_FETCH("kern.ipc.semmnu", &si.semmnu);
        TUNABLE_INT_FETCH("kern.ipc.semmsl", &si.semmsl);
        TUNABLE_INT_FETCH("kern.ipc.semopm", &si.semopm);
        TUNABLE_INT_FETCH("kern.ipc.semume", &si.semume);
        TUNABLE_INT_FETCH("kern.ipc.semusz", &si.semusz);
        TUNABLE_INT_FETCH("kern.ipc.semvmx", &si.semvmx);
        TUNABLE_INT_FETCH("kern.ipc.semaem", &si.semaem);

        /*
         * Calc the size of the stuff we need.
         */
        size_t cb = sizeof(*gpGlobals);
        cb += sizeof(struct semid_kernel) * si.semmni;
        cb += sizeof(struct sem) * si.semmns;
        cb += si.semmnu * si.semusz;

        /*
         * Retake the sem and allocate the memory.
         */
        rc = __libc_spmLock(&RegRec, &pSPMHdr);
        if (rc)
            LIBCLOG_ERROR_RETURN_INT(rc);
        if (!pSPMHdr->pSysVSem)
        {
            pGlobals = (struct __libc_SysV_Sem *)__libc_spmAllocLocked(cb);
            if (!pGlobals)
            {
                __libc_spmUnlock(&RegRec);
                LIBCLOG_ERROR_RETURN_INT(-ENOMEM);
            }

            /* init globals */
            bzero(pGlobals, cb);
            //pGlobals->cUsers    = 0;
            pGlobals->uVersion  = 0x00020000;
            //pGlobals->semtot    = 0;
            uint8_t *pu8  = (uint8_t *)(pGlobals + 1);
            pGlobals->sema      = (struct semid_kernel *)pu8;
            pu8          += sizeof(struct semid_kernel) * si.semmni;
            pGlobals->sem       = (struct sem *)pu8;
            pu8          += sizeof(struct sem) * si.semmns;
            SLIST_INIT(&pGlobals->semu_list);
            pGlobals->semu       = (int *)pu8;
            pGlobals->seminfo    = si;

            /* Set globals. */
            pSPMHdr->pSysVSem = pGlobals;
            gpGlobals = pGlobals;
        }
    }
    else
    {
        if ((pGlobals->uVersion >> 16) != (0x00020000 >> 16))
        {
            __libc_spmUnlock(&RegRec);
            __libc_Back_panic(0, NULL, "sysv_sem - pGlobal->uVersion %#x is not compatible with this LIBC. Don't mix incompatible LIBCs!\n",
                              pGlobals->uVersion);
        }
    }

    /*
     * Open or create the semaphores.
     */
    int i;
    if (!pGlobals->cUsers)
    {
        /* create */
        rc = __libc_Back_safesemMtxCreate(&pGlobals->mtx, 1);
        for (i = 0; i < pGlobals->seminfo.semmni && !rc; i++)
            rc = __libc_Back_safesemMtxCreate(&pGlobals->sema[i].mtx, 1);
        for (i = 0; i < pGlobals->seminfo.semmni && !rc; i++)
            rc = __libc_Back_safesemEvCreate(&pGlobals->sema[i].ev, &pGlobals->sema[i].mtx, 1);
    }
    else
    {
        /* open */
        rc = __libc_Back_safesemMtxOpen(&pGlobals->mtx);
        for (i = 0; i < pGlobals->seminfo.semmni && !rc; i++)
            rc = __libc_Back_safesemMtxOpen(&pGlobals->sema[i].mtx);
        for (i = 0; i < pGlobals->seminfo.semmni && !rc; i++)
            rc = __libc_Back_safesemEvOpen(&pGlobals->sema[i].ev);
    }
    __atomic_increment_u32(&pGlobals->cUsers);

    /*
     * Register exit list handler and release the SPM lock.
     */
    __libc_spmRegTerm(semexit_myhook);
    __libc_spmUnlock(&RegRec);


    /*
     * Update seminfo and return.
     */
    seminfo = pGlobals->seminfo;
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/*
 * Allocate a new sem_undo structure for a process
 * (returns ptr to structure or NULL if no more room)
 */

static struct sem_undo *
semu_alloc(void)
{
	int i;
	struct sem_undo *suptr;
	struct sem_undo **supptr;
	int attempt;

	SEMUNDO_LOCKASSERT(MA_OWNED);
	/*
	 * Try twice to allocate something.
	 * (we'll purge an empty structure after the first pass so
	 * two passes are always enough)
	 */

	for (attempt = 0; attempt < 2; attempt++) {
		/*
		 * Look for a free structure.
		 * Fill it in and return it if we find one.
		 */

		for (i = 0; i < seminfo.semmnu; i++) {
			suptr = SEMU(i);
			if (suptr->un_proc == 0) {
				SLIST_INSERT_HEAD(&gpGlobals->semu_list, suptr, un_next);
				suptr->un_cnt = 0;
				suptr->un_proc = fibGetPid();
				return(suptr);
			}
		}

		/*
		 * We didn't find a free one, if this is the first attempt
		 * then try to free a structure.
		 */

		if (attempt == 0) {
			/* All the structures are in use - try to free one */
			int did_something = 0;

			SLIST_FOREACH_PREVPTR(suptr, supptr, &gpGlobals->semu_list,
			    un_next) {
				if (suptr->un_cnt == 0) {
					suptr->un_proc = 0;
					did_something = 1;
					*supptr = SLIST_NEXT(suptr, un_next);
					break;
				}
			}

			/* If we didn't free anything then just give-up */
			if (!did_something)
				return(NULL);
		} else {
			/*
			 * The second pass failed even though we freed
			 * something after the first pass!
			 * This is IMPOSSIBLE!
			 */
			panic("semu_alloc - second attempt failed");
		}
	}
	return (NULL);
}

/*
 * Adjust a particular entry for a particular proc
 */

static int
semundo_adjust(supptr, semid, semnum, adjval)
	struct sem_undo **supptr;
	int semid, semnum;
	int adjval;
{
	pid_t  pid = fibGetPid();
	struct sem_undo *suptr;
	struct undo *sunptr;
	int i;

	SEMUNDO_LOCKASSERT(MA_OWNED);
	/* Look for and remember the sem_undo if the caller doesn't provide
	   it */

	suptr = *supptr;
	if (suptr == NULL) {
		SLIST_FOREACH(suptr, &gpGlobals->semu_list, un_next) {
			if (suptr->un_proc == pid) {
				*supptr = suptr;
				break;
			}
		}
		if (suptr == NULL) {
			if (adjval == 0)
				return(0);
			suptr = semu_alloc();
			if (suptr == NULL)
				return(ENOSPC);
			*supptr = suptr;
		}
	}

	/*
	 * Look for the requested entry and adjust it (delete if adjval becomes
	 * 0).
	 */
	sunptr = &suptr->un_ent[0];
	for (i = 0; i < suptr->un_cnt; i++, sunptr++) {
		if (sunptr->un_id != semid || sunptr->un_num != semnum)
			continue;
		if (adjval != 0) {
			adjval += sunptr->un_adjval;
			if (adjval > seminfo.semaem || adjval < -seminfo.semaem)
				return (ERANGE);
		}
		sunptr->un_adjval = adjval;
		if (sunptr->un_adjval == 0) {
			suptr->un_cnt--;
			if (i < suptr->un_cnt)
				suptr->un_ent[i] =
				    suptr->un_ent[suptr->un_cnt];
		}
		return(0);
	}

	/* Didn't find the right entry - create it */
	if (adjval == 0)
		return(0);
	if (adjval > seminfo.semaem || adjval < -seminfo.semaem)
		return (ERANGE);
	if (suptr->un_cnt != seminfo.semume) {
		sunptr = &suptr->un_ent[suptr->un_cnt];
		suptr->un_cnt++;
		sunptr->un_adjval = adjval;
		sunptr->un_id = semid; sunptr->un_num = semnum;
	} else
		return(EINVAL);
	return(0);
}

static void
semundo_clear(semid, semnum)
	int semid, semnum;
{
	struct sem_undo *suptr;

	SEMUNDO_LOCKASSERT(MA_OWNED);
	SLIST_FOREACH(suptr, &gpGlobals->semu_list, un_next) {
		struct undo *sunptr = &suptr->un_ent[0];
		int i = 0;

		while (i < suptr->un_cnt) {
			if (sunptr->un_id == semid) {
				if (semnum == -1 || sunptr->un_num == semnum) {
					suptr->un_cnt--;
					if (i < suptr->un_cnt) {
						suptr->un_ent[i] =
						  suptr->un_ent[suptr->un_cnt];
						continue;
					}
				}
				if (semnum != -1)
					break;
			}
			i++, sunptr++;
		}
	}
}

static int
semvalid(semid, semaptr)
	int semid;
	struct semid_kernel *semaptr;
{

	return ((semaptr->u.sem_perm.mode & SEM_ALLOC) == 0 ||
	    semaptr->u.sem_perm.seq != IPCID_TO_SEQ(semid) ? EINVAL : 0);
}


/**
 * semctl syscall
 */
int __libc_Back_sysvSemCtl(int semid, int semnum, int cmd, union semun real_arg)
{
	LIBCLOG_ENTER("semid=%d semnum=%d cmd=%d real_arg=%p\n", semid, semnum, cmd, (void *)real_arg.buf);
	u_short *array;
	int i, rval, error;
	struct semid_ds sbuf;
	struct semid_kernel *semaptr;
	u_short usval, count;
        uid_t uid;
        __LIBC_PSAFESEMMTX pmtx;
        int semid_in = semid;

        /*
         * Lazy init.
         */
        if (!gpGlobals)
        {
            error = seminit();
            if (error)
                LIBCLOG_ERROR_RETURN_INT(error);
        }
	struct sem * sem = gpGlobals->sem;
	struct semid_kernel * sema = gpGlobals->sema;

	array = NULL;
	error = 0;
	rval = 0;
        pmtx = NULL;

	switch(cmd) {
	case SEM_STAT:
		if (semid < 0 || semid >= seminfo.semmni)
			LIBCLOG_ERROR_RETURN_INT(-EINVAL);
		semaptr = &sema[semid];
                __libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((semaptr->u.sem_perm.mode & SEM_ALLOC) == 0) {
			error = EINVAL;
			goto done2;
		}
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_R)))
			goto done2;
                __libc_Back_safesemMtxUnlock(pmtx); pmtx = NULL;
		memcpy(real_arg.buf, &semaptr->u, sizeof(struct semid_ds));
		rval = IXSEQ_TO_IPCID(semid, semaptr->u.sem_perm);
		LIBCLOG_RETURN_INT(rval);
	}

	semid = IPCID_TO_IX(semid);
	if (semid < 0 || semid >= seminfo.semmni)
		LIBCLOG_ERROR_RETURN_INT(-EINVAL);

	semaptr = &sema[semid];

	switch (cmd) {
	case IPC_RMID:
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_M)))
			goto done2;
                uid = __libc_spmGetId(__LIBC_SPMID_EUID);
		semaptr->u.sem_perm.cuid = uid;
		semaptr->u.sem_perm.uid = uid;
		gpGlobals->semtot -= semaptr->u.sem_nsems;
		for (i = semaptr->u.sem_base - sem; i < gpGlobals->semtot; i++)
			sem[i] = sem[i + semaptr->u.sem_nsems];
		for (i = 0; i < seminfo.semmni; i++) {
			if ((sema[i].u.sem_perm.mode & SEM_ALLOC) &&
			    sema[i].u.sem_base > semaptr->u.sem_base)
				sema[i].u.sem_base -= semaptr->u.sem_nsems;
		}
		semaptr->u.sem_perm.mode = 0;
		SEMUNDO_LOCK();
		semundo_clear(semid, -1);
		SEMUNDO_UNLOCK();
		__libc_Back_safesemEvWakeup(&semaptr->ev);
		break;

	case IPC_SET:
		if ((error = copyin(real_arg.buf, &sbuf, sizeof(sbuf))) != 0)
			goto done2;
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_M)))
			goto done2;
		semaptr->u.sem_perm.uid = sbuf.sem_perm.uid;
		semaptr->u.sem_perm.gid = sbuf.sem_perm.gid;
		semaptr->u.sem_perm.mode = (semaptr->u.sem_perm.mode & ~0777) |
		    (sbuf.sem_perm.mode & 0777);
		semaptr->u.sem_ctime = fibGetUnixSeconds();
		break;

	case IPC_STAT:
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_R)))
			goto done2;
		sbuf = semaptr->u;
		__libc_Back_safesemMtxUnlock(pmtx); pmtx = NULL;
		error = copyout(semaptr, real_arg.buf,
				sizeof(struct semid_ds));
		break;

	case GETNCNT:
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_R)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->u.sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		rval = semaptr->u.sem_base[semnum].semncnt;
		break;

	case GETPID:
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_R)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->u.sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		rval = semaptr->u.sem_base[semnum].sempid;
		break;

	case GETVAL:
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_R)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->u.sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		rval = semaptr->u.sem_base[semnum].semval;
		break;

	case GETALL:
		array = _hmalloc(sizeof(*array) * semaptr->u.sem_nsems);
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_R)))
			goto done2;
		for (i = 0; i < semaptr->u.sem_nsems; i++)
			array[i] = semaptr->u.sem_base[i].semval;
		__libc_Back_safesemMtxUnlock(pmtx); pmtx = NULL;
		error = copyout(array, real_arg.array,
		    i * sizeof(real_arg.array[0]));
		break;

	case GETZCNT:
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_R)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->u.sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		rval = semaptr->u.sem_base[semnum].semzcnt;
		break;

	case SETVAL:
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_W)))
			goto done2;
		if (semnum < 0 || semnum >= semaptr->u.sem_nsems) {
			error = EINVAL;
			goto done2;
		}
		if (real_arg.val < 0 || real_arg.val > seminfo.semvmx) {
			error = ERANGE;
			goto done2;
		}
		semaptr->u.sem_base[semnum].semval = real_arg.val;
		SEMUNDO_LOCK();
		semundo_clear(semid, semnum);
		SEMUNDO_UNLOCK();
		__libc_Back_safesemEvWakeup(&semaptr->ev);
		break;

	case SETALL:
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
raced:
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		count = semaptr->u.sem_nsems;
		__libc_Back_safesemMtxUnlock(pmtx); pmtx = NULL;
		array = _hmalloc(sizeof(*array) * count);
		error = copyin(real_arg.array, array, count * sizeof(*array));
		if (error)
			break;
		__libc_Back_safesemMtxLock(pmtx = &semaptr->mtx);
		if ((error = semvalid(semid_in, semaptr)) != 0)
			goto done2;
		/* we could have raced? */
		if (count != semaptr->u.sem_nsems) {
			free(array);
			array = NULL;
			goto raced;
		}
		if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, IPC_W)))
			goto done2;
		for (i = 0; i < semaptr->u.sem_nsems; i++) {
			usval = array[i];
			if (usval > seminfo.semvmx) {
				error = ERANGE;
				break;
			}
			semaptr->u.sem_base[i].semval = usval;
		}
		SEMUNDO_LOCK();
		semundo_clear(semid, -1);
		SEMUNDO_UNLOCK();
		__libc_Back_safesemEvWakeup(&semaptr->ev);
		break;

	default:
		error = EINVAL;
		break;
	}

done2:
	if (pmtx)
		__libc_Back_safesemMtxUnlock(pmtx);

	if (array != NULL)
		free(array);
        if (error)
		rval = -error;
	if (rval >= 0)
		LIBCLOG_RETURN_INT(rval);
	LIBCLOG_ERROR_RETURN_INT(rval);
}


/**
 * sysget syscall.
 */
int __libc_Back_sysvSemGet(key_t key, int nsems, int semflg)
{
	LIBCLOG_ENTER("key=%#lx nsems=%d semflg=%#x\n", key, nsems, semflg);
	int error = 0;
        int semid;

        /*
         * Lazy init.
         */
        if (!gpGlobals)
        {
            error = seminit();
            if (error)
                LIBCLOG_ERROR_RETURN_INT(error);
        }
	struct sem * sem = gpGlobals->sem;
	struct semid_kernel * sema = gpGlobals->sema;

	error = __libc_Back_safesemMtxLock(&gpGlobals->mtx);
        if (error)
            LIBCLOG_ERROR_RETURN_INT(error);

	if (key != IPC_PRIVATE) {
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].u.sem_perm.mode & SEM_ALLOC) &&
			    sema[semid].u.sem_perm.key == key)
				break;
		}
		if (semid < seminfo.semmni) {
			DPRINTF(("found public key\n"));
			if ((error = -__libc_spmCanIPC(&sema[semid].u.sem_perm,
			    semflg & 0700))) {
				goto done2;
			}
			if (nsems > 0 && sema[semid].u.sem_nsems < nsems) {
				DPRINTF(("too small\n"));
				error = EINVAL;
				goto done2;
			}
			if ((semflg & IPC_CREAT) && (semflg & IPC_EXCL)) {
				DPRINTF(("not exclusive\n"));
				error = EEXIST;
				goto done2;
			}
			goto found;
		}
	}

	DPRINTF(("need to allocate the semid_ds\n"));
	if (key == IPC_PRIVATE || (semflg & IPC_CREAT)) {
		if (nsems <= 0 || nsems > seminfo.semmsl) {
			DPRINTF(("nsems out of range (0<%d<=%d)\n", nsems,
			    seminfo.semmsl));
			error = EINVAL;
			goto done2;
		}
		if (nsems > seminfo.semmns - gpGlobals->semtot) {
			DPRINTF((
			    "not enough semaphores left (need %d, got %d)\n",
			    nsems, seminfo.semmns - gpGlobals->semtot));
			error = ENOSPC;
			goto done2;
		}
		for (semid = 0; semid < seminfo.semmni; semid++) {
			if ((sema[semid].u.sem_perm.mode & SEM_ALLOC) == 0)
				break;
		}
		if (semid == seminfo.semmni) {
			DPRINTF(("no more semid_ds's available\n"));
			error = ENOSPC;
			goto done2;
		}
		DPRINTF(("semid %d is available\n", semid));
		sema[semid].u.sem_perm.key = key;
		sema[semid].u.sem_perm.cuid = sema[semid].u.sem_perm.uid = __libc_spmGetId(__LIBC_SPMID_EUID);
		sema[semid].u.sem_perm.cgid = sema[semid].u.sem_perm.gid = __libc_spmGetId(__LIBC_SPMID_EGID);
		sema[semid].u.sem_perm.mode = (semflg & 0777) | SEM_ALLOC;
		sema[semid].u.sem_perm.seq =
		    (sema[semid].u.sem_perm.seq + 1) & 0x7fff;
		sema[semid].u.sem_nsems = nsems;
		sema[semid].u.sem_otime = 0;
		sema[semid].u.sem_ctime = fibGetUnixSeconds();
		sema[semid].u.sem_base = &sem[gpGlobals->semtot];
		gpGlobals->semtot += nsems;
		bzero(sema[semid].u.sem_base,
		    sizeof(sema[semid].u.sem_base[0])*nsems);
		DPRINTF(("sembase = %p, next = %p\n", (void *)sema[semid].u.sem_base, (void *)&sem[gpGlobals->semtot]));
	} else {
		DPRINTF(("didn't find it and wasn't asked to create it\n"));
		error = ENOENT;
		goto done2;
	}

found:
        error = -IXSEQ_TO_IPCID(semid, sema[semid].u.sem_perm);
done2:
	__libc_Back_safesemMtxUnlock(&gpGlobals->mtx);
	if (!error)
		LIBCLOG_RETURN_INT(-error);
	LIBCLOG_ERROR_RETURN_INT(-error);
}

/**
 * Args to __libc_Back_sysvSemSleepDone.
 */
struct SleepDoneArgs
{
    int semid;
    struct semid_kernel *semaptr;
    uint16_t volatile *pu16;
};

/**
 * Wakeup function, called before signals are delivered.
 */
static void SleepDone(void *pvUser)
{
    struct SleepDoneArgs *pArgs = (struct SleepDoneArgs *)pvUser;

    /*
     * Make sure that the semaphore still exists
     */
    if ((pArgs->semaptr->u.sem_perm.mode & SEM_ALLOC) == 0 ||
	pArgs->semaptr->u.sem_perm.seq != IPCID_TO_SEQ(pArgs->semid)) {
	    return;
    }

    /*
     * The semaphore is still alive.  Readjust the count of
     * waiting processes.
     */
    __atomic_decrement_u16(pArgs->pu16);
}

/**
 * semop syscall.
 */
int __libc_Back_sysvSemOp(int semid, struct sembuf *sops_user, size_t nsops)
{
	LIBCLOG_ENTER("semid=%d sops_user=%p nsops=%d\n", semid, (void *)sops_user, nsops);
#define SMALL_SOPS	8
        struct sembuf *sops;
	struct sembuf small_sops[SMALL_SOPS];
	struct semid_kernel *semaptr;
	struct sembuf *sopptr = 0;
	struct sem *semptr = 0;
	struct sem_undo *suptr;
	size_t i, j, k;
	int error;
	int do_wakeup, do_undos;
        int semid_in = semid;

        /*
         * Lazy init
         */
        if (!gpGlobals)
        {
            error = seminit();
            if (error)
                LIBCLOG_ERROR_RETURN_INT(error);
        }
	struct semid_kernel * sema = gpGlobals->sema;


	semid = IPCID_TO_IX(semid);	/* Convert back to zero origin */

	if (semid < 0 || semid >= seminfo.semmni)
		LIBCLOG_ERROR_RETURN_INT(-EINVAL);

	/* Allocate memory for sem_ops */
	if (nsops <= SMALL_SOPS)
		sops = small_sops;
	else if (nsops <= seminfo.semopm)
		sops = _hmalloc(nsops * sizeof(*sops));
	else {
		DPRINTF(("too many sops (max=%d, nsops=%d)\n", seminfo.semopm,
		    nsops));
		LIBCLOG_ERROR_RETURN_INT(-E2BIG);
	}
	if ((error = copyin(sops_user, sops, nsops * sizeof(sops[0]))) != 0) {
		DPRINTF(("error = %d from copyin(%p, %p, %d)\n", error,
		    (void *)sops_user, (void *)sops, nsops * sizeof(sops[0])));
		if (sops != small_sops)
			free(sops);
		LIBCLOG_ERROR_RETURN_INT(error);
	}

	semaptr = &sema[semid];
	error = __libc_Back_safesemMtxLock(&semaptr->mtx);
        if (error) {
                if (sops != small_sops)
                        free(sops);
                LIBCLOG_ERROR_RETURN_INT(error);
        }

	if ((semaptr->u.sem_perm.mode & SEM_ALLOC) == 0) {
		error = EINVAL;
		goto done2;
	}
	if (semaptr->u.sem_perm.seq != IPCID_TO_SEQ(semid_in)) {
		error = EINVAL;
		goto done2;
	}
	/*
	 * Initial pass thru sops to see what permissions are needed.
	 * Also perform any checks that don't need repeating on each
	 * attempt to satisfy the request vector.
	 */
	j = 0;		/* permission needed */
	do_undos = 0;
	for (i = 0; i < nsops; i++) {
		sopptr = &sops[i];
		if (sopptr->sem_num >= semaptr->u.sem_nsems) {
			error = EFBIG;
			goto done2;
		}
		if (sopptr->sem_flg & SEM_UNDO && sopptr->sem_op != 0)
			do_undos = 1;
		j |= (sopptr->sem_op == 0) ? SEM_R : SEM_A;
	}

	if ((error = -__libc_spmCanIPC(&semaptr->u.sem_perm, j))) {
		DPRINTF(("error = %d from ipaccess\n", error));
		goto done2;
	}

	/*
	 * Loop trying to satisfy the vector of requests.
	 * If we reach a point where we must wait, any requests already
	 * performed are rolled back and we go to sleep until some other
	 * process wakes us up.  At this point, we start all over again.
	 *
	 * This ensures that from the perspective of other tasks, a set
	 * of requests is atomic (never partially satisfied).
	 */
	for (;;) {
		do_wakeup = 0;
		error = 0;	/* error return if necessary */

		for (i = 0; i < nsops; i++) {
			sopptr = &sops[i];
			semptr = &semaptr->u.sem_base[sopptr->sem_num];

			DPRINTF((
			    "semop:  semaptr=%p, sem_base=%p, "
			    "semptr=%p, sem[%d]=%d : op=%d, flag=%s\n",
			    (void *)semaptr, (void *)semaptr->u.sem_base, (void *)semptr,
			    sopptr->sem_num, semptr->semval, sopptr->sem_op,
			    (sopptr->sem_flg & IPC_NOWAIT) ?
			    "nowait" : "wait"));

			if (sopptr->sem_op < 0) {
				if (semptr->semval + sopptr->sem_op < 0) {
					DPRINTF(("semop:  can't do it now\n"));
					break;
				} else {
					semptr->semval += sopptr->sem_op;
					if (semptr->semval == 0 &&
					    semptr->semzcnt > 0)
						do_wakeup = 1;
				}
			} else if (sopptr->sem_op == 0) {
				if (semptr->semval != 0) {
					DPRINTF(("semop:  not zero now\n"));
					break;
				}
			} else if (semptr->semval + sopptr->sem_op >
			    seminfo.semvmx) {
				error = ERANGE;
				break;
			} else {
				if (semptr->semncnt > 0)
					do_wakeup = 1;
				semptr->semval += sopptr->sem_op;
			}
		}

		/*
		 * Did we get through the entire vector?
		 */
		if (i >= nsops)
			goto done;

		/*
		 * No ... rollback anything that we've already done
		 */
		DPRINTF(("semop:  rollback 0 through %d\n", i-1));
		for (j = 0; j < i; j++)
			semaptr->u.sem_base[sops[j].sem_num].semval -=
			    sops[j].sem_op;

		/* If we detected an error, return it */
		if (error != 0)
			goto done2;

		/*
		 * If the request that we couldn't satisfy has the
		 * NOWAIT flag set then return with EAGAIN.
		 */
		if (sopptr->sem_flg & IPC_NOWAIT) {
			error = EAGAIN;
			goto done2;
		}

                struct SleepDoneArgs Args;
                Args.semaptr = semaptr;
                Args.semid = semid;
                Args.pu16 = sopptr->sem_op == 0 ? &semptr->semzcnt : &semptr->semncnt;
		__atomic_increment_u16(Args.pu16);
		DPRINTF(("semop:  good night (%d)!\n", sopptr->sem_op));
		error = -__libc_Back_safesemEvSleep(&semaptr->ev, SleepDone, &Args);
		DPRINTF(("semop:  good morning (error=%d)!\n", error));
		/* return code is checked below, after sem[nz]cnt-- */

		/*
		 * Make sure that the semaphore still exists
		 */
		if ((semaptr->u.sem_perm.mode & SEM_ALLOC) == 0 ||
		    semaptr->u.sem_perm.seq != IPCID_TO_SEQ(semid_in)) {
			error = EIDRM;
			goto done2;
		}

		/*
		 * Is it really morning, or was our sleep interrupted?
		 * (Delayed check of msleep() return code because we
		 * need to decrement sem[nz]cnt either way.)
		 */
		if (error != 0) {
			error = EINTR;
			goto done2;
		}
		DPRINTF(("semop:  good morning!\n"));
	}

done:
	/*
	 * Process any SEM_UNDO requests.
	 */
	if (do_undos) {
		SEMUNDO_LOCK();
		suptr = NULL;
		for (i = 0; i < nsops; i++) {
			/*
			 * We only need to deal with SEM_UNDO's for non-zero
			 * op's.
			 */
			int adjval;

			if ((sops[i].sem_flg & SEM_UNDO) == 0)
				continue;
			adjval = sops[i].sem_op;
			if (adjval == 0)
				continue;
			error = semundo_adjust(&suptr, semid,
			    sops[i].sem_num, -adjval);
			if (error == 0)
				continue;

			/*
			 * Oh-Oh!  We ran out of either sem_undo's or undo's.
			 * Rollback the adjustments to this point and then
			 * rollback the semaphore ups and down so we can return
			 * with an error with all structures restored.  We
			 * rollback the undo's in the exact reverse order that
			 * we applied them.  This guarantees that we won't run
			 * out of space as we roll things back out.
			 */
			for (j = 0; j < i; j++) {
				k = i - j - 1;
				if ((sops[k].sem_flg & SEM_UNDO) == 0)
					continue;
				adjval = sops[k].sem_op;
				if (adjval == 0)
					continue;
				if (semundo_adjust(&suptr, semid,
				    sops[k].sem_num, adjval) != 0)
					panic("semop - can't undo undos");
			}

			for (j = 0; j < nsops; j++)
				semaptr->u.sem_base[sops[j].sem_num].semval -=
				    sops[j].sem_op;

			DPRINTF(("error = %d from semundo_adjust\n", error));
			SEMUNDO_UNLOCK();
			goto done2;
		} /* loop through the sops */
		SEMUNDO_UNLOCK();
	} /* if (do_undos) */

	/* We're definitely done - set the sempid's and time */
	for (i = 0; i < nsops; i++) {
		sopptr = &sops[i];
		semptr = &semaptr->u.sem_base[sopptr->sem_num];
		semptr->sempid = fibGetPid();
	}
	semaptr->u.sem_otime = fibGetUnixSeconds();

	/*
	 * Do a wakeup if any semaphore was up'd whilst something was
	 * sleeping on it.
	 */
	if (do_wakeup) {
		DPRINTF(("semop:  doing wakeup\n"));
		__libc_Back_safesemEvWakeup(&semaptr->ev);
		DPRINTF(("semop:  back from wakeup\n"));
	}
	DPRINTF(("semop:  done\n"));
done2:
	__libc_Back_safesemMtxUnlock(&semaptr->mtx);
	if (sops != small_sops)
		free(sops);
	if (!error)
		LIBCLOG_RETURN_INT(-error);
	LIBCLOG_ERROR_RETURN_INT(-error);
}

/*
 * Go through the undo structures for this process and apply the adjustments to
 * semaphores.
 */
static void
semexit_myhook(void)
{
    	LIBCLOG_ENTER("\n");
	static volatile int fDone = 0;
	struct sem_undo *suptr;
	struct sem_undo **supptr;

        /*
         * If no pointer to globals then there is nothing to do (nearly impossible).
         */
        if (!gpGlobals || fDone)
	    LIBCLOG_RETURN_VOID();
        fDone = 1;

	/*
	 * Go through the chain of undo vectors looking for one
	 * associated with this process.
	 */
        pid_t  pid = fibGetPid();
	SEMUNDO_LOCK();
	SLIST_FOREACH_PREVPTR(suptr, supptr, &gpGlobals->semu_list, un_next) {
		if (suptr->un_proc == pid)
			break;
	}
	SEMUNDO_UNLOCK();

	if (suptr == NULL)
		goto dereference;

	DPRINTF(("proc %d has undo structure with %d entries\n", pid,
	    suptr->un_cnt));

	/*
	 * If there are any active undo elements then process them.
	 */
	if (suptr->un_cnt > 0) {
		int ix;

		for (ix = 0; ix < suptr->un_cnt; ix++) {
			int semid = suptr->un_ent[ix].un_id;
			int semnum = suptr->un_ent[ix].un_num;
			int adjval = suptr->un_ent[ix].un_adjval;
			struct semid_kernel *semaptr;

			semaptr = &gpGlobals->sema[semid];
			SEMUNDO_LOCK();
			__libc_Back_safesemMtxUnlock(&semaptr->mtx);
			if ((semaptr->u.sem_perm.mode & SEM_ALLOC) == 0) {
				panic("semexit - semid not allocated");
				goto skip;
			}
			if (semnum >= semaptr->u.sem_nsems) {
				panic("semexit - semnum out of range");
				goto skip;
			}

			DPRINTF((
			    "semexit:  %d id=%d num=%d(adj=%d) ; sem=%d\n",
			    suptr->un_proc, suptr->un_ent[ix].un_id,
			    suptr->un_ent[ix].un_num,
			    suptr->un_ent[ix].un_adjval,
			    semaptr->u.sem_base[semnum].semval));

			if (adjval < 0) {
				if (semaptr->u.sem_base[semnum].semval < -adjval)
					semaptr->u.sem_base[semnum].semval = 0;
				else
					semaptr->u.sem_base[semnum].semval +=
					    adjval;
			} else
				semaptr->u.sem_base[semnum].semval += adjval;

			__libc_Back_safesemEvWakeup(&semaptr->ev);
			DPRINTF(("semexit:  back from wakeup\n"));
		    skip:
			__libc_Back_safesemMtxUnlock(&semaptr->mtx);
			SEMUNDO_UNLOCK();
		}
	}

	/*
	 * Deallocate the undo vector.
	 */
	DPRINTF(("removing vector\n"));
	suptr->un_proc = 0;
	*supptr = SLIST_NEXT(suptr, un_next);


	/*
	 * Dereference the globals.
	 *
	 * This means zombies may waste global sems, but assuming zombies are not happening it's faster
	 * having OS/2 cleanup semaphores than doing it here.
	 */
dereference:			
	__atomic_decrement_u32(&gpGlobals->cUsers);
	gpGlobals = NULL;
        LIBCLOG_RETURN_VOID();
}

#if 0
static int
sysctl_sema(SYSCTL_HANDLER_ARGS)
{

	return (SYSCTL_OUT(req, sema,
	    sizeof(struct semid_ds) * seminfo.semmni));
}
#endif

