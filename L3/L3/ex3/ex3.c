/*************************************
* Lab 3 Exercise 3
* Name: Le Trung Hieu
* Student No: A0161308M
* Lab Group: 6
*************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

/** STARTS  ******************************************************
    Semaphore related functions and data structure 
    - Mostly function wrappers for handling an array of semaphores
        in shared memory
******************************************************************/

typedef struct 
{
    sem_t* semArray;
    int shdMemId;
} sharedSemaphore;

//Create "n" semaphores in a shared memory region
void newSemaphoreArray( sharedSemaphore* newSharedSemaphore, int n )
{
    //Create a shared memory region for the semaphores
    newSharedSemaphore->shdMemId = 
        shmget( IPC_PRIVATE, sizeof(sem_t) * n, IPC_CREAT | 0666 );
    if (newSharedSemaphore->shdMemId < 0){
        perror("Cannot Allocate Shared Memory");
        exit(1);
    }
   
    //Points the pointers to right place
    newSharedSemaphore->semArray = 
        (sem_t*) shmat(newSharedSemaphore->shdMemId, NULL, 0);
}

//Deallocate the shared memory region used for the semaphores
void destroySempahoreArray( sharedSemaphore* semInfo )
{
    shmdt( (char*)semInfo->semArray );
    shmctl(semInfo->shdMemId, IPC_RMID, NULL); 
}

//Function wrapper for sem_init(). Perform the initialization
// on one of the semaphores
int semaphoreArrayInit( sharedSemaphore semInfo, int which, 
        int pshared, unsigned int value)
{
    return sem_init( &(semInfo.semArray[which]), pshared, value);
}

//Function wrapper for sem_wait(). Perform the wait
// on one of the semaphores
void semaphoreArrayWait( sharedSemaphore semInfo, int which )
{
    sem_wait( &semInfo.semArray[which] );
}

//Function wrapper for sem_post(). Perform the wait
// on one of the semaphores
void semaphoreArrayPost( sharedSemaphore semInfo, int which )
{
    sem_post( &semInfo.semArray[which] );
}

/** ENDS *********************************************************
    Semaphore related functions 
******************************************************************/

/** STARTS  ******************************************************
    Mancala Game Related Functions and Data Structure
******************************************************************/


//Mancala DEFINES
#define RED 10000
#define GREEN 100
#define BLUE 1

int makeMancalaPlate(int nRed, int nGreen, int nBlue)
{
    return (nRed * RED) + (nGreen * GREEN) + (nBlue * BLUE);
}

void setupMancala(int *mancala, int seed, int size)
{
    int i, nRed, nGreen, nBlue;

    srand(seed);
    for (i = 0; i < size; i++) {
	nRed = rand() % 21 + 10;
	nGreen = rand() % 21 + 10;
	nBlue = rand() % 21 + 10;
        mancala[i] = makeMancalaPlate(nRed, nGreen, nBlue);  
    }
}

int countColor(int color, int plate )
{
    return (plate/color) % 100; 
}

void countMancala (int* mancala, int size, int* colorCount)
{
    int i;

    colorCount[0] = colorCount[1] = colorCount[2] = 0;

    for (i = 0; i < size; i++) {
        colorCount[0] += countColor( RED, mancala[i]); 
        colorCount[1] += countColor( GREEN, mancala[i]); 
        colorCount[2] += countColor( BLUE, mancala[i]); 
    }
}

void removeColorFromPlate(int color, int* plate, int amount)
{
    int mapIndex[] = {RED, GREEN, BLUE};
    int nColors[3], i ;

    nColors[0] = countColor( RED, *plate);   
    nColors[1] = countColor( GREEN, *plate);   
    nColors[2] = countColor( BLUE, *plate);   

    for (i = 0; i < 3; i++){
        if (mapIndex[i] == color){
            nColors[i] -= amount;
            break;
        }
    }
    *plate = makeMancalaPlate(nColors[0], nColors[1], nColors[2]);
}

void addColorToPlate(int color, int* plate, int amount)
{
    int mapIndex[] = {RED, GREEN, BLUE};
    int nColors[3], i;

    nColors[0] = countColor( RED, *plate);   
    nColors[1] = countColor( GREEN, *plate);   
    nColors[2] = countColor( BLUE, *plate);   

    for (i = 0; i < 3; i++){
        if (mapIndex[i] == color){
            nColors[i] += amount;
            break;
        }
    }
    *plate = makeMancalaPlate(nColors[0], nColors[1], nColors[2]);
}

