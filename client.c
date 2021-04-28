/**
 * @file client.c
 * @brief Client side of a local client-server system, implemented using named pipes
 * @version 0.1
 * @date 2021-04-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define ERROR -1
#define CLIENT_WAITING_TIMEOUT 5
#define MILLISEC_TO_NANOSEC_MULTIPLIER 1000000  //1 millisecond = 1,000,000 nanoseconds
#define IWANT "IWANT"
//#define RECVD "RECVD"
//#define TSKEX "TSKEX"
//#define TSKDN "TSKDN"
#define GOTRS "GOTRS"
//#define TLATE "2LATE"
#define CLOSD "CLOSD"
#define GAVUP "GAVUP"
//#define FAILD "FAILD"

typedef struct message { int rid; pid_t pid; pthread_t tid; int tskload; int tskres; } message_t;

/*-------------------------GLOBAL VARIABLES-------------------------*/

pthread_mutex_t lock; //structure used to implement mutex

char * public_fifo_path;  //public fifo file path

bool public_fifo_closed = false; //check if public fifo was closed on the read side (server)

bool time_is_up = false; //keep track of execution time

bool debug = false;  //controls if console printfs are on or off

time_t initial_time;  //holds initial time of program execution

int nsecs;  //number of seconds inputted by user

/*-----------------------END GLOBAL VARIABLES-----------------------*/


/*-------------------------UTIL FUNCTIONS-------------------------*/

/**
 * @brief register the program logs in format "inst ; i ; t ; pid ; tid ; res ; oper\n" to stdout
 * 
 * @param i the unique identifier of thread request
 * @param t random value between 1-9
 * @param pid process id
 * @param tid thread id
 * @param res res from client or server
 * @param oper the operation that is being registered
 */
void reg(char *i, char *t, char *pid, char  *tid, char *res, char *oper){
    time_t inst;
    time(&inst);

    char reg_string[160], inst_s[20];

    sprintf(inst_s, "%ld", inst);
    strcat(reg_string, inst_s);
    strcat(reg_string, " ; ");
    strcat(reg_string, i);
    strcat(reg_string, " ; ");
    strcat(reg_string, t);
    strcat(reg_string, " ; ");
    strcat(reg_string, pid);
    strcat(reg_string, " ; ");
    strcat(reg_string, tid);
    strcat(reg_string, " ; ");
    strcat(reg_string, res);
    strcat(reg_string, " ; ");
    strcat(reg_string, oper);
    strcat(reg_string, "\n");

    fprintf(stdout, reg_string);
}

/**
 * @brief handles the response that it is give by the server
 * 
 * @param message the message struct sent by server
 * @param i_s the client unique identifier
 * @param t_s the number of the task
 * @param process_id_s the clinet process id
 * @param thread_id_s the client thread id
 */
void handle_server_response(message_t message, char * i_s, char * t_s, char * process_id_s, char * thread_id_s){
    char result[80];
    sprintf(result, "%d", message.tskres);
    
    if(message.tskres == -1){
        fprintf(stderr, "[client thread] The request wasn't attended, because server service is closed\n");
        reg(i_s, t_s, process_id_s, thread_id_s, "-1", CLOSD);
    }
    else{
        reg(i_s, t_s, process_id_s, thread_id_s, result, GOTRS);
    }
}

/**
 * @brief stops mutex and closes thread
 * 
 */
void close_client_inner_thread(){
    pthread_mutex_unlock(&lock);
    pthread_exit((void*) 0);
}

/**
 * @brief sends requests to server through client's threads
 * 
 * @param arg the i from the lopp, part of unique identifier of thread
 * @return void* 
 */
