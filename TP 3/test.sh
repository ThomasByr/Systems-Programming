#!/bin/sh

PROG=${PROG:=./reseau} # nom du programme par défaut
TMP="/tmp/$$"

! which timeout &&
  echo "install GNU coreutils:" &&
  echo "for OSX: brew install coreutils" &&
  echo "for Linux (debian-like): apt install coreutils" &&
  exit 1

##############################################################################
# Fonctions utilitaires

# teste si un fichier passé en arg est vide
check_empty() {
  test -s $1 && echo "échec => $1 non vide" && return 0
  return 1
}

# teste si un fichier passé en arg n'est pas vide
check_non_empty() {
  ! test -s $1 && echo "échec => $1 vide" && return 0
  return 1
}

# teste si le pg a échoué
# - code de retour du pg doit être égal à 1
# - stdout doit être vide
echec() {
  test $1 -ne 1 && echo "échec => code de retour != 1" && return 0
  check_empty $TMP/stdout && return 0

  return 1
}

# teste la chaine sur stderr
# - doit être de la forme "usage: ..."
syntax() {
  ! grep -q "usage: " $TMP/stderr 2>&1 &&
    echo "échec => sortie erreur ne contient pas la syntaxe du pg" &&
    return 0
}

# teste si le pg a réussi
# - code de retour du pg doit être égal à 0
# - stdout ne doit pas être vide
# - stderr doit être vide
success() {
  test $1 -ne 0 && echo "échec => code de retour != 0" && return 0

  check_non_empty $TMP/stdout && return 0
  check_empty $TMP/stderr && return 0

  return 1
}

##############################################################################
# début des tests

test_1() {
  echo "Test 1 - syntaxe du programme"

  ##########################################################################
  echo -n "Test 1.1 - sans argument............................"
  $PROG >$TMP/stdout 2>$TMP/stderr
  echec $? && return 1
  syntax && return 1
  echo "OK"

  ##########################################################################
  echo -n "Test 1.2 - avec trop d'arguments...................."
  $PROG 1 2 >$TMP/stdout 2>$TMP/stderr
  echec $? && return 1
  syntax && return 1
  echo "OK"

  # ########################################################################
  echo -n "Test 1.3 - nb_sta n'est pas un nombre..............."
  $PROG a >$TMP/stdout 2>$TMP/stderr
  echec $? && return 1
  check_non_empty $TMP/stderr && return 1
  echo "OK"

  # ########################################################################
  echo -n "Test 1.4 - nb_sta > MAXSTA.........................."
  $PROG 11 >$TMP/stdout 2>$TMP/stderr
  echec $? && return 1
  check_non_empty $TMP/stderr && return 1
  echo "OK"

  # ########################################################################
  echo -n "Test 1.5 - syntaxe valide..........................."
  rm -f STA_1 STA_2
  ./trame 1 2 aaaa
  ./trame 2 1 bbbb

  $PROG 2 >$TMP/stdout 2>$TMP/stderr
  success $? && return 1
  echo "OK"

  rm -f STA_1 STA_2
  return 0
}

test_2() {
  echo "Test 2 - transmission unicast avec 2 stations"

  ##########################################################################
  echo -n "Test 2.1 - 2 stations, 1 trame par station.........."
  rm -f STA_1 STA_2
  ./trame 1 2 aaaa
  ./trame 2 1 bbbb
  cat >$TMP/sortie <<EOF
1 - 2 - 1 - bbbb
2 - 1 - 2 - aaaa
EOF
  timeout 2 $PROG 2 >$TMP/stdout 2>$TMP/stderr
  RES="$?"
  echo $RES
  test $RES -eq 124 && echo "échec : attente infinie" && return 1
  success $RES && return 1

  sort $TMP/stdout >$TMP/stdout2
  ! cmp $TMP/stdout2 $TMP/sortie >/dev/null 2>&1 &&
    echo "échec : stdout non conforme" &&
    return 1
  echo "OK"

  ##########################################################################
  echo -n "Test 2.2 - 2 stations, 2 trames par station........."
  rm -f STA_1 STA_2
  ./trame 1 2 aaaa
  ./trame 2 1 bbbb
  ./trame 1 2 cccc
  ./trame 2 1 dddd
  cat >$TMP/sortie <<EOF
1 - 2 - 1 - bbbb
1 - 2 - 1 - dddd
2 - 1 - 2 - aaaa
2 - 1 - 2 - cccc
EOF
  timeout 2 $PROG 2 >$TMP/stdout 2>$TMP/stderr
  RES="$?"
  test $RES -eq 124 && echo "échec : attente infinie" && return 1
  success $RES && return 1

  sort $TMP/stdout >$TMP/stdout2
  ! cmp $TMP/stdout2 $TMP/sortie >/dev/null 2>&1 &&
    echo "échec : stdout non conforme" &&
    return 1
  echo "OK"

  rm -f STA_*
  return 0
}

