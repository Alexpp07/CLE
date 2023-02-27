#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <stdbool.h>
#include <string.h>

char is_vowel(int c){
    printf("before %X\n",c);
    if (c >= 0xE0) 
        c = c - 0x20;  // upper case
    printf("after %X\n",c);
    // A ou ÀÁÂÃ
    if (c == 0x41 || (0xC0 <= c && c <= 0xC3))
        return 'A'; 
    
    // E ou ÈÉÊ
    if (c == 0x45 || (0xC8 <= c && c <= 0xCA))
        return 'E'; 

    // I ou ÌÍ
    if (c == 0x49 || (0xCC <= c && c <= 0xCD))
        return 'I'; 
    
    // O ou ÒÓÔÕ
    if (c == 0x4F || (0xD2 <= c && c <= 0xD5))
        return 'O'; 
    
    // U ou ÙÚ
    if (c == 0x55 || (0xD9 <= c && c <= 0xDA))
        return 'U';
    
    return 'n';
}

int is_alpha(int c){
    if (c >= 0xE0) c -= 0x20;  // upper case
    // a-z A-Z
    return ((0x41 <= c && c <= 0x5A) || (0x61 <= c && c <= 0x7A)) || 
        // apostrophe ('), cedilla (Ç)
        c == 0x27 || c == 0xC7 || 
        // ÀÁÂÃ ÈÉÊ ÌÍ
        (0xC0 <= c && c <= 0xC3) || (0xC8 <= c && c <= 0xCA) || (0xCC <= c && c <= 0xCD) ||
        // ÒÓÔÕ ÙÚ
        (0xD2 <= c && c <= 0xD5) || (0xD9 <= c && c <= 0xDA);
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
    int words = 0; int as = 0; int es = 0; int is = 0; int os = 0; int us = 0;
    int previousVowelCheck[5] = {0,0,0,0,0};
    bool inWord = false;

    for (int i=1; i<argc; i++){
        int byte, lbyte;
        char vowel;
        FILE *fp;

        fp = fopen(argv[i], "rb");

        if (fp == NULL) {
            printf("Error! File %s not found.\n", argv[i]);
            exit(1);
        }

        while (1){
            while (EOF != (byte = f_getc(fp))){
                if (is_alpha(byte)){
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
                }
                else{
                    previousVowelCheck[0] = 0;
                    previousVowelCheck[1] = 0;
                    previousVowelCheck[2] = 0;
                    previousVowelCheck[3] = 0;
                    previousVowelCheck[4] = 0;
                    inWord = false;
                }
            }
            if (byte == EOF) break;
        }
        fclose(fp);
    }

    printf("%d \n", words);
    printf("A: %d\nE: %d\nI: %d\nO: %d\nU: %d\n", as, es, is, os ,us);

    return 0;
}