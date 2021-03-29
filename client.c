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
#define USERFILE "users"
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

int stay_alive = 1;

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

    pthread_cond_broadcast(&cond);
    
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

    char *message = circular_command_fifo[tail];
    tail = (tail + 1) % MAX_COMMANDS_BUFFER;

    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&lock);
    //printf("Removed: %s\n", message);
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

    if(bytes_received < 1)
    {
        exit(0);
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

void get_command(char *buffer)
{
    fgets(buffer, MAX_COMMAND_SIZE, stdin);
    buffer[strlen(buffer) - 1] = 'C';
    __fpurge(stdin);  
}

void get_username(char *buffer)
{
    fgets(buffer, MAX_USERNAME, stdin);
    while(strlen(buffer) < 2)
    {
        printf("Username must be at least one character. Try again:\n");
        fgets(buffer, MAX_USERNAME, stdin);
    }
    buffer[strlen(buffer) - 1] = '\0';
    __fpurge(stdin);
}

void get_password(char *buffer)
{
    fgets(buffer, MAX_PASSWORD, stdin);
    while(strlen(buffer) < 2)
    {
        printf("Password must be at least one character. Try again:\n");
        fgets(buffer, MAX_PASSWORD, stdin);
    }
    buffer[strlen(buffer) - 1] = '\0';
    __fpurge(stdin);
}

void get_message(char *buffer)
{
    fgets(buffer, MAXBUFFERSIZE - 1, stdin);
    while(strlen(buffer) < 2)
    {
        printf("Message must be at least one character. Try again:\n");
        fgets(buffer, MAXBUFFERSIZE - 1, stdin);
    }
    buffer[strlen(buffer) - 1] = 'D';
    __fpurge(stdin);
}

void public_message(int socket_fd)
{
    send_to(socket_fd, PUBLIC_MESSAGE, strlen(PUBLIC_MESSAGE));
    char *message = fifopop();

    if(strcmp(message, NO_ACTIVE_USERS) == 0)
    {
        free(message);
        printf("No other users online.\n");
        return;
    }

    printf("Enter message:\n");

    get_message(message);

    send_to(socket_fd, message, strlen(message));
    free(message);
    message = fifopop();

    if(strcmp(message, MESSAGE_SENT) == 0)
    {
        printf("Message sent.\n");
    }
    else
    {
        printf("Send failed%s.\n", message);
    }
    free(message);
}

void direct_message(int socket_fd)
{
    send_to(socket_fd, DIRECT_MESSAGE, strlen(DIRECT_MESSAGE));
    // List of active users
    char *message = fifopop();

    if (strcmp(message, NO_ACTIVE_USERS) == 0)
    {
        free(message);
        printf("\nNo other users online.\n\n");
        return;
    }

    // Remove trailing C
    message[strlen(message) - 1] = '\0';

    printf("\nActive users:\n%s\nChoose user:\n", message);

    // Choose a user
    get_username(message);

    // Send chosen user
    send_to(socket_fd, message, strlen(message));
    free(message);

    // Message could be RTRC/UNFC/UOC
    message = fifopop();

    if(strcmp(message, READY_TO_RECEIVE) == 0)
    {
        printf("Enter message:\n");
        // Prompt user for message
        get_message(message);

        // Send direct message
        send_to(socket_fd, message, strlen(message));
        free(message);

        // Get confirmation
        message = fifopop();

        if(strcmp(message, MESSAGE_SENT) == 0)
        {
            free(message);
            printf("Message sent.\n");
            return;
        }
        free(message);
        printf("Message failed. User no longer online.\n");
        return;
    }

    if(strcmp(message, USER_NOT_FOUND) == 0)
    {
        free(message);
        printf("User does not exist.\n");
        return;
    }

    if(strcmp(message, USER_OFFLINE) == 0)
    {
        free(message);
        printf("User offline.\n");
        return;
    }
}

void *command_handler(void *p_client_fd)
{
    int socket_fd = * (int *) p_client_fd;
    char command[MAX_COMMAND_SIZE];

    printf("You are connected.\n");

    while(stay_alive)
    {
        printf("PM = PUBLIC MESSAGE | DM = DIRECT MESSAGE | EX = EXIT\nEnter command:\n");
        get_command(command);

        if(strcmp(command, PUBLIC_MESSAGE) == 0)
        {
            public_message(socket_fd);
        }
        else if (strcmp(command, DIRECT_MESSAGE) == 0)
        {
            direct_message(socket_fd);
        }
        else if (strcmp(command, EXIT) == 0)
        {
            stay_alive = 0;
            send_to(socket_fd, EXIT, strlen(EXIT));
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
    char command[MAX_COMMAND_SIZE];
    int socket_fd;
    printf("Creating socket...\n");
    socket_helper(&socket_fd, AF_INET, SOCK_STREAM, 0, argv[1], argv[2]);
    printf("Socket ready.\n");

    receive(socket_fd, command, MAX_COMMAND_SIZE - 1);

    if(strcmp(command, SERVER_FULL) == 0)
    {
        close(socket_fd);
        exit(1);
    }

    printf("Connected.\nEnter username:\n");

    get_username(username);

    send_to(socket_fd, username, strlen(username));
    receive(socket_fd, command, MAX_COMMAND_SIZE - 1);

    if(strcmp(command, USER_NOT_FOUND) == 0)
    {
        printf("User not found.\nCreating new user...\nEnter new user password:\n");
    }
    else
    {
        printf("User found.\nEnter password:\n");
    }
    
    get_password(password);
    
    send_to(socket_fd, password, strlen(password));
    receive(socket_fd, command, MAX_COMMAND_SIZE - 1);

    if(strcmp(command, INCORRECT_PASSWORD) == 0)
    {
        printf("Incorrect password. Connection terminated, exiting..\n");
        close(socket_fd);
        exit(1);
    }

    if(strcmp(command, USER_FOUND) == 0)
    {
        printf("User already exists, exiting..\n");
        close(socket_fd);
        exit(1);
    }

    if (strcmp(command, USER_ALREADY_ONLINE) == 0)
    {
        printf("User already online, exiting..\n");
        close(socket_fd);
        exit(1);
    }

    pthread_t thread;
    void *return_val;

    if (pthread_create(&thread, NULL, command_handler, &socket_fd) != 0)
    {
        perror("Thread create error.");
        exit(1);
    }

    int bytes_received = 0;
    while(stay_alive)
    {
        char *message = malloc(MAXBUFFERSIZE);
        bytes_received = recv(socket_fd, message, MAXBUFFERSIZE - 2, 0);
        if (bytes_received > 0)
        {
            if(message[bytes_received - 1] == 'D')
            {
                message[bytes_received - 1] = '\0';
                printf("\n--INCOMING MESSAGE--\n%s\n\nContinue input:\n", message);
                free(message);
            }
            else //if(message[bytes_received - 1] == 'C')
            {
                message[bytes_received] = '\0';
                fifoadd(message);
            }
            // else
            // {
            //     //message[bytes_received] = '\0';
            //     printf("Shouldn't be here..\n");
            //     free(message);
            // }
        }
        else
        {
            printf("Connection ended.\n");
            stay_alive = 0;
        }
    }

    if (pthread_join(thread, &return_val) != 0)
    {
        perror("Thread join error.");
        exit(3);
    }

    // for(int i = 0; i < MAX_COMMANDS_BUFFER; i++)
    // {
    //     if (circular_command_fifo[i] != NULL)
    //     {
    //         free(circular_command_fifo[i]);
    //     }
    // }

    return 0;
}