test_3() {
  echo "Test 3 - unicast / broadcast avec plusieurs stations"

  ##########################################################################
  echo -n "Test 3.1 - transmission unicast avec 3 stations....."
  rm -f STA_1 STA_2 STA_3
  ./trame 1 2 aaaa
  ./trame 1 3 bbbb
  ./trame 2 1 cccc
  ./trame 2 3 dddd
  ./trame 3 1 eeee
  ./trame 3 2 ffff
  cat >$TMP/sortie <<EOF
1 - 2 - 1 - cccc
1 - 3 - 1 - eeee
2 - 1 - 2 - aaaa
2 - 3 - 2 - ffff
3 - 1 - 3 - bbbb
3 - 2 - 3 - dddd
EOF
  timeout 2 $PROG 3 >$TMP/stdout 2>$TMP/stderr
  RES="$?"
  test $RES -eq 124 && echo "échec : attente infinie" && return 1
  success $RES && return 1

  sort $TMP/stdout >$TMP/stdout2
  ! cmp $TMP/stdout2 $TMP/sortie >/dev/null 2>&1 &&
    echo "échec : stdout non conforme" &&
    return 1
  echo "OK"

  ##########################################################################
  echo -n "Test 3.2 - transmission unicast/broadcast..........."
  rm -f STA_1 STA_2 STA_3 STA_4
  ./trame 1 2 aaaa
  ./trame 2 3 bbbb
  ./trame 3 4 cccc
  ./trame 4 0 dddd
  ./trame 4 5 eeee
  cat >$TMP/sortie <<EOF
1 - 4 - 0 - dddd
1 - 4 - 5 - eeee
2 - 1 - 2 - aaaa
2 - 4 - 0 - dddd
2 - 4 - 5 - eeee
3 - 2 - 3 - bbbb
3 - 4 - 0 - dddd
3 - 4 - 5 - eeee
4 - 3 - 4 - cccc
EOF
  timeout 2 $PROG 4 >$TMP/stdout 2>$TMP/stderr
  RES="$?"
  test $RES -eq 124 && echo "échec : attente infinie" && return 1
  success $RES && return 1

  sort $TMP/stdout >$TMP/stdout2
  ! cmp $TMP/stdout2 $TMP/sortie >/dev/null 2>&1 &&
    echo "échec : stdout non conforme" &&
    return 1
  echo "OK"

  rm -f STA_*
  return 0
}

test_4() {
  echo -n "Test 4 - test mémoire..............................."

  [ $(uname) = "Darwin" ] && echo "valgrind not available on OSX, skip this test" && return 1

  rm -f STA_1 STA_2 STA_3 STA_4
  ./trame 1 2 aaaa
  ./trame 2 3 bbbb
  ./trame 3 4 cccc
  ./trame 4 0 dddd
  ./trame 4 5 eeee

  valgrind --leak-check=full --trace-children=yes --error-exitcode=100 $PROG 4 >/dev/null 2>$TMP/stderr
  test $? = 100 && echo "échec => log de valgrind dans $TMP/stderr" && return 1
  echo "OK"

  rm -f STA_*
  return 0
}

run_all() {
  # Lance la série des tests
  for T in $(seq 1 4); do
    if test_$T; then
      echo "== Test $T : ok $T/4\n"
    else
      echo "== Test $T : échec"
      return 1
    fi
  done

  rm -R $TMP
  rm -f STA_*
}

# répertoire temp où sont stockés tous les fichiers et sorties du pg
mkdir $TMP

if [ $# -eq 1 ]; then
  case $1 in 1) test_1 ;;
  2) test_2 ;;
  3) test_3 ;;
  4) test_4 ;;
  *)
    echo "test inexistant"
    return 1
    ;;
  esac
else
  run_all
fi
