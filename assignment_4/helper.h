/*
 * Shuja Uddin
 * sm2849sr
 * 
 * This file includes the headers and definitions required for the server and the client.
 */

#include <sys/socket.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define MAXBUFFERSIZE 2050
#define BACKLOG 20
#define MAX_ONLINE_USERS 20
#define TABLE_SIZE 100
#define MAX_USERNAME 51
#define MAX_PASSWORD 101
#define USERFILE "saved_users"
#define MAX_ACTIVE_USERS 20 // No more than 40, or active username list will be too large to send.
#define SERVER_FULL "SFC"
#define USER_FOUND "UFC"
#define USER_NOT_FOUND "UNFC"
#define USER_ALREADY_ONLINE "UAOC"
#define USER_OFFLINE "UOC"
#define INCORRECT_PASSWORD "IPWC"
#define PUBLIC_MESSAGE "PMC"
#define DIRECT_MESSAGE "DMC"
#define EXIT "EXC"
#define READY_TO_RECEIVE "RTRC"
#define MESSAGE_SENT "MSC"
#define NO_ACTIVE_USERS "NAUC"
#define MAX_COMMAND_SIZE 5
#define MAX_COMMANDS_BUFFER 50

struct sockaddr socket_helper(int *socket_fd, int family, int socket_type, int flag, char address[], char port[]);