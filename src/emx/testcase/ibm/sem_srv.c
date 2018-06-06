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
/* test case:    sem_srv.c                                           */
/*                                                                   */
/* objective:    semaphores and shared memory example  - server pgm  */
/*                                                                   */
/* description:  This program shows all of the semaphore and         */
/*               shared memory API's.  Basically this example        */
/*               serves as a client server model, with the buffer    */
/*               being a shared memory segment, and the process      */
/*               synchronization being done by semaphores.  This     */
/*               function is the "server" job.  This program is to   */
/*               be executed before the client program is run.       */
/*                                                                   */
/* external routines:                                                */
/*               semctl                                              */
/*               semget                                              */
/*               semop                                               */
/*               shmget                                              */
/*               shmat                                               */
/*               shmdt                                               */
/*               shmctl                                              */
/*                                                                   */
/* usage notes:  Compile this program using CRTBNDC                  */
/*               Call it with no parameters before running client    */
/*               program. The server program will end after the      */
/*               client program has been called twice.               */
/*********************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define SEMKEY 8888             /* Key passed into semget operation  */
#define SHMKEY 9999             /* Key passed into shmget operation  */

#define NUMSEMS 2               /* Num of sems in created sem set    */
#define SIZEOFSHMSEG 50         /* Size of the shared mem segment    */

#define NUMMSG 2                /* Server thread only doing two
                                   "receives" on shm segment         */

int main(int argc, char *argv[])
{
    int rc, semid, shmid, i;
    void *shm_address;
    struct sembuf operations[2];
    struct shmid_ds shmid_struct;
    unsigned short init_vals[NUMSEMS];
    unsigned short *sem_array;

    /* Create a semaphore set with the constant key.  The number of  */
    /* semaphores in the set is two.  If a semaphore set already     */
    /* exists for the key, give an error.  The permissions are given */
    /* as everyone has read/write access to the semaphore set.       */

    semid = semget( SEMKEY, NUMSEMS, 0666 | IPC_CREAT | IPC_EXCL );
    if ( semid == -1 )
      {
        printf("main: semget() failed\n");
        return ( -1 );
      }

    /* Initialize the semaphores in the set to 0 for the first one,  */
    /* and 0 for the second one.                                     */
    /*                                                               */
    /* The first semaphore in the sem set means:                     */
    /*        '1' --  The shared memory segment is being used        */
    /*        '0' --  The shared memory segment is freed             */
    /* The second semaphore in the sem set means:                    */
    /*        '1' --  The shared memory segment has been modified by */
    /*                the client.                                    */
    /*        '0' --  The shared memory segment has not been         */
    /*                modified by the client.                        */

    init_vals[0] = 0;
    init_vals[1] = 0;

    sem_array = init_vals;

    /* The '1' on this command is a no op, since the SETALL command  */
    /* is used                                                       */
    rc = semctl( semid, 1, SETALL, sem_array);
    if(rc == -1)
      {
        printf("main: semctl() initialization failed\n");
        return ( -1 );
      }

    /* Create a shared memory segment with the constant key.  The    */
    /* size of the segment is a constant.  The permissions are given */
    /* as everyone has read/write access to the semaphore set.  If a */
    /* shared memory segment already exists for this key, give an    */
    /* error                                                         */
    shmid = shmget(SHMKEY, SIZEOFSHMSEG, 0666 | IPC_CREAT | IPC_EXCL);
    if (shmid == -1)
      {
        printf("main: shmget() failed\n");
        return ( -1 );
      }

    /* Next, attach the shared memory segment to the server process. */
    shm_address = shmat(shmid, NULL, 0);
    if ( shm_address==NULL )
      {
        printf("main: shmat() failed\n");
        return ( -1 );
      }
    printf("Ready for client jobs\n");

    /* Only loop a specified number of times for this example        */
    for (i=0; i < NUMMSG; i++)
      {
        /* Set the structure passed into the semop() to first wait   */
        /* for the second semval to equal 1, then decrement it to    */
        /* allow the next signal that the client has wrote to it.    */
        /* Next, set the first semaphore to equal 1, which means     */
        /* that the shared memory segment is busy.                   */
        operations[0].sem_num = 1;    /* Operate on the second sem   */
        operations[0].sem_op = -1;    /* Decrement the semval by one */
        operations[0].sem_flg = 0;    /* Allow a wait to occur       */

        operations[1].sem_num = 0;    /* Operate on the first sem    */
        operations[1].sem_op =  1;    /* Increment the semval by 1   */
        operations[1].sem_flg = IPC_NOWAIT; /* Do not allow to wait  */

        rc = semop( semid, operations, 2 );
        if (rc == -1)
          {
            printf("main: semop() failed\n");
            return ( -1 );
          }

        /* Print the shared memory contents */
        printf("Server Thread Received : \"%s\"\n", (char *) shm_address);

        /* Signal the first semaphore to free the shared memory      */
        operations[0].sem_num = 0;
        operations[0].sem_op  = -1;
        operations[0].sem_flg = IPC_NOWAIT;

        rc = semop( semid, operations, 1 );
        if (rc == -1)
          {
            printf("main: semop() failed\n");
            return ( -1 );
        }

      }  /* End of FOR LOOP */

    /* Cleanup the environment by removing the semid structure,      */
    /* detatching the shared memory segment, then performing the     */

    /* delete on the shared memory segment id.                       */

    rc = semctl( semid, 1, IPC_RMID, sem_array);
    if (rc==-1)
      {
        printf("main: semctl() remove id failed\n");
        return ( -1 );
      }
    rc = shmdt(shm_address);
    if (rc==-1)
      {
        printf("main: shmdt() failed\n");
        return ( -1 );
      }
    rc = shmctl(shmid, IPC_RMID, &shmid_struct);
    if (rc==-1)
      {
        printf("main: shmctl() failed\n");
        return ( -1 );
      }
return(0);
}

