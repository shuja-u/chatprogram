/*
 * Shuja Uddin
 * sm2849sr
 * 
 * This file contains a function that is used by both the server and the client.
 */

#include "helper.h"

/*
 * Function: socket_helper
 * ----------------------------
 * Creates and binds/connects a socket.
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