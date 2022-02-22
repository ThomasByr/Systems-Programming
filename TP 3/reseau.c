#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits.h>
#include <signal.h>
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

#define MAXSTA 10
#define PAYLOAD_SIZE 4

#define CHK(op)            \
    do {                   \
        if ((op) == -1)    \
            alert(1, #op); \
    } while (0)

noreturn void alert(int syserr, const char *msg, ...) {
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
    long no_sta; // number of stations

    if (argc != 2)
        alert(0, "usage: %s <no_sta>", argv[0]);

    char *endptr;
    no_sta = strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0') {
        alert(1, "no_sta is not a number");
    }
    if (errno == ERANGE)
        alert(1, "no_sta out of range [%ld, %ld]", LONG_MIN, LONG_MAX);
    if (no_sta < 1 || no_sta > MAXSTA)
        alert(0, "no_sta should be in [1, %d]", MAXSTA);

    int pipes[MAXSTA + 1][2];
    for (long i = 0; i < no_sta + 1; i++) {
        CHK(pipe(pipes[i]));
    }

    for (long i = 1; i < no_sta + 1; i++) {
        switch (fork()) {

        case -1:
            alert(1, "fork");

        case 0:
            // closing unused pipes
            CHK(close(pipes[0][0]));
            CHK(close(pipes[i][1]));
            for (long j = 1; j < no_sta + 1; j++) {
                if (j != i) {
                    CHK(close(pipes[j][0]));
                    CHK(close(pipes[j][1]));
                }
            }

            exit(EXIT_SUCCESS);
        }
    }

    // closing unused pipes for parent
    CHK(close(pipes[0][1]));
    for (long i = 1; i < no_sta + 1; i++) {
        CHK(close(pipes[i][0]));
    }

    printf("%ld\n", no_sta);
}
