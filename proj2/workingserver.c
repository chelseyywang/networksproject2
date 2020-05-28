#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>


#define PORT     8080
#define MAXLINE 1024

void printRecv(char * pbuffer);
int getAck(char * pbuffer);
int getSeq(char * pbuffer);
char * makeHeader(int pintSeqNum, int pintAckNum, char pflags);

int main() {
    int sockfd;
    char buffer[524] = {0};
    int finSent = 0;
    char *hello = "Hello from server";
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        fprintf(stderr, "Creating socket failed\n");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,
            sizeof(servaddr)) < 0 )
    {

        fprintf(stderr, "Bind failed. Port is already in use. Usage: ./server <port number>. Default port number is 9090. \n");
        exit(1);
    }
    // initializing
    finSent = 0;
    char header[12] = {0};
    char seqNum[5] = "221zz";
    char ackNum[5] = "00000";
    char modSeqNum[6];
    memcpy(modSeqNum, seqNum, 5);
    char modAckNum[5];
    memcpy(modAckNum, ackNum, 5);
    // a = ACK
    // b = SYN
    // c = FIN
    // d = ACK, SYN
    // e = ACK, FIN
    // f = SYN, FIN
    // g = ACK, SYN, FIN
    char z = 'z';
    char flags = 'd';
    strncat(modSeqNum, &z, 1);
    strncat(modAckNum, &flags, 1);
    sprintf(header, "%s%s", modSeqNum, modAckNum);


    while(1)
    {
        finSent = 0;
        memcpy(seqNum, "221zz", 5);
        bzero(modSeqNum, 6);
        memcpy(modSeqNum, seqNum, 5);
        strncat(modSeqNum, &z, 1);

        printf("Listening for first handshake...\n");
        int len, n;

        len = sizeof(cliaddr);  //len is value/resuslt
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0';
        fprintf(stderr, "client sent : %s\n", buffer);
        bzero(modAckNum, 6);
        for (int i = 0; i < 5; i++)
        {
            if (buffer[i] != 'z') //if digit
                strncat(modAckNum, &buffer[i], 1); // save all digits into modSeqNum
        }
        int intAckNum = atoi(modAckNum); // turn to int; get incremented in string form
        char recmodSeqNum[6];
        bzero(recmodSeqNum, 6);
        for (int i = 6; i < 12; i++)
        {
            if (buffer[i] != 'z') //if digit
                strncat(recmodSeqNum, &buffer[i], 1); // save all digits into modSeqNum
        }
        int intSeqNum = atoi(recmodSeqNum); // turn to int; get incremented in string form
        if (buffer[11] == 'a')
                printf("RECV %i %i ACK\n", intAckNum, intSeqNum);
        if (buffer[11] == 'b')
            printf("RECV %i %i SYN\n", intAckNum, intSeqNum);
        if (buffer[11] == 'c')
            printf("RECV %i %i FIN\n", intAckNum, intSeqNum);
        if (buffer[11] == 'd')
            printf("RECV %i %i SYN ACK\n", intAckNum, intSeqNum);
        if (buffer[11] == 'e')
            printf("RECV %i %i FIN ACK\n", intAckNum, intSeqNum);
        if (buffer[11] == 'f')
            printf("RECV %i %i SYN FIN\n", intAckNum, intSeqNum);
        if (buffer[11] == 'g')
            printf("RECV %i %i SYN FIN ACK\n", intAckNum, intSeqNum);
        while(1) // wait for client to send first handshake
        {
            if (finSent == 1)
                break;
            // check if client sent first handshake
            if (buffer[11] == 'b') // syn
            {
                // parse header

                intAckNum = intAckNum + 1;
                int ackNumLen = sprintf(ackNum, "%i", intAckNum); // turn int to string seqNum; get its length
                int numZ = 5 - ackNumLen;
                bzero(modAckNum, 6);
                memcpy(modAckNum, ackNum, ackNumLen);
                for (int i = 0; i < numZ; i++)
                    strncat(modAckNum, &z, 1); // add correct amt of Z's
                // shake back; send syn + ack
                printf("sent - ACK SYN \n");
                flags = 'd';
                bzero(header, 12);
                strncat(modAckNum, &flags, 1);
                sprintf(header, "%s%s", modSeqNum, modAckNum);
                int sir = 0;
                // fprintf(stderr, "crustys header: %s\n", header);
                // fprintf(stderr, "0 spot: %c\n", header[0]);
                // fprintf(stderr, "1 spot: %c\n", header[1]);
                // fprintf(stderr, "2 spot: %c\n", header[2]);
                // fprintf(stderr, "3 spot: %c\n", header[3]);
                // fprintf(stderr, "4 spot: %c\n", header[4]);
                // fprintf(stderr, "5 spot: %c\n", header[5]);
                // fprintf(stderr, "6 spot: %c\n", header[6]);
                // fprintf(stderr, "7 spot: %c\n", header[7]);
                // fprintf(stderr, "8 spot: %c\n", header[8]);
                // fprintf(stderr, "9 spot: %c\n", header[9]);
                // fprintf(stderr, "10 spot: %c\n", header[10]);
                // fprintf(stderr, "11 spot: %c\n", header[11]);
                // fprintf(stderr, "12 spot: %c\n", header[12]);
                for (int i = 0; i <= 12; i++)
                {
                    if (i == 5 && header[i] != 'z')
                    {
                        sir = 1;
                        fprintf(stderr, "HEADER FA;SLDKFJ\n");
                    }
                    if (sir == 1)
                        header[i] = header[i+1];
                    if ( i == 12)
                        header[12] = '\0';
                }
                sendto(sockfd, (const char *)header, 12,
                MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
                    len);

                fprintf(stderr, "server sent: %s\n", header);
                printf("SEND 221 %i SYN ACK\n", intAckNum);
                printf("Listening for third handshake...\n");
                int prevIntSeqNum = 0;
                while(1)
                {
                                    printf("Listening for 3 handshake...\n");

                    bzero(buffer, 524);
                    // reading third handshake from client
                    n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                            MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                            &len);
                    buffer[n] = '\0';
                    if (finSent == 1 && buffer[11] == 'a')
                    {
                        // check if the seq and ack numbers are correct
                        printRecv(buffer);
                        fprintf(stderr, "bye client :( \n");
                        break;
                    }
                    if (buffer[11] == 'a') // if client sends an ACK
                    {
                        //fprintf(stderr, "Client3 : %s\n", buffer);
                        char fileBuff[512] = {0};
                        int c = 0;
                        fprintf(stderr, "this is n: %i\n", n);
                        while (c+12 < n)
                        {
                            fileBuff[c] = buffer[12+c];
                            c++;
                        }
                        int fileBuffSize = n - 12;

                        char cwd[100];
                        if (getcwd(cwd, sizeof(cwd)) != NULL) {
                            printf("Current working dir: %s\n", cwd);
                        } else {
                            perror("getcwd() error");
                            return 1;
                        }

                        char buf[100] = "1.file";
                        FILE *fp = fopen("1.file" , "a");
                        int errnum;
                        printf("hiiiiii\n");
                        char mode[] = "0777";
                        int i;
                        i = strtol(mode, 0, 8);
                        if (chmod ("1.file",i) < 0)
                        {
                            fprintf(stderr, "error w chmod\n");
                            perror("Error printed by perror");
                            exit(1);
                        }
                        if ( fp == NULL )
                        {
                            errnum = errno;
                            fprintf(stderr, "Value of errno: %d\n", errno);
                            perror("Error printed by perror");
                            fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
                            exit(1);
                        }
                        int fwriteLen = fwrite(&fileBuff, 1, fileBuffSize, fp);
                        //fprintf(stderr, "wrote to file %i bytes: %s\n", fwriteLen, fileBuff);
                        //fprintf(stderr, "sketych last byte: %c\n", fileBuff[fileBuffSize-1]);
                        fclose(fp);
                    } // end if ack



                        bzero(modAckNum, 6);
                        for (int i = 0; i < 5; i++)
                        {
                            if (buffer[i] != 'z') //if digit
                                strncat(modAckNum, &buffer[i], 1); // save all digits into modSeqNum
                        }
                        intAckNum = atoi(modAckNum); // turn to int; get incremented in string form
                        bzero(modSeqNum, 6);
                        for (int i = 6; i < 12; i++)
                        {
                            if (buffer[i] != 'z') //if digit
                                strncat(modSeqNum, &buffer[i], 1); // save all digits into modSeqNum
                        }
                        intSeqNum = atoi(modSeqNum); // turn to int; get incremented in string form
                        if (buffer[11] == 'a')
                            printf("RECV %i %i ACK\n", intAckNum, intSeqNum);
                        if (buffer[11] == 'b')
                            printf("RECV %i %i SYN\n", intAckNum, intSeqNum);
                        if (buffer[11] == 'c')
                            printf("RECV %i %i FIN\n", intAckNum, intSeqNum);
                        if (buffer[11] == 'd')
                            printf("RECV %i %i SYN ACK\n", intAckNum, intSeqNum);
                        if (buffer[11] == 'e')
                            printf("RECV %i %i FIN ACK\n", intAckNum, intSeqNum);
                        if (buffer[11] == 'f')
                            printf("RECV %i %i SYN FIN\n", intAckNum, intSeqNum);
                        if (buffer[11] == 'g')
                            printf("RECV %i %i SYN FIN ACK\n", intAckNum, intSeqNum);
                        intAckNum = intAckNum + n - 12;

                        if (buffer[11] == 'c') // if client sends a FIN
                        {
                            fprintf(stderr, "got fin from the client\n");
                            intAckNum = intAckNum + 1;
                        }


                        flags = 'a';
                        ackNumLen = sprintf(ackNum, "%i", intAckNum); // turn int to string seqNum; get its length
                        numZ = 5 - ackNumLen;
                        bzero(modAckNum, 6);
                        memcpy(modAckNum, ackNum, ackNumLen);
                        int seqNumLen = sprintf(seqNum, "%i", intSeqNum); // turn int to string seqNum; get its length
                        if (buffer[11] == 'c')
                            {
                                bzero(seqNum, 5);
                                // printf("hi prev2: %i\n", prevIntSeqNum);
                                seqNumLen = sprintf(seqNum, "%i", prevIntSeqNum);
                                intSeqNum = prevIntSeqNum;
                            }
                        int numSZ = 5 - seqNumLen;
                        bzero(modSeqNum, 6);
                        memcpy(modSeqNum, seqNum, seqNumLen);
                        //  printf("hi prev3: %s\n", modSeqNum);

                        for (int i = 0; i < numZ; i++)
                            strncat(modAckNum, &z, 1); // add correct amt of Z's
                        for (int i = 0; i < numSZ; i++)
                            strncat(modSeqNum, &z, 1); // add correct amt of Z's
                        bzero(header, 12);
                        strncat(modSeqNum, &z, 1);
                        strncat(modAckNum, &flags, 1);
                        sprintf(header, "%s%s", modSeqNum, modAckNum);
                        sendto(sockfd, (const char *)header, 12, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
                        printf("actually sent: %s\n", header);
                        printf("SEND %i %i ACK\n", intSeqNum, intAckNum);

                        // send my fin
                        if (buffer[11] == 'c')
                        {
                            flags = 'c';
                            intAckNum = 0;
                            ackNumLen = sprintf(ackNum, "%i", intAckNum); // turn int to string seqNum; get its length
                            numZ = 5 - ackNumLen;
                            bzero(modAckNum, 6);
                            memcpy(modAckNum, ackNum, ackNumLen);
                                    bzero(seqNum, 5);
                                    // printf("hi prev2: %i\n", prevIntSeqNum);
                                    seqNumLen = sprintf(seqNum, "%i", prevIntSeqNum);
                                    intSeqNum = prevIntSeqNum;
                            int numSZ = 5 - seqNumLen;
                            bzero(modSeqNum, 6);
                            memcpy(modSeqNum, seqNum, seqNumLen);
                            //  printf("hi prev3: %s\n", modSeqNum);

                            for (int i = 0; i < numZ; i++)
                                strncat(modAckNum, &z, 1); // add correct amt of Z's
                            for (int i = 0; i < numSZ; i++)
                                strncat(modSeqNum, &z, 1); // add correct amt of Z's
                            bzero(header, 12);
                            strncat(modSeqNum, &z, 1);
                            strncat(modAckNum, &flags, 1);
                            sprintf(header, "%s%s", modSeqNum, modAckNum);
                            sendto(sockfd, (const char *)header, 12, MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
                            printf("actually sent: %s\n", header);
                            printf("SEND %i %i FIN\n", intSeqNum, intAckNum);
                            finSent = 1;
                        }

                        prevIntSeqNum = intSeqNum;
                        printf("hi prev: %i\n", prevIntSeqNum);
                        bzero(buffer, 524);


                }
            } // did not get handshake, try again
        } // end while first handshake
    }
    return 0;
}

// function that will take in the buffer, parse the sequence number, increment and return as ack
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