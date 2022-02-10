#
# Ce Makefile contient les cibles suivantes :
#
# all   : compile le programme
# clean : supprime fichiers temporaires
CC = gcc

PROG = ordonnanceur
CHRONO = chrono

CFLAGS = -march=znver3 -pipe -g -Wall -Wextra -Wpedantic -Werror # obligatoires

.PHONY: all clean

all: $(PROG) $(CHRONO)

clean:
	rm -f $(PROG) $(CHRONO) *.o
	rm -f *.aux *.log *.out
