#ifndef TEXT_PROCESSING_FUNCTIONS_H
#define TEXT_PROCESSING_FUNCTIONS_H

extern int is_alpha_numeric(int c, bool inWord);

extern char is_vowel(int c);

extern int is_separator(char c);

extern void processFileName(int argc, char **files, char *fileNames[]);

extern void savePartialResults(int numWords, int as, int es, int is, int os, int us, int ys, int fileID);

extern void printResults();

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

extern struct State{
    unsigned char buffer[4];
    int buffer_pos;
    int bytes_remaining;
} State;

#endif