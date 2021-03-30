# -*- Makefile -*-

all: chatServer chatClient

chatServer: chatServer.o helper.o
	gcc chatServer.o helper.o -o chatServer -l pthread

chatServer.o: server/chatServer.c server/helper.h
	gcc -c server/chatServer.c

chatClient: chatClient.o helper.o
	gcc chatClient.o helper.o -o chatClient -l pthread

chatClient.o: client/chatClient.c server/helper.h
	gcc -c client/chatClient.c -iquote server

helper.o: server/helper.c server/helper.h
	gcc -c server/helper.c

clean:
	rm -f chatServer chatServer.o chatClient chatClient.o helper.o