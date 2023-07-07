#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>

int item_to_produce, curr_buf_size, item_to_consume;
int total_items, max_buf_size, num_workers, num_masters;
int total_item_consumed;
pthread_mutex_t lock;
pthread_cond_t empty, full;

//pthread_mutex_t lock = PTHREAD_MUTEX_INTIALIZER;

int *buffer;

void print_produced(int num, int master) {

  printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker) {

  printf("Consumed %d by worker %d\n", num, worker);
  
}


//produce items and place in buffer
//modify code below to synchronize correctly
void *generate_requests_loop(void *data)
{
  int thread_id = *((int *)data);

  while(1)
    {   
      
      pthread_mutex_lock(&lock);
      while(curr_buf_size == max_buf_size){
          pthread_cond_wait(&empty,&lock);
      }
      if(item_to_produce >= total_items) {
          pthread_cond_broadcast(&full);
          pthread_mutex_unlock(&lock); 
	      break;
      }
      
      buffer[curr_buf_size++] = item_to_produce;
      print_produced(item_to_produce, thread_id);
      item_to_produce++;
      pthread_cond_signal(&full);
      pthread_mutex_unlock(&lock);
    }
  return 0;
}

//write function to be run by worker threads
void *generate_process_loop(void *data)
{
    int thread_ids = *((int *)data);

    while(1)
    {
        pthread_mutex_lock(&lock);
        while (curr_buf_size == 0 && total_item_consumed < total_items){
            pthread_cond_wait(&full,&lock);
        }
        if (item_to_produce == 0 || total_item_consumed >=total_items || curr_buf_size < 0 ){
            pthread_mutex_unlock(&lock);
            break;
        }
         curr_buf_size--;
        item_to_consume = buffer[curr_buf_size];
        total_item_consumed ++;
        print_consumed(item_to_consume,thread_ids);
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&lock);
    }
    return 0;
}


//ensure that the workers call the function print_consumed when they consume an item

int main(int argc, char *argv[])
{
  int *master_thread_id;
  int *consumer_thread_id;
  pthread_t *master_thread;
  pthread_t *consumer_thread;

  item_to_produce = 0;
  curr_buf_size = 0;
  total_item_consumed = 0;
  
  int i;
  
   if (argc < 5) {
    printf("./master-worker #total_items #max_buf_size #num_workers #masters e.g. ./exe 10000 1000 4 3\n");
    exit(1);
  }
  else {
    num_masters = atoi(argv[4]);
    num_workers = atoi(argv[3]);
    total_items = atoi(argv[1]);
    max_buf_size = atoi(argv[2]);
  }

   buffer = (int *)malloc (sizeof(int) * max_buf_size);

   //create master producer threads
   master_thread_id = (int *)malloc(sizeof(int) * num_masters);
   master_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_masters);
  for (i = 0; i < num_masters; i++)
    master_thread_id[i] = i;

  for (i = 0; i < num_masters; i++)
    pthread_create(&master_thread[i], NULL, generate_requests_loop, (void *)&master_thread_id[i]);
  
 // create worker consumer threads
  consumer_thread_id = (int *)malloc(sizeof(int) * num_workers);
  consumer_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);

  for (i = 0; i < num_workers; i++)
    consumer_thread_id[i] = i;

  for (i = 0; i < num_workers; i++)
    pthread_create(&consumer_thread[i], NULL, generate_process_loop, (void *)&consumer_thread_id[i]);

  
  //wait for all threads to complete
  for (i = 0; i < num_masters; i++)
    {
      pthread_join(master_thread[i], NULL);
      printf("master %d joined\n",master_thread_id[i]);
    }
  
  for (int j = 0; j < num_workers; j++)
    {
      pthread_join(consumer_thread[j], NULL);
      printf("consumer %d joined\n",consumer_thread_id[j]);
    }
  /*----Deallocating Buffers---------------------*/
  free(buffer);
  free(master_thread_id);
  free(master_thread);
  free(consumer_thread_id);
  free(consumer_thread);
  
  return 0;
}
