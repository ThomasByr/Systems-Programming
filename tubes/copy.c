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

void copy(int fd_in, int fd_out) {
    char buf[BUFSIZ];
    ssize_t n;

    while ((n = read(fd_in, buf, sizeof(buf))) > 0)
        CHK(write(fd_out, buf, n));

    if (n == -1)
        alert(1, "read");
}

int main(void) {
    int status, exit_status;
    int p[2];
    CHK(pipe(p));

    switch (fork()) {
    case -1:
        alert(1, "fork");

    case 0:
        CHK(close(p[0]));
        copy(STDIN_FILENO, p[1]);
        CHK(close(p[1]));

        exit(EXIT_SUCCESS);

    default:
        CHK(close(p[1]));
        copy(p[0], STDOUT_FILENO);
        CHK(close(p[0]));

        CHK(wait(&status));
        if (WIFEXITED(status)) {
            if ((exit_status = WEXITSTATUS(status)) != EXIT_SUCCESS)
                alert(0, "child exited with status %d", exit_status);
        } else
            alert(0, "child terminated abnormally");
    }
    return EXIT_SUCCESS;
}
