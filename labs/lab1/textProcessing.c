#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>
#include <string.h>

char is_vowel(int c){
    if (c >= 0xE0)  // 0xE0 -> à => if is bigger then 0xE0 it will convert the character in unicode to its uppercase equivalent 
        c = c - 0x20;  // upper case
    // a ou AÀÁÂÃ
    if ( c == 0x41 || (0xC0 <= c && c <= 0xC3))
        return 'A'; 
    
    // e ou EÈÉÊ
    if (c == 0x45 || (0xC8 <= c && c <= 0xCA))
        return 'E'; 

    // i ou IÌÍ
    if ( c == 0x49 || (0xCC <= c && c <= 0xCD))
        return 'I'; 
    
    // o ou OÒÓÔÕ
    if ( c == 0x4F || (0xD2 <= c && c <= 0xD5))
        return 'O'; 
    
    // u ou UÙÚ
    if ( c == 0x55 || (0xD9 <= c && c <= 0xDA))
        return 'U';

    // y ou Y
    if (c == 0x79 || c == 0x59)
        return 'Y';
    
    return 'n';
}

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

int f_getc(FILE *fp) {
  int bytes, ch = fgetc(fp);
  if (ch == EOF) return EOF;
  if ((ch & 0x80) == 0) return ch;

  for (bytes = 1; ch & (0x80 >> bytes); bytes++);
  ch &= (1 << (7 - bytes)) - 1;

  for (; bytes > 1; bytes--) {
    int c = fgetc(fp);
    if (c == EOF) return EOF;
    ch = (ch << 6) | (c & 0x3F);
  }
  return ch;
}

int main(int argc, char *argv[]) {
    for (int i=1; i<argc; i++){
        int words = 0; int as = 0; int es = 0; int is = 0; int os = 0; int us = 0; int ys = 0;
        int previousVowelCheck[6] = {0,0,0,0,0,0};
        bool inWord = false;
        int byte, lbyte;
        char vowel;
        FILE *fp;

        fp = fopen(argv[i], "rb");

        if (fp == NULL) {
            printf("Error! File %s not found.\n", argv[i]);
            exit(1);
        }

        printf("\nFile name: %s\n", argv[i]);

        while (1){
            while (EOF != (byte = f_getc(fp))){
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
            if (byte == EOF) break;
        }
        fclose(fp);

        printf("Total number of words = %d \n", words);
        printf("A: %d   E: %d   I: %d   O: %d   U: %d   Y: %d\n", as, es, is, os , us, ys);
    }

    return 0;
}