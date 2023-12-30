/******************************************************************************
 * echo_client.c                                                               *
 *                                                                             *
 * Description: This file contains the C source code for an echo client.  The  *
 *              client connects to an arbitrary <host,port> and sends input    *
 *              from stdin.                                                    *
 *                                                                             *
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/stat.h>
#include "parse.h"
#include <fcntl.h>

#define PORT 9999

int main(int argc, char *argv[])
{
    char buf[20 * 4096];
    int sock;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    struct addrinfo *servinfo;
	if (argc < 3)
    {
        fprintf(stderr, "usage: %s <server-ip> <port>", argv[0]);
        return EXIT_FAILURE;
    }
    
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    int status;
    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)))
    {
        fprintf(stderr, "getaddrinfo error: %s \n", gai_strerror(status));
        return EXIT_FAILURE;
    }
    if ((sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
    {
        fprintf(stderr, "Socket failed");
        return EXIT_FAILURE;
    }
    if (connect(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
    {
        fprintf(stderr, "Connect");
        return EXIT_FAILURE;
    }
    fd_in = open(argv[3], O_RDONLY);
    if (fd_in < 0)
    {
        printf("Failed.\n");
        return 0;
    }
    int SENDBYTE= read(fd_in, buf, 20 * 4096);
    int RECEIVEBYTE;
    buf[SENDBYTE] = 0;
    fprintf(stdout, "------Sending------ \n%s", buf);
    send(sock, buf, strlen(buf), 0);
    fprintf(stdout, "------Received------ \n");
    do{
        if ((RECEIVEBYTE = recv(sock, buf,4096*20,0)) > 1)
        {
            if (!strcmp(buf, "all stop")) break;
            buf[RECEIVEBYTE] = 0;
            fprintf(stdout, "\n%s", buf);
        }
    }while(1);
    freeaddrinfo(servinfo);
    close(sock);
    return EXIT_SUCCESS;
}
