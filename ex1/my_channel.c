#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <WinSock2.h>
#include <stdio.h>
#include <errno.h>
/*#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>*/
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib") /* Winsock library */

#define true 1

#define MESSAGE_LEN 31

void flipBit(int index, int offset, char *buff);

int main(int argc, char **argv)
{
    char buffer[MESSAGE_LEN] = {0};
    int wordIndex;
    int wordOffset;
    int hamIndex;
    int hamOffset;
    int totalBytesRead;
    FILE *fp;
    char answer[100];
    int wsaRes;
    WSADATA wsaData;
    struct sockaddr_in channelAddrS;
    struct sockaddr_in channelAddrR;
    SOCKET socketFDS; /* sender*/
    SOCKET socketFDStag;
    SOCKET socketFDR; /* receiver*/
    SOCKET socketFDRtag;
    int i, j;
    int bytesRead = 0;
    int wordsRead;
    int packetsNum;
    int bytesReceived;
    int isDet; /* is flag deterministic */
    int n;
    int seed;
    int pick;
    int bitsNum; /* indicates what is the bits' index is inside the word, meaning the i'th bit need to be flipped*/
    int bitsFlipped;
    int totalBytesReceived;
    int packetsCounter;
    int stepsToPacket = 0;
    int bitIndex = 0;
    int nextPacket;
    int currPacket = 0;
    char hostNameS[1024];
    char hostNameR[1024];
    char* IPS;
    char* IPR;
    struct hostent* hostS;
    struct hostent* hostR;
    int sizeS;
    int sizeR;
    if (argc != 3 & argc != 4)
    {
        fprintf(stderr, "got %i arguments instead of 3\n", argc);
        exit(1);
    }
    if (strcmp(argv[1], "-d") == 0)
    {
        isDet = 1; /* 1 for det, 0 for random */
    }
    else
    {
        isDet = 0;
        seed = atoi(argv[3]);
    }
    n = atoi(argv[2]); /* anyway */
    wsaRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaRes != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
    }
    if ((socketFDS = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "failed to initiate socket: %s\n", strerror(errno));
        exit(1);
    }
    if ((socketFDR = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "failed to initiate socket: %s\n", strerror(errno));
        exit(1);
    }
    /*memset(&channelAddrS, 0, sizeof(channelAddrS)); *//* check whether important */
    channelAddrS.sin_family = AF_INET;
    channelAddrS.sin_port = htons( 0 );
    channelAddrS.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketFDS, (struct sockaddr *)&channelAddrS, sizeof(channelAddrS)) < 0)
    {
        fprintf(stderr, "Error in binding with sender: %s\n", strerror(errno));
        exit(1);
    }
    sizeS = sizeof(channelAddrS);
    getsockname(socketFDS, (struct sockaddr*)&channelAddrS, &sizeS);
    gethostname(hostNameS, 1024);
    hostS = gethostbyname(hostNameS);
    IPS = inet_ntoa(*((struct in_addr*)hostS->h_addr_list[0]));
    printf("sender socket: IP address = %s port = %d\n", IPS, ntohs(channelAddrS.sin_port));

    channelAddrR.sin_family = AF_INET;
    channelAddrR.sin_port = htons( 0 );
    channelAddrR.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socketFDR, (struct sockaddr *)&channelAddrR, sizeof(channelAddrR)) < 0)
    {
        fprintf(stderr, "Error in binding with receiver: %s\n", strerror(errno));
        exit(1);
    }
    sizeR = sizeof(channelAddrR);
    getsockname(socketFDR, (struct sockaddr*) &channelAddrR, &sizeR);
    gethostname(hostNameR, 1024);
    hostR = gethostbyname(hostNameR);
    IPR = inet_ntoa(*((struct in_addr*)hostR->h_addr_list[0]));
    printf("receiver socket: IP address = %s port = %d\n", IPR, ntohs(channelAddrR.sin_port));
    if (listen(socketFDS, 10) < 0)
    {
        fprintf(stderr, "Error, listen to sender failed: %s\n", strerror(errno));
        exit(1);
    }
    if (listen(socketFDR, 10) < 0)
    {
        fprintf(stderr, "Error, listen to receiver failed: %s\n", strerror(errno));
        exit(1);
    }
    while (true)
    {
        socketFDStag = accept(socketFDS, NULL, NULL);
        if (socketFDStag < 0)
        {
            fprintf(stderr, "Error, accept failed: %s\n", strerror(errno));
            exit(1);
        }
        socketFDRtag = accept(socketFDR, NULL, NULL);
        if (socketFDRtag < 0)
        {
            fprintf(stderr, "Error, accept failed: %s\n", strerror(errno));
            exit(1);
        }
        bitsNum = -1;
        totalBytesReceived = 0;
        while (true)
        {
            printf("in while loop \n");
            bytesReceived = recv(socketFDStag, buffer, MESSAGE_LEN, 0);
            if (!bytesReceived) { /* bytesReceived==0 */
                break;
            }
            if (bytesReceived < 0)
            {
                fprintf(stderr, "Error : Receiving message failed. %s \n", strerror(errno));
                exit(1);
            }
            totalBytesReceived += bytesReceived;
            printf("bytes received: %i\n", bytesReceived);
            printf("buffer: %s\n",buffer);
            if (isDet)
            { /* deterministic case */
                if (n != 0)
                {
                    if (n <= MESSAGE_LEN * 8)
                    {
                        while (bitsNum < MESSAGE_LEN * 8)
                        {
                            bitsNum += n;
                            wordIndex = (bitsNum) / 8;
                            wordOffset = (bitsNum) % 8;
                            flipBit(wordIndex, wordOffset, buffer); /* flip the bit */
                            bitsFlipped++;
                        }
                        bitsNum -= (8 * MESSAGE_LEN + n); /* update as needed, n is decremented because of incrementation at the beginning of while loop */
                    }

                    else
                    { /* n>MESSAGE_LEN*8 */
                        if (stepsToPacket > 1)
                        {
                            stepsToPacket--;
                        }
                        else
                        {
                            if (stepsToPacket)
                            { /* stepsToPacket =1 */
                                wordIndex = (bitsNum) / 8;
                                wordOffset = (bitsNum) % 8;
                                flipBit(wordIndex, wordOffset, buffer); /* flip the bit */
                                bitsFlipped++;
                            }
                            bitIndex += n;
                            bitsNum = bitIndex % (8 * MESSAGE_LEN);
                            nextPacket = bitIndex / (8 * MESSAGE_LEN);
                            stepsToPacket = nextPacket - currPacket;
                            currPacket = nextPacket;
                        }
                    }
                }
            }

            else
            { /* random case */
                srand(seed);
                wordIndex = 0;
                wordOffset = 0;
                for (i = 0; i < 8 * bytesRead; i++)
                {
                    pick = rand();
                    if (pick <= n / 2)
                    {
                        flipBit(wordIndex, wordOffset, buffer);
                        bitsFlipped++;
                    }
                    wordOffset++;
                    if (wordOffset == 8)
                    {
                        wordOffset = 0;
                        wordIndex++;
                    }
                }
            }

            if (send(socketFDRtag, buffer, MESSAGE_LEN, 0) < 0)
            {
                fprintf(stderr, "Error : Receiving message failed. %s \n", strerror(errno));
                exit(1);
            }
        }
        /* after sending all information */
        if (closesocket(socketFDRtag) < 0)
        {
            fprintf(stderr, "Error : Socket closure failed. %s \n", strerror(errno));
            exit(1);
        }

        printf("retransmitted %i bytes, flipped %i bits\n", totalBytesReceived, bitsFlipped);
        printf("continue? (yes/no)\n");
        gets(answer); /* getting file name, as long as length<=100 */

        if (strcmp(answer, "no") == 0)
        {
            break;
        }
        if (strcmp(answer, "yes") != 0)
        {
            break;
        }
    }
    WSACleanup();
}

void flipBit(int index, int offset, char *buff)
{
    buff[index] ^= 1 << (7 - offset); /* xor with 1 flips the bit */
}