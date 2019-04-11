
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
void *sharedMemPtr;

/* The name of the received file */
const char recvFileName[] = "recvfile";


/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the shared memory
 * @param sharedMemPtr - the pointer to the shared memory
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
	shmid = shmget(key,SHARED_MEMORY_CHUNK_SIZE,IPC_CREAT | 0666);
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
 * The main loop
 */
void mainLoop()
{
	printf("Starting main loop.\n");
	/* The size of the mesage */
	int msgSize = 0;

	/* Open the file for writing */
	FILE* fp = fopen(recvFileName, "w");

	/* Error checks */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}

	/* TODO: Receive the message and get the message size. The message will
     * contain regular information. The message will be of SENDER_DATA_TYPE
     * (the macro SENDER_DATA_TYPE is defined in msg.h).  If the size field
     * of the message is not 0, then we copy that many bytes from the shared
     * memory region to the file. Otherwise, if 0, then we close the file and
     * exit.
     *
     * NOTE: the received file will always be saved into the file called
     * "recvfile"
     */

	/* Keep receiving until the sender set the size to 0, indicating that
 	 * there is no more data to send
 	 */

	message rMsg, sMsg;
	int status;
	//msgSize = -1; //Give it a default non valid number

	do
	{

		printf("Recieving message to denote size in shared memory\n");
		status = msgrcv(msqid, &rMsg, sizeof(rMsg) - sizeof(long), SENDER_DATA_TYPE, 0);
		if( status == -1) //Is it size of int?
		{
			perror("Couldn't receive message to denote size in shared memory");
			exit(1);
		}
		printf("Recieved Message to denote size in shared memory");

		msgSize = rMsg.size; //Store the size of data in shared memory obtained in message
		printf("Data size in shared memory (obtained from message) is: %d\n", msgSize);

		/* If the sender is not telling us that we are done, then get to work */
		if(msgSize != 0)
		{
			/* Save the shared memory to file */
			if(fwrite(sharedMemPtr, sizeof(char), msgSize, fp) < 0)
			{
				perror("fwrite");
			}

			/* TODO: Tell the sender that we are ready for the next file chunk.
 			 * I.e. send a message of type RECV_DONE_TYPE (the value of size field
 			 * does not matter in this case).
 			 */
			printf("Sending the message to ask for more data\n");
			sMsg.mtype = RECV_DONE_TYPE;
			sMsg.size = 0;
			status = msgsnd(msqid, &sMsg, 0,0);
			if (status == -1){
				perror("Couldn't send message to ask for more data");
				exit(1);
			}
			printf("Sent the message to ask for more data\n");
		}
			/* We are done */
		else
		{
			/* Close the file */
			fclose(fp);
		}
	}while(msgSize != 0);
	
	/* Print out contents of recieve file */
	printf("\n\n%s\n",string(50,'~').c_str());
	fp = fopen(recvFileName, "r"); //Have to open it in read mode
	int c;
	while ((c = getc(fp)) != EOF){
       		putchar(c);
	}
	printf("\n%s\n\n\n",string(50,'~').c_str());
}



/**
 * Perfoms the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */

void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
	shmdt(sharedMemPtr);

	/* TODO: Deallocate the shared memory chunk */
	//(IPC_RMID removes after all other processes detach)
	shmctl(shmid, IPC_RMID, NULL);

	/* TODO: Deallocate the message queue */
	msgctl(msqid, IPC_RMID, NULL);
}

/**
 * Handles the exit signal
 * @param signal - the signal type
 */

void ctrlCSignal(int signal)
{
	/* Free system V resources */
	cleanUp(shmid, msqid, sharedMemPtr);
}

int main(int argc, char** argv)
{

	/* TODO: Install a singnal handler (see signaldemo.cpp sample file).
 	 * In a case user presses Ctrl-c your program should delete message
 	 * queues and shared memory before exiting. You may add the cleaning functionality
 	 * in ctrlCSignal().
 	 */
	signal(SIGINT, ctrlCSignal);


	/* Initialize */
	init(shmid, msqid, sharedMemPtr);
	printf("Finishied initialzing, starting main loop.\n");

	/* Go to the main loop */
	mainLoop();
	printf("Main loop over, now cleaning up.\n");

	cleanUp(shmid, msqid, sharedMemPtr);
	printf("Everything is cleaned up. Shutting down.\n");

	return 0;
}
