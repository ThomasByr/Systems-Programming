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

volatile sig_atomic_t count = 0, stop = 0;

void sigint_handler(int sig) {
    (void)sig;
    fprintf(stderr, "\rCaught SIGINT %d times", ++count);
    fflush(stderr);

    if (count >= 5)
        stop = 1;
}

int main(void) {
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        alert(1, "signal");

    while (!stop) {
        pause();
    }
    return 0;
}
