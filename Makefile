CC = gcc
CC_OPTS = -Wall -Wextra -O2 -std=c99

all: bsearch strsearch

clean:
	rm -f bsearch bsearch.o
	rm -f strsearch strsearch.o

bsearch: bsearch.o
	$(CC) -o $@ $<

strsearch: strsearch.o
	$(CC) -o $@ $<

%.o: %.c
	$(CC) -o $@ -c $< $(CC_OPTS)
