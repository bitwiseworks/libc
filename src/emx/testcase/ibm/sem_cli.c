/*

Client/Server programs that use semaphores and shared memory

The server program acts as a server to the client program. The buffer
is a shared memory segment. The process synchronization is done using
semaphores.

The client program acts as a client to the server program. The program
is run after a message appears from the server program.

Choose your browser's option to save to local disk and then reload this
document to download this code example. Send the program to your AS/400
and compile it using the development facilities supplied there. This
program was developed on a V3R1 system and tested on V3R1, V3R2 and
V3R6, and V4R4 systems.

This small program that is furnished by IBM is a simple example to
provide an illustration. This example has not been thoroughly tested
under all conditions. IBM, therefore, cannot guarantee or imply
reliability, serviceability, or function of this program. All programs
contained herein are provided to you "AS IS". THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.

 */
/*********************************************************************/
/*                                                                   */
/* test case:      sem_cli.c                                         */
/*                                                                   */
/* objective:      semaphores and shared memory example - client pgm */
/*                                                                   */
/* description:    This program serves as a client to the server pgm */
/*                 above. After the message "Ready for client jobs"  */
/*                 Appears on the server program, this program is    */
/*                 to be run.                                        */
/*                                                                   */
/* external routines:                                                */
/*               semget                                              */
/*               semop                                               */
/*               shmget                                              */
/*               shmat                                               */
/*               shmdt                                               */
/*                                                                   */
/* usage notes:  Compile this program using CRTBNDC                  */
/*               Call it with no parameters after server program has */
/*               been called. Call it twice; the server program will */
/*               end after it receives two messages from the client  */
/*               program.                                            */
/*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define SEMKEY 8888
#define SHMKEY 9999

#define NUMSEMS 2
#define SIZEOFSHMSEG 50


int main(int argc, char *argv[])
{
    struct sembuf operations[2];
    void         *shm_address;
    int semid, shmid, rc;

    /* Get the already created semaphore id associated with key.    */
    /* If the semaphore set does not exist, then it will not be     */
    /* created, and an error will occur.                            */
    semid = semget( SEMKEY, NUMSEMS, 0666);
    if ( semid == -1 )
      {
        printf("main: semget() failed\n");
        return ( -1 );
      }

    /* Get the already created shared memory id associated with key.*/
    /* If the shared memory id does not exist, then it will not be  */
    /* created, and an error will occur.                            */

    shmid = shmget(SHMKEY, SIZEOFSHMSEG, 0666);
    if (shmid == -1)
      {
        printf("main: shmget() failed\n");
        return ( -1 );
      }

    /* Next, attach the shared memory segment to the client process.*/
    shm_address = shmat(shmid, NULL, 0);
    if ( shm_address==NULL )
      {
        printf("main: shmat() failed\n");
        return ( -1 );
      }

    /* First, check to see if the first semaphore is a zero.  If it */
    /* isn't, it is busy right now.  The semop() command will wait  */
    /* for the semaphore to reach zero before executing the semop() */
    /* When it is zero, increment the first semaphore to show that  */
    /* the shared memory segment is busy.                           */
    operations[0].sem_num = 0;        /* Operate on the first sem   */
    operations[0].sem_op =  0;        /* Wait for the value to be=0 */
    operations[0].sem_flg = 0;        /* Allow a wait to occur      */

    operations[1].sem_num = 0;        /* Operate on the first sem   */
    operations[1].sem_op =  1;        /* Increment the semval by one*/
    operations[1].sem_flg = 0;        /* Allow a wait to occur      */

    rc = semop( semid, operations, 2 );
    if (rc == -1)
      {
        printf("main: semop() failed\n");
        return ( -1 );
      }

    strcpy((char *) shm_address, "Hello from Client");


    /* Release the shared memory segmnent by decrementing the in use */
    /* semaphore (the first one).  Increment the second semaphore to */
    /* show that the client is finished with it.                     */
    operations[0].sem_num = 0;        /* Operate on the first sem    */
    operations[0].sem_op =  -1;       /* Decrement the semval by one */
    operations[0].sem_flg = 0;        /* Allow a wait to occur       */

    operations[1].sem_num = 1;        /* Operate on the second sem   */
    operations[1].sem_op =  1;        /* Increment the semval by one */
    operations[1].sem_flg = 0;        /* Allow a wait to occur       */

    rc = semop( semid, operations, 2 );
    if (rc == -1)
      {
        printf("main: semop() failed\n");
        return ( -1 );
      }

    /* Detach the shared memory segment from the current process     */
    rc = shmdt(shm_address);
    if (rc==-1)
      {
        printf("main: shmdt() failed\n");
        return ( -1 );
      }

return(0);
}

