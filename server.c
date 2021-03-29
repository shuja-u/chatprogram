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

struct sockaddr socket_helper(int *socket_fd, int family, int socket_type,
                                   int flag, char address[], char port[]);

void receive(int socket_fd, char message[], int message_len);

void send_to(int socket_fd, char message[], int message_len);

//Mutex just for the hash table
static pthread_mutex_t hash_table_lock = PTHREAD_MUTEX_INITIALIZER;

//Read-write lock for active users array, allowing multiple readers but only one writer
static pthread_rwlock_t activeusers_lock = PTHREAD_RWLOCK_INITIALIZER;

//Return value for a terminated thread
int thread_exit_return_val;

//Users will be saved to file in this format
typedef struct userinfo
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} userinfo;

//Users will be stored in hash table in this format
typedef struct userlinkedlist
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int user_fd; // file descriptor for active users
    int active_index;
    struct userlinkedlist *next;
} userlinkedlist;

FILE *saved_users;
userlinkedlist *users_hash_table[TABLE_SIZE]; // hash set of users
userlinkedlist *active_users[MAX_ACTIVE_USERS]; // array of active users
int active_user_count = 0;

unsigned int hash(char *username)
{
    int length = strnlen(username, MAX_USERNAME);

    unsigned int hash = 0;

    for (int i = 0; i < length; i++)
    {
        hash += username[i];
        hash = (hash * username[i]) % 100;
    }
    return hash;
}

void init_tables()
{
    for (int i = 0; i < TABLE_SIZE; i++)
    {
        users_hash_table[i] = NULL;
    }

    for (int j = 0; j < MAX_ACTIVE_USERS; j++)
    {
        active_users[j] = NULL;
    }
}

void add(userlinkedlist *userll)
{
    pthread_mutex_lock(&hash_table_lock);
    if (userll == NULL)
    {
        printf("User pointer is NULL\n");
        return;
    }

    int index = hash(userll->username);

    userll->next = users_hash_table[index];
    users_hash_table[index] = userll;
    pthread_mutex_unlock(&hash_table_lock);
    return;
}

userlinkedlist *contains(char *username)
{
    pthread_mutex_lock(&hash_table_lock);
    if (username == NULL)
    {
        printf("Username pointer is NULL");
        return NULL;
    }

    int index = hash(username);

    userlinkedlist *current_node = users_hash_table[index];

    while (current_node != NULL && strcmp(username, current_node->username) != 0)
    {
        current_node = current_node->next;
    }
    pthread_mutex_unlock(&hash_table_lock);
    return current_node;
}

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
        pthread_exit(&thread_exit_return_val);
    }

    if(bytes_received < 1)
    {
        pthread_exit(&thread_exit_return_val);
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
        pthread_exit(&thread_exit_return_val);
    }
}

void add_active_user(userlinkedlist *user)
{
    pthread_rwlock_wrlock(&activeusers_lock);
    for(int i = 0; i < MAX_ACTIVE_USERS; i++)
    {
        if (active_users[i] == NULL)
        {
            user->active_index = i;
            active_users[i] = user;
            active_user_count++;
            break;
        }
    }
    pthread_rwlock_unlock(&activeusers_lock);
}

void remove_active_user(userlinkedlist *user)
{
    pthread_rwlock_wrlock(&activeusers_lock);
    active_users[user->active_index] = NULL;
    user->active_index = -1;
    user->user_fd = -1;
    active_user_count--;
    pthread_rwlock_unlock(&activeusers_lock);
}

void send_to_all(char *message, userlinkedlist *sending_user)
{
    pthread_rwlock_rdlock(&activeusers_lock);
    for(int i = 0; i < MAX_ACTIVE_USERS; i++)
    {
        if (active_users[i] != NULL && sending_user->active_index != i)
        {
            send_to(active_users[i]->user_fd, message, strlen(message));
        }
    }
    pthread_rwlock_unlock(&activeusers_lock);
}

