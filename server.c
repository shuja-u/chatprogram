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

#define MAXBUFFERSIZE 2049
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


typedef struct userinfo
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} userinfo;

typedef struct userlinkedlist
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    struct userlinkedlist *next;
} userlinkedlist;

typedef struct activeuser
{
    char username[MAX_USERNAME];
    int user_fd;
} activeuser;

FILE *saved_users;
userlinkedlist * users[SET_SIZE]; // hash set of users
activeuser * active_users[MAX_ACTIVE_USERS];
int active_user_count = 0;

unsigned int hash(char *username)
{
    int length = strnlen(username, 50);

    unsigned int hash = 0;

    for (int i = 0; i < length; i++)
    {
        hash += username[i];
        hash = (hash * username[i]) % 100;
    }
    return hash;
}

void init_hash_set(userlinkedlist **users)
{
    for (int i = 0; i < 100; i++)
    {
        users[i] = NULL;
    }
}

int add(userlinkedlist **users, userlinkedlist *userll)
{
    if (userll == NULL)
    {
        printf("User is NULL\n");
        return 0;
    }

    int index = hash(userll->username);

    userll->next = users[index];
    users[index] = userll;
    return 1;
}

userlinkedlist * contains(userlinkedlist **users, char *username)
{
    if (username == NULL)
    {
        printf("Username is NULL");
        return NULL;
    }

    int index = hash(username);

    userlinkedlist *current_node = users[index];

    while (current_node != NULL && strncmp(username, current_node->username, MAX_USERNAME) != 0)
    {
        current_node = current_node->next;
    }

    return current_node;
}

int delete(userlinkedlist **users, userlinkedlist *userll)
{
    if (userll == NULL)
    {
        printf("User is NULL\n");
        return -1;
    }

    int index = hash(userll->username);

    if(users[index] == NULL)
    {
        printf("User doesn't exist.\n");
        return 0;
    }

    userlinkedlist * current_node = users[index];
    userlinkedlist * previous_node = NULL;

    while (current_node != NULL && strncmp(current_node->username, userll->username, 50) != 0)
    {
        previous_node = current_node;
        current_node = current_node->next;        
    }

    if (previous_node == NULL)
    {
        users[index] = NULL;
    }
    else
    {
        previous_node->next = current_node->next;
    }
    return 1;
}

struct sockaddr socket_helper(int *socket_fd, int family, int socket_type,
                                   int flag, char address[], char port[]);

void receive(int socket_fd, char message[], int message_len);

void send_to(int socket_fd, char message[], int message_len);

void print_checksum(unsigned char checksum[]);

