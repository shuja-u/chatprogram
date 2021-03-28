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

#define MAXBUFFERSIZE 2050
#define BACKLOG 20
#define MAX_ONLINE_USERS 10
#define SET_SIZE 100
#define MAX_USERNAME 51
#define MAX_PASSWORD 101
#define USERFILE "users"
#define MAX_ACTIVE_USERS 20
#define SERVER_FULL "SFC"
#define USER_FOUND "UFC"
#define USER_NOT_FOUND "UNFC"
#define INCORRECT_PASSWORD "IPWC"
#define PUBLIC_MESSAGE "PMC"
#define DIRECT_MESSAGE "DMC"
#define EXIT "EXC"
#define READY_TO_RECEIVE "RTRC"
#define MESSAGE_SENT "MSC"
#define MAX_COMMAND_SIZE 11
#define MAX_COMMANDS_BUFFER 50

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

char *circular_command_fifo[MAX_COMMANDS_BUFFER];
int head = 0;
int tail = 0;
int full = 0;

void fifoadd(char *message)
{
    pthread_mutex_lock(&lock);

    while(full)
    {
        pthread_cond_wait(&cond, &lock);
    }

    circular_command_fifo[head] = message;
    head = (head + 1) % MAX_COMMANDS_BUFFER;

    if(head == tail)
    {
        full = 1;
    }

    pthread_mutex_unlock(&lock);
    return;
}

char *fifopop()
{
    pthread_mutex_lock(&lock);

    while(!full && head == tail)
    {
        pthread_cond_wait(&cond, &lock);
    }

    char *message;
    message = circular_command_fifo[tail];
    tail = (tail + 1) % MAX_COMMANDS_BUFFER;

    pthread_mutex_unlock(&lock);
    return message;
}

/*
 * Function: create_bind_socket
 * ----------------------------
 * Creates and binds a socket. It can also be used to just create a socket 
 * without binding it.
 * 
 * *socket_fd: the socket file descriptor will be stored here once the 
 *             function creates it.
 * family: socket family (eg. IPv4/IPv6).
 * socket_type: socket type (eg. TCP/UDP).
 * flag: can include extra information for getaddrinfo
 * address[]: socket address.
 * port[]: port number.
 * 
 * returns: a sockaddr struct that contains socket address information. 
 */

struct sockaddr socket_helper(int *socket_fd, int family, int socket_type,
                                   int flag, char address[], char port[]);

void receive(int socket_fd, char message[], int message_len);

void send_to(int socket_fd, char message[], int message_len);

struct sockaddr socket_helper(int *socket_fd, int family, int socket_type,
                                   int flag, char address[], char port[])
{
    struct addrinfo init_info, *addrinfo_list, *next;
    int return_val;
    struct sockaddr result;
    int yes = 1;

    memset(&init_info, 0, sizeof init_info);
    init_info.ai_family = family;
    init_info.ai_socktype = socket_type;
    if (flag)
    {
        init_info.ai_flags = flag;
    }

    return_val = getaddrinfo(address, port, &init_info, &addrinfo_list);

    if (return_val != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(return_val));
        exit(1);
    }

    for (next = addrinfo_list; next != NULL; next = next->ai_next)
    {
        *socket_fd = socket(next->ai_family, next->ai_socktype,
                            next->ai_protocol);

        if (*socket_fd == -1)
        {
            perror("Couldn't create socket");
            continue;
        }

        if (flag && setsockopt(*socket_fd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &yes, sizeof(yes)) == -1)
        {
            perror("Address is in use");
            exit(1);
        }

        if (flag && bind(*socket_fd, next->ai_addr, next->ai_addrlen) == -1)
        {
            close(*socket_fd);
            perror("Couldn't bind");
            continue;
        }

        if (!flag && connect(*socket_fd, next->ai_addr, next->ai_addrlen) == -1)
        {
            close(*socket_fd);
            perror("Couldn't connect");
            continue;
        }
        break;
    }

    if (next == NULL)
    {
        fprintf(stderr, "Socket failed.\n");
        exit(1);
    }

    result = *next->ai_addr;
    freeaddrinfo(addrinfo_list);
    return result;
}

/*
 * Function: receive_from
 * ----------------------
 * Uses the recvfrom function and marks the end of the received message. 
 * Additionally, if the process was unsuccessful, it will print the 
 * appropriate message and exit the program.
 * 
 * socket_fd: socket file descriptor.
 * message[]: where the received message will be stored.
 * message_len: how much message[] can hold.
 * *address: this is where the source address will be stored.
 * *addr_len: size of the source address will be stored here.
 */

void receive(int socket_fd, char message[], int message_len)
{
    int bytes_received;
    if ((bytes_received = recv(socket_fd, message, message_len, 0)) == -1)
    {
        perror("Unable to receive");
        exit(1);
    }

    message[bytes_received] = '\0';
}

/*
 * Function: send_to
 * -----------------
 * Uses the sendto function and prints an appropriate message if sending was 
 * unsuccessful.
 * 
 * socket_fd: socket file descriptor.
 * message[]: the message to send.
 * message_len: length of message[].
 * *address: destination address.
 * addr_len: size of destination address.
 */

