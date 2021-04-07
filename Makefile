# -*- Makefile -*-

all: chatServer chatClient

chatServer: chatServer.o helper.o
	gcc chatServer.o helper.o -o chatServer -l pthread

chatServer.o: chatServer.c helper.h
	gcc -c chatServer.c

chatClient: chatClient.o helper.o
	gcc chatClient.o helper.o -o chatClient -l pthread

chatClient.o: chatClient.c helper.h
	gcc -c chatClient.c

helper.o: helper.c helper.h
	gcc -c helper.c

clean:
	rm -f chatServer chatServer.o chatClient chatClient.o helper.o