/* reseau.c

Small program to simulate a ethernet network of some stations connected via a
commutator. The parent process is the commutator, the child processes are the
stations.

The main issue of this program is that the stations all write to the commutator
at the same time and on the same file descriptor. This is not a problem for
the commutator, but it is a problem for the stations (the stations must be
able to read from the commutator at the same time though). For example, if
the task scheduler forces a child process to quit before it has finished
writing to the commutator, the pipe will be left in an inconsistent state.

The solution could be to use a semaphore to protect the commutator from the
stations (that or either to use signals to protect the stations from writing all
at the same time).
*/

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

#define MAXSTA 10u
#define PAYLOAD_SIZE 4ul

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

/// structure that holds the raw data of a packet
struct sta_s {
    int dest;                   // destination station
    char payload[PAYLOAD_SIZE]; // payload
};

/// structure that holds the data of a packet after it has been decoded
struct info_s {
    int src;                    // source station
    int dest;                   // destination station
    char payload[PAYLOAD_SIZE]; // payload
};

/**
 * @brief child process that simulates a station
 *
 * 1. Reads packets from a file
 * 2. Decodes the packets
 * 3. Writes the packets to the commutator
 * 4. Waits for the commutator to read the packets
 * 5. Reads new packets from the commutator
 * 6. Writes the new packets to a file (standard output)
 *
 * @param id the id of the station
 * @param in the file descriptor of the pipe to read from
 * @param out the file descriptor of the pipe to write to
 */
void child_main(int id, int in, int out) {
    size_t k;
    int fd, n;
    char filename[10];
    struct sta_s sta;
    struct info_s info;

    sprintf(filename, "STA_%d", id);
    CHK((fd = open(filename, O_RDONLY)));

    // format : (dest payload)
    while ((n = read(fd, &sta, sizeof(sta))) > 0) {
        // decode
        info.src = id; // here, decoding is adding the source station
        info.dest = sta.dest;
        strncpy(info.payload, sta.payload, PAYLOAD_SIZE);

        // send (dest payload) to parent via pipe
        CHK(write(out, &info, sizeof(info)));
    }
    if (n == -1) {
        alert(1, "reading from %s", filename);
    }

    CHK(close(fd));
    CHK(close(out));

    while ((n = read(in, &info, sizeof(info))) > 0) {
        // wait for parent to send back (src dest payload)
        // print (id - src - dest - payload)
        fprintf(stdout, "%d - %d - %d - ", id, info.src, info.dest);
        k = fwrite(info.payload, STDOUT_FILENO, PAYLOAD_SIZE, stdout);
        if (k != PAYLOAD_SIZE)
            alert(1, "fwrite ended with %zu", k);

        fprintf(stdout, "\n");
        fflush(stdout);
    }
    if (n == -1) {
        alert(1, "reading from parent");
    }

    CHK(close(in));
}

/**
 * @brief function that simulates a commutator
 *
 * 1. Reads packets from a pipe
 * 2. Determine the destination of the packet
 * 3. Writes the packet to the destination station
 *
 * @param pipes the array of file descriptors of the pipes
 * @param nb_sta the number of stations
 */
void parent_main(int (*pipes)[2], long nb_sta) {
    // read (src dest payload) from children
    struct info_s info;
    int n;
    while ((n = read(pipes[0][0], &info, sizeof(info))) > 0) {
        // send (src dest payload) dest children via pipes
        // if dest is undefined, send to all children except src
        if (1 <= info.dest && info.dest <= nb_sta) {
            CHK(write(pipes[info.dest][1], &info, sizeof(info)));
        } else {
            for (long j = 1; j < nb_sta + 1; j++) {
                if (j != info.src) {
                    CHK(write(pipes[j][1], &info, sizeof(info)));
                }
            }
        }
    }
    if (n == -1) {
        alert(1, "reading from children");
    }

    CHK(close(pipes[0][0]));
    for (long i = 1; i < nb_sta + 1; i++) {
        CHK(close(pipes[i][1]));
    }
}

int main(int argc, char *argv[]) {
    long nb_sta; // number of stations

    if (argc != 2) {
        alert(0, "usage: %s <nb_sta>", argv[0]);
    }

    char *endptr;
    nb_sta = strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || *endptr != '\0') {
        alert(1, "nb_sta is not a number");
    }
    if (errno == ERANGE)
        alert(1, "nb_sta out of range [%ld, %ld]", LONG_MIN, LONG_MAX);
    if (nb_sta < 1 || nb_sta > MAXSTA)
        alert(0, "nb_sta should be in [1, %d]", MAXSTA);

    int pipes[MAXSTA + 1][2];
    CHK(pipe(pipes[0])); // children -> parent

    for (long i = 1; i < nb_sta + 1; i++) {
        CHK(pipe(pipes[i])); // parent -> child

        switch (fork()) {

        case -1:
            alert(1, "fork");

        case 0:
            // closing unused pipes before calling child_main
            CHK(close(pipes[0][0]));
            CHK(close(pipes[i][1]));
            for (long j = 1; j < i; j++) {
                CHK(close(pipes[j][0]));
                CHK(close(pipes[j][1]));
            }

            // calling child_main
            // this function will close all pipes before exiting
            child_main(i, pipes[i][0], pipes[0][1]);

            exit(EXIT_SUCCESS);
        }
    }

    // closing unused pipes for parent before calling parent_main
    CHK(close(pipes[0][1]));
    for (long i = 1; i < nb_sta + 1; i++) {
        CHK(close(pipes[i][0]));
    }

    // calling parent_main
    // this function will close all pipes before exiting
    parent_main(pipes, nb_sta);

    // wait for all children
    int status, exit_status = EXIT_SUCCESS;
    for (long i = 1; i < nb_sta + 1; i++) {
        CHK(wait(&status));

        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
            exit_status = EXIT_FAILURE;
        }
    }
    return exit_status;
}
