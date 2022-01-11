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

int main(int argc, char *argv[]) {
    pid_t pid;
    int reason;

    if (argc != 2)
        complain(0, "Usage: %s <path to file>\n", argv[0]);

    char *path = argv[1];
    int fd = open(path, O_RDONLY);
    CHK(fd);

    // read the file
    ssize_t n;
    char buf;

    switch (pid = fork()) {

    case -1:
        complain(1, "fork");

    case 0:
        // child process
        while ((n = read(fd, &buf, 1)) > 0) {
            printf("\033[0;31m");
            CHK(write(STDOUT_FILENO, &buf, n));
        }
        CHK(n);
        CHK(close(fd));
        exit(EXIT_SUCCESS);

    default:
        while ((n = read(fd, &buf, 1)) > 0) {
            printf("\033[0;32m");
            CHK(write(STDOUT_FILENO, &buf, n));
        }
        CHK(n);
        CHK(close(fd));

        CHK(wait(&reason));
        printf("\033[0m\n");
        if (WIFEXITED(reason))
            printf("exit(%d)\n", WEXITSTATUS(reason));
        else if (WIFSIGNALED(reason))
            printf("signal %d\n", WTERMSIG(reason));
        else
            printf("other reason\n");

        return 0;
    }
}
