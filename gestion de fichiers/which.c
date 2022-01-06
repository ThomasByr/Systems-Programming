#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define CHK(op)            \
    do {                   \
        if ((op) == -1)    \
            raler(1, #op); \
    } while (0)

noreturn void raler(int syserr, const char *msg, ...) {
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    if (syserr == 1)
        perror("");

    exit(EXIT_FAILURE);
}

/**
 * @brief search for a command in PATH
 *
 */
void find(const char *cmd) {
    char *path = getenv("PATH");
    if (path == NULL)
        raler(0, "getenv");

    struct stat st;
    int exists = 0;
    char fullpath[PATH_MAX];

    char *p = strtok(path, ":");
    while (p != NULL) {
        sprintf(fullpath, "%s/%s", p, cmd);
        exists = stat(fullpath, &st);
        if (exists == 0 && S_ISREG(st.st_mode)) {
            printf("%s\n", fullpath);
            return;
        }
        p = strtok(NULL, ":");
    }
    printf("%s: not found\n", cmd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <cmd>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    find(argv[1]);
    return 0;
}