void sigchld_handler(int s);

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

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void conn_handler(int client_fd)
{
    char username[MAX_USERNAME];
    char message[MAXBUFFERSIZE];

    if (active_user_count >= MAX_ACTIVE_USERS)
    {
        strcpy(message, SERVER_FULL);
        send_to(client_fd, message, strlen(message));
        close(client_fd);
        return;
    }

    receive(client_fd, username, MAX_USERNAME - 1);

    userlinkedlist *userclient = contains(users, username);
    userinfo usersave;
    activeuser *active_user = malloc(sizeof(activeuser));
    int index;

    if (userclient == NULL)
    {
        send_to(client_fd, USER_NOT_FOUND, strlen(USER_NOT_FOUND));
        receive(client_fd, message, MAX_PASSWORD - 1);

        strncpy(userclient->username, username, MAX_USERNAME);
        strncpy(userclient->password, message, MAX_PASSWORD);
        strncpy(usersave.username, username, MAX_USERNAME);
        strncpy(usersave.password, message, MAX_PASSWORD);
        userclient->next = NULL;
        strncpy(active_user, username, MAX_USERNAME);
        active_user->user_fd = client_fd;

        add(users, userclient);
        for (index = 0; index < MAX_ACTIVE_USERS; index++)
        {
            if (active_users[index] == NULL)
            {
                active_users[index] = active_user;
                active_user_count++;
            }
        }
        fwrite(&usersave, sizeof(userinfo), 1, saved_users);
        fflush(saved_users);
    }
    else
    {
        send_to(client_fd, USER_FOUND, strlen(USER_FOUND));
        receive(client_fd, message, MAX_PASSWORD - 1);

        if (strcmp(userclient->password, message) != 0)
        {
            send_to(client_fd, INCORRECT_PASSWORD, strlen(INCORRECT_PASSWORD));
            close(client_fd);
            return;
        }

        strncpy(active_user, username, MAX_USERNAME);
        active_user->user_fd = client_fd;

        for (index = 0; index < MAX_ACTIVE_USERS; index++)
        {
            if (active_users[index] == NULL)
            {
                active_users[index] = active_user;
                active_user_count++;
            }
        }
    }

    int keep_alive = 1;

    while (keep_alive)
    {
        receive(client_fd, message, MAX_COMMAND_SIZE - 1);
        if (strcmp(PUBLIC_MESSAGE, message) == 0)
        {
            send_to(client_fd, READY_TO_RECEIVE, strlen(READY_TO_RECEIVE));
            receive(client_fd, message, MAXBUFFERSIZE - 1);
            for(index = 0; index < MAX_ACTIVE_USERS; index++)
            {
                if (active_users[index] != NULL)
                {
                    send_to(active_users[index]->user_fd, message, strlen(message));
                }
            }
            send_to(client_fd, MESSAGE_SENT, strlen(MESSAGE_SENT));
        }
        else if (strcmp(DIRECT_MESSAGE, message) == 0)
        {
            char *active_usernames = malloc((MAX_ACTIVE_USERS - 1) * (MAX_USERNAME + 1));
            active_usernames[0] = '\0';
            for(index = 0; index < MAX_ACTIVE_USERS; index++)
            {
                if (active_users[index] != NULL && active_users[index]->user_fd != client_fd)
                {
                    strcat(active_usernames, active_users[index]->username);
                    strcat(active_usernames, "\n");
                }
            }
            send_to(client_fd, active_usernames, strlen(active_usernames));
            free(active_usernames);
            receive(client_fd, message, MAX_USERNAME - 1);

            int sent = 0;                
            for(index = 0; index < MAX_ACTIVE_USERS; index++)
            {
                if (active_users[index] != NULL && strcmp(message, active_users[index]->username) == 0)
                {
                    send_to(client_fd, READY_TO_RECEIVE, strlen(READY_TO_RECEIVE));
                    receive(client_fd, message, MAXBUFFERSIZE - 1);
                    int bytes_sent = send(active_users[index]->user_fd, message, strlen(message), 0);
                    if (bytes_sent != -1)
                    {
                        sent = 1;
                    }
                    break;
                }
            }
            if (sent)
            {
                send_to(client_fd, MESSAGE_SENT, strlen(MESSAGE_SENT));
            }
            else
            {
                send_to(client_fd, USER_NOT_FOUND, strlen(USER_NOT_FOUND));
            }
        }
        else if (strcmp(EXIT, message) == 0)
        {
            for(index = 0; index < MAX_ACTIVE_USERS; index++)
            {
                if (active_users[index] != NULL && active_users[index]->user_fd == client_fd)
                {
                    active_users[index] = NULL;
                    active_user_count--;
                    break;
                }
            }
            close(client_fd);
            return;
        }
    }
}

int main (int argc, char *argv[]) {
    if (argc != 2)
    {
        printf("Arguments: Port\n");
        exit(1);
    }

    int server_fd, client_fd;
    struct sockaddr_storage client_address;
    socklen_t addr_len = sizeof client_address;
    pthread_t thread;
    struct sigaction sa;    
    init_hash_set(users);

    socket_helper(&server_fd, AF_INET, SOCK_STREAM, AI_PASSIVE, NULL, argv[1]);
    printf("socket created and bound.\n");

    printf("Reading existing users from file.\n");

    saved_users = fopen(USERFILE, "ab+");

    if (saved_users == NULL)
    {
        perror("File error.");
        exit(1);
    }

    userinfo user;

    while(fread(&user, sizeof(userinfo), 1, saved_users))
    {
        userlinkedlist *userll = malloc(sizeof(userlinkedlist));
        strncpy(userll->username, &user.username, MAX_USERNAME);
        strncpy(userll->password, &user.password, MAX_PASSWORD);
        userll->next = NULL;

        add(users, userll);
    }

    printf("Done reading existing users.\n");

    if (listen(server_fd, BACKLOG) == -1)
    {
        perror("Listen failed.");
        exit(1);
    }

    printf("listening\n");

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr*)&client_address, &addr_len);

        if(client_fd == -1) {
            perror("Couldn't accept connection.");
            continue;
        }

        printf("Connection accepted\n");
        conn_handler(client_fd);
    }
    return 0;
}