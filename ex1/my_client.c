

#include <WinSock2.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>

#define MESSAGE_LEN 31
#define true 1
#define BYTES_TO_READ 26

int main(int argc, char **argv)
{
    char buffer[31] = {0};
    int wordIndex;
    int wordOffset;
    int hamIndex;
    int hamOffset;
    int totalBytesRead;
    FILE *fp;
    char fileName[100];
    int wsaRes;
    WSADATA wsaData;
    struct sockaddr_in clientAddr;
    int socketFD;
    int i, j;
    int bytesRead;
    int wordsRead;
    int packetsNum;
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
    while (true)
    {
        if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            fprintf(stderr, "failed to initiate socket: %s\n", strerror(errno));
            exit(1);
        }
        memset(&clientAddr, 0, sizeof(clientAddr)); /* check whether important */
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port = htons(argv[2]);
        clientAddr.sin_addr.s_addr = inet_addr(argv[1]);

        if (connect(socketFD, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0)
        {
            fprintf(stderr, "Error : Connect failed. %s \n", strerror(errno));
            exit(1);
        }
        /* now it's supposed to be connected with server */
        printf("enter file name:\n");
        gets(fileName); /* getting file name, as long as length<=100 */
        if (strcmp(fileName, "quit") != 0)
        {
            break;
        }
        fp = fopen(fileName, "r"); /* open file for reading */
        totalBytesRead = 0;
        packetsNum = 0;
        while (bytesRead = read(fp, buffer, BYTES_TO_READ) != 0)
        {
            wordsRead = (bytesRead * 8) / 26;
            hamOffset = 0;
            hamIndex = 26;
            totalBytesRead += bytesRead;
            packetsNum++;
            for (i = 0; i < wordsRead; i++)
            {
                /* calculate x_i -> x_1, x_2, x_4, x_8, x_16 by XOR */
                /* we have x_i -> x_1, x_2, x_4, x_8, x_16 */
                for (j = 0; j < 5; j++)
                {
                    int x_i;
                    /* calculate x_i*/
                    x_i = calculateXors(pow(2, j), wordIndex, wordOffset, buffer);
                    if (x_i == 1)
                    {
                        setBit(hamOffset, hamIndex, buffer);
                    }
                    hamOffset++;
                    if (hamOffset == 8)
                    {
                        hamIndex++;
                        hamOffset = 0;
                    }
                }
                wordIndex = wordIndex + 26 / 8;     /* whole division */
                wordOffset = (wordOffset + 26) % 8; /* modulo */
            }
            /*if (bytesRead < BYTES_TO_READ)
            {
                buff[MESSAGE_LEN - 1] = '/0'; /* TODO:check what the correct index is */
            /*}*/
            /* right now we have encoded into the buffer all of the words read */
            if (send(socketFD, buffer, MESSAGE_LEN, 0)<0) {
                fprintf(stderr, "Error : Receiving message failed. %s \n", strerror(errno));
                exit(1);
            }
        }

        if (closesocket(socketFD) < 0)  
        {
            fprintf(stderr, "Error : Socket closure failed. %s \n", strerror(errno));
            exit(1);
        }
        close(fp);
        printf("file length: %i bytes\n", totalBytesRead);
        printf("sent: %i bytes\n", packetsNum * MESSAGE_LEN);
    }
    WSACleanup();
    /* end run*/
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

void setBit(int offset, int index, char *buff)
{
    buff[index] |= 1 << (7 - offset); /* or with 1 makes the bit '1' */
}

