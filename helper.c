/*
 * Shuja Uddin
 * sm2849sr
 * 
 * This file contains functions that are used by both the server and the 
 * client.
 */

#include "helper.h"

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

void receive_from(int socket_fd, char message[], int message_len,
                  struct sockaddr *address, socklen_t *addr_len)
{
    int bytes_received;
    if ((bytes_received = recvfrom(socket_fd, message, message_len, 0,
                                   address, addr_len)) == -1)
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

void send_to(int socket_fd, char message[], int message_len,
             struct sockaddr *address, socklen_t addr_len)
{
    int bytes_sent;
    if ((bytes_sent = sendto(socket_fd, message, message_len, 0, address,
                             addr_len)) == -1)
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