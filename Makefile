CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Wextra 

.PHONY: all
all: nyush

nyush: nyush.c


.PHONY: clean
clean:
	rm -f nyush
