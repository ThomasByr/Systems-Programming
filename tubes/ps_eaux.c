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

void wait_for(void) {
    int status, exit_status;
    CHK(wait(&status));
    if (WIFEXITED(status)) {
        if ((exit_status = WEXITSTATUS(status)) != 0)
            alert(0, "child exited with status %d", exit_status);
    } else {
        alert(0, "child terminated abnormally");
    }
}

ssize_t wc_l(int fd) {
    // count lines
    char buf[BUFSIZ];
    ssize_t n, n_lines = 0;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        char *p = buf;
        while ((p = strchr(p, '\n')) != NULL) {
            n_lines++;
            p++;
        }
    }

    if (n == -1)
        alert(1, "read");

    return n_lines;
}

int main(int argc, char *argv[]) {
    int p0[2], p1[2];
    char user[BUFSIZ], username[256], *env;

    if (argc > 2)
        alert(0, "usage: %s <username>", argv[0]);

    if (argc == 1) {
        if ((env = getenv("USER")) == NULL)
            alert(0, "USER env var not set");

        strncpy(username, env, sizeof(username));
    } else {
        strncpy(username, argv[1], sizeof(username));
    }

    // insert '^' to the beginning of the username
    CHK(snprintf(user, sizeof(user), "^%s", username));

    // command : ps eaux | grep "^<username>" | wc -l

    CHK(pipe(p0));

    switch (fork()) {
    case -1:
        alert(1, "fork");

    case 0: // ps eaux
        CHK(close(p0[0]));
        CHK(dup2(p0[1], STDOUT_FILENO));
        CHK(close(p0[1]));

        CHK(execlp("ps", "ps", "eaux", NULL));
        alert(1, "execlp");
    }

    CHK(close(p0[1]));
    CHK(pipe(p1));

    switch (fork()) {
    case -1:
        alert(1, "fork");

    case 0: // grep "^<username>"
        CHK(close(p1[0]));
        CHK(dup2(p0[0], STDIN_FILENO));
        CHK(close(p0[0]));

        CHK(dup2(p1[1], STDOUT_FILENO));
        CHK(close(p1[1]));

        CHK(execlp("grep", "grep", user, NULL));
        alert(1, "execlp");
    }

    // wc -l

    CHK(close(p0[0]));
    CHK(close(p1[1]));

    CHK(dup2(p1[0], STDIN_FILENO));
    CHK(close(p1[0]));

    execlp("wc", "wc", "-l", NULL);
    alert(1, "execlp");
}
