all: chatclient

chatclient: chatclient.o
	gcc -g -Wall -pedantic -std=gnu99 -o chatclient chatclient.o

chatclient.o: chatclient.c
	gcc -g -Wall -pedantic -std=gnu99 -c chatclient.c

clean:
	rm chatclient.o
	rm chatclient
