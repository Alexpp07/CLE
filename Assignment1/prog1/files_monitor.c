#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "files_monitor.h"

/** \brief struct to store the counters of a file */
struct FileCounters {
   char* file_name;                                   /* file name */  
   int total_num_of_words;                            /* Number of total words */
   int count_total_vowels[6];                         /* Number of words containing each vowel (a,e,i,o,u,y) */
};

/** \brief total number of files to process */
static int numberOfFiles;

/** \brief storage region for counters */
static struct FileCounters * mem;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/* 
    Function to store the filenames and start the counters for each file.
*/
void processFileName(int argc, char *argv[], char *fileNames[]){

    numberOfFiles = argc - 1;

    for(int i=0; i<numberOfFiles; i++){
       fileNames[i] = malloc(strlen(argv[i+1]) + 1);
       strcpy(fileNames[i], argv[i+1]); 
    }

    if ((pthread_mutex_lock (&accessCR)) != 0) {                    
       perror ("error on entering monitor(CF)");
       int status = EXIT_FAILURE;
       pthread_exit(&status);
    }

    mem = malloc(numberOfFiles * sizeof(struct FileCounters));

    for (int i=0; i<numberOfFiles; i++) {
        mem[i].file_name = fileNames[i];
        mem[i].total_num_of_words = 0;
        mem[i].count_total_vowels[0] = 0;
        mem[i].count_total_vowels[1] = 0;
        mem[i].count_total_vowels[2] = 0;
        mem[i].count_total_vowels[3] = 0;
        mem[i].count_total_vowels[4] = 0;
        mem[i].count_total_vowels[5] = 0;
    }   

    if ((pthread_mutex_unlock (&accessCR)) != 0) {
       perror ("error on exiting monitor(CF)");
       int status = EXIT_FAILURE;
       pthread_exit(&status);
    }

}