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

struct sta_s {
    int dest;
    char payload[PAYLOAD_SIZE];
};

struct info_s {
    int src;
    int dest;
    char payload[PAYLOAD_SIZE];
};

void child_main(int id, int in, int out) {
    int fd, n;
    char filename[10];
    struct sta_s sta;
    struct info_s info;

    sprintf(filename, "STA_%d", id);
    CHK((fd = open(filename, O_RDONLY)));

    // format : (dest payload)
    while ((n = read(fd, &sta, sizeof(sta))) > 0) {
        info.src = id;
        info.dest = sta.dest;
        strncpy(info.payload, sta.payload, PAYLOAD_SIZE);

        // send (dest payload) to parent via pipe
        CHK(write(out, &info, sizeof(info)));
    }
    if (n == -1)
        alert(1, "read");

    CHK(close(fd));
    CHK(close(out));

    while ((n = read(in, &info, sizeof(info))) > 0) {
        // wait for parent to send back (src dest payload)
        // print (id - src - dest - payload)
        fprintf(stdout, "%d - %d - %d - ", id, info.src, info.dest);
        fwrite(info.payload, STDOUT_FILENO, PAYLOAD_SIZE, stdout);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
    if (n == -1)
        alert(1, "read");

    CHK(close(in));
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
    } // todo: do not create to many useless pipes

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

            child_main(i, pipes[i][0], pipes[0][1]);

            exit(EXIT_SUCCESS);
        }
    }

    // closing unused pipes for parent
    CHK(close(pipes[0][1]));
    for (long i = 1; i < no_sta + 1; i++) {
        CHK(close(pipes[i][0]));
    }

    // read no_sta (src dest payload) from all children
    struct info_s info;
    int n;
    while ((n = read(pipes[0][0], &info, sizeof(info))) > 0) {

        // send (src dest payload) dest children via pipes
        // if dest is undefined, send to all children except src

        if (1 <= info.dest && info.dest <= no_sta) {
            CHK(write(pipes[info.dest][1], &info, sizeof(info)));
        } else {
            for (long j = 1; j < no_sta + 1; j++) {
                if (j != info.src) {
                    CHK(write(pipes[j][1], &info, sizeof(info)));
                }
            }
        }
    }
    if (n == -1)
        alert(1, "read");

    CHK(close(pipes[0][0]));
    for (long i = 1; i < no_sta + 1; i++) {
        CHK(close(pipes[i][1]));
    }

    // wait for all children
    int status, exit_status = EXIT_SUCCESS;
    for (long i = 1; i < no_sta + 1; i++) {
        CHK(wait(&status));

        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
            exit_status = EXIT_FAILURE;
        }
    }
    return exit_status;
}
