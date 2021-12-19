#!/bin/sh

PROG="./compare"
TMP="/tmp/$$"

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
check_echec()
{
    if [ $1 -ne 1 ]; then
        echo "échec => code de retour == 0 alors que 1 attendu"
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
# - stdout et stderr doivent être vides
check_success()
{
    if [ $1 -ne 0 ]; then
        echo "échec => code de retour == 1 alors que 0 attendu"
        return 0
    fi

    if check_empty $TMP/stdout; then
       echo "échec => sortie standard non vide"
       return 0
    fi

    if check_empty $TMP/stderr; then
        echo "échec => sortie erreur non vide"
        return 0
    fi

    return 1
}

# vérifie le message d'erreur
cmp_sortie()
{
    MSG=`cat $TMP/stderr`

    if [ "$MSG" != "$1" ]; then
        echo "sortie erreur différente"
        echo -n "sortie du pg:\t"
        echo $MSG
        echo -n "devrait être :\t"
        echo $1
        return 0
    fi

    return 1
}

test_1()
{
    echo "Test 1 - tests sur les arguments du programme"

    echo -n "Test 1.1 - sans argument.........................."
    $PROG                          > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                             then return 1; fi
    echo "OK"

    echo -n "Test 1.2 - 1 argument............................."
    $PROG test.sh                  > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                             then return 1; fi
    echo "OK"

    echo -n "Test 1.3 - trop d'arguments......................."
    $PROG test.sh test.sh test.sh  > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                             then return 1; fi
    echo "OK"

    echo -n "Test 1.4 - fichier inexistant....................."
    $PROG ldjksqld test.sh         > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                             then return 1; fi

    $PROG test.sh fdsfsdf          > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                             then return 1; fi
    echo "OK"

    echo -n "Test 1.5 - syntaxe ok............................."
    touch $TMP/vide
    $PROG $TMP/vide $TMP/vide      > $TMP/stdout 2> $TMP/stderr
    if check_success $?;                           then return 1; fi
    echo "OK"
}

test_2 ()
{
    echo "Test 2 - tests sur des fichiers de même taille"

    echo -n "Test 2.1 - petits fichiers identiques............."
    echo "abc" > $TMP/toto ; echo "abc" > $TMP/titi
    $PROG $TMP/titi $TMP/toto      > $TMP/stdout 2> $TMP/stderr
    if check_success $?;                                                then return 1; fi
    echo "OK"

    echo -n "Test 2.2 - petits fichiers différents............."
    echo "acb" > $TMP/toto
    $PROG $TMP/titi $TMP/toto      > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                                                  then return 1; fi
    if cmp_sortie  "$TMP/titi $TMP/toto differ: byte 2, line 1";        then return 1; fi
    echo "OK"

    echo -n "Test 2.3 - grands fichiers identiques............."
    $PROG /bin/ls /bin/ls          > $TMP/stdout 2> $TMP/stderr
    if check_success $?;                                                then return 1; fi
    echo "OK"

    echo -n "Test 2.4 - grands fichiers différents............."
    cp /bin/ls $TMP/toto ; cp /bin/ls $TMP/titi
    echo "a" >> $TMP/toto    ; echo "b" >> $TMP/titi
    $PROG $TMP/titi $TMP/toto      > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                                                  then return 1; fi
    LC_ALL=C cmp $TMP/titi $TMP/toto > $TMP/tmp
    B=`cat $TMP/tmp | tr -d ',' | cut -d ' ' -f5`
    L=`cat $TMP/tmp | tr -d ',' | cut -d ' ' -f7`
    if cmp_sortie  "$TMP/titi $TMP/toto differ: byte $B, line $L"; then return 1; fi
    echo "OK"
}

test_3 ()
{
    echo "Test 3 - tests sur des fichiers de taille diff."

    echo -n "Test 3.1 - petits fichiers avec début identique..."
    echo -n "abc" > $TMP/toto ; echo -n "ab" > $TMP/titi
    $PROG $TMP/toto $TMP/titi      > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                                                  then return 1; fi
    if cmp_sortie  "EOF on $TMP/titi after byte 2, line 1";             then return 1; fi
    echo "OK"

    echo -n "Test 3.2 - grands fichiers avec début identique..."
    cp /bin/ls $TMP/toto ; echo -n "a" >> $TMP/toto
    $PROG /bin/ls $TMP/toto        > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                                                  then return 1; fi
    LC_ALL=C cmp /bin/ls $TMP/toto 2> $TMP/tmp
    B=`cat $TMP/tmp | tr -d ',' | cut -d ' ' -f7`
    L=`cat $TMP/tmp | tr -d ',' | cut -d ' ' -f10`
    if cmp_sortie "EOF on /bin/ls after byte $B, line $L";         then return 1; fi
    echo "OK"

    echo -n "Test 3.3 - grands fichiers avec diff.............."
    if ! type "sed" > /dev/null; then echo "please install sed for running this test"; return 1; fi
    LC_ALL=C sed "s/%N/%X/" /bin/ls > $TMP/toto
    $PROG /bin/ls $TMP/toto        > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                                                  then return 1; fi
    LC_ALL=C cmp /bin/ls $TMP/toto > $TMP/tmp
    B=`cat $TMP/tmp | tr -d ',' | cut -d ' ' -f5`
    L=`cat $TMP/tmp | tr -d ',' | cut -d ' ' -f7`
    if cmp_sortie  "/bin/ls $TMP/toto differ: byte $B, line $L";   then return 1; fi
    echo "OK"

    echo -n "Test 3.4 - un fichier vide........................"
    touch $TMP/tata
    $PROG $TMP/toto $TMP/tata      > $TMP/stdout 2> $TMP/stderr
    if check_echec $?;                                                  then return 1; fi
    if cmp_sortie  "EOF on $TMP/tata which is empty";                   then return 1; fi
    echo "OK"
}

test_4()
{
    echo -n "Test 4 - test mémoire............................."
    valgrind --leak-check=full --error-exitcode=100 $PROG $TMP/titi $TMP/toto > /dev/null 2> $TMP/stderr
    test $? = 100 && echo "échec => log de valgrind dans $TMP/stderr" && return 1
    echo "OK"

    return 0
}

# répertoire temp où sont stockés tous les fichiers et sorties du pg
mkdir $TMP

# Lance les 4 séries de tests
for T in $(seq 1 4)
do
	if test_$T; then
		echo "== Test $T : ok $T/4\n"
	else
		echo "== Test $T : échec"
		return 1
	fi
done

rm -R $TMP
