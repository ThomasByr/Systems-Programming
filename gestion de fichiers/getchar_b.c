#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "macro.h"

#define END_OF_FILE -1
#define BUFFER_SIZE 2048

int my_getchar(void) {
    static unsigned char buf[BUFFER_SIZE];
    static unsigned char *bufp = buf;
    static int i = 0;

    if (i == 0) {
        i = read(0, buf, BUFFER_SIZE);
        bufp = buf;
    }
    if (--i >= 0)
        return *bufp++;

    return END_OF_FILE;
}

int main(void) {
    int c;

    while ((c = my_getchar()) != END_OF_FILE)
        CHK(putchar(c));

    return 0;
}
