/******************************************************************************
 * echo_server.c                adddddddd                                      *
 *                                                                             *
 * Description: This file contains the C source code for an echo server.  The  *
 *              server runs on a hard-coded port and simply write back anything*
 *              sent to it by connected clients.  It does not support          *
 *              concurrent clients.                                            *
 *                                                                             *
 *******************************************************************************/
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
int max_sd;
int socketCs[1024];
#include "parse.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/select.h>
#include <fcntl.h>
#define ECHO_PORT 9999
void log_write(char *ip, int id, char *msg, int tag, int siz, int flag);
void response_HEAD_GET_404(Request *request, char *buf, char *ip, int id,int tag);
int close_socket(int sock);

char log_buf[1024];
struct sockaddr_in addr, cli_addr;
fd_set readfds; /* Read file descriptors for incoming conn */

int main(int argc, char *argv[])
{
    int sock, client_sock;
    ssize_t readret;
    socklen_t cli_size;
    char buf[20 * 4096];
    char buf_new[20 * 4096];
    int sfds;

    /* Initialize client connections */
    for (int i = 0; i < 1024; i++) socketCs[i] = 0;
    fprintf(stdout, "----- Liso Server -----\n");
    /* all networked programs must create a socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        char *ip = inet_ntoa(cli_addr.sin_addr);                 
        int id = getpid();                                       
        log_write(ip, id, "Failed creating socket.\n", 0, 0, 0); 
        return EXIT_FAILURE;
    }
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ECHO_PORT);

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        char *ip = inet_ntoa(cli_addr.sin_addr);                
        int id = getpid();                                      
        log_write(ip, id, "Failed binding socket.\n", 0, 0, 0); 
        return EXIT_FAILURE;
    }
    if (listen(sock, 5)){
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        char *ip = inet_ntoa(cli_addr.sin_addr);                  
        int id = getpid();                                        
        log_write(ip, id, "Failed listening socket.\n", 0, 0, 0); 
        return EXIT_FAILURE;
    }
    /* finally, loop waiting for input and then write it back */
    do{
        /* clear the socket set */
        FD_ZERO(&readfds); 
        FD_SET(sock, &readfds);
        max_sd = sock;
        int count = 0;
        for (int i = 0; i < 1024; i++){
            client_sock = socketCs[i];
            if (client_sock > 0){
                count++;
                FD_SET(client_sock, &readfds);
            }
            if (client_sock > max_sd) max_sd = client_sock;
        }
        sfds = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        cli_size = sizeof(cli_addr);
        if (FD_ISSET(sock, &readfds))
        {
            if ((client_sock = accept(sock, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_size)) < 0)
            {
                close(sock);
                fprintf(stderr, "Error accepting connection.\n");
                char *ip = inet_ntoa(cli_addr.sin_addr);                     
                int id = getpid();                                           
                log_write(ip, id, "Error accepting connection.\n", 0, 0, 0); 
                return EXIT_FAILURE;
            }
            for (int i = 0; i < 1024; i++){
                if (socketCs[i])continue;
                socketCs[i] = client_sock;
                break;
            }
        }
        for (int i = 0; i < 1024; i++)
        {   readret = 0;
            client_sock = socketCs[i];
            getpeername(client_sock, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_size);
            if (FD_ISSET(client_sock, &readfds))
            {
                if ((readret = read(client_sock, buf, 4096 * 20)) > 0)
                {
                    strcpy(buf_new, buf);
                    int tag = 0;
                    do{
                        if ( (readret - tag == 2 && buf[readret - 2] == '\r' && buf[readret - 1] == '\n') | tag >= readret)
                        {
                            strcpy(buf_new, "all stop");
                            send(client_sock, buf_new, strlen(buf_new), 0);
                            break;
                        }
                        Request *request = parse(buf, readret, fd_in, &tag);
                        char *ip = inet_ntoa(cli_addr.sin_addr); 
                        int id = getpid();                       
                        char bad[50] = "HTTP/1.1 400 Bad request\r\n\r\n";
                        char not[50] = "HTTP/1.1 501 Not Implemented\r\n\r\n";
                        char ver[50] = "HTTP/1.1 505 HTTP Version not supported\r\n\r\n";
                        if (request == NULL || strlen(request->headers->header_name) + strlen(request->headers->header_value) > 4096*2)
                        {
                            strcpy(buf_new, bad);
                            log_write(ip, id, "Bad request\n", 0, 400, 0); 
                        }
                        else if (strcmp(request->http_version, "HTTP/1.1") != 0)
                        {
                            strcpy(buf_new, ver);
                            log_write(ip, id, "HTTP Version not supported\n", 0, 505, 0);
                        }
                        else if (strcmp(request->http_method, "GET") != 0 && strcmp(request->http_method, "HEAD") != 0 && strcmp(request->http_method, "POST") != 0)
                        {
                            strcpy(buf_new, not );
                            log_write(ip, id, "Not Implemented\n", 0, 501, 0); 
                        }
                        else if (!strcmp(request->http_method, "GET") || strcmp(request->http_method, "HEAD") == 0)
                        {
                            response_HEAD_GET_404(request, buf_new, ip, id,i);
                            sprintf(log_buf, "%s %s %s", request->http_method, request->http_uri, request->http_version); 
                            log_write(ip, id, log_buf, readret, 200, 1);                                                  

                        }
                        else if (!strcmp(request->http_method, "POST"))
                        {
                            sprintf(log_buf, "%s %s %s", request->http_method, request->http_uri, request->http_version); 
                            log_write(ip, id, log_buf, readret, 200, 1);                                                  

                        }
                        if (send(client_sock, buf_new, strlen(buf_new), 0) != strlen(buf_new))
                        {
                            int id;
							close_socket(client_sock);
                            close_socket(sock);
                            fprintf(stderr, "Error sending to client.\n");
                            char *ip =inet_ntoa(cli_addr.sin_addr);                  
                            id= getpid();                                        
                            log_write(ip, id, "Error sending to client.\n", 0, 0, 0); 
                            return EXIT_FAILURE;
                        }
                    }while(1);
                    int sd = socketCs[i];
                    socketCs[i] = 0;
                    close_socket(sd);
                }else
                {
                    int sd = socketCs[i];
                    socketCs[i] = 0;
                    close_socket(sd);
                }
                memset(buf_new, 0, 20 * 4096);
            }
        } 
    }while(1);
    close_socket(sock);
    return EXIT_SUCCESS;
}

