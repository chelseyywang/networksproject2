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
    // char seqNum[5];
    // int seqNumLen = sprintf(seqNum, "%i", number);
    // int intAckNum = 0;
    // char ackNum[5];
    // int ackNumLen = sprintf(ackNum, "%i", intAckNum);
    // char modSeqNum[6];
    // memcpy(modSeqNum, seqNum, 5);
    // char modAckNum[6];
    // memcpy(modAckNum, ackNum, 5);
    // char z = 'z';
    // char flags = 'b';
    // int numZ = 5 - seqNumLen;
    // for (int i = 0; i < numZ; i++)
    //     strncat(modSeqNum, &z, 1);
    // strncat(modSeqNum, &z, 1);
    // int numZA = 5 - ackNumLen;
    // for (int i = 0; i < numZA; i++)
    //     strncat(modAckNum, &z, 1);
    // strncat(modAckNum, &flags, 1);
    // sprintf(header, "%s%s", modSeqNum, modAckNum);

    // while(1)
    // {
        // Sequence Number field, an Acknowledgment Number field, and ACK , SYN , and FIN flags
        int n, len;
        // send first syn
        sendto(sockfd, (const char *) header, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        fprintf(stderr, "sent header: %s\n", header);

        //printf("SEND %s %s SYN\n", seqNum, ackNum);
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
                //flags = 'a';
                // prepping msg
                int intAckNum = getAck(buffer);
                intAckNum = intAckNum + 1;
                // woah
                int intSeqNum = getSeq(buffer);

                bzero(header, 12);
                memcpy(header, makeHeader(intSeqNum, intAckNum, 'a'), 12);

                // int ackNumLen = sprintf(ackNum, "%i", intAckNum); // turn int to string seqNum; get its length
                // int numZ = 5 - ackNumLen;
                // bzero(modAckNum, 6);
                // memcpy(modAckNum, ackNum, ackNumLen);



                // int seqNumLen = sprintf(seqNum, "%i", intSeqNum); // turn int to string seqNum; get its length
                // int numSZ = 5 - seqNumLen;
                // bzero(modSeqNum, 6);
                // memcpy(modSeqNum, seqNum, seqNumLen);
                // for (int i = 0; i < numZ; i++)
                //     strncat(modAckNum, &z, 1); // add correct amt of Z's
                // for (int i = 0; i < numSZ; i++)
                //     strncat(modSeqNum, &z, 1); // add correct amt of Z's
                // bzero(header, 12);
                // strncat(modSeqNum, &z, 1);
                // strncat(modAckNum, &flags, 1);
                // sprintf(header, "%s%s", modSeqNum, modAckNum);


                // trying to send a whole a$$ file
                char content[512]={0};
                FILE *fp = fopen("binaryfile", "r");
                if (fp == NULL)
                {
                    fprintf(stderr, "Unable to open requested file\n");
                    exit(1);
                }
                int prev=ftell(fp);
                fseek(fp, 0L, SEEK_END);
                int wholeSize=ftell(fp);
                fseek(fp,prev,SEEK_SET); //go back to where we were
                fprintf(stderr, "Whole file's size: %i\n", wholeSize);
                int contentLen = fread(content, sizeof(char), 512, fp);
                //sendto(sockfd, (const char *) content, contentLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                printf("cliennt sent %i bytes: %s\n", contentLen, content);

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
                fprintf(stderr, "planning on sending this bytes: %i : %s\n", tempBuffLen, tempBuff);
                sendto(sockfd, (const char *) tempBuff, tempBuffLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                fprintf(stderr, "sent this: %s\n", tempBuff);
                printf("SEND %i %i ACK\n", intSeqNum, intAckNum);
                bzero(tempBuff, 524);


                //receive server's last message
                bzero(buffer, 524);
                while(1)
                {
                    n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                    printRecv(buffer);

                    // bzero(modAckNum, 6);
                    // for (int i = 0; i < 5; i++)
                    // {
                    //     if (buffer[i] != 'z') //if digit
                    //         strncat(modAckNum, &buffer[i], 1); // save all digits into modSeqNum
                    // }
                    // intAckNum = atoi(modAckNum); // turn to int; get incremented in string form
                    // bzero(modSeqNum, 6);
                    // for (int i = 6; i < 12; i++)
                    // {
                    //     if (buffer[i] != 'z') //if digit
                    //         strncat(modSeqNum, &buffer[i], 1); // save all digits into modSeqNum
                    // }
                    // intSeqNum = atoi(modSeqNum); // turn to int; get incremented in string form
                    // if (buffer[11] == 'a')
                    //     printf("RECV %i %i ACK\n", intAckNum, intSeqNum);
                    // if (buffer[11] == 'b')
                    //     printf("RECV %i %i SYN\n", intAckNum, intSeqNum);
                    // if (buffer[11] == 'c')
                    //     printf("RECV %i %i FIN\n", intAckNum, intSeqNum);
                    // if (buffer[11] == 'd')
                    //     printf("RECV %i %i ACK SYN\n", intAckNum, intSeqNum);
                    // if (buffer[11] == 'e')
                    //     printf("RECV %i %i ACK FIN\n", intAckNum, intSeqNum);
                    // if (buffer[11] == 'f')
                    //     printf("RECV %i %i SYN FIN\n", intAckNum, intSeqNum);
                    // if (buffer[11] == 'g')
                    //     printf("RECV %i %i ACK SYN FIN\n", intAckNum, intSeqNum);
                    // bzero(buffer, 524);
                    // prepping for fin
                    if (contentLen == wholeSize)
                    {
                        // send out fin
                        // seqNumLen = sprintf(seqNum, "%i", intSeqNum);
                        // intAckNum = 0;
                        // bzero(ackNum, 5);
                        // int ackNumLen = sprintf(ackNum, "%i", intAckNum);
                        // bzero(modSeqNum, 6);
                        // memcpy(modSeqNum, seqNum, 5);
                        // bzero(modAckNum, 6);
                        // memcpy(modAckNum, ackNum, 5);
                        // // a = ACK
                        // // b = SYN
                        // // c = FIN
                        // // d = ACK, SYN
                        // // e = ACK, FIN
                        // // f = SYN, FIN
                        // // g = ACK, SYN, FIN
                        // flags = 'c';
                        // numZ = 5 - seqNumLen;
                        // for (int i = 0; i < numZ; i++)
                        //     strncat(modSeqNum, &z, 1);
                        // strncat(modSeqNum, &z, 1);
                        // numZA = 5 - ackNumLen;
                        // for (int i = 0; i < numZA; i++)
                        //     strncat(modAckNum, &z, 1);
                        // strncat(modAckNum, &flags, 1);
                        bzero(header, 12);
                        // sprintf(header, "%s%s", modSeqNum, modAckNum);
                        // *header = makeHeader(getSeq(buffer), 0, 'c');
                        memcpy(header, makeHeader(getSeq(buffer), 0, 'c'), 12);
                        printf("plan on sending header: %s\n", header);
                        sendto(sockfd, (const char *) header, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                        printf("SEND %i 0 FIN\n", getSeq(buffer));
                        printf("HELLO????\n");

                        // receive server's ack of fim
                        while(1)
                        {
                            printf("SIR???????\n");
                            bzero(buffer, 524);
                            n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                            printf("DID YOU RECEIVE ANYTHING?? U GOOD?\n");
                            printf("here: %s\n", buffer);
                            printRecv(buffer);
                            // send
                            char finheader[12];
                            bzero(finheader, 12);
                            memcpy(finheader, makeHeader(getSeq(buffer), getAck(buffer)+1, 'a'), 12);

                            char finack[50] = {0};
                            sprintf(finack, "SEND %i %i ACK\n", getSeq(buffer), getAck(buffer)+1);

                            while(1)
                            {
                                printf("LLOKING FOR FINNNNNNN\n");
                                bzero(buffer, 524);
                                n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                                printf("here: %s\n", buffer);
                                printRecv(buffer);
                                if (buffer[11] == 'c') // if server sends a fin
                                {
                                    printf("this wahat ai wasnna send: %s\n", finheader);
                                    sendto(sockfd, (const char *) finheader, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                                    printf("%s\n", finack);
                                }// send

                                while (1)
                                {}
                            }
                        }
                        // receive server's fin
                        // send ack of fin

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