void *send_request_and_wait_response(void * arg)
{
    //locking code wiht mutex
    pthread_mutex_lock(&lock);

    //opening public fifo and checking if server has created it, if not waits for CLIENT_WAITING_TIMEOUT
    //int timeout = 0;
    int public_fd = ERROR;
    time_t cur_secs;

    while(/*timeout != CLIENT_WAITING_TIMEOUT*/!time_is_up && public_fd == ERROR && !public_fifo_closed){
        public_fd = open(public_fifo_path, O_WRONLY | O_NONBLOCK);
        sleep(1);
        //if(debug) printf("timeout open: %d\n", timeout + 1); //DEBUG
        //timeout++;

        //checking if nsecs have passed already
        time(&cur_secs);

        if (cur_secs - initial_time >= nsecs){
            fprintf(stderr, "\n[client] Execution time has ended!\n");
            time_is_up = true;
        }
    }

    //when the public fifo wasn't created or it was closed
    if(public_fd == ERROR){ 
        fprintf(stderr, "[client thread] Error while opening public fifo file!\n");
        public_fifo_closed = true;  
        close_client_inner_thread();
    }

    //creating message request to send to server
    message_t message;
    
    message.tskload = rand() % 9 + 1;
    message.pid = getpid();
    message.tid = pthread_self();
    message.tskres = -1;

    char t_s[10], i_s[10], thread_id_s[20], process_id_s[10]; //converting integers to strings

    sprintf(t_s, "%d", message.tskload);  //DEBUG
    sprintf(thread_id_s, "%ld", message.tid); //DEBUG

    sprintf(process_id_s, "%d", message.pid);
    sprintf(i_s, "%ld", (long int) arg);
    strcat(i_s, process_id_s);

    message.rid = atoi(i_s);

    if(debug) printf("i: %d\tt: %d\tpid: %u\ttid: %lu\tres: %d", message.rid, message.tskload, message.pid, message.tid, message.tskres); //DEBUG 

    //creating private fifo file path ("/tmp/pid.tid") to receive server response
    char priv_fifo_path[40];

    strcat(priv_fifo_path, "/tmp/");
    strcat(priv_fifo_path, process_id_s);
    strcat(priv_fifo_path, ".");
    strcat(priv_fifo_path, thread_id_s);

    if(debug) printf("\t priv_fifo: %s\n", priv_fifo_path);  //DEBUG

    // Creating private fifo file
    if(mkfifo(priv_fifo_path, 0777) == ERROR){
        close_client_inner_thread();                                                  
    }

    //sending request to server
    write(public_fd, &message, sizeof(message_t));

    //the server has closed the read side of pipe
    if(errno == EPIPE){
        fprintf(stderr, "[client thread] The server has closed the public fifo read side\n");
        public_fifo_closed = true;
        close_client_inner_thread();
    }

    //registering operation to stdout
    reg(i_s, t_s, process_id_s, thread_id_s, "-1", IWANT);

    //getting the response from the server
    int private_fd = open(priv_fifo_path, O_RDONLY);

    if(private_fd == ERROR){ 
        fprintf(stderr, "[client thread] Error while opening private fifo file!\n");
        close_client_inner_thread();
    }

    //getting the response from server
    message_t response;

    int bytes_read = ERROR;
    int tout = 1;  //DEBUG

    while(bytes_read == ERROR){
        bytes_read = read(private_fd, &response, sizeof(message_t)); 
        if(bytes_read != ERROR || time_is_up){
            break;
        }
        if(debug) printf("timeout read: %d", tout);  //DEBUG
        tout++;  //DEBUG
        sleep(1);
    }
    
    if(time_is_up && bytes_read == ERROR){
        reg(i_s, t_s, process_id_s, thread_id_s, "", GAVUP);

        //deleting private fifo file
        if(remove(priv_fifo_path) != 0){
            fprintf(stderr, "[client thread] Not successfully deleted private file\n");
        }

        close_client_inner_thread();
    }
    else{
        handle_server_response(response, i_s, t_s, process_id_s, thread_id_s);
    }
    
    //closing fifo files
    close(public_fd);
    close(private_fd);

    //deleting private fifo file
    if(remove(priv_fifo_path) != 0){
        fprintf(stderr, "[client thread] Not successfully deleted private file\n");
    }

    //closing thread and unlocking code with mutex
    close_client_inner_thread();

    return NULL;
}

