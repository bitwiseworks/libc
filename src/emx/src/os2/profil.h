/* profil.h -- written by Holger Veit, donated to the public domain */

typedef ULONG DWORD;
typedef USHORT WORD;

#define	PRF_CM_INIT	0	/* initialize, must pass pid and profcmd */
#define	PRF_CM_START	1	/* start profiling (pid) */
#define	PRF_CM_STOP	2	/* stop profiling (pid) */
#define	PRF_CM_CLEAR	3	/* clear profile counters (pid) */
#define PRF_CM_DUMP	4	/* read out profiling data (pid, profret) */
#define PRF_CM_EXIT	5	/* exit profiling, delete profiling structures */

#pragma pack(1)

typedef struct {
	DWORD	sl_vaddr;	/* start of VA segment to profile */
	DWORD	sl_size;	/* length of VA segment */
	DWORD	sl_mode;	/* !=0 use PRF_VA* flags, */
				/* =0, simple count */
#define PRF_SL_SIMPCNT	0
#define PRF_SL_PRFVA	1

} PRFSLOT;

typedef struct {
	PRFSLOT	*cm_slots;	/* Virtual address slots */
	WORD	cm_nslots;	/* # of VA slots < 256 (!) */
	WORD	cm_flags;	/* command */
#define PRF_PROCESS_MT	0	/* profile proc+threads */
#define PRF_PROCESS_ST	1	/* profile proc only */
#define PRF_KERNEL	2	/* profile kernel */

#define PRF_VADETAIL	0	/* create detailed page counters */
#define PRF_VAHIT	4	/* create hit table */
#define PRF_VATOTAL	8	/* create total count for VA only */

#define PRF_FLGBITS	0x40	/* has a flgbits structure (?) */
#define PRF_WRAP	0x80	/* don't use: if hit table full, wrap */
				/* there is a bug in kernel, which */
				/* prevents this from correct working! */

/* status bits, don't ever set these (won't work, not masked, bug!) */
#define PRFS_RUNNING	0x100	/* profiling is active */
#define PRFS_THRDS	0x200	/* also profiling threads */
#define PRFS_HITOVFL	0x800	/* overflow in hit buffer */
#define PRFS_HEADER	0x1000	/* internally used */

	DWORD	cm_bufsz;	/* reserve # of bytes for buffers */
				/* e.g. for hit buffer or detailed */
				/* counters */
	WORD	cm_timval;	/* timer resolution */
				/* if 0, use default == 1000 */
	/* valid if PRF_FLAGBITS set */
	BYTE*	cm_flgbits;	/* vector of flag bits (?) */
	BYTE	cm_nflgs;	/* # of flag bits >= 2 if present */
} PRFCMD;	/* 19 bytes */

typedef struct {
	DWORD	va_vaddr;	/* virtual address of segment */
	DWORD	va_size;	/* length of segment */
	DWORD	va_flags;	/* == 8, va_cnt is valid */
	DWORD	va_reserved;	/* internally used */
	DWORD	va_cnt;		/* profile count */
} PRFVA;

typedef struct {
	BYTE	us_cmd;		/* command */
#define PRF_RET_GLOBAL	0	/* return global data */
				/* set us_thrdno for specific thread */
				/* us_buf = struct PRFRET0 */
#define	PRF_RET_VASLOTS	1	/* return VA slot data (PRFRET1) */
#define PRF_RET_VAHITS	2	/* return hit table (PRFRET2) */
#define PRF_RET_VADETAIL 3	/* return detailed counters (PRFRET3) */
				/* specify us_vaddr */

	WORD	us_thrdno;	/* thread requested for cmd=0 */
	DWORD	us_vaddr;	/* VA for cmd=3*/
	DWORD	us_bufsz;	/* length of return buffer */
	VOID	*us_buf;	/* return buffer */
} PRFRET;	/* 15 bytes */

typedef struct {
	WORD	r0_flags;	/* profile flags */
				/* see PRF_* defines */
	BYTE	r0_shift;	/* shift factor */
				/* 2^N = length of a segment for */
				/* detailed counters */
	DWORD	r0_idle;	/* count if process is idle */
	DWORD	r0_vm86;	/* count if process is in VM mode */
	DWORD	r0_kernel;	/* count if process is in kernel */
	DWORD	r0_shrmem;	/* count if process is in shr mem */
	DWORD	r0_unknown;	/* count if process is elsewhere */
	DWORD	r0_nhitbufs;	/* # of dwords in hitbufs */
	DWORD	r0_hitbufcnt;	/* # of entries in hit table */
	DWORD	r0_reserved1;	/* internally used */
	DWORD	r0_reserved2;	/* internally used */
	WORD	r0_timval;	/* timer resolution */
	BYTE	r0_errcnt;	/* error count */
	WORD	r0_nstruc1;	/* # of add structures 1 (?) */
	WORD	r0_nstruc2;	/* # of add structures 2 (?) */
} PRFRET0;

typedef struct {
	BYTE	r1_nslots;	/* # of slots (bug: prevents */
				/* correct # if #slots >255) */
	PRFVA	r1_slots[1];	/* slots */
} PRFRET1;

typedef struct {
	DWORD	r2_nhits;	/* # of entries in table */
	DWORD	r2_hits[1];	/* hit table */
} PRFRET2;

typedef struct {
	DWORD	r3_size;	/* size of segment */
	DWORD	r3_ncnts;	/* # of entries in table */
	DWORD	r3_cnts[1];	/* counters */
} PRFRET3;

#pragma pack()

ULONG DosProfile (DWORD func, PID pid, PRFCMD *profcmd, PRFRET *profret);
