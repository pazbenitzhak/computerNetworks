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

#pragma comment(lib,"ws2_32.lib") /* Winsock library */

#define true 1

#define MESSAGE_LEN 31

#define NUM_OF_WORDS 26

int getBitHammString(int num, int index, int offset, char *buffer);
void flipBit(int position, int index, int offset, char *buff);
int calculateXors(int type, int index, int offset, char *buff);
int getBit(int position, int index, int offset, char *buff);

int main(int argc, char **argv)
{
    char buffer[31] = {0};
    FILE *fp;
    char fileName[20];
    int wsaRes;
    WSADATA wsaData;
    struct sockaddr_in receiverAddr;
    SOCKET socketFD;
    int bytesReceived;
    int totalBytesReceived;
    int errosFound;
    int wordsReceived;
    int hamOffset;
    int hamIndex;
    int wordIndex=0;
    int wordOffset=0;
    int i, j;
    int errorsCorrected;
    int packetsNum=0;
    int bytesWritten;
    if (argc != 3)
    {
        fprintf(stderr, "got %i arguments instead of 3\n", argc);
        exit(1);
    }
    wsaRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaRes != NO_ERROR)
    {
        printf("Error at WSAStartup()\n");
    }
    while (1)
    {

        if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            fprintf(stderr, "failed to initiate socket: %s\n", strerror(errno));
            exit(1);
        }
        memset(&receiverAddr, 0, sizeof(receiverAddr)); /* check whether important */
        receiverAddr.sin_family = AF_INET;
        receiverAddr.sin_port = htons(atoi(argv[2]));
        receiverAddr.sin_addr.s_addr = inet_addr(argv[1]);

        if (connect(socketFD, (struct sockaddr *)&receiverAddr, sizeof(receiverAddr)) < 0)
        {
            fprintf(stderr, "Error : Connect failed. %s \n", strerror(errno));
            exit(1);
        }
        printf("enter file name:\n");
        gets(fileName); /* getting file name, as long as length<=100 */
        if (strcmp(fileName, "quit") == 0)
        {
            break;
        }
        fp = fopen(fileName, "w"); /* open file to write */
        if (fp==NULL) {
            fprintf(stderr, "Error : Filename invalid. %s \n", strerror(errno));
            exit(1);
        }
        bytesReceived = 0;
        totalBytesReceived = 0;
        errosFound = 0;
        bytesWritten = 0;
        while (true)
        {
            bytesReceived = recv(socketFD, buffer, MESSAGE_LEN, 0);
            if (!bytesReceived) {
                break;
            }
            if (bytesReceived < 0)
            {
                fprintf(stderr, "Error : Receiving message failed. %s \n", strerror(errno));
                exit(1);
            }
            packetsNum++;
            totalBytesReceived += bytesReceived;
            hamOffset = 0;
            hamIndex = 26;
            /* from SENDER */
            for (i = 0; i < NUM_OF_WORDS; i++)
            {
                /* calculate x_i -> x_1, x_2, x_4, x_8, x_16 by XOR */
                /* we have x_i -> x_1, x_2, x_4, x_8, x_16 */
                int erroredIndex = 0;
                for (j = 0; j < 5; j++)
                {
                    int parity_check_j;
                    /* calculate x_i*/
                    parity_check_j = getBitHammString(j, hamIndex, hamOffset, buffer) ^ calculateXors(pow(2, j), wordIndex, wordOffset, buffer);
                    if (parity_check_j != getBitHammString(j, hamIndex, hamOffset, buffer))
                    {
                        erroredIndex += pow(2, j);
                    }
                }
                if ((erroredIndex % 2) == 0 || erroredIndex == 1)
                { /* erroredIndex is parity bit */
                    if (erroredIndex != 0)
                    { /* if zero than no error detected */
                        errorsCorrected++;
                    }
                    /*if fliped bit is parity_bit -> no use in flipping it, since reciever doesn't write to file the parity section */
                }
                else
                { /* not parity, and there is an error */
                    flipBit(erroredIndex, wordIndex, wordOffset, buffer);
                    errorsCorrected++;
                }

                wordIndex = wordIndex + 26 / 8;     /* whole division */
                wordOffset = (wordOffset + 26) % 8; /* modulo */
                hamIndex = hamIndex + 5 / 8;
                hamOffset = (hamOffset + 5) % 8;
            }
            fwrite(buffer, 1, NUM_OF_WORDS, fp); /* need to write wordsReceived*26/8 bytes, since number of bits to write is wordsReceived*26 */
        }
        fclose(fp);
        printf("received: %i bytes\n",totalBytesReceived);
        printf("wrote: %i bytes\n",packetsNum*NUM_OF_WORDS);
        printf("corrected %i errors\n",errorsCorrected);
    }

    /* end of while */
    WSACleanup();
}

int getBitHammString(int num, int index, int offset, char *buffer)
{
    int newOffset;
    int newIndex;
    newOffset = (offset + num) % 8;        /*what position in new bucket are we */
    newIndex = index + (offset + num) / 8; /* how many buckets should we move */

    return (buffer[newIndex] >> 7 - newOffset) & 1;
}