/**
 * @brief stops execution for a random number of milliseconds from 0 to 500 milliseconds
 * 
 * @return ERROR value when error occurred in nanosleep system call, else 0
 */
int random_mili_sleep(){
    //used to set the srand function
    time_t t;
    
    //setting seed for thread random creation
    srand((unsigned) time(&t));

    //stores the pseudo random time between thread creation
    long int rand_nano;

    //structure needed to use nanosleep
    struct timespec req, rem;

    rand_nano = (rand() % 500 + 1) * MILLISEC_TO_NANOSEC_MULTIPLIER;  //interval of (0 - 0.5 seconds or 0 - 500 milliseconds

    req.tv_sec = 0;  //number of seconds                        
    req.tv_nsec = rand_nano;  //number of nanoseconds 
    //req.tv_nsec = 500000000; //DEBUG   
  
    return nanosleep(&req , &rem);
}

/**
 * @brief Checks if the given string is a existing file and of fifo type
 *
 * @param file_name, the string representing the directory or the file to be checked
 * @return true if it is exists, false otherwise
 */
bool fifo_file_checker(char file_name[]){

    bool file_exists = false;

    struct stat buffer;

    if(stat(file_name, &buffer) == 0) file_exists = true;

    return file_exists && ((buffer.st_mode & S_IFMT) == S_IFIFO);
}

/**
 * @brief error on user input
 * 
 * @return ERROR value (-1)
 */
int error_on_input(){
    fprintf(stderr, "Usage: ./c <-t nsecs> <fifoname>\n");
    return ERROR;
}

/*-----------------------END UTIL FUNCTIONS-------------------------*/


/*-------------------------MAIN THREAD-------------------------*/

/**
 * @brief main thread (c0)
 * 
 * @param argc number of arguments passed to program
 * @param argv actual argmuents passed to the program
 * @return 0 when succeeded, -1 otherwise
 */
int main(int argc, char* argv[]){

    /*-------------------------PROGRAM INITIALIZATION-------------------------*/

    //getting initial time of program execution
    time(&initial_time);

    //checking if the coreect number of arguments was given
    if(argc != 4){
        return error_on_input();
    }

    //used to get current seconds in main thread loop
    time_t cur_secs;
        
    //ARG 1: seconds identifier
    char *secs_identifier = argv[1]; 

    if(strcmp(secs_identifier, "-t") != 0){
        return error_on_input();
    }
    
    //ARG 2: nº of seconds which the program should run in
    nsecs = atoi(argv[2]);  

    //ARG 3: public fifo file path (relative or absolute)
    public_fifo_path = argv[3];

    //initializing pthread mutex structure
    if (pthread_mutex_init(&lock, NULL) != 0) {
        fprintf(stderr, "Mutex init has failed\n");
        return ERROR;
    }

    //thread unique identifier
    long int id = 1;

    /*-------------------------CREATING REQUEST THREADS-------------------------*/

    //main thread (c0) continously creates new threads with server requests
    while(!public_fifo_closed && !time_is_up) {
        //sleeping for a few milliseconds
        if(random_mili_sleep() == ERROR) return ERROR;

        //creating threads
        pthread_t thread_id;

        if(debug) printf("created thread number: %ld \n", id); //DEBUG

        if(pthread_create(&thread_id, NULL, &send_request_and_wait_response, (void*)id) != 0) return ERROR;

        //checking if nsecs have passed already
        time(&cur_secs);

        if (cur_secs - initial_time >= nsecs){
            fprintf(stderr, "\n[client] Execution time has ended!\n");
            time_is_up = true;
        }

        id++;
    }

    /*-------------------------ENDING PROGRAM-------------------------*/

    fprintf(stderr, "[client] Waiting for other threads to finish...\n");

    //main thread waits for all threads to exit
    pthread_exit(NULL);
    
    //releasing pthread mutex structure
    pthread_mutex_destroy(&lock);

    return 0;
}
