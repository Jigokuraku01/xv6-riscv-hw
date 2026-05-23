CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c11 -D_FILE_OFFSET_BITS=64
LDFLAGS =

BINS = getinodeinfo getinodedata parsedirent

all: $(BINS)

getinodeinfo: getinodeinfo.o ext2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

getinodedata: getinodedata.o ext2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

parsedirent: parsedirent.o ext2.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c ext2.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: all
	./test.sh

valgrind: all
	./test.sh valgrind

clean:
	rm -f *.o $(BINS)

distclean: clean
	rm -rf ext2.img mnt testdata extracted *.sha512

.PHONY: all test valgrind clean distclean
