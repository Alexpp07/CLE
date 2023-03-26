#ifndef TEXT_PROCESSING_FUNCTIONS_H
#define TEXT_PROCESSING_FUNCTIONS_H

extern int is_alpha_numeric(int c, bool inWord);

extern char is_vowel(int c);

extern int is_separator(char c);

extern struct State{
    unsigned char buffer[4];
    int buffer_pos;
    int bytes_remaining;
} State;

#endif