char *get_all_active_users(int client_fd)
{
    char *active_usernames = malloc((MAX_ACTIVE_USERS - 1) * (MAX_USERNAME));
    active_usernames[0] = '\0';

    pthread_rwlock_rdlock(&activeusers_lock);
    for(int i = 0; i < MAX_ACTIVE_USERS; i++)
    {
        if (active_users[i] && active_users[i]->user_fd != client_fd)
        {
            strcat(active_usernames, active_users[i]->username);
            strcat(active_usernames, "\n");
        }
    }
    pthread_rwlock_unlock(&activeusers_lock);
    strcat(active_usernames, "C");
    return active_usernames;
}

userlinkedlist *user_setup(int client_fd)
{
    char username[MAX_USERNAME];
    send_to(client_fd, READY_TO_RECEIVE, strlen(READY_TO_RECEIVE));
    receive(client_fd, username, MAX_USERNAME - 1);

    userlinkedlist *userclient = contains(username);
    userinfo usersave;
    char password[MAX_PASSWORD];

    if (userclient == NULL)
    {
        userclient = malloc(sizeof(userlinkedlist));
        send_to(client_fd, USER_NOT_FOUND, strlen(USER_NOT_FOUND));
        receive(client_fd, password, MAX_PASSWORD - 1);

        if(contains(username) != NULL)
        {
            free(userclient);
            send_to(client_fd, USER_FOUND, strlen(USER_FOUND));
            close(client_fd);
            pthread_exit(&thread_exit_return_val);
        }
        //Prepare userlinkedlist structure to insert into hash table
        strncpy(userclient->username, username, MAX_USERNAME);
        strncpy(userclient->password, password, MAX_PASSWORD);
        userclient->user_fd = client_fd;
        userclient->next = NULL;

        //Prepare usersave structure to save to file
        strncpy(usersave.username, username, MAX_USERNAME);
        strncpy(usersave.password, password, MAX_PASSWORD);

        add(userclient);
        add_active_user(userclient);
        fwrite(&usersave, sizeof(userinfo), 1, saved_users);
        fflush(saved_users);

        printf("%s logged in\n", username);
        send_to(client_fd, READY_TO_RECEIVE, strlen(READY_TO_RECEIVE));
        return userclient;        
    }

    send_to(client_fd, USER_FOUND, strlen(USER_FOUND));
    receive(client_fd, password, MAX_PASSWORD - 1);    

    if (strcmp(userclient->password, password) != 0)
    {
        send_to(client_fd, INCORRECT_PASSWORD, strlen(INCORRECT_PASSWORD));
        close(client_fd);
        pthread_exit(&thread_exit_return_val);
    }

    if(userclient->user_fd != -1)
    {
        send_to(client_fd, USER_ALREADY_ONLINE, strlen(USER_ALREADY_ONLINE));
        close(client_fd);
        pthread_exit(&thread_exit_return_val);
    }

    userclient->user_fd = client_fd;
    add_active_user(userclient);

    printf("%s logged in\n", username);
    
    send_to(client_fd, READY_TO_RECEIVE, strlen(READY_TO_RECEIVE));
    return userclient;
}

void public_message(int client_fd, userlinkedlist *sending_user)
{
    pthread_rwlock_rdlock(&activeusers_lock);
    if(active_user_count < 2)
    {
        send_to(client_fd, NO_ACTIVE_USERS, strlen(NO_ACTIVE_USERS));
        pthread_rwlock_unlock(&activeusers_lock);
        return;
    }
    pthread_rwlock_unlock(&activeusers_lock);
    char message[MAXBUFFERSIZE];
    send_to(client_fd, READY_TO_RECEIVE, strlen(READY_TO_RECEIVE));
    receive(client_fd, message, MAXBUFFERSIZE - 2);
    send_to_all(message, sending_user);
    send_to(client_fd, MESSAGE_SENT, strlen(MESSAGE_SENT));    
}

