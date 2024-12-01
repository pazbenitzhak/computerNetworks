#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <WinSock2.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib") /* Winsock library */

#define true 1
#define port 53

char* dnsAddr;
static int id;

/* functions declarations */
int isUrlValid(char* url);
int checkChar(char* url, int firstIndex, int lastIndex);
char* setUpAddress(char* address);
struct hostent* dnsQuery(char* address);

int main(int argc, char** argv) {
    char domain[100] = {0};
    int isValid;
    struct hostent* queryResult;
    char* IP;
    int i;
    int octet;
    if (argc!=2) {
        printf("Error: Number of argumets is invalid\n");
        exit(1);
    }
    dnsAddr = argv[1];
    id = 1;
    while(true) {
        printf("nsclient>");
        scanf("%s",domain);
        if (strcmp(domain,"quit")==0) {
            break; /* so we exit the program */
        }
        /* domain isn't quit */
        isValid = isUrlValid(domain);
        if (!isValid) {
            printf("ERROR: BAD NAME\n"); /*bad name */
            continue;
        }
        /* valid url, going to dnsQuery func */
        queryResult = dnsQuery(domain);
        if (queryResult==NULL) {
            printf("ERROR: NONEXISTENT\n"); /* non-existent */
            continue;
        }
        IP = queryResult->h_addr_list[0];
        for (i=0;i<4;i++) {
            octet = (int) IP[i];
            if (octet<0) {
                octet+=256;
            }
            if (i==3) {
                printf("%i\n",octet);
            }
            printf("%i.",octet);
        }
        /* else: print IP result */
    }
    WSACleanup();



}



int isUrlValid(char* url) {
    /* url includes empty char at the end */
    int len;
    int i;
    int firstIndex=0;
    int lastIndex=0;
    int isRangeValid=0;
    len = (int) strlen(url);
    if ((url[0]=='-') || (url[len-1]=='-') || (url[0]=='.') || (url[len-1]=='.')) { /* breaks rule no. 3 */
        return 0;
    }
    for (i=0;i<len;i++) {
        if (url[i]=='.') {
            if ((i!=len-1) && (url[i+1]=='.')) {/* handle .. cases */
                return 0;
            }
            lastIndex = i-1;
            /* we have a part */
            if (lastIndex-firstIndex+1>=64 && lastIndex-firstIndex==-1) { /* -1 because of 'swap' -> 
            ..' case */
                return 0; /* invalid */
            }
            isRangeValid = checkChar(url, firstIndex, lastIndex);
            if (!isRangeValid) {
                return 0;
            }
            firstIndex = i+1;
        }
    }
    lastIndex = len-1;
    if (!((lastIndex-firstIndex+1>=2) && (lastIndex-firstIndex+1<=6))) {
        return 0; /* invalid */
        }
    isRangeValid = checkChar(url, firstIndex, lastIndex);
    if (!isRangeValid) {
        return 0;
        }
    return 1;
    
}


int checkChar(char* url, int firstIndex, int lastIndex) {
    int index;
    for (index = firstIndex;index<=lastIndex;index++) {
        if (!((url[index]>=48 && url[index]<=57) || (url[index]>=65 && url[index]<=90) || (url[index]>=97 && url[index]<=122) || url[index]==45)) {
            return 0; /* NOT on valid values */
        }
    }
    return 1;
}

char* setUpAddress(char* address) {
    int origLen;
    int newLen;
    char* newAddress;
    int i,j;
    int lastIndex=0;
    int firstIndex=0;
    char octetLen;
    origLen = (int) strlen(address);
    newLen = origLen+1+1; /* for zero byte at the end */
    newAddress = (char*) calloc(newLen,sizeof(char));
    if (!newAddress) {
        perror("ERROR: Memory issues\n");
        exit(1);
    }
    for (i=0;i<origLen;i++) {
        if (address[i]=='.') {
            lastIndex = i-1;
            octetLen = (char) lastIndex-firstIndex+1; /* NO NEED in+48 because of ASCII */
            newAddress[firstIndex] = octetLen;
            for (j=firstIndex+1;j<=i;j++) {
                newAddress[j] = address[j-1];
            }
            firstIndex = i+1;
        }
    }
    lastIndex = origLen-1;
    octetLen = lastIndex-firstIndex+1;
    newAddress[firstIndex] = octetLen;
    for (j=firstIndex+1;j<=origLen;j++) {
                newAddress[j] = address[j-1];
        }
    /* last index of newAddress is zero byte - already initiated by calloc */
    newAddress[newLen-1] = 0;
    return newAddress;

}


