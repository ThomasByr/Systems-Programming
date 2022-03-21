//! does not work

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

#define OK 100
#define EOF_rval -1

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

int get_randint(void) { return rand() % INT_MAX; }

int traite_bille(int prec, int suiv) {
    int size, n;
    CHK(size = read(prec, &n, sizeof(n)));

    if (size == 0)
        return EOF_rval;

    n--;
    if (n <= 0) {
        return 0;
    }

    CHK(write(suiv, &n, sizeof(n)));
    return 1;
}

noreturn void fils(int num, int prec, int suiv, int init) {
    int n;

    switch (num) {
    case 0:
        CHK(read(init, &n, sizeof(n)));
        CHK(write(suiv, &n, sizeof(n)));

        CHK(close(init));
        break;

    default:
        while (traite_bille(prec, suiv))
            ;

        CHK(close(prec));
        CHK(close(suiv));
        break;
    }

    exit(OK);
}

int attendre_fils(void) {
    int n;
    int status, exit_status;

    for (int i = 0; i < 37; i++) {
        CHK(wait(&status));

        if (WIFEXITED(status)) {
            exit_status = WEXITSTATUS(status);
            if (0 <= exit_status && exit_status <= 36) {
                n = i;
            }
        } else {
            alert(1, "bad child status");
        }
    }

    return n;
}

void creer_fils(int tube_pere[]) {

    int prev[2], next[2];
    prev[0] = tube_pere[0];
    prev[1] = tube_pere[1];

    for (int i = 0; i < 37; i++) {
        switch (fork()) {
        case -1:
            alert(1, "fork");
        case 0:
            CHK(pipe(next));
            fils(i, prev[0], next[1], prev[1]);
        default:
            prev[0] = next[0];
            prev[1] = next[1];
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        alert(1, "usage: %s <n°>", argv[0]);

    int n = get_randint();
    int tube_pere[2];
    CHK(pipe(tube_pere));

    creer_fils(tube_pere);

    CHK(write(tube_pere[1], &n, sizeof(n)));
    CHK(close(tube_pere[1]));

    int n_roul = attendre_fils();
    char *msg[] = {"WON", "FAILED"};
    printf("RESULT = %d\n => %s", n_roul, msg[n_roul == n ? 0 : 1]);

    return n_roul == n ? EXIT_SUCCESS : EXIT_FAILURE;
}
