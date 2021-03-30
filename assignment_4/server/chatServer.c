/*
 * Shuja Uddin
 * sm2849sr
 * 
 * This file contains the logic for the server.
 */

#include "helper.h"

// Mutex for the hash table
static pthread_mutex_t hash_table_lock = PTHREAD_MUTEX_INITIALIZER;

//Read-write lock for active users array, allowing multiple readers but only one writer
static pthread_rwlock_t activeusers_lock = PTHREAD_RWLOCK_INITIALIZER;

// Return value for a terminated thread
int thread_exit_return_val;

// Users will be saved to file in this format
typedef struct userinfo
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} userinfo;

// Users will be stored in the hash table in this format
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

/*
 * Function: hash
 * ----------------
 * Used to calculate the index of a user before inserting into the hash table.
 * 
 * *username: hash will be calculated based on username
 * 
 * returns: the hashed index 
 */
unsigned int hash(char *username)
{
    int length = strnlen(username, MAX_USERNAME);

    unsigned int hash = 0;

    for (int i = 0; i < length; i++)
    {
        hash += username[i];
        hash = (hash * username[i]) % TABLE_SIZE;
    }
    return hash;
}

/*
 * Function: init_tables
 * -----------------------
 * Initializes both the hash table and active user array by making sure they're empty.
 */
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

/*
 * Function: add
 * ---------------
 * Adds a user to the hash table in a thread safe fashion.
 * 
 * *userll: the user to be added
 */
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

/*
 * Function: contains
 * ---------------------
 * Checks if a user with a given username exists in the hash table in a thread safe fashion.
 * Returns a pointer to the user, if found.
 * 
 * *username: the username to be matched
 * 
 * returns: a pointer to the user, if found. NULL otherwise.
 */
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

/*
 * Function: receive
 * -------------------
 * Uses the recv function and marks the end of the received message. 
 * Additionally, if the process was unsuccessful, it will print the 
 * appropriate message and exit the thread.
 * 
 * socket_fd: socket file descriptor.
 * message[]: where the received message will be stored.
 * message_len: how much message[] can hold.
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
 * --------------------
 * Uses the send function and prints an appropriate message if sending was 
 * unsuccessful.
 * 
 * socket_fd: socket file descriptor.
 * message[]: the message to send.
 * message_len: length of message[].
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

/*
 * Function: add_active_user
 * -------------------------------
 * Adds a given user to the active user array. Uses a read-write lock for thread safety.
 * 
 * *user: the user to be added.
 */
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

/*
 * Function: remove_active_user
 * ----------------------------------
 * Removes a given user from the active user array. Uses a read-write lock for thread safety.
 * 
 * *user: the user to be removed. 
 */
void remove_active_user(userlinkedlist *user)
{
    pthread_rwlock_wrlock(&activeusers_lock);
    active_users[user->active_index] = NULL;
    user->active_index = -1;
    user->user_fd = -1;
    active_user_count--;
    pthread_rwlock_unlock(&activeusers_lock);
}

/*
 * Function: send_to_all
 * -------------------------
 * Sends the given message to all users except the sending user.
 * 
 * *message: the message to send.
 * sending_user: the user not to send the message to. 
 */
void send_to_all(char *message, int sending_user)
{
    pthread_rwlock_rdlock(&activeusers_lock);
    for(int i = 0; i < MAX_ACTIVE_USERS; i++)
    {
        if (active_users[i] != NULL && sending_user != active_users[i]->user_fd)
        {
            send_to(active_users[i]->user_fd, message, strlen(message));
        }
    }
    pthread_rwlock_unlock(&activeusers_lock);
}

/*
 * Function: get_all_active_users
 * ----------------------------------
 * Builds a string of all active users except the one requesting the list.
 * Every user is on a new line. Marks the list as a command message.
 * Uses a read-write to allow multiple threads simultaneous access.
 * 
 * client_fd: socket descriptor of the user requesting the list.
 * 
 * returns: a pointer to a string containing all active users 
 */
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

/*
 * Function: user_setup
 * ------------------------
 * Handles the initial user setup and returns the created user. Exits the thread if user couldn't be created.
 * 
 * client_fd: socket descriptor that will be associated the created user.
 * 
 * returns: a pointer to a user.
 */
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

/*
 * Function: public_message
 * -----------------------------
 * Handles the PM command. Used to receive a message and send it to all users.
 * 
 * client_fd: socket of the sending user.
 */
void public_message(int client_fd)
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
    send_to_all(message, client_fd);
    send_to(client_fd, MESSAGE_SENT, strlen(MESSAGE_SENT));    
}

/*
 * Function: direct_message
 * -----------------------------
 * Handles the DM command. Used to send a message from one user to another.
 * 
 * client_fd: socket of the sending user.
 */
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

/*
 * Function: command_handler
 * --------------------------------
 * Handles every new connection.
 * 
 * *p_client_fd: pointer to the socket of the new connection.
 * 
 * returns: a void pointer.
 */
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
            public_message(socket_fd);
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

    // Creates file if it doesn't exist, name of file (USERFILE) can be changed in helper.h
    saved_users = fopen(USERFILE, "ab+");

    if (saved_users == NULL)
    {
        perror("File error.");
        exit(1);
    }

    userinfo user;

    // Stores the users from the file into the hash table.
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