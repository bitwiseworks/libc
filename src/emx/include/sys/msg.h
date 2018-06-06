/* sys/msg.h (emx+gcc) */

#ifndef _SYS_MSG_H
#define _SYS_MSG_H

#include <sys/ipc.h>
#include <sys/_types.h>

#if !defined(_TIME_T_DECLARED) && !defined(_TIME_T) /* bird: emx */
typedef	__time_t	time_t;
#define	_TIME_T_DECLARED
#define _TIME_T                         /* bird: emx */
#endif


#if !defined (_MSG)
#define _MSG
struct msg
{
  struct msg *msg_next;
  long msg_type;
  short msg_ts;
  short msg_spot;
};
#endif

#if !defined (_MSQID_DS)
#define _MSQID_DS
struct msqid_ds
{
  struct ipc_perm msg_perm;
  struct msg *msg_first;
  struct msg *msg_last;
  unsigned short msg_cbytes;
  unsigned short msg_qnum;
  unsigned short msg_qbytes;
  unsigned short msg_lspid;
  unsigned short msg_lrpid;
  time_t msg_stime;
  time_t msg_rtime;
  time_t msg_ctime;
};
#endif

#if !defined (_MSGBUF)
#define _MSGBUF
struct msgbuf
{
  long mtype;
  char mtext[1];
};
#endif

#if !defined (_MSGINFO)
#define _MSGINFO
struct msginfo
{
  int msgmap;
  int msgmax;
  int msgmnb;
  int msgmni;
  int msgssz;
  int msgtql;
  unsigned short msgseg;
};
#endif

#if !defined (MSG_R)
#define MSG_R       0400
#define MSG_W       0200

#define	MSG_RWAIT   0x0200
#define	MSG_WWAIT   0x0400

#define	MSG_NOERROR 0x1000

#endif

#endif /* not SYS_MSG_H */