void flipBit(int position, int index, int offset, char *buff)
{
    int newIndex;
    int newOffset;
    int num;
    /* transfer accodring to difference with how many parity bits up to current index+1 */
    if (position == 3)
    {
        num = 0;
    }
    if (position > 4 && position < 8)
    {
        num = position - 4;
    }
    if (position > 8 && position < 16)
    {
        num = position - 5;
    }
    if (position > 16)
    {
        num = position - 6;
    }
    newOffset = (offset + num) % 8;        /*what position in new bucket are we */
    newIndex = index + (offset + num) / 8; /* how many buckets should we move */

    buff[index] ^= 1 << (7 - offset); /* xor with 1 flips the bit */
}

int calculateXors(int type, int index, int offset, char *buff)
{
    int xor_value;
    switch (type)
    {
    case 1:
        xor_value = getBit(3, index, offset, buff) ^ getBit(5, index, offset, buff) ^ getBit(7, index, offset, buff) ^ getBit(9, index, offset, buff) ^
                    getBit(11, index, offset, buff) ^ getBit(13, index, offset, buff) ^ getBit(15, index, offset, buff) ^ getBit(17, index, offset, buff) ^ getBit(19, index, offset, buff) ^
                    getBit(21, index, offset, buff) ^ getBit(23, index, offset, buff) ^ getBit(25, index, offset, buff) ^ getBit(27, index, offset, buff) ^ getBit(29, index, offset, buff) ^ getBit(31, index, offset, buff);
        return xor_value;
        break;
    case 2:
        xor_value = getBit(3, index, offset, buff) ^ getBit(6, index, offset, buff) ^ getBit(7, index, offset, buff) ^ getBit(10, index, offset, buff) ^
                    getBit(11, index, offset, buff) ^ getBit(14, index, offset, buff) ^ getBit(15, index, offset, buff) ^ getBit(18, index, offset, buff) ^ getBit(19, index, offset, buff) ^
                    getBit(22, index, offset, buff) ^ getBit(23, index, offset, buff) ^ getBit(26, index, offset, buff) ^ getBit(27, index, offset, buff) ^ getBit(30, index, offset, buff) ^ getBit(31, index, offset, buff);
        return xor_value;
        break;
    case 4:
        xor_value = getBit(5, index, offset, buff) ^ getBit(6, index, offset, buff) ^ getBit(7, index, offset, buff) ^ getBit(12, index, offset, buff) ^
                    getBit(13, index, offset, buff) ^ getBit(14, index, offset, buff) ^ getBit(15, index, offset, buff) ^ getBit(20, index, offset, buff) ^ getBit(21, index, offset, buff) ^
                    getBit(22, index, offset, buff) ^ getBit(23, index, offset, buff) ^ getBit(28, index, offset, buff) ^ getBit(29, index, offset, buff) ^ getBit(30, index, offset, buff) ^ getBit(31, index, offset, buff);
        return xor_value;
        break;
    case 8:
        xor_value = getBit(9, index, offset, buff) ^ getBit(10, index, offset, buff) ^ getBit(11, index, offset, buff) ^ getBit(12, index, offset, buff) ^
                    getBit(13, index, offset, buff) ^ getBit(14, index, offset, buff) ^ getBit(15, index, offset, buff) ^ getBit(24, index, offset, buff) ^
                    getBit(25, index, offset, buff) ^ getBit(26, index, offset, buff) ^ getBit(27, index, offset, buff) ^ getBit(28, index, offset, buff) ^ getBit(29, index, offset, buff) ^ getBit(30, index, offset, buff) ^ getBit(31, index, offset, buff);
        return xor_value;
        break;
    case 16:
        xor_value = getBit(17, index, offset, buff) ^ getBit(18, index, offset, buff) ^ getBit(19, index, offset, buff) ^ getBit(20, index, offset, buff) ^
                    getBit(21, index, offset, buff) ^ getBit(22, index, offset, buff) ^ getBit(23, index, offset, buff) ^ getBit(24, index, offset, buff) ^ getBit(25, index, offset, buff) ^
                    getBit(26, index, offset, buff) ^ getBit(27, index, offset, buff) ^ getBit(28, index, offset, buff) ^ getBit(29, index, offset, buff) ^ getBit(30, index, offset, buff) ^ getBit(31, index, offset, buff);
        return xor_value;
        break;

    default:
        return 0;
    }
    return 0;
}

int getBit(int position, int index, int offset, char *buff)
{
    int newIndex;
    int newOffset;
    int num;
    /* transfer accodring to difference with how many parity bits up to current index+1 */
    if (position == 3)
    {
        num = 0;
    }
    if (position > 4 && position < 8)
    {
        num = position - 4;
    }
    if (position > 8 && position < 16)
    {
        num = position - 5;
    }
    if (position > 16)
    {
        num = position - 6;
    }
    newOffset = (offset + num) % 8;        /*what position in new bucket are we */
    newIndex = index + (offset + num) / 8; /* how many buckets should we move */

    return (buff[newIndex] >> 7 - newOffset) & 1;
}