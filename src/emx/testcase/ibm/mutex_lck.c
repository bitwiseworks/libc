/*
Program that uses mutex locking and unlocking

In this program, multiple threads attempt to lock and unlock a mutex using
the pthread_mutex_lock() function multiple times. These attempts cause each
thread to wait for the mutex to be unlocked. This example shows how a mutex
can be used to allow the creation of a simple thread-safe resource that is
scoped to a process. It also shows how too much serialization can decrease
the throughput of a multithreaded process.

Choose your browser's option to save to local disk to download this code example.
Send the program to your AS/400 and compile it using the development facilities
supplied there. This program was developed and tested on V4R4.

This small program that is furnished by IBM is a simple example to provide an
illustration. This example has not been thoroughly tested under all conditions.
IBM, therefore, cannot guarantee or imply reliability, serviceability, or function
of this program. All programs contained herein are provided to you "AS IS".
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE EXPRESSLY DISCLAIMED.

 */
/********************************************************************/
/*                                                                  */
/* test case:     mutex_lck.c                                       */
/*                                                                  */
/* objective:     Show semantics of pthread_mutex_lock() and        */
/*                   pthread_mutex_unlock()                         */
/*                                                                  */
/* scenario:      Create pthread_mutex                              */
/*                Lock mutex                                        */
/*                create a process scoped resource to serialize     */
/*                 access to with the mutex.                        */
/*                Create THREAD_COUNT lock threads                  */
/*                Wait for all lock threads to start.               */
/*                      Each lock thread trys to lock mutex (blocks)*/
/*                Unlock mutex - causes a thread to lock mutex      */
/*                      After aquireing mutex, lock thread uses     */
/*                       the process scoped resource.               */
/*                      Each lock thread delays and unlocks mutex   */
/*                      Each Lock thread repeats lock & unlock      */
/*                       for a DOIT_COUNT times & exits             */
/*                Join to all threads                               */
/*                Show the non corrupted resource after multiple    */
/*                 thread access.                                   */
/*                Destroy mutex                                     */
/*                                                                  */
/* description:   This demo is a valid case to have multiple        */
/*                   threads attempt to lock and unlock a mutex via */
/*                   pthread_mutex_lock() multiple times.           */
/*                   causes each to wait for the unlock of the      */
/*                   mutex. It shows how a mutex is used to allow   */
/*                   the creation of a simple thread safe process   */
/*                   scoped resource.                               */
/*                   This demo also shows how too much serialization*/
/*                   can decrease the throughput of a multithreaded */
/*                   process                                        */
/*                                                                  */
/* internal routines:   lock_thread()                               */
/*                                                                  */
/* external routines:   pthread_create()                            */
/*                      pthread_join()                              */
/*                      pthread_cancel()                            */
/*                      pthread_detach()                            */
/*                      pthread_mutex_init()                        */
/*                      pthread_mutex_destroy()                     */
/*                      pthread_mutex_lock()                        */
/*                      pthread_mutex_unlock()                      */
/*                      gettimeofday()                              */
/*                      sleep()                                     */
/*                                                                  */
/* usage notes:  Compile this program using                         */
/*               CRTCMOD  DEFINE('_MULTI_THREADED') and CRTPGM      */
/*               Call it with no parameters.                        */
/*               When using kernel threads on you must start a      */
/*               threaded program by using spawn(). You must set a  */
/*               special flag in the inheritance structure that     */
/*               causes spawn() to make the child process enabled   */
/*               for kernel threads. There is no support for        */
/*               running a threaded application in an interactive   */
/*               job. This means that a threaded application cannot */
/*               use the terminal for user interaction.             */
/*               Call it with no parameters. Output goes only       */
/*               to the screen.  To have output go to the CPA trace */
/*               user space and allow viewing via                   */
/*               DSPCPATRC  TYPE(*KERNEL)                           */
/*               uncomment the line '#define CPATRC_OUTPUT' below.  */
/*                                                                  */
/********************************************************************/

#include <pthread.h>
#include <errno.h>
#include <qp0ztrc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h> /* for usleep */