void send_to(int socket_fd, char message[], int message_len)
{
    int bytes_sent;
    if ((bytes_sent = send(socket_fd, message, message_len, 0)) == -1)
    {
        perror("Unable to send");
        exit(1);
    }
}

void *command_handler(void *client_fd)
{
    int socket_fd = * (int *) client_fd;
    free(client_fd);
    char *message;

    while(1)
    {
        printf("PM = PUBLIC MESSAGE | DM = DIRECT MESSAGE | EX = EXIT\nEnter command:\n");
        fgets(message, MAXBUFFERSIZE - 1, stdin);
        message[strlen(message) - 1] = 'C';

        if(strcmp(message, PUBLIC_MESSAGE) == 0)
        {
            send_to(socket_fd, message, strlen(PUBLIC_MESSAGE));
            message = fifopop();
            if(strcmp(message, READY_TO_RECEIVE) != 0)
            {
                printf("Expected message: ready to receive | Received: %s\n", message);
                exit(1);
            }

            printf("Enter message:\n");

            fgets(message, MAXBUFFERSIZE - 1, stdin);
            message[strlen(message) - 1] = 'D';
            
            send_to(socket_fd, message, strlen(message));
            message = fifopop();

            if(strcmp(message, MESSAGE_SENT) == 0)
            {
                printf("Message sent.\n");
            }
            else
            {
                printf("Send failed.\n");
            }
        }
        else if (strcmp(message, DIRECT_MESSAGE) == 0)
        {
            // Send DIRECT MESSAGE command
            send_to(socket_fd, message, strlen(DIRECT_MESSAGE));
            // Receive list of active users
            message = fifopop();

            printf("%sChoose user:\n", message);

            // Choose a user
            fgets(message, MAX_USERNAME, stdin);
            message[strlen(message) - 1] = '\0';

            // Send chosen user
            send_to(socket_fd, message, strlen(message));
            // Message could be READY TO RECEIVE or USER NOT FOUND
            message = fifopop();

            if(strcmp(message, READY_TO_RECEIVE) == 0)
            {
                // Get message from user
                fgets(message, MAXBUFFERSIZE - 1, stdin);
                message[strlen(message) - 1] = 'D';

                // Send direct message
                send_to(socket_fd, message, strlen(message));
                // Get confirmation
                message = fifopop();

                if(strcmp(message, MESSAGE_SENT))
                {
                    printf("Message sent.\n");
                    continue;
                }
                printf("User no longer online.\n");
                continue;
            }
            printf("Invalid user.\n");
        }
        else if (strcmp(message, EXIT) == 0)
        {
            send_to(socket_fd, message, strlen(EXIT));
            printf("Terminating connection...\n");
            close(socket_fd);
            return NULL;
        }
        else
        {
            printf("--INVALID COMMAND--\n");
        }

    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Arguments: IP Port\n");
        exit(1);
    }

    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char *message;
    int socket_fd;
    socket_helper(&socket_fd, AF_INET, SOCK_STREAM, 0, argv[1], argv[2]);

    /*
     * Check if the passed argument is a file, if it is change the value of
     * message to the file's contents and get its length. Otherwise just get
     * the passed argument's length.
     */

    receive(socket_fd, message, MAXBUFFERSIZE - 2);

    if(strcmp(message, SERVER_FULL) == 0)
    {
        close(socket_fd);
        exit(1);
    }

    fgets(username, MAX_USERNAME, stdin);
    username[strlen(username) - 1] = '\0'; 

    send_to(socket_fd, username, strlen(username));
    receive(socket_fd, message, MAX_COMMAND_SIZE - 1);

    if(strcmp(message, USER_NOT_FOUND) == 0)
    {
        printf("User not found.\nCreating new user...\nEnter new user password:\n");
    }
    else
    {
        printf("User found.\nEnter password:\n");
    }
    
    fgets(password, MAX_PASSWORD, stdin);
    password[strlen(password) - 1] = '\0';
    
    send_to(socket_fd, password, strlen(password));
    receive(socket_fd, message, MAX_COMMAND_SIZE - 1);

    if(strcmp(message, INCORRECT_PASSWORD) == 0)
    {
        printf("Incorrect password. Connection terminated.\n");
        close(socket_fd);
        exit(1);
    }

    pthread_t thread;
    int *client_fd = malloc(sizeof(int));
    *client_fd = socket_fd;

    pthread_create(&thread, NULL, command_handler, client_fd);

    int bytes_received;
    while(1)
    {
        message = malloc(MAXBUFFERSIZE);
        bytes_received = recv(socket_fd, message, MAXBUFFERSIZE - 2, 0);
        if (bytes_received > -1)
        {
            if(message[bytes_received - 1] == 'D')
            {
                message[bytes_received - 1] = '\0';
                printf("\n--INCOMING MESSAGE--\n%s\n", message);                
            }
            else
            {
                message[bytes_received] = '\0';
                fifoadd(message);
            }
        }
    }

    close(socket_fd);

    return 0;
}