# simpliest makefile

all: udpclient udpserver

udpclient: udpclient.c
	$(CC) -o $@ $^ -lpthread -lncurses

udpserver: udpserver.c udpservermain.c
	$(CC) -o $@ $^ -lpthread -lncurses

clean:
	rm -f udpserver udpclient


.PHONY: clean all