struct hostent* dnsQuery(char* address) {
    /* accepts a NULL-terminated string as an argument,
and returns a pointer to a static struct hostent. */
    WSADATA wsaData;
    SOCKET socketFD;
    struct hostent* h;
    struct sockaddr_in servaddress;
    DWORD timeout;
    char* message;
    char* addressInFormat;
    char buffer[4096];
    int res;
    int messageLen;
    int i;
    int recLen;
    char* ipAddress;
    int senderr, recverr;
    uint8_t rcodeVal;
    int answerLen;
    res = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (res != NO_ERROR){
        printf("Error at WSAStartup()\n"); }
    if ((socketFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0) /* DGRAM for UDP */
        {
            fprintf(stderr, "failed to initiate socket: %s\n", strerror(errno));
            exit(1);
        }
    memset(&servaddress, 0, sizeof(servaddress));
    // Filling server information 
    servaddress.sin_family = AF_INET; 
    servaddress.sin_port = htons(port); 
    servaddress.sin_addr.s_addr = inet_addr(dnsAddr);

    timeout = 2*1000;
    if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout,
                sizeof(timeout)) < 0){
        perror("setsockopt failed\n");
        exit(1);
    }
    addressInFormat = setUpAddress(address);
    messageLen = 16+(int)strlen(address)+2; /* (6*16 from header + 2*16 from query)/sizeof(char) + len of addressInFormat - need to add 2*/
    message = (char* ) calloc(messageLen,sizeof(char));
    if (!message) {
        perror("ERROR: Memory issues\n");
        exit(1);
    }
    for (int i=0;i<messageLen;i++) {
        message[i] = (unsigned char) 0;
    }
    /* keep message[0] all zeros */
    message[1] = id;
    message[2] = 1;
    id++;
    if (id==pow(2,8)) {
        id=0;
    }
    message[5]= 1; /* QDCOUNT=1, zeros up to index 11*/
    for (i=12;i<messageLen-4;i++) { /* QNAME */
        message[i] = addressInFormat[i-12];
    }
    message[messageLen-3] = 1; /* QTYPE = 1 */
    message[messageLen-1] = 1; /* QCLASS = INTERNET */
    senderr = sendto(socketFD, (const char *)message, messageLen, 
        0, (const struct sockaddr *) &servaddress,  
            sizeof(servaddress));
    if (senderr==SOCKET_ERROR){
        printf("Error at sendto()\n");
        exit(1);}
    

    recLen = sizeof(servaddress);
    recverr = recvfrom(socketFD, (char*)buffer, 4096,  
                0, (struct sockaddr*) &servaddress, 
                &recLen);
    if (recverr==EWOULDBLOCK) {
        printf("2 seconds have passed with no response\n");
        return NULL;
    }
    if (recverr==SOCKET_ERROR){
        printf("Error at recvfrom()\n");
        exit(1); 
    }
    answerLen = recverr;
    rcodeVal = buffer[3];
    rcodeVal = rcodeVal & 0x0f;
    if ((rcodeVal==1) || (rcodeVal==2) ||(rcodeVal==3)||(rcodeVal==4)||(rcodeVal==5)) {
        return NULL;
    }
    /* copy IP address */
    ipAddress = (char*) calloc(4,sizeof(char));
    for (i=4;i>0;i--) {
        ipAddress[4-i] = buffer[answerLen-i];
    }
    h=(struct hostent *)calloc(1,sizeof(struct hostent));
    if (!h) {
        printf("Error at allocating memory for h\n");
        exit(1); 
    }
    h->h_addrtype = AF_INET;
    h->h_length = 4;
    h->h_addr_list = (char** ) calloc(2,sizeof(char*));
    if (!h->h_addr_list) {
        printf("Error at allocating memory for h->addr_list\n");
        exit(1); 
    }
    h->h_addr_list[0] = (char*) calloc(4,sizeof(char));
    if (!h->h_addr_list[0]) {
        printf("Error at allocating memory for h->addr_list[0]\n");
        exit(1); 
    }
    memcpy(h->h_addr_list[0], ipAddress, 4); /* copy IP address to structure */
    closesocket(socketFD);
    return h;
}
