#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "sharedRegion.h"
#include "textProcessingFunctions.h"

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

/** \brief main thread return status */
int statusMain;

/** \brief worker life cycle routine */
static void *worker(void *par);

/** \brief function to process each chunk */
static void processTextChunk(unsigned char * chunk_pointer, int size, struct ParRes * partialResults);

/** \brief function created to check the next chunk of text, and returns true if it was successful */
static bool readTextChunk(struct Chunk * chunk, struct ParRes * parRes, int workerID);

/** \brief function to split the text file into chunks */
int splitTextIntoChunks(FILE * file, char **chunks);

/** \brief function that returns the Unicode code points for the characters in the chunk */
int f_getc(unsigned char **chunk_pointer, struct State *state);

/** \brief function that check what vowel a byte is */
char is_vowel(int c);

/** \brief function to check if a byte is a alpha numeric one*/
int is_alpha_numeric(int c, bool inWord);

/** \brief execution time measurement */
static double get_delta_time(void);

/** \brief print command usage */
static void printUsage (char *cmdName);

/** \brief number of worker threads */
#define CHUNK_SIZE 4096

/*  */
int main(int argc, char *argv[]){

   int c;
   int nThreads = 0;
   char **files = NULL;
   int nFiles = 0;

   while ((c = getopt(argc, argv, "t:f:h")) != -1) {
      switch (c) {
         case 't':
            nThreads = atoi(optarg);
            if (nThreads <= 0) {
               fprintf(stderr, "%s: non positive number of threads\n", argv[0]);
               printUsage(argv[0]);
               return EXIT_FAILURE;
            }
            break;
         case 'f':
            nFiles = argc - optind + 1;
            files = (char **)malloc(nFiles * sizeof(char *));
            if (files == NULL) {
               fprintf(stderr, "%s: could not allocate memory for file names\n", argv[0]);
               return EXIT_FAILURE;
            }
            for (int i = 0; i < nFiles; i++) {
               files[i] = argv[optind + i - 1];
            }
            break;
         case 'h':
            printUsage(argv[0]);
            return EXIT_SUCCESS;
         case '?':
            if (optopt == 't' || optopt == 'f') {
               fprintf(stderr, "%s: option -%c requires an argument\n", argv[0], optopt);
            } else if (isprint(optopt)) {
               fprintf(stderr, "%s: unknown option `-%c'\n", argv[0], optopt);
            } else {
               fprintf(stderr, "%s: unknown option character `\\x%x'\n", argv[0], optopt);
            }
            printUsage(argv[0]);
            return EXIT_FAILURE;
         default:
            printUsage(argv[0]);
            return EXIT_FAILURE;
      }
    }

    if (nThreads == 0 || nFiles == 0) {
        fprintf(stderr, "%s: missing required argument(s)\n", argv[0]);
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    printf("Number of threads: %d\n", nThreads);
    printf("Number of files: %d\n", nFiles);
    printf("Files:\n");
    for (int i = 0; i < nFiles; i++) {
        printf("- %s\n", files[i]);
    }

   (void) get_delta_time ();

   /* save filenames in the shared region and initialize counters to 0 */
   char *fileNames[nFiles];
   processFileName(nFiles, files, fileNames);

   /* create worker threads */
   pthread_t th[nThreads];
   unsigned int workers[nThreads];
   workersStatus = malloc(nThreads * sizeof(int));   /* allocate memory to save the status of each worker */
   int *status_p;                                           /* pointer to execution status */
   int i;
      
   for (i=0; i<nThreads; i++){
      workers[i] = i;
      if(pthread_create(&th[i], NULL, worker, &workers[i]) != 0){
         perror("Failed to create thread");
         exit (EXIT_FAILURE);
      }
   }

   /* generate the chunks to be processed by the workers threads */
   for(int i=0; i<nFiles; i++){
      FILE * fp;

      /* open the input file in binary mode */
      fp = fopen(files[i], "rb");
      if (fp == NULL) {
         printf("It occoured an error while openning file: %s \n", argv[i]);
         exit(EXIT_FAILURE);
      }

      fseek(fp, 0, SEEK_END);
      long file_size = ftell(fp);
      rewind(fp);
      
      char **chunks = malloc(file_size + 1); /* allocate memory for all chunks */
      if (chunks == NULL) {
         perror("Failed to allocate memory");
         return 1;
      }

      int chunk_index = splitTextIntoChunks(fp, chunks);

      /* save the chunks */
      for (int j = 0; j < chunk_index; j++) {
         //printf("\nchunks: %s\n", chunks[j]);
         saveChunk(chunks[j], strlen(chunks[j]), i);
      }

      /* free memory */
      for (int i = 0; i < chunk_index; i++) {
         free(chunks[i]);
      }
      free(chunks);
      
      fclose(fp);
   }

   /* free memory for the files */
   free(files);

   /* save a chunk for each worker in fifo that represents the end of asking for chunks */
   for (int i = 0; i < nThreads; i++) {
      saveChunk(NULL, -1, -1);
   }

   /* waiting for the termination of the intervening entities threads */
   for (i = 0; i < nThreads; i++){ 
      if (pthread_join (th[i], (void *) &status_p) != 0){                               /* thread worker */
         perror ("error on waiting for worker thread");
         exit(EXIT_FAILURE);
      }
      // printf ("thread worker, with id %u, has terminated: ", i);
      // printf ("its status was %d\n", *status_p);
   }

   /* print results for all files */
   printResults();

   /* print the execution time */
   printf ("\nElapsed time = %.6f s\n", get_delta_time ());

   exit(EXIT_SUCCESS);
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
   struct Chunk chunk;
   struct ParRes parRes;
    
   while (readTextChunk(&chunk, &parRes, id)){

      /* perform text processing on the chunk */
      processTextChunk(chunk.chunk_pointer, chunk.size, &parRes);
   
      /* free the memory of the buffer if it was allocated */
      free(chunk.chunk_pointer);

      /* save partial results */
      savePartialResults(id, parRes.numberOfWords, parRes.vowelWords[0], parRes.vowelWords[1], parRes.vowelWords[2], parRes.vowelWords[3], parRes.vowelWords[4], parRes.vowelWords[5], parRes.fileID);
      
   }

   workersStatus[id] = EXIT_SUCCESS;
   pthread_exit (&workersStatus[id]);
    
}

/**
 *  \brief Function created to check the next chunk of text, and returns true if it was successful.
 * 
 *  \param chunk pointer to a chunk struct
 *  \param parRes pointer to a partial results struct
 *  \param workerID worker identification
 */
static bool readTextChunk(struct Chunk* chunk, struct ParRes* parRes, int workerID){

   /* get chunk from fifo */
   (*chunk) = retrieveChunk(workerID);

   /* checks if it is the chunk that tells that there are no more chunks to process */
   if ((chunk->chunk_pointer == NULL) && (chunk->fileId == -1)) return false;

   /* initialize the vars in parRes*/
   parRes->fileID = chunk->fileId;
   parRes->numberOfWords = 0;
   for (int i = 0; i < 6; i++) {
      parRes->vowelWords[i] = 0;
   }

   return true;
}

/**
 *  \brief Function created to process one text chunk.
 * 
 *  \param chunk_pointer pointer to a chunk of text
 *  \param size size of the chunk
 *  \param parRes pointer to a partial results struct
 */
static void processTextChunk(unsigned char * chunk_pointer, int size, struct ParRes * partialResults){

   int words = 0; int as = 0; int es = 0; int is = 0; int os = 0; int us = 0; int ys = 0;
   int previousVowelCheck[6] = {0,0,0,0,0,0};
   bool inWord = false;
   int byte;
   char vowel;
   struct State state = {0};
   for (size_t i = 0; i < size; i++) {
      unsigned char ch = chunk_pointer[i]; // read the byte at index i
      byte = f_getc(ch, &state);
      if (byte != -1 ){
         if (is_alpha_numeric(byte, inWord)){
            if (inWord == false){
               inWord = true;
               words++;
            }
            vowel = is_vowel(byte);
            if (vowel == 'A' && previousVowelCheck[0] == 0){
               as++;
               previousVowelCheck[0] = 1;
            }
            else if (vowel == 'E' && previousVowelCheck[1] == 0){
               es++;
               previousVowelCheck[1] = 1;
            }
            else if (vowel == 'I' && previousVowelCheck[2] == 0){
               is++;
               previousVowelCheck[2] = 1;
            }
            else if (vowel == 'O' && previousVowelCheck[3] == 0){
               os++;
               previousVowelCheck[3] = 1;
            }
            else if (vowel == 'U' && previousVowelCheck[4] == 0){
               us++;
               previousVowelCheck[4] = 1;
            }
            else if (vowel == 'Y' && previousVowelCheck[5] == 0){
               ys++;
               previousVowelCheck[5] = 1;
            }
         }
         else{
            previousVowelCheck[0] = 0;
            previousVowelCheck[1] = 0;
            previousVowelCheck[2] = 0;
            previousVowelCheck[3] = 0;
            previousVowelCheck[4] = 0;
            previousVowelCheck[5] = 0;
            inWord = false;
         }
      }
   }
   partialResults->numberOfWords = words;
   partialResults->vowelWords[0] = as;
   partialResults->vowelWords[1] = es;
   partialResults->vowelWords[2] = is;
   partialResults->vowelWords[3] = os;
   partialResults->vowelWords[4] = us;
   partialResults->vowelWords[5] = ys;

}

/**
 *  \brief Function created to check split the text file into several text chunks.
 * 
 *  \param file pointer to a file
 *  \param chunks pointer to a pointer to a character, which represents a dynamic array of strings
 */
int splitTextIntoChunks(FILE *file, char **chunks) {
   int chunk_index = 0;
   int start_index = 0;
   int bytes_to_read;

   while (!feof(file)) {
      /* allocate memory for each chunk separately */
      char *buffer = malloc(CHUNK_SIZE + 1);
      if (buffer == NULL) {
         perror("Failed to allocate memory");
         return -1;
      }
      bytes_to_read = CHUNK_SIZE - start_index;
      int bytes_read = fread(&buffer[start_index], 1, bytes_to_read, file);
      int num_bytes_read = start_index + bytes_read;

      if (bytes_read == 0) {
         free(buffer);
         break;
      }

      /* find the last separator before the end of the chunk */
      int last_separator_index = num_bytes_read - 1;
      while (last_separator_index >= 0 && !is_separator(buffer[last_separator_index])) {
         last_separator_index--;
      }

      /* if no separator was found, roll back to the previous separator and truncate the chunk */
      if (last_separator_index < 0) {
         fseek(file, -num_bytes_read, SEEK_CUR);
         free(buffer);
         continue;
      }

      /* add the chunk to the array */
      char *chunk = malloc(last_separator_index + 2); // increase size by 1 for null terminator
      if (chunk == NULL) {
         perror("Failed to allocate memory");
         free(buffer);
         return 1;
      }
      memcpy(chunk, buffer, last_separator_index + 1);
      chunk[last_separator_index + 1] = '\0';
      chunks[chunk_index++] = chunk;

      /* move the file cursor back by the number of remaining bytes */
      int remaining_bytes = num_bytes_read - last_separator_index - 1;
      fseek(file, -remaining_bytes, SEEK_CUR);
      start_index = 0;

      free(buffer);
   }

   return chunk_index;
}

/* The functions below were obtained from the code provided by the teacher for the producer/consumer problem */

/**
 *  \brief Get the process time that has elapsed since last call of this time.
 *
 *  \return process elapsed time
 */

static double get_delta_time(void)
{
  static struct timespec t0, t1;

  t0 = t1;
  if(clock_gettime (CLOCK_MONOTONIC, &t1) != 0)
  {
    perror ("clock_gettime");
    exit(1);
  }
  return (double) (t1.tv_sec - t0.tv_sec) + 1.0e-9 * (double) (t1.tv_nsec - t0.tv_nsec);
}

/**
 *  \brief Print command usage.
 *
 *  A message specifying how the program should be called is printed.
 *
 *  \param cmdName string with the name of the command
 */

static void printUsage (char *cmdName)
{
  fprintf (stderr, "\nSynopsis: %s [OPTIONS]\n"
           "  OPTIONS:\n"
           "  -t nThreads  --- set the number of threads to be created (default: 4)\n"
           "  -f           --- set the text files to be processed\n"
           "  -h           --- print this help\n", cmdName);
}