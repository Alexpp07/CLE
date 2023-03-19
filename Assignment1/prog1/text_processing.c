#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "files_monitor.h"

/** \brief struct to manage the variables of a chunk*/
struct ParRes{
    int numberOfWords;  /* Total number of words*/
    int vowelWords[6];  /* Number of words containing each vowel (a,e,i,o,u,y) */
    int fileID;         /* File id of the chunk */
};

/* Id of the file from where the chunk is */
int fileID;

/* Variable crated for the purpose of knowing if the file was opened*/
bool openFile;

/** \brief worker threads return status array */
int *workersStatus;

/** \brief worker life cycle routine */
static void *worker(void *par);

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/** \brief number of worker threads */
#define THREAD_NUMBER 2

/*  */
int main(int argc, char *argv[]){

   /* Save filenames in the shared region and initialize counters to 0 */
   char *fileNames[argc-1];
   processFileName(argc, argv, fileNames);

   /* Create worker threads */
   pthread_t th[THREAD_NUMBER];
   unsigned int workers[THREAD_NUMBER];
   workersStatus = malloc(THREAD_NUMBER * sizeof(int));   /* Allocate memory to save the status of each worker */
   int *status_p;                                           /* pointer to execution status */
   int i;
   for (i=0; i<THREAD_NUMBER; i++){
      if(pthread_create(&th[i], NULL, worker, &workers[i]) != 0){
         perror("Failed to create thread");
         exit (EXIT_FAILURE);
      }
   }


   /* generate the chunks to be processed by the workers threads*/


   //printResults();

   /* waiting for the termination of the intervening entities threads */

   printf ("\nFinal report\n");
   for (i = 0; i < THREAD_NUMBER; i++){ 
      if (pthread_join (th[i], (void *) &status_p) != 0){                               /* thread worker */
         perror ("error on waiting for worker thread");
         exit(EXIT_FAILURE);
      }
      printf ("thread worker, with id %u, has terminated: ", i);
      printf ("its status was %d\n", *status_p);
   }

  // printf ("\nElapsed time = %.6f s\n", get_delta_time ());
}

/**
 *  \brief Function worker.
 *
 *  Its role is to simulate the life cycle of a worker.
 *
 *  \param par pointer to application defined worker identification
 */
static void *worker(void *par) {
    
    unsigned int id = *((unsigned int *) par);      // worker id

    printf("worker\n");
    
    // while (readTextChunk())
    // {
    //    //processTextChunk();

    //    //savePartialResults();
    // }

    workersStatus[id] = EXIT_SUCCESS;
    pthread_exit (&workersStatus[id]);
    
}

/* Fucntion created to get the next chunk of text, and returns true if it was successful */
bool readTextChunk(unsigned char *buffer, int *psize, struct ParRes *parRes){
    return true;
}

static void processTextChunk(unsigned char *buffer, int *psize, struct ParRes *parRes){

}

/*  
void printResults(){

    if ((pthread_mutex_lock (&accessCR)) != 0) {                  
       perror ("error on entering monitor(CF)");
       int status = EXIT_FAILURE;
       pthread_exit(&status);
    }

    for (int i=0; i<numberOfFiles; i++) {
        printf("\nFile name: %s\n",mem[i].file_name);
        printf("Number total of words: %d\n",mem[i].total_num_of_words);
        printf("A: %d\n",mem[i].count_total_vowels[0]);
        printf("E: %d\n",mem[i].count_total_vowels[1]);
        printf("I: %d\n",mem[i].count_total_vowels[2]);
        printf("O: %d\n",mem[i].count_total_vowels[3]);
        printf("U: %d\n",mem[i].count_total_vowels[4]);
        printf("Y: %d\n",mem[i].count_total_vowels[5]);
    }

    if ((pthread_mutex_unlock (&accessCR)) != 0) {                                              
       perror ("error on exiting monitor(CF)");
       int status = EXIT_FAILURE;
       pthread_exit(&status);
    }

}
*/