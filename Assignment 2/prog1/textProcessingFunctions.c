#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>
#include <string.h>

struct State{
    unsigned char buffer[4];
    int buffer_pos;
    int bytes_remaining;
};

/** \brief struct to store the counters of a file */
struct FileCounters {
   char* file_name;                                   /* file name */  
   int total_num_of_words;                            /* Number of total words */
   int count_total_vowels[6];                         /* Number of words containing each vowel (a,e,i,o,u,y) */
};

/** \brief total number of files to process */
static int numberOfFiles;

/** \brief storage region for counters */
static struct FileCounters * mem_counters;

int is_separator(char c){
   return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/**
 *  \brief Function to check what vowel is.
 */
char is_vowel(int c){
    if (c >= 0xE0)
        c = c - 0x20;  // upper case
    // a ou AÀÁÂÃ
    if (c == 0x61 || c == 0x41 || (0xC0 <= c && c <= 0xC3))
        return 'A'; 
    
    // e ou EÈÉÊ
    if (c == 0x65 || c == 0x45 || (0xC8 <= c && c <= 0xCA))
        return 'E'; 

    // i ou IÌÍ
    if (c == 0x69 || c == 0x49 || (0xCC <= c && c <= 0xCD))
        return 'I'; 
    
    // o ou OÒÓÔÕ
    if (c == 0x6F || c == 0x4F || (0xD2 <= c && c <= 0xD5))
        return 'O'; 
    
    // u ou UÙÚ
    if (c == 0x75 || c == 0x55 || (0xD9 <= c && c <= 0xDA))
        return 'U';

    // y ou Y
    if (c == 0x79 || c == 0x59)
        return 'Y';
    
    return 'n';
}

/**
 *  \brief Function to check if a char is alphanumeric.
 */
int is_alpha_numeric(int c, bool inWord){
    if (c >= 0xE0) c -= 0x20;  // upper case

    if (c == 0x1FF9 || c == 0x1FF8) c = 0x27;  // convert single quotation marks to apostrophe

    if (c == 0x27 && inWord == false)   // if an apostrophe is alone then is not considered to be counted as a word
        return false;

    // a-z A-Z
    return ((0x41 <= c && c <= 0x5A) || (0x61 <= c && c <= 0x7A)) || 
        // apostrophe ('), cedilla (Ç), underscore (_)
        c == 0x27 || c == 0xC7 || c == 0x5F ||
        // ÀÁÂÃ ÈÉÊ ÌÍ
        (0xC0 <= c && c <= 0xC3) || (0xC8 <= c && c <= 0xCA) || (0xCC <= c && c <= 0xCD) ||
        // ÒÓÔÕ ÙÚ
        (0xD2 <= c && c <= 0xD5) || (0xD9 <= c && c <= 0xDA) ||
        // 1 2 3 4 5 6 7 8 9
        (0x30 <= c && c <= 0x39);
}

/**
 *  \brief This function reads a byte of data, determines whether it is a single-byte character or part of a multi-byte sequence in UTF-8 encoding.
 */
int f_getc(unsigned char byte, struct State *state) {
    int ch;

    // If there are bytes remaining in the buffer, store the input byte and continue processing
    if (state->bytes_remaining > 0) {
        state->buffer[state->buffer_pos++] = byte;
        state->bytes_remaining--;

        // If we have read all bytes of the multi-byte sequence, process it
        if (state->bytes_remaining == 0) {
            ch = state->buffer[0] & ((1 << (7 - state->buffer_pos)) - 1);

            for (int i = 1; i < state->buffer_pos; i++) {
                ch = (ch << 6) | (state->buffer[i] & 0x3F);
            }

            // Reset the buffer state
            state->buffer_pos = 0;
            return ch;
        }

        return -1; // Indicate that this byte is part of a multi-byte sequence and should be ignored
    }

    // Otherwise, process the input byte
    ch = byte;

    // If the byte is a single-byte character, return it as the Unicode code point
    if ((ch & 0x80) == 0) return ch;

    // Determine the size of the utf-8 character sequence
    for (state->bytes_remaining = 1; ch & (0x80 >> state->bytes_remaining); state->bytes_remaining++);

    // Store the first byte of the multi-byte sequence and update the buffer state
    state->buffer[0] = byte;
    state->buffer_pos = 1;
    state->bytes_remaining--;

    return -1; // Indicate that this byte is part of a multi-byte sequence and should be ignored
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
}

/**
 *  \brief Save partial results
 *
 *  Operation carried out by the dispatcher.
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
void savePartialResults(int numWords, int as, int es, int is, int os, int us, int ys, int fileID){
  mem_counters[fileID].total_num_of_words += numWords;
  mem_counters[fileID].count_total_vowels[0] += as;
  mem_counters[fileID].count_total_vowels[1] += es;
  mem_counters[fileID].count_total_vowels[2] += is;
  mem_counters[fileID].count_total_vowels[3] += os;
  mem_counters[fileID].count_total_vowels[4] += us;
  mem_counters[fileID].count_total_vowels[5] += ys;
}

/**
 *  \brief Print all the results for all files.
 *
 *  Operation carried out by dispatcher.
 */
void printResults(){
  for (int i = 0; i < numberOfFiles; i++){
    printf("\nFile name: %s\n", mem_counters[i].file_name);
    printf("Total number of words = %d \n", mem_counters[i].total_num_of_words);
    printf("A: %d   E: %d   I: %d   O: %d   U: %d   Y: %d\n", mem_counters[i].count_total_vowels[0], mem_counters[i].count_total_vowels[1], mem_counters[i].count_total_vowels[2], mem_counters[i].count_total_vowels[3] , mem_counters[i].count_total_vowels[4], mem_counters[i].count_total_vowels[5]);
  }
}