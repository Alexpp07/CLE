#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

/** \brief struct to store the information of one chunk*/
struct Chunk {
  int fileId;        /* file identifier */  
  int size;    /* Number of bytes of the chunk */
  unsigned char * chunk_pointer;  /* Pointer to the start of the chunk */
};

/** \brief struct to store the counters of a file */
struct FileCounters {
   char* file_name;                                   /* file name */  
   int total_num_of_words;                            /* Number of total words */
   int count_total_vowels[6];                         /* Number of words containing each vowel (a,e,i,o,u,y) */
};

/** \brief return status on monitor initialization */
extern int statusInitMon;

/** \brief main producer thread return status */
extern int statusMain;

/** \brief consumer threads return status array */
extern int *workersStatus;

/** \brief total number of files to process */
static int numberOfFiles;

/** \brief storage region for counters */
static struct FileCounters * mem_counters;

/** \brief storage region for chunks */
static struct Chunk * mem_chunks;    // 10 -> data transfer region nominal capacity (in number of values that can be stored) in the FIFO

/** \brief insertion pointer */
static unsigned int ii;

/** \brief retrieval pointer */
static unsigned int ri;

/** \brief flag signaling the data transfer region is full */
static bool full;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR_SR = PTHREAD_MUTEX_INITIALIZER;

/** \brief flag which warrants that the data transfer region is initialized exactly once */
static pthread_once_t init = PTHREAD_ONCE_INIT;;

/** \brief producers synchronization point when the data transfer region is full */
static pthread_cond_t fifoFull;

/** \brief consumers synchronization point when the data transfer region is empty */
static pthread_cond_t fifoEmpty;

/**
 *  \brief Initialization of the data transfer region.
 *
 *  Internal monitor operation.
 */
static void initialization (void)
{
  if (((mem_chunks = malloc (10 * sizeof (struct Chunk))) == NULL))
	 { fprintf (stderr, "error on allocating space to the data transfer region\n");
	   statusInitMon = EXIT_FAILURE;
	   pthread_exit (&statusInitMon);
     }
                                                                        /* initialize FIFO in empty state */
    ii = ri = 0;                                                          /* FIFO insertion and retrieval pointers set to the same value */
    full = false;                                                           /* FIFO is not full */

    pthread_cond_init (&fifoFull, NULL);                                  /* initialize main synchronization point */
    pthread_cond_init (&fifoEmpty, NULL);                                 /* initialize workers synchronization point */
}

/**
 *  \brief Store a chunk in the data transfer region.
 *
 *  Operation carried out by main.
 *
 *  \param buffer pointer to the start of the chunk
 *  \param size number of bytes of the chunk
 *  \param fileId file identifier
 */
void saveChunk(unsigned char * buffer, unsigned int size, unsigned int fileId){
    if ((statusMain = pthread_mutex_lock (&accessCR)) != 0){                                   /* enter monitor */
        errno = statusMain;                                                            /* save error in errno */
        perror ("error on entering monitor(CF)");
        statusMain = EXIT_FAILURE;
        pthread_exit (&statusMain);
    }
    pthread_once (&init, initialization);                                              /* internal data initialization */

    while (full)                                                           /* wait if the data transfer region is full */
    { if ((statusMain = pthread_cond_wait (&fifoFull, &accessCR)) != 0)
        { errno = statusMain;                                                          /* save error in errno */
            perror ("error on waiting in fifoFull");
            statusMain = EXIT_FAILURE;
            pthread_exit (&statusMain);
        }
    }

    if (fileId == -1) mem_chunks[ii].chunk_pointer = NULL;             /* when fileId equals to -1 it means that there is no more chunks to process */
    else{
      unsigned char *chunk_copy = malloc(size); 
      if (chunk_copy == NULL) {
        perror("Failed to allocate memory for chunk_copy");
        return;
      }
      memcpy(chunk_copy, buffer, size);
      mem_chunks[ii].chunk_pointer = chunk_copy; 
    }

    mem_chunks[ii].fileId = fileId;
    mem_chunks[ii].size = size;                                                                      /* store value in the FIFO */
    ii = (ii + 1) % 10;
    full = (ii == ri);

    if ((statusMain = pthread_cond_signal (&fifoEmpty)) != 0)      /* let a consumer know that a value has been stored */
        { errno = statusMain;                                                             /* save error in errno */
        perror ("error on signaling in fifoEmpty");
        statusMain = EXIT_FAILURE;
        pthread_exit (&statusMain);
        }

    if ((statusMain= pthread_mutex_unlock (&accessCR)) != 0){                                /* exit monitor */
        errno = statusMain;                                                            /* save error in errno */
        perror ("error on exiting monitor(CF)");
        statusMain = EXIT_FAILURE;
        pthread_exit (&statusMain);
    }
}

/**
 *  \brief Get a chunk from the data transfer region.
 *
 *  Operation carried out by the workers.
 *
 *  \param workerId worker identification
 *
 *  \return chunk
 */

struct Chunk retrieveChunk (unsigned int workerId)
{
  struct Chunk chunk;                                                                               /* retrieved value */

  if ((workersStatus[workerId] = pthread_mutex_lock (&accessCR)) != 0)                                   /* enter monitor */
     { errno = workersStatus[workerId];                                                            /* save error in errno */
       perror ("error on entering monitor(CF)");
       workersStatus[workerId] = EXIT_FAILURE;
       pthread_exit (&workersStatus[workerId]);
     }
  pthread_once (&init, initialization);                                              /* internal data initialization */

