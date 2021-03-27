/*
 * Shuja Uddin
 * sm2849sr
 * 
 * This file includes the headers and definitions required for the server and 
 * the client.
 * 
 * It also includes function definitions for some helper functions located in 
 * server_client_helper.
 */

#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#define TIMESTAMPLENGTH 26
#define MAXBUFFERSIZE 2049
#define BACKLOG 20

struct sockaddr socket_helper(int *socket_fd, int family, int socket_type,
                                   int flag, char address[], char port[]);

void receive_from(int socket_fd, char message[], int message_len, 
                  struct sockaddr *address, socklen_t *addr_len);

void send_to(int socket_fd, char message[], int message_len, 
             struct sockaddr *address, socklen_t addr_len);

void print_checksum(unsigned char checksum[]);

void sigchld_handler(int s);