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

volatile sig_atomic_t stop = 0;

void sig_handler(int sig) {
    psignal(sig, "blabla");
    stop = 1;
}

int main(void) {
    // handle all signals
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGTERM, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGQUIT, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGABRT, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGSEGV, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGBUS, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGFPE, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGILL, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGTRAP, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGSYS, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGXCPU, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGXFSZ, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGUSR1, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGUSR2, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGPIPE, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGALRM, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGSTKFLT, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGCHLD, sig_handler) == SIG_ERR)
        alert(1, "signal");
    if (signal(SIGCONT, sig_handler) == SIG_ERR)
        alert(1, "signal");

    while (!stop)
        pause();
    return 0;
}
