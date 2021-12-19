#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "macro.h"

#define BUFF_SIZE 2048

/**
 * @brief copy of a file from one place to another using descriptors
 *
 */
int copy(int in, int out) {
    char buf[BUFF_SIZE];
    int n;
    while ((n = read(in, buf, sizeof(buf))) > 0)
        CHK(write(out, buf, n));
    return n < 0 ? -1 : 0;
}

int main(int argc, char *argv[]) {
    int in, out;
    if (argc != 3) {
        fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
        exit(1);
    }
    CHK(in = open(argv[1], O_RDONLY));
    CHK(out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666));
    CHK(copy(in, out));
    exit(0);
}
