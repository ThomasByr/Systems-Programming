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
// holds the parent process needed information
typedef struct env_s env_t;

// global variables

volatile sig_atomic_t received = 0; // signal received

// [child] 0: pending, 1: running
// [parent] 0: nothing, 1: tick, 2: donned, 3: child terminated
volatile sig_atomic_t status = 0;

// globals for child processes

volatile sig_atomic_t count = 0; // number of quantum processed
volatile sig_atomic_t total = 0; // total number of quantum to process
volatile int id = 0;             // id of the current process

// globals for parent process

volatile sig_atomic_t term_count = 0; // number of terminated child processes
volatile sig_atomic_t tick = 0;       // tick received
volatile sig_atomic_t donned = 0;     // child process stopped successfully
volatile sig_atomic_t term = 0;       // child process terminated

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
 * @brief generic function to handle signals for the child processes
 *
 * @param sig signal number received
 * @note because of the way signals are received, we can't receive more than one
 * signal at a time so a single variable is used to store the signal
 */
void child_sig_handler(int sig) {
    received = 1;
    switch (sig) {
    case SIGUSR1:
        status = 1; // [child] resume process
        break;
    case SIGUSR2:
        status = 0; // [child] make the process stop
        break;
    }
}

/**
 * @brief generic function to handle signals for the parent processes
 *
 * @param sig signal number received
 * @note status is set to a non-zero value if we received a signal we want to
 * process, the actual signal(s) (because we could receive more than 1 at a time
 * trough the parent main loop) are recorded in some other global variables
 */
void parent_sig_handler(int sig) {
    received = 1;
    switch (sig) {
    case SIGUSR1:
        status = 2; // [parent] record the pending process
        donned = 1;
        break;
    case SIGALRM:
        status = 1; // [parent] send the process running
        tick = 1;
        break;
    case SIGCHLD:
        status = 3; // [parent] record the terminated process(es)
        term++;
        break;
    }
}

/**
 * @brief callback function to init the child process
 *
 * @param t total number of quantum to process
 * @param i id of the process (to print)
 */
void child_on_startup(int t, int i) {
    total = t;
    id = i;
}

/**
 * @brief things to do when the child process has terminated
 *
 */
void child_on_exit(void) {
    if (total != count)
        alert(0, "process %d terminated before all quantum processed", id);
}

/**
 * @brief sleep(1)
 *
 * @note this function has a major flow which is that we can receive a signal
 * just in between the sleep(1) call and the signal being blocked or unblocked
 * which will result in an additional sleep(1) call
 * we can resolve this by calling sleep with an arbitrary large value so that we
 * only sleep once, during which we "should" receive a signal properly
 */
void do_something(void) { sleep(1); }

/**
 * @brief child process main loop
 *
 */
void child_main_loop(void) {
    sigset_t mask, empty;
    CHK(sigemptyset(&empty));
    CHK(sigemptyset(&mask));
    CHK(sigaddset(&mask, SIGUSR1));
    CHK(sigaddset(&mask, SIGUSR2));

    while (count < total) {

        // block signals
        if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
            alert(1, "sigprocmask on mask block");

        // wait for signal
        if (!received) {
            sigsuspend(&empty);
            if (errno != EINTR)
                alert(1, "sigsuspend");
        }

        // unblock signals
        if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
            alert(1, "sigprocmask on mask unblock");

        received = 0;

        // process the signal
        switch (status) {

        case 0: // do nothing
            break;

        case 1: // process running
            count++;
            fprintf(stdout, "SURP - process %d\n", id);
            fflush(stdout);

            while (status == 1) {
                if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
                    alert(1, "sigprocmask on mask unblock");
                do_something();
                if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
                    alert(1, "sigprocmask on mask block");
            }

            CHK(kill(getppid(), SIGUSR1));
            break;
        }
    }
}

/**
 * @brief set the corresponding pid in the array to -1
 *
 * @param pids array of pids
 * @param pid pid to set to 0
 * @param nb_p number of pids in the array
 * @return int - the index of the pid in the array or -1 if not found
 */
int set_term(pid_t *pids, pid_t pid, int nb_p) {
    for (int i = 0; i < nb_p; i++) {
        if (pids[i] == pid) {
            pids[i] = -1;
            return i;
        }
    }
    return -1;
}

/**
 * @brief parent process main loop
 *
 * @param env the set of variables to work with
 */
void parent_main_loop(env_t *env) {
    int qt = (int)env->qt;
    pid_t *pids = env->pids;
    pid_t pid;
    int exit_status, exit_code;
    int count;
    int k;
    int nb_p = env->nb_processes;

    sigset_t mask, empty;
    CHK(sigemptyset(&empty));
    CHK(sigemptyset(&mask));
    CHK(sigaddset(&mask, SIGALRM));
    CHK(sigaddset(&mask, SIGUSR1));
    CHK(sigaddset(&mask, SIGCHLD));

    int index = 0;
    while (term_count < nb_p) {

        if (status == 0) {
            // send a process running
            CHK(kill(pids[index], SIGUSR1));
            alarm(qt);
        }

        // block signals
        if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
            alert(1, "sigprocmask on mask block");

        // wait for signal
        if (!received) {
            sigsuspend(&empty);
            if (errno != EINTR)
                alert(1, "sigsuspend");
        }

        // unblock signals
        if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
            alert(1, "sigprocmask on mask unblock");

        received = 0;

        // process the signal
        if (status == 0)
            continue;

        // SIGALRM
        if (tick) {
            CHK(kill(pids[index], SIGUSR2));
            tick = 0;
        }

        // SIGUSR1
        if (donned) {
            fprintf(stdout, "EVIP - process %d\n", index);
            fflush(stdout);

            // choose the next process to run
            count = 0;
            do {
                count++;
                index = (index + 1) % nb_p;
            } while (pids[index] == -1 && count < nb_p);
            donned = 0;
            status = 0; // only reset here to send a process running
        }

        // SIGCHLD
        while (term) {
            CHK((pid = wait(&exit_status)));
            k = set_term(pids, pid, nb_p);
            if (k == -1)
                alert(0, "pid not found");

            // the process has terminated
            term_count++;
            fprintf(stdout, "TERM - process %d\n", k);
            fflush(stdout);

            // check the exit code of the process
            if (WIFEXITED(exit_status) &&
                (exit_code = WEXITSTATUS(exit_status)) != EXIT_SUCCESS)
                alert(0, "child process %jd exited with status %d\n",
                      (intmax_t)pid, exit_code);

            term--; // terminate one process at a time if needed
        }
    }
}

int main(int argc, char *argv[]) {
    // usage: ./ordonnanceur <quantum in seconds> <number of quantum> <...>

    pid_t pid;
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
    if (t_array == NULL)
        alert(0, "malloc");

    // array that holds each process pid
    pid_t *pid_array = malloc(sizeof(pid_t) * n);
    if (pid_array == NULL)
        alert(0, "malloc");

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
    act.sa_handler = child_sig_handler;
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

    // modify the signal handler for the parent process
    act.sa_handler = parent_sig_handler;
    CHK(sigaction(SIGUSR1, &act, NULL));
    CHK(sigaction(SIGCHLD, &act, NULL));
    CHK(sigaction(SIGALRM, &act, NULL));

    // interract with child processes
    parent_main_loop(&env);

    // we already registered terminated processes with wait in the parent main
    // loop so we can just exit and free the remaining pid array (once)

    // free
    free(pid_array);
    return 0;
}
