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
    if (argc != 2)
        complain(0, "usage: %s <dirname>", argv[0]);

    // get the current time
    struct timeval tv;
    CHK(gettimeofday(&tv, NULL));
    printf("%ju.%06ju\n", (uintmax_t)tv.tv_sec, (uintmax_t)tv.tv_usec);

    pid_t pid;
    int status;

    switch (pid = fork()) {

    case -1:
        complain(1, "pid = fork()");

    case 0:
        // call ls -l on the directory
        CHK(execlp("ls", "ls", "-l", argv[1], NULL));
        exit(EXIT_FAILURE);

    default:
        // wait for the child to finish
        CHK(waitpid(pid, &status, 0));

        if (WIFEXITED(status))
            printf("exit(%d)\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("signal %d\n", WTERMSIG(status));
        else
            printf("other status\n");

        // get the current time
        CHK(gettimeofday(&tv, NULL));
        printf("%ju.%06ju\n", (uintmax_t)tv.tv_sec, (uintmax_t)tv.tv_usec);
        return 0;
    }
}
