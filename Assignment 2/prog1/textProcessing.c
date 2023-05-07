#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <mpi.h>
#include <time.h>

#include "textProcessingFunctions.h"

/** \brief struct to manage the variables of a chunk*/
struct ParRes {
    int numberOfWords;  /* Total number of words*/
    int vowelWords[6];  /* Number of words containing each vowel (a,e,i,o,u,y) */
    int fileID;         /* File id of the chunk */
};

/** \brief worker life cycle routine */
static void worker(int rank);

/** \brief function to process each chunk */
static void processTextChunk(unsigned char *chunk_pointer, int size, struct ParRes *partialResults);

/** \brief function created to check the next chunk of text, and returns true if it was successful */
static bool readTextChunk(struct Chunk *chunk, struct ParRes *parRes, int workerID);

/** \brief function to split the text file into chunks */
int splitTextIntoChunks(FILE *file, char **chunks);

/** \brief function that returns the Unicode code points for the characters in the chunk */
int f_getc(unsigned char chunk_pointer, struct State *state);

/** \brief function that check what vowel a byte is */
char is_vowel(int c);

/** \brief function to check if a byte is an alpha numeric one */
int is_alpha_numeric(int c, bool inWord);

/** \brief execution time measurement */
static double get_delta_time(void);

/** \brief print command usage */
static void printUsage(char *cmdName);

/** \brief number of processes */
int nProcesses;

/** \brief work status */
int workStatus;

/** \brief number of worker threads */
#define CHUNK_SIZE 4096

int main(int argc, char *argv[]) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    nProcesses = size - 1;

    if (rank == 0) {
        char **files = NULL;
        int nFiles = 0;

        int c;
        while ((c = getopt(argc, argv, "f:h")) != -1) {
            switch (c) {
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
                    if (optopt == 'f') {
                        fprintf(stderr, "%s: option -%c requires an argument\n", argv[0], optopt);
                    } else if (isprint(optopt)) {
                        fprintf(stderr, "%s: unknown option `-%c'\n", argv[0], optopt);
                    } else {
                        fprintf(stderr, "%s: unknown option character `\\x%x'\n", argv[0], optopt);
                    }
                    printUsage(argv[0]);
                    return EXIT_FAILURE;
                default:
                    abort();
            }
        }

        if (argc == 1 || files == NULL) {
            fprintf(stderr, "%s: option -f must be given\n", argv[0]);
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }

        (void) get_delta_time ();

        int current_process = 1;  // rank of the process to assign next chunk
        int chunks_sent = 0;      // chunks that were send by the dispatcher

        /* save filenames in the shared region and initialize counters to 0 */
        char *fileNames[nFiles];
        processFileName(nFiles, files, fileNames);
        workStatus = 1; /* there is work to do */

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

                int length = (int) strlen(chunks[j]);
                /* send to the worker: */
                MPI_Send(&workStatus, 1, MPI_INT, current_process, 0, MPI_COMM_WORLD); /* a flag saying if there is work to do */
                MPI_Send(&length, 1, MPI_INT, current_process, 0, MPI_COMM_WORLD);/* the size of the chunk */
                MPI_Send(&i, 1, MPI_INT, current_process, 0, MPI_COMM_WORLD);/* the fileID */
                MPI_Send(chunks[j], CHUNK_SIZE, MPI_UNSIGNED_CHAR, current_process, 0, MPI_COMM_WORLD);/* the chunk buffer */

                /* Update current_worker_to_receive_work and number_of_chunks_sent variables */
                current_process = (current_process%nProcesses)+1;
                chunks_sent++;
            }

            /* free memory */
            for (int i = 0; i < chunk_index; i++) {
                free(chunks[i]);
            }
            free(chunks);
            
            fclose(fp);
        }

        /* no more work to be done */
        workStatus = 0;
        /* inform workers that all files are process and they can exit */
        for (int i = 1; i <= nProcesses; i++)
            MPI_Send(&workStatus, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        free(files);

        while (chunks_sent > 0){
            for (int i = 1; i <= nProcesses && chunks_sent > 0; i++) {
                int nWords, file, nVowels[6];
                /* Receive the processing results from each worker process */
                MPI_Recv(&file, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&nWords, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&nVowels, 6, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                savePartialResults(nWords, nVowels[0], nVowels[1], nVowels[2], nVowels[3], nVowels[4], nVowels[5], file);

                chunks_sent--;
            }
        }

        /* print results for all files */
        printResults();

        /* print the execution time */
        printf ("\nElapsed time = %.6f s\n", get_delta_time ());

    } else {
        /* worker */
        worker(rank);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}

/**
 *  \brief Function worker.
 *
 *  Its role is to simulate the life cycle of a worker.
 *
 *  \param par pointer to application defined worker identification
 */
static void worker(int rank) {

    struct ParRes parRes;
    struct Chunk chunk;

    // Alocate memory to read the chunk information
    chunk.chunk_pointer = (unsigned char*) malloc(CHUNK_SIZE);
    
    while (readTextChunk(&chunk, &parRes, rank)){

        /* perform text processing on the chunk */
        processTextChunk(chunk.chunk_pointer, chunk.size, &parRes);

        /* save partial results */      
        MPI_Send(&parRes.fileID, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&parRes.numberOfWords, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&parRes.vowelWords, 6, MPI_INT, 0, 0, MPI_COMM_WORLD);

        /* free the memory of the buffer if it was allocated */
        // free(chunk.chunk_pointer);
        // printf("free\n");
    }
    
}

/**
 *  \brief Function created to check the next chunk of text, and returns true if it was successful.
 * 
 *  \param chunk pointer to a chunk struct
 *  \param parRes pointer to a partial results struct
 *  \param workerID worker identification
 */
static bool readTextChunk(struct Chunk* chunk, struct ParRes* parRes, int workerID){

    MPI_Recv(&workStatus, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    /* checks if there are no more chunks to process */
    if (workStatus == 0) return false;

    MPI_Recv(&chunk->size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&chunk->fileId, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Recv(chunk->chunk_pointer, CHUNK_SIZE, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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

/* Other function implementations have been omitted for brevity. */

static void printUsage(char *cmdName) {
    fprintf(stderr, "Usage: %s -f <file1> <file2> ...\n", cmdName);
}

