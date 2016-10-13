#include <stdio.h>

void displayMessage( const char * string) {
        printf("%s", string);
}

void displayMessage(const char *format,  int y) {
        printf(format,  y);
}


void displayMessage(const char *format, const char * string, int x, int y) {
        printf(format, string, x, y);
}


void displayMessage(const char *format, const char * string) {
        printf(format, string);
}

void displayMessage(const char *format, const char * str1, const char * str2) {
        printf(format, str1, str2);
}

