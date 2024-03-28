#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
# include <pthread.h>

void check(int err){
    if(err!=0) {
        perror("Something went wrong in thread creation");
        exit(EXIT_FAILURE);
    }
}

typedef struct my_semaphore {
    volatile unsigned int V; //number of seats in the bus
    pthread_mutex_t lock;
    pthread_cond_t varcond; //varcond for wake passengers
    pthread_cond_t varcond_driver; //varcond for wake driver
    volatile bool tour_started; //the bus has left?
} my_semaphore;

#define thread_n 100
#define seats_bus 75

void *passengers(void *ptr);
void *bus(void *ptr);
my_semaphore *ms;

int my_sem_init(my_semaphore *ms , unsigned int v){
    if(ms==NULL) return -1;
    if(pthread_mutex_init(&ms->lock, NULL)!=0) return -1; 

    if(pthread_cond_init(&ms->varcond, NULL)!=0){
        pthread_mutex_destroy(&ms->lock); 
        return -1;
    }
    if(pthread_cond_init(&ms->varcond_driver, NULL)!=0){
        pthread_mutex_destroy(&ms->lock); 
        return -1;
    }
    ms->V = v;
    return 0; 
}

int my_sem_wait(my_semaphore *ms, int* id){
    pthread_mutex_lock(&ms->lock); 
    if(ms->V==0) {//no seats longer disponible, thread need to get off the bus :(
        pthread_cond_signal(&ms->varcond_driver); //the bus is full, the (sleepy) driver need to know
        pthread_cond_wait(&ms->varcond, &ms->lock); //i can't join the tour, adios
        pthread_mutex_unlock(&ms->lock);
        printf("Thread %d DENIED to join the tour\n",*id);
    }
    else{//a thread take a seat and waits until the bus starts the tour
        ms->V--;
        printf("Thread %d: sat on a seat\n", *id);//a lucky thread got a seat
        if(!ms->tour_started) pthread_cond_wait(&ms->varcond, &ms->lock);//wait for the tour beings
        pthread_mutex_unlock(&ms->lock);
        printf("Thread %d: finshed the tour\n",*id);
    }
    return 0;
}

/*int my_sem_signal(my_semaphore *ms){
    pthread_mutex_lock(&ms->lock); 
    ms->V++;
    pthread_cond_signal(&ms->varcond); //sveglio qualche scemo
    pthread_mutex_unlock(&ms->lock);
    return 0;
}*/

int my_sem_destroy(my_semaphore *ms){
    pthread_mutex_destroy(&ms->lock);
    pthread_cond_destroy(&ms->varcond);
    free(ms);
    return 0;
}

void *bus(void *ptr){
    printf("\nDRIVER: I'm the driver, prepare for departure!\n");

    pthread_mutex_lock(&ms->lock); 
    if(ms->V!=0) {
        printf("DRIVER: The bus is not full yet, i'll wait\n");
        pthread_cond_wait(&ms->varcond_driver, &ms->lock); //se il bus deve ancora riempirsi dormo
    }

    printf("\nDRIVER: Bus is full, now i can go\n");
    ms->tour_started = true;
    pthread_mutex_unlock(&ms->lock); 

    printf("Colosseo\n");
    sleep(1);
    printf("Effeil Tower\n");
    sleep(1);
    printf("Tour bus concluded\n\n");

    pthread_cond_broadcast(&ms->varcond);//all the passengers need to get off the bus
    return NULL;
}

void *passengers(void *ptr){
    int *id = (int*)ptr;
    printf("\nThread %d: is waiting at the bus stop\n", *id);
    if(my_sem_wait(ms, id)!=0) printf("Error wait occurred\n");
    return NULL;
}

int main(){
    ms = (my_semaphore *)malloc(sizeof(my_semaphore));
    if (my_sem_init(ms, seats_bus)!=0) exit(EXIT_FAILURE); //errore

    printf("BUS TURISTICO: posti disponibili %d\n", seats_bus);
    printf("PASSEGGERI ALLA FERMATA: %d", thread_n);

    pthread_t th_bus;//th bus
    pthread_t th[thread_n];//th passenger
    unsigned int id[thread_n];

    for (unsigned int i=0; i<thread_n; ++i) id[i]=i;
    
    int err=0;
    err=pthread_create(&th_bus,NULL,bus,NULL);
    check(err);

    for (unsigned int i=0; i<thread_n; ++i){
        err=pthread_create(th+i,NULL,passengers,id+i);
        check(err);
    }

    for (unsigned int i=0; i<thread_n; ++i){
        err=pthread_join(*(th+i),NULL);
        check(err);
    }
    err=pthread_join(th_bus,NULL);
    check(err);
    my_sem_destroy(ms);
    return 0;
}

