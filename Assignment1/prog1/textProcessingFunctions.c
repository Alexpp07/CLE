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