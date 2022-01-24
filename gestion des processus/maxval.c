#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#define MAXSIZ 1 << 10
#define c_t char // custom type

#define CHK(op)               \
    do {                      \
        if ((op) == -1)       \
            complain(1, #op); \
    } while (0)

noreturn void complain(int syserr, const char *msg, ...) {
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    if (syserr == 1)
        perror("");

    exit(EXIT_FAILURE);
}

int make_arr(int fd, c_t *arr) {
    // file format : byte, byte, byte, ...
    // return the number of numbers in the file

    int n;
    char buf[BUFSIZ];
    int count = 0;
    while ((n = read(fd, buf, BUFSIZ)) > 0) {
        char *p = buf;
        int i = 0;
        while (p < buf + n) {
            c_t x = 0;
            while (*p != ',' && *p != ' ' && *p != '\n' && p < buf + n)
                x = x * 10 + *p++ - '0';
            arr[i++] = x;
            p++;
        }
        count += i;
    }
    CHK(n);
    return count;
}

c_t find_max(c_t *arr, int i, int j) {
    c_t max = arr[i];
    for (int k = i; k < j; k++)
        if (arr[k] > max)
            max = arr[k];
    return max;
}

int main(int argc, char *argv[]) {
    pid_t pid;
    int status;

    if (argc != 3)
        complain(0, "Usage: %s <max_chunk_size> <filename>\n", argv[0]);

    char *path = argv[2];
    int fd = open(path, O_RDONLY);
    CHK(fd);

    c_t arr[MAXSIZ] = {0};
    int n = make_arr(fd, arr);

    c_t max = 0;
    // divide the array into chunks of size at most max_chunk_size
    // and find the maximum in each chunk
    // all the chunks are processed in parallel
    // the maximum of all the chunks is the maximum of the array
    int max_chunk_size = atoi(argv[1]);
    if (max_chunk_size < 1)
        complain(0, "max_chunk_size must be positive\n");
    else if (max_chunk_size > n)
        max_chunk_size = n;

    for (int i = 0; i < n; i += max_chunk_size) {
        switch (pid = fork()) {

        case -1:
            complain(1, "fork");

        case 0:
            max = find_max(arr, i, i + max_chunk_size);
            exit(max);
        }
    }

    CHK(close(fd));
    for (int i = 0; i < n; i += max_chunk_size) {
        CHK(wait(&status));
        if (WIFEXITED(status) && WEXITSTATUS(status) > max)
            max = WEXITSTATUS(status);
        else if (WIFSIGNALED(status))
            complain(0, "child terminated by signal %d\n", WTERMSIG(status));
    }
    printf("%jd\n", (intmax_t)max);
    return 0;
}
