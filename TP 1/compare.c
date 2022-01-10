#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 4096ul

#define CHK(op)            \
    do {                   \
        if ((op) == -1)    \
            raler(1, #op); \
    } while (0)

noreturn void raler(int syserr, const char *msg, ...) {
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    if (syserr == 1)
        perror("");

    exit(EXIT_FAILURE);
}

/**
 * @brief compare two buffers
 *
 * @param buf1 buffer 1
 * @param buf2 buffer 2
 * @param size size of the shortest buffer
 * @return ssize_t - position of the byte that differs or -1 if buffers are
 * equals
 */
ssize_t buf_cmp(const unsigned char *buf1, const unsigned char *buf2,
                ssize_t size) {
    for (ssize_t i = 0; i < size; i++)
        if (buf1[i] != buf2[i])
            return i;

    return -1;
}

/**
 * @brief count number of newlines in buffer
 *
 * @param buf buffer
 * @param size size of buffer
 * @return ssize_t - number of newlines
 */
ssize_t count_nl(const unsigned char *buf, ssize_t size) {
    ssize_t nl = 0;

    for (ssize_t i = 0; i < size; i++)
        if (buf[i] == '\n')
            nl++;

    return nl;
}

/**
 * @brief compare two files
 *
 * @param fd1 file descriptor of the first file
 * @param fd2 file descriptor of the second file
 * @param filename1 name of the first file
 * @param filename2 name of the second file
 * @return int - 0 if the files are identical, 1 otherwise
 */
int compare(int fd1, int fd2, const char *filename1, const char *filename2) {
    unsigned char buf1[BUFSIZE];
    unsigned char buf2[BUFSIZE];
    ssize_t nread1 = -1, nread2 = -1;
    ssize_t bytes_read = 0;  // number of bytes read from the file
    ssize_t line_number = 1; // number of the line (starts from 1)

    while (!(nread1 == 0 && nread2 == 0)) {
        CHK(nread1 = read(fd1, buf1, BUFSIZE));
        CHK(nread2 = read(fd2, buf2, BUFSIZE));

        // if we reached the end of the file at the beginning
        if (bytes_read == 0 && nread1 * nread2 == 0 && nread1 != nread2) {
            fprintf(stderr, "EOF on %s which is empty\n",
                    nread1 == 0 ? filename1 : filename2);
            return 1;
        }

        // assume buffers are not the same length
        ssize_t shorter = // shorter buffer size
            nread1 < nread2 ? nread1 : nread2;
        const unsigned char *buf_shorter = // shorter buffer
            nread1 < nread2 ? buf1 : buf2;
        const char *f1 = // shorter file name
            nread1 < nread2 ? filename1 : filename2;

        // check if buffers are the same at least partially
        ssize_t cmp = buf_cmp(buf1, buf2, shorter);
        if (cmp >= 0) {
            bytes_read += cmp + 1; // add 1 because we start counting from 0
            line_number += count_nl(buf_shorter, cmp);
            fprintf(stderr, "%s %s differ: byte %ld, line %ld\n", filename1,
                    filename2, bytes_read, line_number);
            return 1;
        }

        // if buffers are partially the same, but are not the same length
        if (nread1 != nread2) {
            bytes_read += shorter;
            line_number += count_nl(buf_shorter, shorter);
            fprintf(stderr, "EOF on %s after byte %ld, line %ld\n", f1,
                    bytes_read, line_number);
            return 1;
        }

        // if buffers are the same
        bytes_read += nread1;
        line_number += count_nl(buf1, nread1);
    } // main loop
    return 0;
}

int main(int argc, char *argv[]) {
    int fd1, fd2;
    int result;

    // because argv[0] is always defined
    if (argc != 3) {
        raler(0, "Usage: %s file1 file2", argv[0]);
    }

    CHK(fd1 = open(argv[1], O_RDONLY));
    CHK(fd2 = open(argv[2], O_RDONLY));

    result = compare(fd1, fd2, argv[1], argv[2]);

    CHK(close(fd1));
    CHK(close(fd2));
    return result;
}
