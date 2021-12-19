#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "macro.h"

#define END_OF_FILE -1

/**
 * @brief read a char from stdin
 *
 */
int my_getchar(void) {
    int c;
    return read(0, &c, 1) <= 0 ? END_OF_FILE : c;
}

int main(void) {
    int c;

    while ((c = my_getchar()) != END_OF_FILE)
        CHK(putchar(c));

    return 0;
}
