#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include "zemaphore.h"

void zem_init(zem_t *s, int value) {
    s->counter = value;
    s->lock = PTHREAD_MUTEX_INITIALIZER;
    s->cond = PTHREAD_COND_INITIALIZER;
}

void zem_down(zem_t *s) {
    pthread_mutex_lock(&(s->lock));
    if(s->counter >= 0) {
        s->counter--;
    }
    while(s->counter < 0){
        // unlocks the mutex just before it sleeps (as you note), 
        // but then it reaquires the mutex (which may require waiting) when it is signalled, 
        // before it wakes up. So if the signalling thread holds the mutex (the usual case), 
        // the waiting thread will not proceed until the signalling thread also unlocks the mutex
        pthread_cond_wait(&(s->cond),&(s->lock));
    }
    // s->counter--;
    pthread_mutex_unlock(&(s->lock));

}

void zem_up(zem_t *s) {
    pthread_mutex_lock(&(s->lock));
    s->counter++;
    pthread_cond_signal(&(s->cond));
    pthread_mutex_unlock(&(s->lock));

}