  while ((ii == ri) && !full)                                           /* wait if the data transfer region is empty */
  { if ((workersStatus[workerId] = pthread_cond_wait (&fifoEmpty, &accessCR)) != 0)
       { errno = workersStatus[workerId];                                                          /* save error in errno */
         perror ("error on waiting in fifoEmpty");
         workersStatus[workerId] = EXIT_FAILURE;
         pthread_exit (&workersStatus[workerId]);
       }
  }

  chunk = mem_chunks[ri];                                                                   /* retrieve a  value from the FIFO */
  ri = (ri + 1) % 10;
  full = false;

  if ((workersStatus[workerId] = pthread_cond_signal (&fifoFull)) != 0)       /* let the main know that a value has been retrieved */
     { errno = workersStatus[workerId];                                                             /* save error in errno */
       perror ("error on signaling in fifoFull");
       workersStatus[workerId] = EXIT_FAILURE;
       pthread_exit (&workersStatus[workerId]);
     }

  if ((workersStatus[workerId] = pthread_mutex_unlock (&accessCR)) != 0)                                   /* exit monitor */
     { errno = workersStatus[workerId];                                                             /* save error in errno */
       perror ("error on exiting monitor(CF)");
       workersStatus[workerId] = EXIT_FAILURE;
       pthread_exit (&workersStatus[workerId]);
     }

  return chunk;
}

/**
 *  \brief Save partial results in the data transfer region.
 *
 *  Operation carried out by the workers.
 *
 *  \param workerId worker identification
 *  \param numWords number of words
 *  \param as number of words with an a
 *  \param es number of words with an e
 *  \param is number of words with an i
 *  \param os number of words with an o
 *  \param us number of words with an u
 *  \param ys number of words with an y
 *  \param fileID file identification
 */
void savePartialResults(unsigned int workerId, int numWords, int as, int es, int is, int os, int us, int ys, int fileID){
  if ((workersStatus[workerId] = pthread_mutex_lock (&accessCR_SR)) != 0){                                   /* enter monitor */
    errno = workersStatus[workerId];                                                            /* save error in errno */
    perror ("error on entering monitor(CF)");
    workersStatus[workerId] = EXIT_FAILURE;
    pthread_exit (&workersStatus[workerId]);
  }

  mem_counters[fileID].total_num_of_words += numWords;
  mem_counters[fileID].count_total_vowels[0] += as;
  mem_counters[fileID].count_total_vowels[1] += es;
  mem_counters[fileID].count_total_vowels[2] += is;
  mem_counters[fileID].count_total_vowels[3] += os;
  mem_counters[fileID].count_total_vowels[4] += us;
  mem_counters[fileID].count_total_vowels[5] += ys;

  if ((workersStatus[workerId] = pthread_mutex_unlock (&accessCR_SR)) != 0){                            /* exit monitor */
    errno = workersStatus[workerId];                                                             /* save error in errno */
    perror ("error on exiting monitor(CF)");
    workersStatus[workerId] = EXIT_FAILURE;
    pthread_exit (&workersStatus[workerId]);
  }
}

/**
 *  \brief Print all the results for all files.
 *
 *  Operation carried out by main.
 */
void printResults(){

  if ((pthread_mutex_lock (&accessCR_SR)) != 0){                                   /* enter monitor */
    perror ("error on entering monitor(CF)");
    statusMain = EXIT_FAILURE;
    pthread_exit (&statusMain);
  }

  for (int i = 0; i < numberOfFiles; i++){
    printf("\nFile name: %s\n", mem_counters[i].file_name);
    printf("Total number of words = %d \n", mem_counters[i].total_num_of_words);
    printf("A: %d   E: %d   I: %d   O: %d   U: %d   Y: %d\n", mem_counters[i].count_total_vowels[0], mem_counters[i].count_total_vowels[1], mem_counters[i].count_total_vowels[2], mem_counters[i].count_total_vowels[3] , mem_counters[i].count_total_vowels[4], mem_counters[i].count_total_vowels[5]);
  }

  if ((statusMain= pthread_mutex_unlock (&accessCR_SR)) != 0){                                /* exit monitor */
    errno = statusMain;                                                            /* save error in errno */
    perror ("error on exiting monitor(CF)");
    statusMain = EXIT_FAILURE;
    pthread_exit (&statusMain);
  }
}

/**
 *  \brief Function to store the filenames and start the counters for each file.
 *
 *  Operation carried out by main.
 *
 *  \param nFiles number of files
 *  \param files files to be stored 
 *  \param fileNames names of files
 */
void processFileName(int nFiles, char **files, char *fileNames[]){

  numberOfFiles = nFiles;

  for(int i=0; i<numberOfFiles; i++){
    fileNames[i] = malloc(strlen(files[i]) + 1);
    strcpy(fileNames[i], files[i]); 
  }

  if ((pthread_mutex_lock (&accessCR_SR)) != 0) {                    
      perror ("error on entering monitor(CF)");
      int status = EXIT_FAILURE;
      pthread_exit(&status);
  }

  mem_counters = malloc(numberOfFiles * sizeof(struct FileCounters));

  for (int i=0; i<numberOfFiles; i++) {
    mem_counters[i].file_name = fileNames[i];
    mem_counters[i].total_num_of_words = 0;
    mem_counters[i].count_total_vowels[0] = 0;
    mem_counters[i].count_total_vowels[1] = 0;
    mem_counters[i].count_total_vowels[2] = 0;
    mem_counters[i].count_total_vowels[3] = 0;
    mem_counters[i].count_total_vowels[4] = 0;
    mem_counters[i].count_total_vowels[5] = 0;
  }   

  if ((pthread_mutex_unlock (&accessCR_SR)) != 0) {
    perror ("error on exiting monitor(CF)");
    int status = EXIT_FAILURE;
    pthread_exit(&status);
  }
}