/*  Uncomment the following line to have all output go to the        */
/*  CPA trace user space so it can be displayed with                 */
/*  DSPCPATRC TYPE(*KERNEL)                                          */
#define CPATRC_OUTPUT
#ifdef CPATRC_OUPUT
 #undef printf
 #define printf Qp0zUprintf
#endif

/* Global constants                                                  */
#define  THREAD_COUNT   5
#define  DOIT_COUNT     5

/* Prototype of child threads, using the prototype to match the      */
/* pthread_startroutine_t is easiest                                 */
static void *lock_thread(void *);

/* Global variables for all threads to access                        */
static pthread_mutex_t   mymutex;
/* some resource that all threads are using. Access is serialized    */
/* using the mutex                                                   */
volatile static int      resource;

/* Any information can be passed to the thread start routine         */
typedef struct {
    int         tnum;       /* Thread number             */
    char        string[50]; /* Short description         */
} threadparmdata_t;

/* The main routine creates a process scoped resource and mutex      */
/* protecting that resource from multiple threads accessing it.      */
/* it then creates THREAD_COUNT threads who will manipulate the      */
/* resource.                                                         */
/* After all threads have completed, the resource will be displayed  */
/* no data corruption should result on the resource because of the   */
/* mutex to synchronize access to it                                 */
int main (int argc, char *argv[])
{
    int                  status, dstatus;
    int                  i;
    int                 *join_status;
    pthread_t            threads[THREAD_COUNT];
    threadparmdata_t     threadparms[THREAD_COUNT];
    struct timeval       current_time;


    printf("main: Entering %s\n", argv[0]);

    printf("main: Create a mutex\n");
    status = pthread_mutex_init(&mymutex, NULL);
    if (status == -1) {
        printf("main: Create mutex failed = %d\n", errno);
      }

    gettimeofday(&current_time, NULL);
    printf("main: Time before attempting to lock = %d\n",
           current_time.tv_sec);
    status = pthread_mutex_lock(&mymutex);
    if ( status != 0)
      {
        printf("main: Lock mutex failed, errno = %d\n", errno);
        /* Try to destroy mutex. NOTE: it may fail if another thread     */
        /* is holding the mutex. At this point, we don't care, _BUT_     */
        /* If this mutex destroy fails and the CPA process is exited with*/
        /* the mutex still existing, a Synchronization VLOG will most    */
        /* likely be cut indicating this.  mutexes should be destroyed by*/
        /* the application program and not allowed to simply go out of   */
        /* scope and be desctroyed by the system. The system treats this */
        /* as an abnormal condition                                      */
        ( void )pthread_mutex_destroy(&mymutex);
        return(1);
      } /* endif */

    printf("main: This thread is holding the mutex and can manipulate the\n"
           "main: resource at will without concern for other threads\n"
           "main: interacting in evil ways with the resource\n");
    resource = 0;
    printf("main: Create threads that will block on locking the mutex\n");
    /* Loop to create all secondary threads                                  */
    for ( i = 0; i < THREAD_COUNT ; i++ )
      {
        gettimeofday(&current_time, NULL);
        threadparms[i].tnum = i;
        sprintf(threadparms[i].string, "Thread #%d,time=%d\n",
                i, current_time.tv_sec);
        status = pthread_create(&threads[i],
				(const pthread_attr_t *)NULL,
                                lock_thread,
                                (void *)&threadparms[i]);
        if ( status != 0 ) {
            /* Create thread failed - break from loop and quit               */
            printf("main: Create thread # %d failed - terminate\n", i);
            /* Try to destroy mutex. NOTE: it may fail if another thread     */
            /* is holding the mutex. At this point, we don't care, _BUT_     */
            /* If this mutex destroy fails and the CPA process is exited with*/
            /* the mutex still existing, a Synchronization VLOG will most    */
            /* likely be cut indicating this.  mutexes should be destroyed by*/
            /* the application program and not allowed to simply go out of   */
            /* scope and be desctroyed by the system. The system treats this */
            /* as an abnormal condition                                      */
            ( void )pthread_mutex_destroy(&mymutex);
            return(2);
          } /* endif */
      } /* endfor */

    printf("main: sleep until all threads are blocked on mutex\n");
    sleep(10);

    gettimeofday(&current_time, NULL);
    printf("main: Time before attempting to unlock = %d\n",
           current_time.tv_sec);
    status = pthread_mutex_unlock(&mymutex);
    if ( status != 0 )
      {
        /* Mutex unlock failed  - break from loop and quit                 */
        printf("main: Unlock mutex failed - terminate\n");
        /* Try to destroy mutex. NOTE: it may fail if another thread       */
        /* is holding the mutex. At this point, we don't care, _BUT_       */
        /* If this mutex destroy fails and the CPA process is exited with  */
        /* the mutex still existing, a Synchronization VLOG will most      */
        /* likely be cut indicating this.  mutexes should be destroyed by  */
        /* the application program and not allowed to simply go out of     */
        /* scope and be desctroyed by the system. The system treats this   */
        /* as an abnormal condition                                        */
         ( void )pthread_mutex_destroy(&mymutex);
         printf("main: Testcase failed\n");
         return(3);
      } /* endif */

    for ( i = 0; i < THREAD_COUNT; i++ )
      {
        printf("main: Join to child thread %d\n", i+1);
        status = pthread_join(threads[i], NULL);
        if ( status!= 0 )
          {
            printf("main: Join to Child thread %d failed, errno=%d\n"
                   "main: Testcase failed\n", errno);
            return(-1);
          } /* endif */
      } /* endfor */

    printf("main: After %d threads performing %d manipulations,\n"
           "main: the resource is %d\n", THREAD_COUNT,
           DOIT_COUNT, resource);
    (void)pthread_mutex_destroy(&mymutex);

    printf("main: Testcase successful\n");
    return(0);

} /* end */


