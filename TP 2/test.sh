#!/bin/sh

PROG="./ordonnanceur"
TMP="/tmp/$$"

##############################################################################
# Fonctions utilitaires

# teste si un fichier passé en arg est vide
check_empty ()
{
    if [ -s $1 ]; then
        return 0;
    fi

    return 1
}

# teste si le pg a échoué
# - code de retour du pg doit être égal à 1
# - stdout doit être vide
# - stderr doit contenir un message d'erreur
echec()
{
    if [ $1 -ne 1 ]; then
        echo "échec => code de retour != 1"
        return 0
    fi

    if check_empty $TMP/stdout; then
        echo "échec => sortie standard non vide"
        return 0
    fi

    if ! check_empty $TMP/stderr; then
        echo "échec => sortie erreur vide"
        return 0
    fi

    return 1
}

# teste si le pg a réussi
# - code de retour du pg doit être égal à 0
# - stderr doit être vide
# - stdout ne doit pas être vide
success()
{
    if [ $1 -ne 0 ]; then
        echo "échec => code de retour != 0"
        return 0
    fi

    if ! check_empty $TMP/stdout; then
        echo "échec => sortie standard vide"
        return 0
    fi

    if check_empty $TMP/stderr; then
        echo "échec => sortie erreur non vide"
        return 0
    fi

    return 1
}

##############################################################################
# Une sorte de chronomètre
#
# La commande date ne permet pas de récupérer l'heure avec une précision
# supérieure à la seconde (si l'on s'en tient à POSIX). On se fait donc
# la nôtre, dont les unités sont toutes en ms.
# Cette commande est ici placée dans un fichier à part, compilé avec Makefile
#

CHRONO=./chrono         # notre chronomètre (déjà compilé via Makefile)

init_chrono ()
{
    [ ! -x $CHRONO ] && fail "Il faut compiler '$CHRONO' (cf Makefile)"
}

