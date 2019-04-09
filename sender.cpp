#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "msg.h"    /* For the message struct */

using namespace std;

/* The size of the shared memory chunk */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 */

void init(int& shmid, int& msqid, void*& sharedMemPtr)
{

    /* TODO: 1. Create a file called keyfile.txt containing string "Hello world" (you may do
             so manually or from the code).
             2. Use ftok("keyfile.txt", 'a') in order to generate the key.
         3. Use the key in the TODO's below. Use the same key for the queue
            and the shared memory segment. This also serves to illustrate the difference
            between the key and the id used in message queues and shared memory. The id
            for any System V object (i.e. message queues, shared memory, and sempahores)
            is unique system-wide among all System V objects. Two objects, on the other hand,
            may have the same key.
     */

    printf("Initializing the  ftok get...\n");
    /* Generate Unique Key */
    key_t key = ftok("keyfile.txt",'a');
    if (key == -1){ //It doesn't exist so create it.
        printf("Creating keyfile.txt");
        ofstream ofs;
        ofs.open("keyfile.txt");
        ofs << "Hello World";
        ofs.close();
        key = ftok("keyfile.txt", 'a');

        //If it fails again, then throw an error.
        if (key == -1){
            perror("Ftok has failed to generate key");
            exit(1);
        }
    }

    printf("Initializing the  shmget get...\n");
    /* TODO: Allocate a piece of shared memory. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE. */
    /* Returns the ID of the shared memory */
    shmid = shmget(key,SHARED_MEMORY_CHUNK_SIZE,0666);
    if (shmid == -1){
        perror("Shmget id is not valid");
        exit(1);
    }

    printf("Initializing the  shmat...\n");
    /* TODO: Attach to the shared memory */
    /* Using ID of shared memory, attach to it */
    sharedMemPtr = shmat(shmid,(void*)0,0);
    if (sharedMemPtr == (char *)(-1)){
        perror("Shmat failed to attack to shared memory");
        exit(1);
    }

    printf("Initializing the  msgget get...\n");
    /* TODO: Create a message queue */
    /* Store ID for msgqueue  */
    msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1){
        perror("msgget failed to create a message queue");
    }
}


/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
    /* TODO: Detach from shared memory */
    shmdt(sharedMemPtr);
}


/**
 * The main send function
 * @param fileName - the name of the file
 */
void send(const char* fileName)
{
    /* Open the file for reading */
    FILE* fp = fopen(fileName, "r");

    /* A buffer to store message */
    message sMsg, rMsg;
    int status;

    /* Was the file open? */
    if(!fp)
    {
        perror("fopen");
        exit(-1);
    }

    /* Read the whole file */
    while(!feof(fp))
    {
        /* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and store them in shared memory.
         * fread will return how many bytes it has actually read (since the last chunk may be less
         * than SHARED_MEMORY_CHUNK_SIZE).
         */
        if((sMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
        {
            perror("fread");
            exit(-1);
        }

        /* TODO: Send a message to the receiver telling him that the data is ready
         * (message of type SENDER_DATA_TYPE)
         */
        printf("Sending part of the file\n");
        sMsg.mtype = SENDER_DATA_TYPE;
        status = msgsnd(msqid, &sMsg, sizeof(sMsg) - sizeof(long), 0);
        if( status == -1)
        {
            perror("Couldn't send the part of the file!");
            exit(1);
        }
        printf("Sent the part of the file size: %d\n", sMsg.size);

        /* TODO: Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us
         * that he finished saving the memory chunk.
         */
        printf("Recieving message to continue\n");
        status = msgrcv(msqid, &rMsg, 0, RECV_DONE_TYPE, 0);
        if(status == -1)
        {
            perror("Couldn't recieve message to continue");
            exit(1);
        }
        printf("Recieved message to continue size is: %d\n",rMsg.size);
    }


    /** TODO: once we are out of the above loop, we have finished sending the file.
      * Lets tell the receiver that we have nothing more to send. We will do this by
      * sending a message of type SENDER_DATA_TYPE with size field set to 0.
      */
    printf("Sending the final message to signify file is done\n");
    sMsg.mtype = SENDER_DATA_TYPE;
    sMsg.size = 0;
    status = msgsnd(msqid, &sMsg, sizeof(sMsg) - sizeof(long), 0);
    if (status == -1){
        perror("Couldn't send final message to signify file is done");
        exit(1);
    }
    printf("Sent the final message to signify file is done\n");

    /* Close the file */
    fclose(fp);
}

int main(int argc, char** argv)
{

    /* Check the command line arguments */
    if(argc < 2)
    {
        fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
        exit(-1);
    }

    /* Connect to shared memory and the message queue */
    init(shmid, msqid, sharedMemPtr);
    printf("Finishied initialzing, starting to send the file.\n");

    /* Send the file */
    send(argv[1]);
    printf("Finishied sending file, cleaning up now.\n");

    /* Cleanup */
    cleanUp(shmid, msqid, sharedMemPtr);
    printf("Everything is cleaned up. Shutting down.\n");

    return 0;
}
