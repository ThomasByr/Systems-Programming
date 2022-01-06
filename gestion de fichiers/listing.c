#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
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

void print_type(int fd) {
    struct stat s;
    CHK(fstat(fd, &s));

    // get permissions (you ugly piece of code)
    printf("%c%c%c%c%c%c%c%c%c%c", S_ISDIR(s.st_mode) ? 'd' : '-',
           s.st_mode & S_IRUSR ? 'r' : '-', s.st_mode & S_IWUSR ? 'w' : '-',
           s.st_mode & S_IXUSR ? 'x' : '-', s.st_mode & S_IRGRP ? 'r' : '-',
           s.st_mode & S_IWGRP ? 'w' : '-', s.st_mode & S_IXGRP ? 'x' : '-',
           s.st_mode & S_IROTH ? 'r' : '-', s.st_mode & S_IWOTH ? 'w' : '-',
           s.st_mode & S_IXOTH ? 'x' : '-');
    // get number of hard links
    printf(" %lu", s.st_nlink);
    // get user id
    printf(" %u", s.st_uid);
    // get group id
    printf(" %u", s.st_gid);
    // get size
    printf(" %lld", (long long)s.st_size);
    // get file name
    printf(" %s\n", s.st_mode & S_IFDIR   ? "."
                    : s.st_mode & S_IFREG ? ".."
                    : s.st_mode & S_IFLNK ? "symlink"
                                          : "unknown");

    // get last access time
    printf(" %s", ctime(&s.st_atime));
    // get last modification time
    printf(" %s", ctime(&s.st_mtime));
    // get last status change time
    printf(" %s", ctime(&s.st_ctime));
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        fprintf(stderr, "usage: %s <file>", argv[0]);

    int fd = open(argv[1], O_RDONLY);
    CHK(fd);

    print_type(fd);

    CHK(close(fd));
    return 0;
}
