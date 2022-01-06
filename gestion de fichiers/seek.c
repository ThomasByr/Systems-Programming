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

int pattern_in_buf(const char *pattern, const char *buf) {
    size_t len = strlen(pattern);
    size_t n = strlen(buf);
    size_t i, j;
    for (i = 0; i < n; i++) {
        if (buf[i] == pattern[0]) {
            for (j = 0; j < len; j++) {
                if (buf[i + j] != pattern[j])
                    break;
            }
            if (j == len)
                return 1;
        }
    }
    return 0;
}

int seek_in_file(int fd, const char *pattern) {
    char buf[BUFSIZ];
    int n;
    int found = 0;

    while ((n = read(fd, buf, BUFSIZ)) > 0) {
        if (pattern_in_buf(pattern, buf)) {
            found = 1;
            break;
        }
    }
    if (n == -1)
        raler(1, "read");
    return found;
}

void seek(const char *path, const char *pattern) {
    DIR *dir = opendir(path);
    if (dir == NULL)
        raler(1, "opendir");

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        char filepath[PATH_MAX];
        snprintf(filepath, PATH_MAX, "%s/%s", path, de->d_name);
        int fd = open(filepath, O_RDONLY);
        if (fd == -1)
            raler(1, "open");

        struct stat st;
        CHK(fstat(fd, &st));

        if (S_ISDIR(st.st_mode)) {
            seek(filepath, pattern);
        } else if (S_ISREG(st.st_mode)) {
            if (seek_in_file(fd, pattern))
                printf("%s\n", filepath);
        }
        CHK(close(fd));
    }
    if (errno != 0)
        raler(1, "readdir");

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <path> <pattern>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    seek(argv[2], argv[1]);
    return EXIT_SUCCESS;
}
