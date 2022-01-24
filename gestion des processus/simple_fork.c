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

int main(void) {
    pid_t pid;
    int status;

    // create a child process
    pid = fork();

    switch (pid) {

    case -1:
        complain(1, "fork");

    case 0:
        // child process
        printf("Child process\n");
        pid = getpid();
        printf("Child process id: %jd\n", (intmax_t)pid);
        printf("Child process parent id: %jd\n", (intmax_t)getppid());
        exit(pid % 10);

    default:
        // parent process
        printf("Parent process\n");
        printf("Parent process id: %jd\n", (intmax_t)getpid());
        printf("Parent process child id: %jd\n", (intmax_t)pid);
        CHK(wait(&status));

        if (WIFEXITED(status))
            printf("exit(%d)\n", WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("signal %d\n", WTERMSIG(status));
        else
            printf("other status");
        return 0;
    }
}
