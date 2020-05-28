#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT     8080
#define MAXLINE 1024

void printRecv(char * pbuffer);
int getAck(char * pbuffer);
int getSeq(char * pbuffer);
char * makeHeader(int pintSeqNum, int pintAckNum, char pflags);

char hostName[10] = "localhost";
struct hostent *server;
int currentSeqNum = 0;
int currentAckNum = 0;
int lastRecAck = 0;


int main() {
    int sockfd;
    char buffer[524] = {0};
    char *hello = "Hello from client";
    struct sockaddr_in servaddr;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(hostName);
    if (server == NULL) {
        fprintf(stderr,"host error\n");
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    bcopy((char *)server->h_addr,
         (char *)&servaddr.sin_addr.s_addr,
         server->h_length);
    // servaddr.sin_addr.s_addr = INADDR_ANY;

    // generate random ACK
    int number = (rand() % (25600 - 0 + 1)) + 0;
    // getting header values initialized
    char header[12] = {0};
    bzero(header, 12);
    memcpy(header, makeHeader(number, 0, 'b'), 12);

    // while(1)
    // {
        // Sequence Number field, an Acknowledgment Number field, and ACK , SYN , and FIN flags
        int n, len;
        // send first syn
        sendto(sockfd, (const char *) header, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        fprintf(stderr, "sent header: %s\n", header);

        //printf("SEND %s %s SYN\n", seqNum, ackNum);
        currentSeqNum = number;
        currentAckNum = 0;
        printf("SEND %i %i SYN\n", number, 0);

        while(1)
        {
            // n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
            bzero(buffer, 524);
            n = read(sockfd, &buffer, 512);
            fprintf(stderr, "length of what server sent: %i\n", n);
            buffer[n] = '\0';
            fprintf(stderr, "Server sent: %s\n", buffer);
            printRecv(buffer);

            // HANDSHAKE 2 RECEIVED; RECEIVED ACK SYN
            if (buffer[11] == 'd')
            {
                // send ack + data
                // prepping msg
                int intAckNum = getAck(buffer);
                intAckNum = intAckNum + 1;
                // woah
                int intSeqNum = getSeq(buffer);

                bzero(header, 12);
                memcpy(header, makeHeader(intSeqNum, intAckNum, 'a'), 12);

                currentSeqNum = intSeqNum;
                currentAckNum = intAckNum;
                // trying to send a whole a$$ file
                char content[512]={0};
                FILE *fp = fopen("poo", "r");
                if (fp == NULL)
                {
                    fprintf(stderr, "Unable to open requested file\n");
                    exit(1);
                }
                int prev=ftell(fp);
                fseek(fp, 0L, SEEK_END);
                int wholeSize=ftell(fp);
                fseek(fp,prev,SEEK_SET); //go back to where we were

                // playring round
                //fseek(fp,3,SEEK_SET);

                fprintf(stderr, "Whole file's size: %i\n", wholeSize);
                int contentLen = fread(content, sizeof(char), 512, fp);
                //sendto(sockfd, (const char *) content, contentLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                //printf("cliennt sent %i bytes: %s\n", contentLen, content);

                char tempBuff[524] = {0};
                //int tempBuffLen = sprintf(tempBuff, "%s%s", header, content);
                int tempBuffLen = contentLen + 12;
                // new way
                for (int i = 0; i < 12; i++)
                {
                    tempBuff[i] = header[i];
                }
                for (int i = 0; i < contentLen; i++)
                {
                    tempBuff[i+12] = content[i];
                }


                // send cmd
                //fprintf(stderr, "planning on sending this bytes: %i : %s\n", tempBuffLen, tempBuff);
                sendto(sockfd, (const char *) tempBuff, tempBuffLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                //fprintf(stderr, "sent this: %s\n", tempBuff);
                printf("SEND %i %i ACK\n", intSeqNum, intAckNum);
                bzero(tempBuff, 524);

                currentSeqNum = currentSeqNum + contentLen;
                currentAckNum = intAckNum;

                // nine more packets left
                for (int i = 0; i < 9; i++)
                {
                    // new schtufff
                    if (feof(fp))
                    {
                        fprintf(stderr, "initial window done boyyys \n");
                        break;
                    }

                    fprintf(stderr, "more bytes to go boyz\n");
                    bzero(content, 512);
                    contentLen = fread(content, sizeof(char), 512, fp);
                    fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
                    tempBuffLen = contentLen + 12;
                    bzero(header, 12);
                    memcpy(header, makeHeader(currentSeqNum, 0, 'n'), 12);
                    // new way
                    for (int i = 0; i < 12; i++)
                    {
                        tempBuff[i] = header[i];
                    }
                    for (int i = 0; i < contentLen; i++)
                    {
                        tempBuff[i+12] = content[i];
                    }
                    // send cmd
                    //fprintf(stderr, "planning on sending this bytes: %i : %s\n", tempBuffLen, tempBuff);
                    sendto(sockfd, (const char *) tempBuff, tempBuffLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                    printf("SEND %i %i\n", currentSeqNum, 0);
                    currentSeqNum = currentSeqNum + contentLen;
                }

                //receive server's last message

                while(1)
                {
                    bzero(buffer, 524);
                    n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                    printRecv(buffer);
                    // bzero(buffer, 524);
                    if (!feof(fp) && buffer[11] == 'a')
                    {
                        lastRecAck = getAck(buffer);
                        fprintf(stderr, "ohohoh we are not donee yet\n");
                        bzero(content, 512);
                        contentLen = fread(content, sizeof(char), 512, fp);
                        fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
                        tempBuffLen = contentLen + 12;
                        bzero(header, 12);
                        memcpy(header, makeHeader(currentSeqNum, 0, 'n'), 12);
                        // new way
                        for (int i = 0; i < 12; i++)
                        {
                            tempBuff[i] = header[i];
                        }
                        for (int i = 0; i < contentLen; i++)
                        {
                            tempBuff[i+12] = content[i];
                        }
                        // send cmd
                        //fprintf(stderr, "planning on sending this bytes: %i : %s\n", tempBuffLen, tempBuff);
                        sendto(sockfd, (const char *) tempBuff, tempBuffLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                        printf("SEND %i %i\n", currentSeqNum, 0);
                        currentSeqNum = currentSeqNum + contentLen;
                    }





                    // prepping for fin
                    else if (feof(fp) ) //contentLen == wholeSize)
                    {
                        fprintf(stderr, "suk end\n");
                        fprintf(stderr, "whole file done fin time\n");
                        // send out fin
                        // // a = ACK
                        // // b = SYN
                        // // c = FIN
                        // // d = ACK, SYN
                        // // e = ACK, FIN
                        // // f = SYN, FIN
                        // // g = ACK, SYN, FIN
                        // flags = 'c';

                        bzero(header, 12);
                        memcpy(header, makeHeader(currentSeqNum, 0, 'c'), 12);
                        fprintf(stderr, "plan on sending header: %s\n", header);
                        sendto(sockfd, (const char *) header, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                        printf("SEND %i 0 FIN\n", currentSeqNum);
                        fprintf(stderr, "HELLO????\n");

                        // receive server's ack of fim
                        while(1)
                        {
                            fprintf(stderr, "SIR???????\n");
                            bzero(buffer, 524);
                            n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                            lastRecAck = getAck(buffer);
                            fprintf(stderr, "DID YOU RECEIVE ANYTHING?? U GOOD?\n");
                            fprintf(stderr, "here: %s\n", buffer);
                            // if (buffer[11] != 'c')
                            // {
                            //     fprintf(stderr, "u are not fin, i want fin\n");
                            //     continue;
                            // }
                            printRecv(buffer);
                            // send
                            char finheader[12];
                            bzero(finheader, 12);
                            memcpy(finheader, makeHeader(currentSeqNum+1, currentAckNum+1, 'a'), 12);

                            char finack[50] = {0};
                            sprintf(finack, "SEND %i %i ACK\n", currentSeqNum+1, currentAckNum+1);

                            while(1)
                            {
                                fprintf(stderr, "LLOKING FOR FINNNNNNN\n");
                                bzero(buffer, 524);
                                n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                                lastRecAck = getAck(buffer);
                                fprintf(stderr, "here: %s\n", buffer);
                                printRecv(buffer);
                                if (buffer[11] == 'c') // if server sends a fin
                                {
                                    fprintf(stderr, "this wahat ai wasnna send: %s\n", finheader);
                                    sendto(sockfd, (const char *) finheader, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                                    printf("%s\n", finack);
                                    fprintf(stderr, "ok bye server :'( \n");
                                    exit(0);
                                }
                            }
                        }

                    }
                }

            }
        }


    // }
    close(sockfd);
    return 0;
}

void printRecv(char* pbuffer)
{
    char pmodAckNum[6];
    bzero(pmodAckNum, 6);
    for (int i = 0; i < 5; i++)
    {
        if (pbuffer[i] != 'z') //if digit
            strncat(pmodAckNum, &pbuffer[i], 1); // save all digits into modSeqNum
    }
    int pintAckNum = atoi(pmodAckNum); // turn to int; get incremented in string form
    char pmodSeqNum[6];
    bzero(pmodSeqNum, 6);
    for (int i = 6; i < 12; i++)
    {
        if (pbuffer[i] != 'z') //if digit
            strncat(pmodSeqNum, &pbuffer[i], 1); // save all digits into modSeqNum
    }
    int pintSeqNum = atoi(pmodSeqNum); // turn to int; get incremented in string form
    // a = ACK
    // b = SYN
    // c = FIN
    // d = ACK, SYN
    // e = ACK, FIN
    // f = SYN, FIN
    // g = ACK, SYN, FIN
    // n = NOTHING
    if (pbuffer[11] == 'a')
        printf("RECV %i %i ACK\n", pintAckNum, pintSeqNum);
    if (pbuffer[11] == 'b')
        printf("RECV %i %i SYN\n", pintAckNum, pintSeqNum);
    if (pbuffer[11] == 'c')
        printf("RECV %i %i FIN\n", pintAckNum, pintSeqNum);
    if (pbuffer[11] == 'd')
        printf("RECV %i %i SYN ACK\n", pintAckNum, pintSeqNum);
    if (pbuffer[11] == 'e')
        printf("RECV %i %i FIN ACK\n", pintAckNum, pintSeqNum);
    if (pbuffer[11] == 'f')
        printf("RECV %i %i SYN FIN\n", pintAckNum, pintSeqNum);
    if (pbuffer[11] == 'g')
        printf("RECV %i %i SYN FIN ACK\n", pintAckNum, pintSeqNum);
    if (pbuffer[11] == 'n')
        printf("RECV %i %i\n", pintAckNum, pintSeqNum);
}

int getAck(char * pbuffer)
{
    char pmodAckNum[6];
    bzero(pmodAckNum, 6);
    for (int i = 0; i < 5; i++)
    {
        if (pbuffer[i] != 'z') //if digit
            strncat(pmodAckNum, &pbuffer[i], 1); // save all digits into modSeqNum
    }
    int pintAckNum = atoi(pmodAckNum); // turn to int; get incremented in string form
    return pintAckNum;
}

int getSeq(char *pbuffer)
{
    char pmodSeqNum[6];
    bzero(pmodSeqNum, 6);
    for (int i = 6; i < 12; i++)
    {
        if (pbuffer[i] != 'z') //if digit
            strncat(pmodSeqNum, &pbuffer[i], 1); // save all digits into modSeqNum
    }
    int pintSeqNum = atoi(pmodSeqNum); // turn to int; get incremented in string form
    return pintSeqNum;
}

char * makeHeader(int pintSeqNum, int pintAckNum, char pflags)
{
    char *pheader = malloc(13);
    //char pheader[12] = {0};
    bzero(pheader, 12);
    char pseqNum[5];
    bzero(pseqNum, 5);
    int pseqNumLen = sprintf(pseqNum, "%i", pintSeqNum);
    char packNum[5];
    bzero(packNum, 5);
    int packNumLen = sprintf(packNum, "%i", pintAckNum);
    char pmodSeqNum[6];
    bzero(pmodSeqNum, 6);
    memcpy(pmodSeqNum, pseqNum, 5);
    char pmodAckNum[6];
    bzero(pmodAckNum, 6);
    memcpy(pmodAckNum, packNum, 5);
    // a = ACK
    // b = SYN
    // c = FIN
    // d = ACK, SYN
    // e = ACK, FIN
    // f = SYN, FIN
    // g = ACK, SYN, FIN
    char z = 'z';
    int pnumZ = 5 - pseqNumLen;
    for (int i = 0; i < pnumZ; i++)
        strncat(pmodSeqNum, &z, 1);
    strncat(pmodSeqNum, &z, 1);
    int pnumZA = 5 - packNumLen;
    for (int i = 0; i < pnumZA; i++)
        strncat(pmodAckNum, &z, 1);
    strncat(pmodAckNum, &pflags, 1);
    sprintf(pheader, "%s%s", pmodSeqNum, pmodAckNum);
    return strdup(&pheader[0]);
}