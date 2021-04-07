#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_t tid[2];
int counter,counterr;
pthread_mutex_t lock;
pthread_mutex_t lock2;

void* trythis(void* arg)
{
    pthread_mutex_lock(&lock);

    unsigned long i = 0;
    counter += 1;
    printf("\n Job %d has started\n", counter);

    for (i = 0; i < (0xFFFFFFFF); i++)
        ;

    printf("\n Job %d has finished\n", counter);

    pthread_mutex_unlock(&lock);

    return NULL;
}

void* trythis1(void* arg)
{
    pthread_mutex_lock(&lock2);

    unsigned long i = 0;
    counterr += 2;
    printf("\n Job %d has started\n", counterr);

    for (i = 0; i < (0xFFFFFFFF); i++)
        ;

    printf("\n Job %d has finished\n", counterr);

    pthread_mutex_unlock(&lock2);

    return NULL;
}

int main(void)
{
    int i = 0;
    int error, error1;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    if (pthread_mutex_init(&lock2, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    while (1) {
        error = pthread_create(&(tid[0]),
                               NULL,
                               &trythis, NULL);

        error1 = pthread_create(&(tid[1]),NULL,&trythis1,NULL);
        if (error != 0)
            printf("\nThread can't be created :[%s]",
                   strerror(error));
        if (error1 != 0)
            printf("\nThread can't be created :[%s]",
                   strerror(error));
        break;
    }

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_mutex_destroy(&lock);
    pthread_mutex_destroy(&lock2);


    return 0;
}
