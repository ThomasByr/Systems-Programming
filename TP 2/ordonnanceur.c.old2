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

struct env_s {
    long qt;          // quantum duration
    pid_t *pids;      // array of pids
    int nb_processes; // number of processes
};
typedef struct env_s env_t;

volatile sig_atomic_t received = 0; // signal received

// globals for child processes

volatile sig_atomic_t status = 0; // 0: pending, 1: running
volatile sig_atomic_t count = 0;  // number of quantum processed
volatile sig_atomic_t total = 0;  // total number of quantum to process
volatile int id = 0;              // id of the current process

// globals for parent process

volatile sig_atomic_t tick = 0;
volatile sig_atomic_t donned = 0;
volatile sig_atomic_t child_term = 0;
volatile sig_atomic_t term_count = 0;

/**
 * @brief initialize the environment (process info and pids)
 *
 * @param env pointer to the environment
 * @param processes pointer to an array of process info
 * @param pids pointer to an array of pids
 * @param nb_processes number of processes
 */
void env_init(env_t *env, long qt, pid_t *pids, int nb_processes) {
    env->qt = qt;
    env->pids = pids;
    env->nb_processes = nb_processes;
}

/**
 * @brief generic function to handle signals
 *
 * @param sig signal number received
 */
void sig_handler(int sig) {
    switch (sig) {
    case SIGUSR1:     // shared between parent and child
        received = 1; // [child]
        status = 1;   // [child] send the process running
        donned = 1;   // [parent]
        break;
    case SIGUSR2:     // only for child
        received = 1; // [child]
        status = 0;   // [child] make the process stop
        break;
    case SIGALRM: // only for parent
        tick = 1; // [parent]
        break;
    case SIGCHLD:       // only for parent
        child_term = 1; // [parent]
        donned = 1;     // [parent]
        break;
    }
}

void child_on_startup(int t, int i) {
    total = t;
    id = i;
}

void child_on_exit(void) {
    total = 0;
    count = 0;
    status = 0;
    received = 0;
}

void child_main_loop(void) {
    sigset_t mask, empty;
    CHK(sigemptyset(&empty));
    CHK(sigemptyset(&mask));
    CHK(sigaddset(&mask, SIGUSR1));
    CHK(sigaddset(&mask, SIGUSR2));

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        alert(1, "sigprocmask on mask block");

    while (count < total) {

        // wait for signal
        while (!received)
            sigsuspend(&empty);

        received = 0;

        // process the signal
        switch (status) {
        case 0:
            // do nothing
            break;
        case 1:
            count++;
            fprintf(stdout, "SURP - process %d\n", id);
            fflush(stdout);

            while (status == 1) {
                if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
                    alert(1, "sigprocmask on mask unblock");
                sleep(1);
                if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
                    alert(1, "sigprocmask on mask block");
            }

            status = 0;
            CHK(kill(getppid(), SIGUSR1));
            break;
        }
    }
}

void parent_main_loop(env_t *env) {
    int qt = (int)env->qt;
    pid_t *pids = env->pids;
    int nb_p = env->nb_processes;

    sigset_t mask, term, empty;
    CHK(sigemptyset(&empty));
    CHK(sigemptyset(&mask));
    CHK(sigemptyset(&term));
    CHK(sigaddset(&mask, SIGALRM));
    CHK(sigaddset(&mask, SIGUSR1));
    CHK(sigaddset(&term, SIGCHLD));

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        alert(1, "sigprocmask on mask block");
    if (sigprocmask(SIG_BLOCK, &term, NULL) == -1)
        alert(1, "sigprocmask on term block");

    int index = 0;
    while (term_count < nb_p) {
        alarm(qt);
        CHK(kill(pids[index], SIGUSR1));

        while (!tick)
            sigsuspend(&empty);

        tick = 0;

        CHK(kill(pids[index], SIGUSR2));

        while (!donned)
            sigsuspend(&empty);

        fprintf(stdout, "EVIP - process %d\n", index);
        fflush(stdout);

        donned = 0;

        // check if we did receive SIGCHLD
        if (sigprocmask(SIG_UNBLOCK, &term, NULL) == -1)
            alert(1, "sigprocmask on term unblock");
        if (sigprocmask(SIG_BLOCK, &term, NULL) == -1)
            alert(1, "sigprocmask on term block");
        if (child_term) {
            term_count++;
            child_term = 0;
            fprintf(stdout, "TERM - process %d\n", index);
            fflush(stdout);
            pids[index] = -1;
        }

        int count = 0;
        do {
            index = (index + 1) % nb_p;
            count++;
        } while (pids[index] == -1 && count < 3 * nb_p);
    }
}

int main(int argc, char *argv[]) {
    // usage: ./ordonnanceur <quantum in seconds> <number of quantum> <...>

    pid_t pid;
    int status;
    int n = argc - 2;

    // check number of arguments
    if (n <= 0)
        alert(0, "usage: %s <t> t1 [t2] [t3] ...", argv[0]);

    // check for values
    long t = strtol(argv[1], NULL, 10);
    if (t <= 0)
        alert(0, "t must be a positive integer");

    // array that holds each process time to be used in the scheduler
    long *t_array = malloc(sizeof(long) * n);

    // array that holds each process pid
    pid_t *pid_array = malloc(sizeof(pid_t) * n);

    // check for values
    for (int i = 2; i < argc; i++) {
        long ti = strtol(argv[i], NULL, 10);
        if (ti <= 0) {
            free(t_array);
            alert(0, "t%d must be a positive integer", i);
        }
        t_array[i - 2] = ti;
    }

    // initialize the signal handler for the child processes
    struct sigaction act;
    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    CHK(sigemptyset(&act.sa_mask));
    CHK(sigaction(SIGUSR1, &act, NULL));
    CHK(sigaction(SIGUSR2, &act, NULL));

    // launch the child processes
    for (int i = 0; i < n; i++) {
        switch ((pid = fork())) {
        case -1:
            alert(1, "pid = fork()");
        case 0:
            child_on_startup(t_array[i], i); // child process init
            child_main_loop();               // child process main loop
            child_on_exit();                 // child process exit
            free(t_array);
            free(pid_array);
            CHK(kill(getppid(), SIGCHLD));
            exit(EXIT_SUCCESS);
        default:
            pid_array[i] = pid;
            break;
        }
    }

    // initialize the environment
    env_t env; // environment for the parent process
    env_init(&env, t, pid_array, n);
    free(t_array); // we should not use that anymore
    t_array = NULL;

    // modify the signal handlers for the parent process
    CHK(sigaction(SIGCHLD, &act, NULL));
    CHK(sigaction(SIGALRM, &act, NULL));

    // interract with child processes
    parent_main_loop(&env);

    // register the child processes
    for (int i = 0; i < n; i++) {
        CHK((pid = wait(&status)));

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != EXIT_SUCCESS) {
                fprintf(stderr, "child process %jd exited with status %d\n",
                        (intmax_t)pid, exit_status);
                exit(EXIT_FAILURE);
            }
        }
    }

    // free
    free(pid_array);
    return 0;
}
