#
# Ce Makefile contient les cibles suivantes :
#
# all   : compile le programme
# clean : supprime fichiers temporaires
CC = gcc

PROG = reseau trame

CFLAGS = -g -march=znver3 -Wpedantic -Wall -Wextra -Werror # obligatoires

.PHONY: all clean

all: $(PROG)

clean:
	rm -f $(PROG) *.o
	rm -f *.aux *.log *.out
	rm -f STA_*
