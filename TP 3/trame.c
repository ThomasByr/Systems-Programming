#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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

#define PATH 256
#define MAXSTA 10
#define PAYLOAD_SIZE 4

struct file_entry {
    int dst;
    char payload[PAYLOAD_SIZE];
};

int main(int argc, char *argv[]) {
    if (argc != 4)
        raler(0, "usage: %s src dst payload", argv[0]);

    int src;
    if (sscanf(argv[1], "%d", &src) != 1 || src < 1 || src > MAXSTA)
        raler(0, "adresse source dans [1, %d]", MAXSTA);

    char filename[PATH];
    int n = snprintf(filename, PATH, "STA_%d", src);
    if (n < 0 || n >= PATH)
        raler(0, "snprinf");

    int d;
    CHK(d = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666));

    struct file_entry t;
    if (sscanf(argv[2], "%d", &t.dst) != 1 || t.dst < 0 || t.dst > MAXSTA ||
        src == t.dst)
        raler(0, "adresse destination dans [0, %d] et diff de %d", MAXSTA, src);

    if (strlen(argv[3]) != PAYLOAD_SIZE)
        raler(0, "payload doit avoir une taille de %d", PAYLOAD_SIZE);
    strncpy(t.payload, argv[3], PAYLOAD_SIZE);

    CHK(write(d, &t, sizeof t));

    CHK(close(d));

    return 0;
}