void printPlate( int plate )
{
    printf("Red [%02d] | Green [%02d] | Blue [%02d]\n",
            countColor( RED, plate), 
            countColor( GREEN, plate), countColor( BLUE, plate)); 
}

void printMancala( int *mancala, int size)
{
    int i;

    for (i = 0; i < size; i++){

        printf("Plate [%d] = ",i);
        printPlate( mancala[i] );
    }

}


void moveColor(int color, int* fromPlate, int* toPlate,
        int*playCount, int*maxPlayCount, sharedSemaphore semaphores, int id, int size)
{
    int amount, limit, waitAmount;
    struct timespec hesitate = {0};

    limit = 99 - countColor(color, *toPlate);
    amount = countColor(color, *fromPlate);

    //Move between 1 to 5 beads
    if (amount > 5){
        amount = rand()%5 + 1;
    }

    //Move less if the target is almost full
    if (amount > limit){
        amount = limit;
    }
     
    //Can turn on hesitation to simulate workload

#ifdef HESITATE
    waitAmount = HESITATE;
#else
    waitAmount = 0;
#endif

    //1 ms = 1000000 ns
    hesitate.tv_nsec = waitAmount * 1000000; 

#ifdef DEBUG
    printf("\tMove %d of color %d in %d ms\n", 
            amount, color, waitAmount);
#endif
    
    //Record Simultaneous Player for Checking Later
    semaphoreArrayWait(semaphores, 0);

    (*playCount)++;
    if (*playCount > *maxPlayCount){
        *maxPlayCount = *playCount;
    }
    semaphoreArrayPost(semaphores, 0);

    //Revemo color beads from first plate
    removeColorFromPlate( color, fromPlate, amount);

    //High resolution sleep to simulate workload
    nanosleep(&hesitate, NULL);
   
    
    //Place color beads into second plate
    addColorToPlate( color, toPlate, amount);

    //Reduce Simultaneous Count when we are done
    semaphoreArrayWait(semaphores, 0);
    (*playCount)--;
    semaphoreArrayPost(semaphores, 0);
}


//Share Memory DEFINES
//How much extra integers are we using in shared memory?
#define EXTRASPACE 3

//Index for the extra integers in shared memory
#define PLAYCOUNT 0
#define MAXPLAYCOUNT 1
#define SEATS 1
#define CANKILLCHILDREN 2

//TODO: Add additional parameters (e.g. more sempahores) if needed
void mancalaPlayer(int id, int* sharedMem, 
        int size, int nRound, sharedSemaphore semaphores)
{
    int first, second, color, i;
    int *mancala, *playCount, *maxPlayCount, *state;
    int colorArray[] = {RED, GREEN, BLUE};

    mancala = &sharedMem[EXTRASPACE];
    playCount = &sharedMem[PLAYCOUNT];
    maxPlayCount = &sharedMem[MAXPLAYCOUNT];
    first = id;
    second = (id+1) % size; 


    for (i = 0; i < nRound; i++){
        color = colorArray[ rand() % 3 ] ;
#ifdef DEBUG
        printf("Player[%d]: Moving Color %d from [%d] to [%d]\n", 
            id, color, first, second);
#endif
    
        //TODO: Find out what to do before color beads are moved


	// The idea is to use the alogirthm used in
	// dining philosopher, with limited eater version	
	semaphoreArrayWait(semaphores, SEATS);
	semaphoreArrayWait(semaphores, first + 2);
	semaphoreArrayWait(semaphores, second + 2);
        moveColor( color, &mancala[first], &mancala[second], 
            playCount, maxPlayCount, semaphores, id, size);
        //TODO: Find out what to do after color beads are moved
	semaphoreArrayPost(semaphores, first + 2);
	semaphoreArrayPost(semaphores, second + 2);
	semaphoreArrayPost(semaphores, SEATS);
    }

}

void checkState(int initState[], int finalState[])
{
    int i, pass = 1;

    for (i = 0; i < 3; i++){
        printf("[%d] vs [%d] => ", 
            initState[i], finalState[i]);
        if (initState[i] != finalState[i]){
            printf("FAILED!\n");
            pass = 0;
        } else {
            printf("ok\n");
        }
    }
    printf("** Correctness =  %s\n", (pass)?"OK":"FAILED");
}

/** ENDS  ******************************************************
    Mancala Game Related Functions and Data Structure
******************************************************************/
#define MAX_PLAYER 100
pid_t childArr[MAX_PLAYER];
pid_t parentPid;
int playerCount;
int *sharedMem;