/********************************************************************/
/* function:      lock_thread                                       */
/*                                                                  */
/* description:   Thread which locks and unlocks mutex.             */
/*                before accessing the resource that the mutex      */
/*                serializes access to.                             */
/*                                                                  */
/********************************************************************/
void *lock_thread(void *parm)
{
    int                 status;   /* return status of APIs          */
    int                 i;        /* loop variable                  */
    struct timeval      current_time;  /* local time value          */
    int                 tnum = ((threadparmdata_t *)parm)->tnum;
    /* Note: this parameter assignment doesn't copy the data, it    */
    /* still points to the data assigned up in the main thread and  */
    /* any modifications or deallocation of memory there, will have */
    /* adverse affects here                                         */
    char               *descript = ((threadparmdata_t *)parm)->string;


    printf("lt #%d:  %s\n", tnum, descript);

    for ( i=0; i < DOIT_COUNT; ++i )   {
        gettimeofday(&current_time, NULL);
        printf("lt #%d:  Current time in secs = %d\n",
               tnum, current_time.tv_sec);

        /* get the mutex to serialize access to the resource            */
        /* the threads should not access this resource until the mutex  */
        /* is locked                                                    */
        printf("lt #%d:  Lock the mutex\n", tnum);
        status = pthread_mutex_lock(&mymutex);
        if (status != 0) {
            printf("lt #%d:  Mutex lock failed = %d\n", tnum, errno);
            pthread_exit(NULL);
          }

        /* Use the process scoped resource in a thread safe manner.     */
        /* this means that we only use it when holding its mutex        */
        gettimeofday(&current_time, NULL);
        printf("lt #%d:  this thread now holds the mutex to serialize access\n"
               "lt #%d:  to the resource, time = %d\n",
               tnum, tnum, current_time.tv_sec);
        /* pretend we're off processing while holding the mutex and     */
        /* this shows how the serialization on one mutex can slow down  */
        /* the processing of the entire job.  Really, the mutex should  */
        /* be held only around the access/use of the resource           */
        usleep(1);
        ++resource;
        gettimeofday(&current_time, NULL);
        printf("lt #%d:  Current time before unlock= %d\n",
               tnum, current_time.tv_sec);

        /* unlock the mutex, allowing other threads to access the       */
        /* resource.                                                    */
        status = pthread_mutex_unlock(&mymutex);
        if (status != 0) {
            printf("lt #%d:  Mutex unlock failed = %d\n", tnum, errno);
            pthread_exit(NULL);
          }
      } /* endfor */

    return(NULL);
}  /* end */

