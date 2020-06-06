#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <poll.h>
#include <stropts.h>

#define PORT     8080
#define MAXLINE 1024
#define BILLION 1000000000L

void printRecv(char * pbuffer);
int getAck(char * pbuffer);
int getSeq(char * pbuffer);
char * makeHeader(int pintSeqNum, int pintAckNum, char pflags);

char hostName[10] = "localhost";
char fileName[50] = {0};
struct hostent *server;
int portNum = 8080;
int currentSeqNum = 0;
int currentAckNum = 0;
int lastRecvAck = 0;
int lastRecvSeq = 0;
int allAcksRecv = 0;
int wholeSize = 0;
int badPoll = 0;
int roundNum = 0;
int readIn = 0;
int lastlast = 0;
int lastlastAck = 0;
int useless = 0;
int resendFin = 0;
int acktofin = 0;
long long diff;
long long diff2;
long long diff3;
struct timespec start, end;
struct timespec start2, end2;
struct timespec start3, end3;
struct timeval t1, t2;
int ip = 0;


int main(int argc, char* argv[]) {

    // handle arguments
    if (argc > 4)
    {
        fprintf(stderr, "Usage: ./client <hostname> <port> <filename>\n");
        exit(1);
    }
    fprintf(stderr, "argc: %i\n", argc);
    if (argc == 4)
    {
        if (argv[1][0] == '1')
            ip = 1;
        bzero(hostName, 10);
        memcpy(hostName, argv[1], strlen(argv[1]));
        portNum = atoi(argv[2]);
        bzero(fileName, 50);
        memcpy(fileName, argv[3], strlen(argv[3]));
        fprintf(stderr, "host: %s, port: %i, file: %s\n", hostName, atoi(argv[2]), fileName);

    }
    else if (argc == 2)
    {
        bzero(fileName, 50);
        memcpy(fileName, argv[1], strlen(argv[1]));
    }
    // generate random ACK
    int randnumber = (rand() % (25600 - 0 + 1)) + 0;
    // lets get starteeed

    int sockfd;
    char buffer[524] = {0};
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
    servaddr.sin_port = htons(portNum);
    if (ip == 1)
        servaddr.sin_addr.s_addr = inet_addr(hostName);
    else
        bcopy((char *)server->h_addr,
            (char *)&servaddr.sin_addr.s_addr,
            server->h_length);

    // generate random ACK
    int number = (rand() % (25600 - 0 + 1)) + 0;
    number = 2254;
    // getting header values initialized
    char header[12] = {0};
    bzero(header, 12);
    memcpy(header, makeHeader(number, 0, 'b'), 12);

    // Sequence Number field, an Acknowledgment Number field, and ACK , SYN , and FIN flags
    int n;
    unsigned int len = 0;
    // send first syn

    // simulate losing first syn
    // comment out to mock packet loss
    sendto(sockfd, (const char *) header, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    clock_gettime(CLOCK_MONOTONIC, &start);
    fprintf(stderr, "sent header: %s\n", header);

    currentSeqNum = number;
    currentAckNum = 0;
    printf("SEND %i %i SYN\n", number, 0);


    while(1)
    {
        struct pollfd fds[1];
        fds[0].fd = sockfd;
        fds[0].events = POLLERR | POLLHUP | POLLIN;
        int ret = poll(fds, 1, 500);
        if (ret > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                fprintf(stderr, "reading...\n");
                bzero(buffer, 524);
                n = recvfrom(sockfd, (char *)buffer, 512, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                fprintf(stderr, "1done reading: %i\n", n);
                if (n < 0 )
                {
                    fprintf(stderr, "1read error\n");
                    perror("WHY. \n");
                    continue;
                }
                if (n == 0)
                {
                    fprintf(stderr, "1got nothin\n");
                    continue;
                }
                buffer[n] = '\0';
                printRecv(buffer);
                lastRecvSeq = getSeq(buffer);
                lastRecvAck = getAck(buffer);
                fprintf(stderr, "what i just read: %s\n", buffer);
                if (buffer[11] != 'd')
                {
                    fprintf(stderr, "whatever i read was not a SYNACK\n");
                    continue;
                }
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
        fprintf(stderr, "diff: %lld\n", diff);
        if (diff > 500000000 && buffer[11] != 'd')
        {
            printf("TIMEOUT %i\n", number);
            char header[12] = {0};
            bzero(header, 12);
            memcpy(header, makeHeader(number, 0, 'b'), 12);
            sendto(sockfd, (const char *) header, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            clock_gettime(CLOCK_MONOTONIC, &start);
            currentSeqNum = number;
            currentAckNum = 0;
            printf("RESEND %i %i SYN\n", number, 0);
        }
        if (badPoll == 1 || ret == 0)
        {
            fprintf(stderr, "bad poll bool set\n");
            badPoll = 0;
            continue;
        }


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
            FILE *fp = fopen(fileName, "r");
            if (fp == NULL)
            {
                fprintf(stderr, "Unable to open requested file\n");
                exit(1);
            }
            int prev=ftell(fp);
            fseek(fp, 0L, SEEK_END);
            wholeSize=ftell(fp);
            fseek(fp,prev,SEEK_SET); //go back to where we were

            // playring round
            //fseek(fp,3,SEEK_SET);

            fprintf(stderr, "Whole file's size: %i\n", wholeSize);
            int contentLen = fread(content, sizeof(char), 512, fp);
            readIn += contentLen;

            char tempBuff[524] = {0};
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

            // simulate losing 3rd ack
            // commmented out wrong whoops try agian
            // if (useless == 0)
            //     useless = 1;
            // else
                sendto(sockfd, (const char *) tempBuff, tempBuffLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            //fprintf(stderr, "sent this: %s\n", tempBuff);
            printf("SEND %i %i ACK\n", intSeqNum, intAckNum);
            bzero(tempBuff, 524);
            //clock_gettime(CLOCK_MONOTONIC, &start);
            //fprintf(stderr, "start time: %zu\n", t1.tv_sec*1000);
            // JUST SENT FIRST DATA PACKET; START TIMER
            clock_gettime(CLOCK_MONOTONIC, &start2);


            currentSeqNum = currentSeqNum + contentLen;
            if (currentSeqNum > 25600)
            {
                currentSeqNum = currentSeqNum - 25600;
                roundNum++;
            }
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

                //fprintf(stderr, "more bytes to go boyz\n");
                bzero(content, 512);
                contentLen = fread(content, sizeof(char), 512, fp);
                readIn+=contentLen;
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
                // simulate losing second packet
                //fprintf(stderr, "planning on sending this bytes: %i : %s\n", tempBuffLen, tempBuff);
                // if (i == 2)
                //     fprintf(stderr, "pretending to lose: %i %i\n", currentSeqNum, currentAckNum);
                // else
                    sendto(sockfd, (const char *) tempBuff, tempBuffLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                printf("SEND %i %i\n", currentSeqNum, 0);
                currentSeqNum = currentSeqNum + contentLen;
                if (currentSeqNum > 25600)
                {
                    currentSeqNum = currentSeqNum - 25600;
                    roundNum++;
                }
            }

            //receive server's last message
            // yes things are actually received here wahoo
            // no longer initial packets area
            while(1)
            {
                // I WANT TO POLL HERE
                ret = poll(fds, 1, 500);
                if (ret > 0)
                {
                    if (fds[0].revents & POLLIN)
                    {
                        fprintf(stderr, "wantherereading...\n");
                        bzero(buffer, 524);
                        if (currentSeqNum +25600*roundNum - number - 1 < wholeSize)
                            {
                                n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                                lastlast = lastRecvSeq;
                                lastRecvSeq = getSeq(buffer);
                                lastlastAck = lastRecvAck;
                                lastRecvAck = getAck(buffer);
                                fprintf(stderr, "done reading: %i\n", n);
                                if (n < 0 )
                                {
                                    fprintf(stderr, "read error\n");
                                    continue;
                                }
                                if (n == 0)
                                {
                                    fprintf(stderr, "got nothin\n");
                                    continue;
                                }
                                printRecv(buffer);
                                fprintf(stderr, "what i just read: %s\n", buffer);
                            }
                            fprintf(stderr, "quien es?? \n");

                    }
                }

                /////  WORKS FOR NOW DO NOT DELETE /////


                // lastlast = lastRecvSeq;
                // bzero(buffer, 524);
                // fprintf(stderr, "receive here: curseqnum: %i\n", currentSeqNum);
                // if (currentSeqNum +25600*roundNum - number - 1 < wholeSize)
                // {
                //     n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                //     lastRecvSeq = getSeq(buffer);
                //     lastlastAck = lastRecvAck;
                //     lastRecvAck = getAck(buffer);
                // }
                // printf("helloo\n");
                // printRecv(buffer);
                // // if (getSeq(buffer) == currentSeqNum)
                // //     allAcksRecv = 1;
                // printf("hellimoo\n");
                // printf("read: %i\n", readIn);

                ///// ^ WORKS FOR NOW DO NOT DELETE /////


                // just received something, check timer
                clock_gettime(CLOCK_MONOTONIC, &end2);
                diff2 = BILLION * (end2.tv_sec - start2.tv_sec) + end2.tv_nsec - start2.tv_nsec;
                fprintf(stderr, "diff2: %lld\n", diff2);
                if (diff2 > 50000000 && buffer[11] == 'd')
                {
                    // probably lost cleint sending 3rd ack so
                    fprintf(stderr, "waananna poll here\n");
                    fprintf(stderr, "prolly lost third ack so\n");
                    fseek(fp,lastRecvSeq+roundNum*25600-number-1, SEEK_SET);
                    // reset sequence number
                    currentSeqNum = lastRecvSeq;
                    printf("TIMEOUT %i\n", currentSeqNum);
                    for (int i = 0; i < 10; i++)
                    {
                        // new schtufff
                        if (feof(fp))
                        {
                            fprintf(stderr, "initial window done boyyys \n");
                            break;
                        }

                        //fprintf(stderr, "more bytes to go boyz\n");
                        bzero(content, 512);
                        contentLen = fread(content, sizeof(char), 512, fp);
                        //readIn+=contentLen;
                        //fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
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
                        printf("RESEND %i %i\n", currentSeqNum, 0);
                        currentSeqNum = currentSeqNum + contentLen;
                        if (currentSeqNum > 25600)
                        {
                            currentSeqNum = currentSeqNum - 25600;
                            roundNum++;
                        }
                    }
                    clock_gettime(CLOCK_MONOTONIC, &start2);
                }
                if ( diff2 > 500000000 && getSeq(buffer) == lastlast) //timeout, getting same ACKs
                {
                    fprintf(stderr, "timer is over .5 sec\n");
                    fprintf(stderr, "last seq, roundnum: %i %i\n", lastRecvSeq, roundNum);
                    // reposition file pointer
                    fseek(fp,lastRecvSeq+roundNum*25600-number-1, SEEK_SET);
                    // reset sequence number
                    currentSeqNum = lastRecvSeq;
                    printf("TIMEOUT %i\n", currentSeqNum);
                    for (int i = 0; i < 10; i++)
                    {
                        // new schtufff
                        if (feof(fp))
                        {
                            fprintf(stderr, "initial window done boyyys \n");
                            break;
                        }

                        //fprintf(stderr, "more bytes to go boyz\n");
                        bzero(content, 512);
                        contentLen = fread(content, sizeof(char), 512, fp);
                        //readIn+=contentLen;
                        //fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
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
                        printf("RESEND %i %i\n", currentSeqNum, 0);
                        currentSeqNum = currentSeqNum + contentLen;
                        if (currentSeqNum > 25600)
                        {
                            currentSeqNum = currentSeqNum - 25600;
                            roundNum++;
                        }
                    }
                    clock_gettime(CLOCK_MONOTONIC, &start2);
                }
                // if (diff > 500000000 )
                // {
                //     fprintf(stderr, "hit limit\n");
                //     // too much time
                //     // retransmit packets
                //     // reset timer
                //     clock_gettime(CLOCK_MONOTONIC, &start);
                //     // reset file pointer
                //     fprintf(stderr, "last: %i\n", lastRecvAck);
                //     fseek(fp,lastRecvAck-number-1, SEEK_SET);
                //     currentSeqNum = lastRecvAck;
                //     printf("TIMEOUT %i\n", currentSeqNum);
                    // for (int i = 0; i < 10; i++)
                    // {
                    //     // new schtufff
                    //     if (feof(fp))
                    //     {
                    //         fprintf(stderr, "initial window done boyyys \n");
                    //         break;
                    //     }

                    //     //fprintf(stderr, "more bytes to go boyz\n");
                    //     bzero(content, 512);
                    //     contentLen = fread(content, sizeof(char), 512, fp);
                    //     readIn+=contentLen;
                    //     //fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
                    //     tempBuffLen = contentLen + 12;
                    //     bzero(header, 12);
                    //     memcpy(header, makeHeader(currentSeqNum, 0, 'n'), 12);
                    //     // new way
                    //     for (int i = 0; i < 12; i++)
                    //     {
                    //         tempBuff[i] = header[i];
                    //     }
                    //     for (int i = 0; i < contentLen; i++)
                    //     {
                    //         tempBuff[i+12] = content[i];
                    //     }
                    //     // send cmd
                    //     //fprintf(stderr, "planning on sending this bytes: %i : %s\n", tempBuffLen, tempBuff);
                    //     sendto(sockfd, (const char *) tempBuff, tempBuffLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                    //     printf("RESEND %i %i\n", currentSeqNum, 0);
                    //     currentSeqNum = currentSeqNum + contentLen;
                    //     if (currentSeqNum > 25600)
                    //     {
                    //         currentSeqNum = currentSeqNum - 25600;
                    //         roundNum++;
                    //     }
                    // }
                // }





                // bzero(buffer, 524);
                // ONLY SEND IF FRESH ACK
                if (!feof(fp) && buffer[11] == 'a' && lastRecvSeq!=lastlast)
                {
                    fprintf(stderr, "not end, got ack\n");
                    lastRecvSeq = getSeq(buffer);
                    if (lastRecvSeq != getSeq(buffer))
                    {
                        // got new ack, reset timer
                        fprintf(stderr, "reset timer\n");
                        clock_gettime(CLOCK_MONOTONIC, &start2);
                    }

                    //fprintf(stderr, "ohohoh we are not donee yet\n");
                    bzero(content, 512);
                    contentLen = fread(content, sizeof(char), 512, fp);
                    readIn += contentLen;
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
                    sendto(sockfd, (const char *) tempBuff, tempBuffLen, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                    printf("SEND %i %i\n", currentSeqNum, 0);
                    currentSeqNum = currentSeqNum + contentLen;
                    if (currentSeqNum > 25600)
                    {
                        currentSeqNum = currentSeqNum - 25600;
                        roundNum++;
                    }
                    /// TIMER SHOULD BE PUT HERE TOO///
                     ///// YOOYOYOYOYOYO //////
                }



                // prepping for fin
                else if (feof(fp)) //contentLen == wholeSize)
                {
                    int alreadySentFin = 0;
                    fprintf(stderr, "HELLO????\n");

                    // receive server's ack of fim
                    while(1)
                    {
                        //fprintf(stderr, "SIR???????\n");




                        ret = poll(fds, 1, 500);
                        if (ret > 0)
                        {
                            if (fds[0].revents & POLLIN)
                            {
                                fprintf(stderr, "1streading...\n");
                                bzero(buffer, 524);
                                n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                                lastlast = lastRecvSeq;
                                lastRecvSeq = getSeq(buffer);
                                lastRecvAck = getAck(buffer);
                                 fprintf(stderr, "quien es?? \n");
                                fprintf(stderr, "done reading: %i\n", n);
                                if (n < 0 )
                                {
                                    fprintf(stderr, "read error\n");
                                    continue;
                                }
                                if (n == 0)
                                {
                                    fprintf(stderr, "got nothin\n");
                                    continue;
                                }
                                printRecv(buffer);
                                fprintf(stderr, "what i just read: %s\n", buffer);
                            }
                        }
                        // HOOYAH

                        // bzero(buffer, 524);
                        // n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                        // lastlast = lastRecvSeq;
                        // lastRecvSeq = getSeq(buffer);
                        // fprintf(stderr, "quien es?? \n");
                        // printRecv(buffer);

                        // HOOYAH

                        // LOST PACKET///???
                        clock_gettime(CLOCK_MONOTONIC, &end2);

                        diff2 = BILLION * (end2.tv_sec - start2.tv_sec) + end2.tv_nsec - start2.tv_nsec;
                        fprintf(stderr, "diff2: %lld\n", diff2);
                        if (diff2 > 50000000 && buffer[11] == 'd')
                        {
                            // probably lost cleint sending 3rd ack so
                            fprintf(stderr, "DO I EVEN NEED THIS???\n");
                            fprintf(stderr, "prolly lost third ack so\n");
                            fseek(fp,lastRecvSeq+roundNum*25600-number-1, SEEK_SET);
                            // reset sequence number
                            currentSeqNum = lastRecvSeq;
                            printf("TIMEOUT %i\n", currentSeqNum);
                            for (int i = 0; i < 10; i++)
                            {
                                // new schtufff
                                if (feof(fp))
                                {
                                    fprintf(stderr, "initial window done boyyys \n");
                                    break;
                                }

                                //fprintf(stderr, "more bytes to go boyz\n");
                                bzero(content, 512);
                                contentLen = fread(content, sizeof(char), 512, fp);
                                //readIn+=contentLen;
                                //fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
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
                                printf("RESEND %i %i\n", currentSeqNum, 0);
                                currentSeqNum = currentSeqNum + contentLen;
                                if (currentSeqNum > 25600)
                                {
                                    currentSeqNum = currentSeqNum - 25600;
                                    roundNum++;
                                }
                            }
                            clock_gettime(CLOCK_MONOTONIC, &start2);
                        }
                        if ( diff2 > 500000000 && getSeq(buffer) == lastlast) //timeout, getting same ACKs
                        {
                            fprintf(stderr, "DO I EVEN NEED THIS???\n");
                            fprintf(stderr, "timer is over .5 sec\n");
                            fprintf(stderr, "last ack, roundnum: %i %i\n", lastRecvSeq, roundNum);
                            // reposition file pointer
                            fseek(fp,lastRecvSeq+roundNum*25600-number-1, SEEK_SET);
                            // reset sequence number
                            currentSeqNum = lastRecvSeq;
                            printf("TIMEOUT %i\n", currentSeqNum);
                            for (int i = 0; i < 10; i++)
                            {
                                // new schtufff
                                if (feof(fp))
                                {
                                    fprintf(stderr, "initial window done boyyys \n");
                                    break;
                                }

                                //fprintf(stderr, "more bytes to go boyz\n");
                                bzero(content, 512);
                                contentLen = fread(content, sizeof(char), 512, fp);
                                //readIn+=contentLen;
                                //fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
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
                                printf("RESEND %i %i\n", currentSeqNum, 0);
                                currentSeqNum = currentSeqNum + contentLen;
                                if (currentSeqNum > 25600)
                                {
                                    currentSeqNum = currentSeqNum - 25600;
                                    roundNum++;
                                }
                            }
                            clock_gettime(CLOCK_MONOTONIC, &start2);
                        }

                        // LOST PACKETN E;ASKDF
                        if (getSeq(buffer)+roundNum*512 - number - 1 == wholeSize)
                        {
                            fprintf(stderr, "set alr sent fin\n");
                            alreadySentFin = 1;
                        }
                        // send
                        char finheader[12];
                        bzero(finheader, 12);
                        memcpy(finheader, makeHeader(currentSeqNum+1, currentAckNum+1, 'a'), 12);

                        char finack[50] = {0};
                        char dupfinack[50] = {0};
                        sprintf(finack, "SEND %i %i ACK\n", currentSeqNum+1, currentAckNum+1);
                        sprintf(dupfinack, "SEND %i %i DUP-ACK\n", currentSeqNum+1, currentAckNum+1);

                        int llRecvSeqNum = 0;
                        while(1)
                        {
                            // fprintf(stderr, "LLOKING FOR FINNNNNNN\n");
                            fprintf(stderr, "ready set fin: %i\n", alreadySentFin);
                            if (alreadySentFin == 0)
                            {
                                ret = poll(fds, 1, 500);
                                if (ret > 0)
                                {
                                    if (fds[0].revents & POLLIN)
                                    {
                                        fprintf(stderr, "reading...\n");
                                        bzero(buffer, 524);
                                        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                                        lastlast = lastRecvSeq;
                                        lastRecvSeq = getSeq(buffer);
                                        lastRecvAck = getAck(buffer);
                                        fprintf(stderr, "DONE reading: %i\n", n);
                                        if (n < 0 )
                                        {
                                            fprintf(stderr, "read error\n");
                                            continue;
                                        }
                                        if (n == 0)
                                        {
                                            fprintf(stderr, "got nothin\n");
                                            continue;
                                        }
                                        printRecv(buffer);
                                        buffer[n] = '\0';
                                        fprintf(stderr, "what i just read: %s\n", buffer);
                                    }
                                }

                                // comment above hhmm,,,mm
                                // bzero(buffer, 524);
                                // n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                                // lastlast = lastRecvSeq;
                                // lastRecvSeq = getSeq(buffer);
                                // lastRecvAck = getAck(buffer);
                                // printRecv(buffer);

                                // LOST PACKET///???
                                clock_gettime(CLOCK_MONOTONIC, &end2);
                                gettimeofday(&t2, NULL);
                                double hey = t2.tv_sec;
                                fprintf(stderr, "t2: %g\n", hey);
                                diff2 = BILLION * (end2.tv_sec - start2.tv_sec) + end2.tv_nsec - start2.tv_nsec;
                                fprintf(stderr, "end2: %ld\n", end2.tv_sec);
                                fprintf(stderr, "diff2: %lld\n", diff2);
                                fprintf(stderr, "get seq num: %i\n", lastRecvSeq);
                                llRecvSeqNum = lastRecvSeq;
                                fprintf(stderr, "lastlast: %i\n", lastlast);
                                if (diff2 > 50000000 && buffer[11] == 'd')
                                {
                                    // probably lost cleint sending 3rd ack so
                                    fprintf(stderr, "prolly lost third ack so\n");
                                    fseek(fp,lastRecvSeq+roundNum*25600-number-1, SEEK_SET);
                                    // reset sequence number
                                    currentSeqNum = lastRecvSeq;
                                    printf("TIMEOUT %i\n", currentSeqNum);
                                    for (int i = 0; i < 10; i++)
                                    {
                                        // new schtufff
                                        if (feof(fp))
                                        {
                                            fprintf(stderr, "initial window done boyyys \n");
                                            break;
                                        }

                                        //fprintf(stderr, "more bytes to go boyz\n");
                                        bzero(content, 512);
                                        contentLen = fread(content, sizeof(char), 512, fp);
                                        //readIn+=contentLen;
                                        //fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
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
                                        printf("RESEND %i %i\n", currentSeqNum, 0);
                                        currentSeqNum = currentSeqNum + contentLen;
                                        if (currentSeqNum > 25600)
                                        {
                                            currentSeqNum = currentSeqNum - 25600;
                                            roundNum++;
                                        }
                                    }
                                    clock_gettime(CLOCK_MONOTONIC, &start2);
                                }
                                // second param is new
                                if ( diff2 > 500000000 && lastRecvSeq == lastlast || diff2>500000000 && lastRecvSeq == llRecvSeqNum ) //timeout, getting same ACKs
                                {
                                    fprintf(stderr, "timer is over .5 sec\n");
                                    fprintf(stderr, "last seq, roundnum: %i %i\n", lastRecvSeq, roundNum);
                                    // reposition file pointer
                                    fseek(fp,lastRecvSeq+roundNum*25600-number-1, SEEK_SET);
                                    // reset sequence number
                                    currentSeqNum = lastRecvSeq;
                                    printf("TIMEOUT %i\n", currentSeqNum);
                                    for (int i = 0; i < 10; i++)
                                    {
                                        // new schtufff
                                        if (feof(fp))
                                        {
                                            fprintf(stderr, "initial window done boyyys \n");
                                            break;
                                        }

                                        //fprintf(stderr, "more bytes to go boyz\n");
                                        bzero(content, 512);
                                        contentLen = fread(content, sizeof(char), 512, fp);
                                        //readIn+=contentLen;
                                        //fprintf(stderr, "second round: %i bytes: %s\n", contentLen, content);
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
                                        printf("RESEND %i %i\n", currentSeqNum, 0);
                                        currentSeqNum = currentSeqNum + contentLen;
                                        if (currentSeqNum > 25600)
                                        {
                                            currentSeqNum = currentSeqNum - 25600;
                                            roundNum++;
                                        }
                                    }
                                    clock_gettime(CLOCK_MONOTONIC, &start2);
                                }

                                // LOST PACKETN E;ASKDF
                                if ( diff2 > 500000000 && (lastRecvSeq+roundNum*25600-number-2==wholeSize || lastRecvSeq+roundNum*25600-number-1==wholeSize)) //timeout, got ack to client's fin so lost server's fin
                                {
                                    bzero(header, 12);
                                    memcpy(header, makeHeader(currentSeqNum, 0, 'c'), 12);
                                    fprintf(stderr, "plan on sending header: %s\n", header);
                                    sendto(sockfd, (const char *) header, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                                    // OG RESEND FIN
                                    printf("SEND %i 0 FIN\n", currentSeqNum);
                                    clock_gettime(CLOCK_MONOTONIC, &start2);
                                    resendFin = 1;
                                }

                            }
                            alreadySentFin = 0;
                            fprintf(stderr, "get seq, number, rounds, wholesize: %i, %i, %i, %i\n", getSeq(buffer), number, roundNum, wholeSize);
                            // if we receive the last ack, signal that all acks have been received
                            if (getSeq(buffer)+roundNum*25600 - number - 1 == wholeSize && resendFin == 0) // if everything received
                            {
                                fprintf(stderr, "suk end\n");
                                bzero(header, 12);
                                memcpy(header, makeHeader(currentSeqNum, 0, 'c'), 12);
                                fprintf(stderr, "plan on sending header: %s\n", header);
                                // simulate to lose fin
                                sendto(sockfd, (const char *) header, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                                printf("SEND %i 0 FIN\n", currentSeqNum);
                                // reset timer
                                clock_gettime(CLOCK_MONOTONIC, &start3);
                            }
                            if (buffer[11] == 'c') // if server sends a fin
                            {
                                //fprintf(stderr, "this wahat ai wasnna send: %s\n", finheader);


                                // simulate lose ack to fin
                                sendto(sockfd, (const char *) finheader, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                                clock_gettime(CLOCK_MONOTONIC, &start3);
                                printf("%s", finack);
                                clock_gettime(CLOCK_MONOTONIC, &end3);
                                diff3 = BILLION * (end3.tv_sec - start3.tv_sec) + end2.tv_nsec - start2.tv_nsec;
                                while(diff3 < 2 * BILLION)
                                {

                                    ret = poll(fds, 1, 500);
                                    if (ret > 0)
                                    {
                                        if (fds[0].revents & POLLIN)
                                        {
                                            fprintf(stderr, "reading...\n");
                                            bzero(buffer, 524);
                                            n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
                                            lastlast = lastRecvSeq;
                                            lastRecvSeq = getSeq(buffer);
                                            lastRecvAck = getAck(buffer);
                                            fprintf(stderr, "DONE reading: %i\n", n);
                                            if (n < 0 )
                                            {
                                                fprintf(stderr, "read error\n");
                                                continue;
                                            }
                                            if (n == 0)
                                            {
                                                fprintf(stderr, "got nothin\n");
                                                continue;
                                            }
                                            printRecv(buffer);
                                            buffer[n] = '\0';
                                            if (buffer[11] == 'c')//server sends second fin
                                            {
                                                sendto(sockfd, (const char *) finheader, 12, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                                                printf("%s", dupfinack);
                                                clock_gettime(CLOCK_MONOTONIC, &start3);
                                            }
                                        }
                                    }
                                    clock_gettime(CLOCK_MONOTONIC, &end3);
                                    diff3 = BILLION * (end3.tv_sec - start3.tv_sec) + end3.tv_nsec - start3.tv_nsec;
                                }
                                fflush(stdout);
                                fprintf(stderr, "ok bye server :'( \n");
                                exit(0);
                            }
                        }
                    }

                }
            }

        }
    }
    close(sockfd);
    return 0;
}

void printRecv(char* pbuffer)
{
    for (int i = 0; i < 12; i++)
        fprintf(stderr, "%c", pbuffer[i]);
    fprintf(stderr, "\n");
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