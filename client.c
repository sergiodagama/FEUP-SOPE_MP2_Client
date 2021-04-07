#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

#define ERROR -1

int main(int argc, char* argv[]){

    //getting initial time of program execution
    time_t initial_time;
    time(&initial_time);

    //checking if the coreect number of arguments was given
    if(argc != 3){
        printf("Error! Not enough arguments given\n-> please enter input in the format: c <-t nsecs> fifoname\n");
        return ERROR;
    }

    time_t t, cur_secs; //used to set the srand function, and get current seconds in main thread loop, respectively

    //setting seed for thread random creation
    srand((unsigned) time(&t));
    
    //ARG 1: nº of seconds which the program should run in
    int nsecs = atoi(argv[1]);  

    //ARG 2: FIFO file path (relative or absolute)
    char * fifo_path = argv[2];

    //stores the pseudo random time between thread creation
    double rand_mili;

    //main thread (c0) continously creates new threads with server requests
    while(true)
    {
        //setting random miliseconds value and pausing for that amount of time
        //rand_mili = (rand() % 10 + 1) * 1000000;  //converting (1 - 10) miliseconds to nanoseconds
        //nanosleep((const struct timespec[]){{0, rand_mili}}, NULL);
        nanosleep((const struct timespec[]){{0, 10000000000L}}, NULL);
        printf("time\n");

        //creating thread
        //printf("created thread\n");

        //checking if nsecs have passed already
        time(&cur_secs);

        if (cur_secs - initial_time >= nsecs){
            //clean threads memory and exit everything here
            printf("Execution time has ended!\n");
            break;
        }
        
    }
    

    //example on the use of named pipes
    /* 
    //opening fifo file and retrieving file descriptor
    int fd = open(fifo_path, O_WRONLY);

    //checking for errors while opening fifo file
    if(fd == ERROR){ 
        printf("Error while opening fifo file!\n");
        return 1;
    }

    //closing fifo file
    close(fd);
    */

    return 0;
}
