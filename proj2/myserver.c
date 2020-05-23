#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int get = 0;
    int newfd; // client's fd
    struct sockaddr_in servAddr;
    struct sockaddr_in cliAddr;
    char buf[2056]= {0}; // buf is for information received from client
    int sockfd;
    int portNum = 9090;

    if (argc > 2)
    {
        fprintf(stderr, "Usage: ./server <port number>  (default port number is 9090). \n");
        exit(1);
    }
    if (argc == 2)
    {
        portNum = atoi(argv[1]);
    }

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        fprintf(stderr, "Creating socket failed\n");
        exit(1);
    }

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(portNum);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(servAddr.sin_zero, '\0', sizeof(servAddr.sin_zero));
    if(bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        fprintf(stderr, "Bind failed. Port %d is already in use. Usage: ./server <port number>. Default port number is 9090. \n", portNum);
        exit(1);
    }
    if ( listen(sockfd, 100) < 0 )
    {
        fprintf(stderr, "Listen failed\n");
        exit(1);
    }

    //printf("listening...\n");

    socklen_t clientLength = sizeof(cliAddr);
    newfd = accept(sockfd, (struct sockaddr *)&cliAddr, &clientLength);
    if ( newfd < 0)
    {
        fprintf(stderr, "Accept failed\n");
        //continue;
    }
    //printf("Connected...\n");

    while(1)
    {
        if(read(newfd, buf, sizeof(buf)) < 0)
        {
           printf("Reading from client syscall error\n");
           exit(1);
        }
        // printing what client sent
        // printf("%s", buf);
        // checking for GET
        printf("%s",buf);
    }

    return 0;
}