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

char **creer_varg(int taille, char *vecteur[], char *ligne) {
    char **varg = malloc(sizeof(char *) * (taille + 2));

    if (varg == NULL) {
        alert(1, "malloc");
    }

    for (int i = 0; i < taille; i++) {
        varg[i] = vecteur[i];
    }

    varg[taille] = ligne;
    varg[taille + 1] = NULL;

    return varg;
}

noreturn void executer(char *cmd, char *arguments[], int tube_w) {
    execvp(cmd, arguments);

    char *msg = strerror(errno);
    CHK(write(tube_w, msg, strlen(msg)));
    CHK(close(tube_w));

    exit(EXIT_FAILURE);
}

void lancer_fils(char *vecteur[], int tube[]) {
    switch (fork()) {
    case -1:
        alert(1, "fork");
    case 0:
        CHK(close(tube[0]));
        executer(vecteur[0], vecteur, tube[1]);
    default:
        CHK(close(tube[1]));
    }
}

int attendre_fils(int n) {
    int status, return_value = EXIT_SUCCESS;

    for (int i = 0; i < n; i++) {
        CHK(wait(&status));

        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != EXIT_SUCCESS) {
                return_value = WEXITSTATUS(status);
            }
        } else {
            return_value = EXIT_FAILURE;
        }
    }
    return return_value;
}

int traiter_une_ligne(char *ligne, int argc, char *argv[]) {
    char **varg = creer_varg(argc, argv, ligne);
    int tube[2];
    CHK(pipe(tube));

    lancer_fils(varg, tube);
    free(varg);

    char buffer[BUFSIZ];
    CHK(read(tube[0], buffer, BUFSIZ));
    CHK(close(tube[0]));

    if (strlen(buffer) > 0) {
        fprintf(stderr, "%s\n", buffer);
        fflush(stderr);
        return 0;
    } else {
        return 1;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        alert(0, "usage: %s <command> [<command>...]\n", argv[0]);
    }

    int n, count = 0, return_value = EXIT_SUCCESS;
    char ligne[BUFSIZ];
    while ((n = read(STDIN_FILENO, ligne, BUFSIZ)) > 0) {
        ligne[n] = '\0';
        int status = traiter_une_ligne(ligne, argc - 1, argv + 1);
        count++;

        if (status == 0) {
            return_value = EXIT_FAILURE;
            break;
        }
    }
    if (n == -1) {
        alert(1, "read from stdin");
    }

    attendre_fils(count);
    return return_value;
}
