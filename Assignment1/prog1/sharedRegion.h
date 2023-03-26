#ifndef SHARED_REGION_H
#define SHARED_REGION_H

extern void saveChunk(char * buffer, unsigned int size, unsigned int fileId);

extern struct Chunk retrieveChunk (unsigned int workerId);

extern void savePartialResults(unsigned int workerId, int numWords, int as, int es, int is, int os, int us, int ys, int fileID);

extern void printResults();

extern void processFileName(int argc, char **files, char *fileNames[]);

/** \brief struct to store the information of one chunk*/
struct Chunk {
   int fileId;        /* file identifier */  
   int size;    /* Number of bytes of the chunk */
   unsigned char * chunk_pointer;  /* Pointer to the start of the chunk */
} Chunk;

/** \brief struct to store the counters of a file */
struct FileCounters {
   char* file_name;                                   /* file name */  
   int total_num_of_words;                            /* Number of total words */
   int count_total_vowels[6];                         /* Number of words containing each vowel (a,e,i,o,u,y) */
} FileCounters;

#endif
