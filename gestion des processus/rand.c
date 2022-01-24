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
#include <sys/time.h>
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
    if (argc != 3)
        complain(0, "usage: %s <n> <m>", argv[0]);

    int n = atoi(argv[1]); // number of rgn
    int m = atoi(argv[2]); // upper bound of rgn

    if (m > 255)
        complain(0, "m must be less than 256");

    int *rgn = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        rgn[i] = 1 + rand() % m; // [1, m] inclusive
    }

    pid_t pid;
    int status;

    // generate n child processes and make them wait
    for (int i = 0; i < n; i++) {
        switch (pid = fork()) {

        case -1:
            complain(1, "pid = fork()");

        case 0:
            sleep((unsigned)rgn[i]);
            free(rgn);
            exit(rgn[i]);
        }
    }

    // wait for all child processes to finish
    for (int i = 0; i < n; i++) {
        CHK(wait(&status));

        if (WIFEXITED(status))
            printf("exit(%d)\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("signal %d\n", WTERMSIG(status));
        else
            printf("other status\n");
    }
    free(rgn);
    return 0;
}
