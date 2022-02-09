#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

/*
 * Un petit programme pour implémenter un chronomètre :
 * Syntaxe :
 *      ./a.out                 // affiche la date correspondant à maintenant
 *      ./a.out t1              // affiche la durée entre t1 et maintenant
 *      ./a.out t1 min max      // code retour = 0 si durée \in [min, max]
 *
 * Tous les temps sont indiqués en ms
 */

#define MILLION         (1000*1000L)

typedef long double temps ;     // un point dans le temps ou une durée

struct timespec_ms              // idem timespec, mais en ms
{
    intmax_t tv_msec ;          // nb de ms
    intmax_t tv_usec ;          // nb de microsecondes dans la dernière ms
} ;

temps lire_temps (const char *str)
{
    temps t ;
    int n ;

    n = sscanf (str, "%Lf", &t) ;
    if (n == 0)
    {
        fprintf (stderr, "date invalide (%s)\n", str) ;
        exit (1) ;
    }
    return t ;
}

void lire_horloge (struct timespec_ms *tms)
{
    struct timespec ts ;

    clock_gettime (CLOCK_REALTIME, &ts) ;
    tms->tv_msec = (intmax_t)ts.tv_sec*1000 + (intmax_t)ts.tv_nsec/MILLION ;
    tms->tv_usec = (intmax_t)ts.tv_nsec % MILLION ;
}

temps duree (const char *str)
{
    struct timespec_ms tms ;
    temps t1, t2 ;

    t1 = lire_temps (str) ;
    lire_horloge (&tms) ;
    t2 = tms.tv_msec + tms.tv_usec / (temps) MILLION ;
    return t2 - t1 ;
}

int main (int argc, char *argv [])
{
    struct timespec_ms tms ;
    temps d ;
    temps min, max ;
    int r = 0 ;

    switch (argc)
    {
        case 1 :        // démarrer le chrono => date-début
            lire_horloge (&tms) ;
            printf ("%jd.%06jd\n",(intmax_t)tms.tv_msec,(intmax_t)tms.tv_usec) ;
            break ;

        case 2 :        // date-début => durée (= maintenant - début)
            d = duree (argv [1]) ;
            printf ("%Lf\n", d) ;
            break ;

        case 4 :        // date-début min max => durée+exit(0) OU erreur+exit(1)
            d = duree (argv [1]) ;
            min = lire_temps (argv [2]) ;
            max = lire_temps (argv [3]) ;
            if (d < min)
            {
                fprintf (stderr, "durée=%Lf < %Lf (min)\n", d, min) ;
                r = 1 ;
            }
            else if (d > max)
            {
                fprintf (stderr, "durée=%Lf > %Lf (max)\n", d, max) ;
                r = 1 ;
            }
            else printf ("%Lf ok\n", d) ;
            break ;

        default :
            fprintf (stderr, "usage: %s [début [min max]]\n", argv [0]) ;
            r = 1 ;
            break ;

    }
    exit (r) ;
}