int close_socket(int sock){
    if (!close(sock))return 0;
    fprintf(stderr, "Failed closing socket.\n");
    char *ip = inet_ntoa(cli_addr.sin_addr);                
    int id = getpid();                                      
    log_write(ip, id, "Failed closing socket.\n", 0, 0, 0); 
    return 1;
}

void response_HEAD_GET_404(Request *request, char *buf, char *ip, int id,int tag)
{
    char LMT_[64];
    char DT_[64];
    char LP_[4096];
    char response_header[4 * 4096];
    struct tm Time;
    struct stat filemessage;          
    time_t CURRENT_TIME;        
    char PATH[4096];
    char TYPE[64];  

    sprintf(LP_, "%s", getenv("PWD"));
    strcpy(PATH, LP_);
    strcat(PATH, request->http_uri);
    if (!strcmp(request->http_uri, "/"))strcat(PATH, "static_site/index.html");
    else if (request->http_uri[strlen(request->http_uri) - 1] == '/') strcat(PATH, "index.html");
    fprintf(stdout, "%s\n", PATH);
    if (stat(PATH, &filemessage) < 0) {
        strcpy(buf, "HTTP/1.1 404 Not Found\r\n\r\n");
        log_write(ip, id, "Not Found\n", 0, 404, 0);
        return;
    }
    if ((!S_ISREG(filemessage.st_mode)) || !(S_IRUSR & filemessage.st_mode)){
        printf("Access denied.");
        return;
    }
    if (strstr(PATH, ".html"))
        strcpy(TYPE, "text/html");
    else if (strstr(PATH, ".jpg") || strstr(PATH, "jpeg"))
        strcpy(TYPE, "image/jpeg");
    else if (strstr(PATH, ".css"))
        strcpy(TYPE, "text/css");
    else if (strstr(PATH, ".js"))
        strcpy(TYPE, "application/javascript");
    else if (strstr(PATH, ".wav"))
        strcpy(TYPE, "audio/x-wav");
    else if (strstr(PATH, ".gif"))
        strcpy(TYPE, "image/gif");
    else if (strstr(PATH, ".png"))
        strcpy(TYPE, "image/png");
    else
        strcpy(TYPE, "text/plain");
        
    Time = *gmtime(&filemessage.st_mtime); 
    strftime(LMT_, 64, "%a, %d %b %Y %H:%M:%S %Z", &Time); 
    CURRENT_TIME = time(0);  
    Time = *gmtime(&CURRENT_TIME);
    strftime(DT_, 64, "%a, %d %b %Y %H:%M:%S %Z", &Time); 
    sprintf(response_header, "HTTP/1.1 %s\r\n", "200 OK"); 
    sprintf(response_header, "%sServer: liso/1.0\r\n", response_header);
    sprintf(response_header, "%sDate: %s\r\n", response_header, DT_);
    sprintf(response_header, "%sContent-Length: %ld\r\n", response_header, filemessage.st_size);
    sprintf(response_header, "%sContent-type: %s\r\n", response_header, TYPE);
    sprintf(response_header, "%sLast-modified: %s\r\n", response_header, LMT_);
    if (!socketCs[tag])sprintf(response_header, "%sConnection: close\r\n\r\n", response_header);
    else sprintf(response_header, "%sConnection: keep-alive\r\n\r\n", response_header);
    response_header[strlen(response_header)] = '\0';
    if (!strcmp(request->http_method, "HEAD")) strcpy(buf, response_header);
    else if (strcmp(request->http_method, "GET") == 0)
    {
        int fd;
        char content[4 * 4096];
        fd = open(PATH, O_RDONLY);
        if (fd < 0)
        {
            printf("Failed to open the file\n");
            return;
        }
        int readRet = read(fd, content, 4 * 4096);
        content[readRet] = '\0';
        close(fd);
        strcpy(buf, strcat(response_header, content));
    }
}

void log_write(char *ip, int id, char *msg, int tag, int siz, int flag) 
{
    char message[1024];
    char right_time[64];
    char wrong_time[64];
	FILE *fptr = fopen("log.txt", "a+");
    time_t nowtime = time(NULL);
    strftime(right_time, sizeof(right_time), "[%d/%b/%Y:%H:%M:%S %Z]", localtime(&nowtime));
    strftime(wrong_time, sizeof(wrong_time), "[%a %b %d %H:%M:%S %Z]", localtime(&nowtime));
    if (fptr){
        if (!flag) sprintf(message, "%s [server:error] [pid %d] [client %s] %s\r\n", wrong_time, id, ip, msg);
        else sprintf(message, "%s - client %s \"%s\" %d %d\n\r\n", ip, right_time, msg, siz, tag);
        fputs(message, fptr);
        fclose(fptr);
    }
    else exit(0);
    memset(log_buf, 0, 1024);
}