# Démarrer le chronomètre
chrono_start ()
{
    [ $# != 0 ] && echo "ERREUR SYNTAXE chrono_start"
    init_chrono
    CHRONO_DEBUT=$($CHRONO)
}

# Arrêter le chronomètre et vérifier que la durée passée en paramètre
# est dans l'intervalle spécifié
# code retour = 0 (ok) ou 1 (erreur : durée hors de l'intervalle)
chrono_stop ()
{
    [ $# != 2 ] && echo "ERREUR SYNTAXE chrono_stop"
    local min_ms="$1" max_ms="$2"
    $CHRONO $CHRONO_DEBUT "$min_ms" "$max_ms" >&2 || return 1
    return 0
}


##############################################################################
# début des tests

test_1()
{
    echo "Test 1 - syntaxe du programme"

    #################################################################################################
    echo -n "Test 1.1 - sans argument............................"
    $PROG > $TMP/stdout 2> $TMP/stderr
    if echec $?;                   then                                                  return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 1.2 - avec un argument........................."
    $PROG 5 > $TMP/stdout 2> $TMP/stderr
    if echec $?;                   then                                                  return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 1.3 - durée d'un quantum < 1..................."
    $PROG 0 1 > $TMP/stdout 2> $TMP/stderr
    if echec $?;                   then                                                  return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 1.4 - durée d'un process < 1..................."
    $PROG 3 3 -2 1 > $TMP/stdout 2> $TMP/stderr
    if echec $?;                   then                                                  return 1; fi
    echo "OK"

    #################################################################################################
    echo -n "Test 1.5 - syntaxe valide..........................."
    $PROG 1 1 > $TMP/stdout 2> $TMP/stderr
    if success $?;                 then                                                  return 1; fi
    echo "OK"
}

test_2()
{
    echo "Test 2 - affichage du programme sans la terminaison"

    #################################################################################################
    echo -n "Test 2.1 - affichage avec 1 processus..............."
    cat > $TMP/sortie <<EOF
SURP - process 0
EVIP - process 0
EOF
    $PROG 1 1 > $TMP/stdout 2> $TMP/stderr
    if success $?;                 then                                                  return 1; fi
    grep -v TERM $TMP/stdout > $TMP/stdout2
    ! cmp $TMP/stdout2 $TMP/sortie > /dev/null 2>&1 && echo "échec : stdout non conforme" && return 1
    echo "OK"

    #################################################################################################
    echo -n "Test 2.2 - affichage avec 4 processus..............."
    cat > $TMP/sortie <<EOF
SURP - process 0
EVIP - process 0
SURP - process 1
EVIP - process 1
SURP - process 2
EVIP - process 2
SURP - process 3
EVIP - process 3
EOF
    $PROG 1 1 1 1 1 > $TMP/stdout 2> $TMP/stderr
    if success $?;                 then                                                  return 1; fi
    grep -v TERM $TMP/stdout > $TMP/stdout2
    ! cmp $TMP/stdout2 $TMP/sortie > /dev/null 2>&1 && echo "échec : stdout non conforme" && return 1
    echo "OK"
}

test_3()
{
    echo "Test 3 - gestion de la terminaison"

    #################################################################################################
    echo -n "Test 3.1 - 1 processus.............................."
    cat > $TMP/sortie <<EOF
TERM - process 0
EOF
    $PROG 1 1 > $TMP/stdout 2> $TMP/stderr
    if success $?;                 then                                                  return 1; fi
    grep TERM $TMP/stdout > $TMP/stdout2
    ! cmp $TMP/stdout2 $TMP/sortie > /dev/null 2>&1 && echo "échec : stdout non conforme" && return 1
    echo "OK"

    #################################################################################################
    echo -n "Test 3.2 - 4 processus.............................."
    cat > $TMP/sortie <<EOF
TERM - process 0
TERM - process 1
TERM - process 2
TERM - process 3
EOF
    $PROG 1 1 1 1 1 > $TMP/stdout 2> $TMP/stderr
    if success $?;                 then                                                  return 1; fi
    grep TERM $TMP/stdout > $TMP/stdout2
    ! cmp $TMP/stdout2 $TMP/sortie > /dev/null 2>&1 && echo "échec : stdout non conforme" && return 1
    echo "OK"

    #################################################################################################
    echo -n "Test 3.3 - 4 processus avec durée décroissante......"
    cat > $TMP/sortie <<EOF
TERM - process 3
TERM - process 2
TERM - process 1
TERM - process 0
EOF
    $PROG 1 4 3 2 1  > $TMP/stdout 2> $TMP/stderr
    if success $?;                 then                                                  return 1; fi
    grep TERM $TMP/stdout > $TMP/stdout2
    ! cmp $TMP/stdout2 $TMP/sortie > /dev/null 2>&1 && echo "échec : stdout non conforme" && return 1
    echo "OK"

}

test_4 ()
{
    echo "Test 4 - durée du programme"

    #################################################################################################
    echo -n "Test 4.1 - 4 processus durée totale 4 sec..........."
    chrono_start
    $PROG 1 1 1 1 1  > $TMP/stdout 2> $TMP/stderr
    ! chrono_stop 4000 5050 2> $TMP/chrono && echo -n "échec : " && cat $TMP/chrono && return 1
    echo "OK"

    #################################################################################################
    echo -n "Test 4.2 - 4 processus durée totale 10 sec.........."
    chrono_start
    $PROG 1 1 2 3 4  > $TMP/stdout 2> $TMP/stderr
    ! chrono_stop 10000 10050 2> $TMP/chrono && echo -n "échec : " && cat $TMP/chrono && return 1
    echo "OK"

    #################################################################################################
    echo -n "Test 4.3 - 2 processus durée totale 10 sec.........."
    chrono_start
    $PROG 2 3 2 > $TMP/stdout 2> $TMP/stderr
    ! chrono_stop 10000 10050 2> $TMP/chrono && echo -n "échec : " && cat $TMP/chrono && return 1
    echo "OK"
}

test_5()
{
    echo -n "Test 5 - test mémoire..............................."
    valgrind --leak-check=full --trace-children=yes --error-exitcode=100 $PROG 2 3 2 >/dev/null 2> $TMP/stderr
    test $? = 100 && echo "échec => log de valgrind dans $TMP/stderr" && return 1
    echo "OK"

    return 0
}

run_all()
{
    # Lance la série des tests
    for T in $(seq 1 5); do
        if test_$T; then
            echo "== Test $T : ok $T/5\n"
        else
            echo "== Test $T : échec"
            return 1
        fi
    done

    rm -R $TMP
}

# répertoire temp où sont stockés tous les fichiers et sorties du pg
mkdir $TMP

if [ $# -eq 1 ]; then
    case $1 in 1) test_1;;
               2) test_2;;
               3) test_3;;
               4) test_4;;
               5) test_5;;
               *) echo "test inexistant"; return 1;
    esac
else
    run_all
fi