void direct_message(int client_fd)
{
    char *active_usernames = get_all_active_users(client_fd);
    if (strlen(active_usernames) < 2)
    {
        send_to(client_fd, NO_ACTIVE_USERS, strlen(NO_ACTIVE_USERS));
        return;
    }
    char target_username[MAX_USERNAME];
    char message[MAXBUFFERSIZE];

    send_to(client_fd, active_usernames, strlen(active_usernames));
    free(active_usernames);
    receive(client_fd, target_username, MAX_USERNAME - 1);

    userlinkedlist *target_user = contains(target_username);

    if(target_user == NULL || target_user->user_fd == client_fd)
    {
        send_to(client_fd, USER_NOT_FOUND, strlen(USER_NOT_FOUND));
        return;
    }

    if(target_user->user_fd == -1)
    {
        send_to(client_fd, USER_OFFLINE, strlen(USER_OFFLINE));
        return;
    }

    send_to(client_fd, READY_TO_RECEIVE, strlen(READY_TO_RECEIVE));
    receive(client_fd, message, MAXBUFFERSIZE - 2);
    if(target_user->user_fd != -1)
    {
        send_to(target_user->user_fd, message, strlen(message));
        send_to(client_fd, MESSAGE_SENT, strlen(MESSAGE_SENT));
        return;
    }
    send_to(client_fd, USER_OFFLINE, strlen(USER_OFFLINE));
}

void *conn_handler(void *p_client_fd)
{
    int socket_fd = * (int *) p_client_fd;
    free(p_client_fd);
    char command[MAX_COMMAND_SIZE];

    pthread_rwlock_rdlock(&activeusers_lock);
    if (active_user_count >= MAX_ACTIVE_USERS)
    {
        pthread_rwlock_unlock(&activeusers_lock);
        send_to(socket_fd, SERVER_FULL, strlen(SERVER_FULL));
        close(socket_fd);
        pthread_exit(&thread_exit_return_val);
    }
    pthread_rwlock_unlock(&activeusers_lock);

    userlinkedlist *user = user_setup(socket_fd);

    while (1)
    {
        receive(socket_fd, command, MAX_COMMAND_SIZE - 1);
        if (strcmp(PUBLIC_MESSAGE, command) == 0)
        {
            public_message(socket_fd, user);
        }
        else if (strcmp(DIRECT_MESSAGE, command) == 0)
        {
            direct_message(socket_fd);
        }
        else
        {
            remove_active_user(user);
            close(socket_fd);
            printf("%s logged out\n", user->username);
            pthread_exit(&thread_exit_return_val);
        }
    }
}

int main (int argc, char *argv[]) {
    if (argc != 2)
    {
        printf("Arguments: Port\n");
        exit(1);
    }

    int server_fd;
    struct sockaddr_storage client_address;
    socklen_t addr_len = sizeof client_address;
    init_tables();

    socket_helper(&server_fd, AF_INET, SOCK_STREAM, AI_PASSIVE, NULL, argv[1]);
    printf("Socket created and bound.\n");

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
        strncpy(userll->username, user.username, MAX_USERNAME);
        strncpy(userll->password, user.password, MAX_PASSWORD);
        userll->user_fd = -1;
        userll->active_index = -1;
        userll->next = NULL;

        add(userll);
    }

    printf("Done reading existing users.\n");

    if (listen(server_fd, BACKLOG) == -1)
    {
        perror("Listen failed.");
        exit(1);
    }

    printf("Listening...\n");
    
    while (1)
    {
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr*)&client_address, &addr_len);

        if(*client_fd == -1) {
            perror("Couldn't accept connection.");
            continue;
        }

        printf("New connection accepted.\n");

        pthread_t thread;
        pthread_create(&thread, NULL, conn_handler, client_fd);
    }
    return 0;
}