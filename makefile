#
# @brief Makefile for websh
# @author Paul Florian Proell (1525669)
# 

CC			=	gcc
CFLAGS	=	-std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g

all: websh.o
	$(CC) $(CFLAGS) -o websh websh.o

websh.o: websh.c
	$(CC) $(CFLAGS) -c websh.c

clean:
	rm -f websh
	rm -f -R *.o
