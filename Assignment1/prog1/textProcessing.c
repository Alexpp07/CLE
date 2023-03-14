#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>


static void processFileName(int argc, char *argv[], char *fileNames[]);

static void printResults();

/** \brief struct to store the counters of a file */
struct FileCounters {
   char* file_name;                                   /* file name */  
   int total_num_of_words;                            /* Number of total words */
   int count_total_vowels[6];                         /*  */
};

/** \brief total number of files to process */
static int numberOfFiles;

/** \brief storage region for counters */
static struct FileCounters * mem;

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;



/*  */
int main(int argc, char *argv[]){

    char *fileNames[argc-1];
    
    processFileName(argc, argv, fileNames);

    printResults();

    return 0;
}



/*  */
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



/*  */
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