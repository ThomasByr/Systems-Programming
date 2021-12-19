
#
# Ce Makefile contient les cibles suivantes :
#
# all   : compile le programme
# test  : lance les tests

CC = gcc-11
EXEC_FILE = compare
OBJECTS = compare.o

CFLAGS = -Ofast -march=znver3 -c -g -Wall -Wextra -Werror # obligatoires

.PHONY: all clean

all: $(EXEC_FILE)

$(OBJECTS): %.o: %.c
	$(CC) $< $(CFLAGS)

$(EXEC_FILE): $(OBJECTS)
	$(CC) $^ -o $@

test: $(EXEC_FILE)
	./test.sh

clean:
	rm -f $(EXEC_FILE) *.o
	rm -f *.aux *.log *.out
	rm -f moodle.tgz

