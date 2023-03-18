#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "files_monitor.h"

/** \brief locking flag which warrants mutual exclusion inside the monitor */
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;

/*  */
int main(int argc, char *argv[]){

   /* Save filenames in the shared region and initialize counters to 0 */
   char *fileNames[argc-1];
   processFileName(argc, argv, fileNames);

   /* generate the chunks to be processed by the workers threads*/
   

   //printResults();

   return 0;
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