void signalHandler(int signo) {
	int* status;
	if(signo == SIGINT) {

		if(getpid() == parentPid) {
			printf("Parent Cleaning Up: \n");
			for(int i = 0; i < playerCount; i++) {
				kill(childArr[i], SIGINT);			
			}

			sharedMem[CANKILLCHILDREN] = 1;
			for(int i = 0; i < playerCount; i++) {
				waitpid(childArr[i], &status, 0);
				printf("\t[%d] Terminated. \n", childArr[i]);
			}
		}
		
		if(sharedMem[CANKILLCHILDREN] == 1) { 	
			printf("PID[%d] Ended. \n", getpid());
			exit(1);
		} else {
			// in this case, the child process
			// is terminated by the terminal,
			// so, we re-register the handler
			// and send SIGINT again.
			kill(getpid(), SIGINT);
			signal(SIGINT,  signalHandler);
		}
	}
	
}

int main(int argc, char** argv)
{
    parentPid = getpid();
    if (signal(SIGINT, signalHandler) == SIG_ERR) {
	printf("Failed to register handler \n");
    }
	
    int result, i;
    int shdMemSize, shdMemId; 
    int *mancala, nPlayer, nRound, seed;
    int initState[3], finalState[3];

    //Checking and converting cmd line arguments
    //No need to change
    if (argc < 4){
        printf("Usage: %s random_seed n_players n_rounds\n", argv[0]);
        exit(1);
    }
    seed = atoi(argv[1]);
    nPlayer = atoi(argv[2]);
    nRound = atoi(argv[3]);

    playerCount = nPlayer;
    printf("Mancala with %d Players, %d Rounds. Using [%d] seed.\n",
        nPlayer, nRound, seed);

 
    //Create a new shared memory region
    //TODO: You can allocate larger shared memory region if you need

    shdMemSize = sizeof(int) * (nPlayer + EXTRASPACE);

    shdMemId = shmget( IPC_PRIVATE, shdMemSize, IPC_CREAT | 0666 );

    if (shdMemId < 0){
        printf("Cannot create shared memory region!\n");
        exit(1);
    }
    printf("Shared Memory Id is %i\n",shdMemId);

    //Attach a new shared memory region
    sharedMem = (int*) shmat(shdMemId, NULL, 0);
    if ( sharedMem == (int*)-1){
        printf("Cannot attach shared memory region!\n"); exit(1);
    }

    //Initialize 
    mancala = &sharedMem[EXTRASPACE];

    setupMancala( mancala,  seed, nPlayer);

#ifdef DEBUG
    printMancala( mancala, nPlayer);
#endif
    countMancala (mancala, nPlayer, initState);

    //TODO: Add / Modify initialization for additional sempahores etc
 
    sharedMem[CANKILLCHILDREN] = 0;
    sharedSemaphore semaphores;
    newSemaphoreArray( &semaphores, nPlayer + 2);

    //Suggestion: you can use constants / define to give each semaphore
    // index a name for ease of coding 
    // e.g. we use the first sempahore as a mutex, so
    // #define MUTEX 0
    // semaphoreArrayInit(semaphores, MUTEX, 1, 1);

    semaphoreArrayInit(semaphores, 0, 1, 1);
    
    semaphoreArrayInit(semaphores, SEATS, 1, nPlayer - 1); 
    for(int i = 0; i < nPlayer; i ++) {
   	 semaphoreArrayInit(semaphores, i+2, 1, 1);
    }   
 
    //Spawn child processes
    for( i = 0; i < nPlayer; i++){
        result = fork();

        //Each child process is a Mancala Game Player
        if (result == 0) {

            //TODO: Modify the function parameters if needed
            mancalaPlayer(i, sharedMem, nPlayer, nRound, semaphores);

            shmdt( (char*)sharedMem );
            //Dont remove this return! Prevent child from running fork()!
            return 0;   //child ends here
        } else {
	   // childArr stores the children PIDs
	    childArr[i] = result;	
	}
    }
  
  
    int* status;
    //Waiting for all Players
    for( i = 0; i < nPlayer; i++){
#ifdef DEBUG
        printf("Waited [%i]\n", wait( NULL )) ;
#else
        wait(NULL);
#endif
    }

    //Verification and Stats Reporting
    //DO NOT MODIFY

#ifdef DEBUG
    printMancala( mancala, nPlayer);
#endif
    countMancala ( mancala, nPlayer, finalState);

    checkState( initState, finalState );
    printf("** Simultaneous Player Count = %d\n",
        sharedMem[MAXPLAYCOUNT]);

    //Remove shared memory region 
    shmctl(shdMemId, IPC_RMID, NULL); 
    destroySempahoreArray( &semaphores );

    return 0;
}
