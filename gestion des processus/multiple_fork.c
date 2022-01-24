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

int child(void) {
    printf("Child process n°%jd\n", (intmax_t)getpid());
    return 0;
}

int main(int argc, char *argv[]) {
    pid_t pid;
    int status;

    if (argc != 2)
        complain(0, "Usage: %s <positive integer>\n", argv[0]);

    int n = atoi(argv[1]);

    // spawn n child processes
    for (int i = 0; i < n; i++) {
        switch (pid = fork()) {

        case -1:
            complain(1, "fork");

        case 0:
            child();
            exit(EXIT_SUCCESS);
        }
    }

    // wait for all child processes to terminate
    for (int i = 0; i < n; i++) {
        CHK(wait(&status));

        if (WIFEXITED(status))
            printf("exit(%d)\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("signal %d\n", WTERMSIG(status));
        else
            printf("other status");
    }
    return 0;
}
