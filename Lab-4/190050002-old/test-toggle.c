#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>
#include "zemaphore.h"

#define NUM_THREADS 3
#define NUM_ITER 10

zem_t done0;
zem_t done1;
zem_t done2;

void *justprint(void *data)
{
  int thread_id = *((int *)data);

  for(int i=0; i < NUM_ITER; i++)
    {

      if(thread_id == 0){
        zem_down(&done2);
        printf("This is thread %d\n", thread_id); 
        zem_up(&done0); 
      }
      else if(thread_id == 1){
        zem_down(&done0);
        printf("This is thread %d\n", thread_id);
        zem_up(&done1);
      }
      else if(thread_id == 2){
        zem_down(&done1);
        printf("This is thread %d\n", thread_id);
        zem_up(&done2);
      }
      
    }
  return 0;
}

int main(int argc, char *argv[])
{
  zem_init(&done0, 0);
  zem_init(&done1, 0);
  zem_init(&done2, 1);

  pthread_t mythreads[NUM_THREADS];
  int mythread_id[NUM_THREADS];

  
  for(int i =0; i < NUM_THREADS; i++)
    {
      mythread_id[i] = i;
      pthread_create(&mythreads[i], NULL, justprint, (void *)&mythread_id[i]);
    }
  
  for(int i =0; i < NUM_THREADS; i++)
    {
      pthread_join(mythreads[i], NULL);
    }
  
  return 0;
}
