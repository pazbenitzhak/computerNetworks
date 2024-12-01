#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
    FILE* fp;
    fp = fopen("output_test.txt","w");
    fputc('5', fp);
}