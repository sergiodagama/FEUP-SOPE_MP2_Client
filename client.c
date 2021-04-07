#include<stdio.h>
#include <stdlib.h>

#define ERROR -1

int main(int argc, char* argv[]){

    if(argc != 3) printf("Error! Not enough arguments given\n-> please enter input in the format: c <-t nsecs> fifoname\n");

    //ARG 1: nº of seconds which the program should run in
    int nsecs = atoi(argv[1]);  

    //ARG 2: FIFO file path (relative or absolute)
    char * fifo_path = argv[2];

    //opening fifo file and retrieving file descriptor
    int fd = open(fifo_path, O_WRONLY);

    //checking for errors while opening fifo file
    if(fd == ERROR){ 
        pritnf("Error while opening fifo file!\n");
        return 1;
    }

    //closing fifo file
    close(fd);

    return 0;
}