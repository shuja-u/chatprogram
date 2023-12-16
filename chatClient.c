/*
 * Shuja Uddin
 * sm2849sr
 * 
 * This file contains the logic for the client.
 */

#include "helper.h"

int stay_alive = 1;

// Username of the user
char this_user[MAX_USERNAME];

// Mutex for the queue
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Condition variable used to wait for the queue to stop being full/empty
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Used to store incoming commands
char *circular_command_fifo[MAX_COMMANDS_BUFFER];
int head = 0;
int tail = 0;
int full = 0;

/*
 * Function: fifoadd
 * -------------------
 * Adds a command message to the queue in a thread safe fashion.
 * Notifies any sleeping threads once a message has been added.
 * 
 * *message: a pointer to a command message.
 */
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

/*
 * Function: fifopop
 * -------------------
 * Removes a command message from the queue in a thread safe fashion.
 * Notifies any sleeping threads once a message has been removed.
 * 
 * returns: a pointer to the removed message. 
 */
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
 * Function: receive
 * -------------------
 * Uses the recv function and marks the end of the received message. 
 * Additionally, if the process was unsuccessful, it will print the 
 * appropriate message and exit the program.
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
        exit(1);
    }
}

/*
 * Function: get_command
 * ---------------------------
 * Uses fgets to get a command from stdin, terminates the command appropriately.
 * 
 * *buffer: where the command will be stored. 
 */
void get_command(char *buffer)
{
    fgets(buffer, MAX_COMMAND_SIZE, stdin);
    buffer[strlen(buffer) - 1] = 'C';
    __fpurge(stdin);  
}

/*
 * Function: get_username
 * ---------------------------
 * Uses fgets to get a username from stdin.
 * 
 * *buffer: where the username will be stored. 
 */
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

/*
 * Function: get_password
 * --------------------------
 * Uses fgets to get a password from stdin.
 * 
 * *buffer: where the password will be stored. 
 */
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

/*
 * Function: get_message
 * --------------------------
 * Uses fgets to get a message from stdin, it is terminated appropriately.
 * Marks the message with the name of the sending user.
 * 
 * *buffer: where the message will be stored.
 */
void get_message(char *buffer)
{
    buffer[0] = '\0';
    sprintf(buffer, "%s: ", this_user);
    buffer[strlen(buffer)] = '\0';
    char message[MAXBUFFERSIZE];
    fgets(message, MAXBUFFERSIZE - 1, stdin);
    while(strlen(message) < 2)
    {
        printf("Message must be at least one character. Try again:\n");
        fgets(message, MAXBUFFERSIZE - 1, stdin);
    }
    strcat(buffer, message);
    buffer[strlen(buffer) - 1] = 'D';
    __fpurge(stdin);
}

/*
 * Function: public_message
 * -----------------------------
 * Handles the PM command from the user.
 * 
 * socket_fd: used to send and receive messages to/from the server. 
 */
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

/*
 * Function: direct_message
 * -----------------------------
 * Handles the DM command from the user.
 * 
 * socket_fd: used to send and receive messages to/from the server. 
 */
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

/*
 * Function: command_handler
 * --------------------------------
 * Used exclusively by the command handling thread.
 * Gets and processes commands appropriately.
 * 
 * *p_client_fd: pointer to the socket used to communicate.
 * 
 * returns: a void pointer.
 */
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
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Arguments: IP Port Username\n");
        exit(1);
    }

    strncpy(this_user, argv[3], MAX_USERNAME);
    this_user[strlen(argv[3])] = '\0';
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

    // Handle user creation

    send_to(socket_fd, this_user, strlen(this_user));
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
    void *return_val; // Return value when thread joins

    // Start command handler thread
    if (pthread_create(&thread, NULL, command_handler, &socket_fd) != 0)
    {
        perror("Thread create error.");
        exit(1);
    }

    int bytes_received = 0;

    // Continuously receive messages, put them in the queue if they are command messages
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
            else
            {
                message[bytes_received] = '\0';
                fifoadd(message);
            }
        }
        else
        {
            printf("Connection ended.\n");
            stay_alive = 0;
            close(socket_fd);
        }
    }

    if (pthread_join(thread, &return_val) != 0)
    {
        perror("Thread join error.");
        exit(3);
    }
    return 0;
}
