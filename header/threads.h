    
#ifndef THREAD_SCHEDULER_H
#define THREAD_SCHEDULER_H

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "file_list.h"


using namespace std;

typedef struct buffArg{
	char pathname[128];
	unsigned long version;
	uint32_t ip;
	int portnum;
	buffArg();
}buffArg;

typedef struct tasklist{
	struct tasklist* next;
	buffArg* data;
}tasklist;


class ringbuffer{
	public:
		buffArg** args;
		int start;
		int end;
		int max;
		int len;
		ringbuffer(int );
		~ringbuffer();
		void place(buffArg* );
		buffArg* retrieve();
		void print();
};


typedef struct thread_info{
	long int id;
	pthread_t thread;
	struct threadpool *pool;
}thread_info;


typedef struct threadpool{
	thread_info** threads;
	char* savepath;
	ringbuffer* ring;
	int working;
	int alive;
	pthread_mutex_t access;
	pthread_cond_t all_idle;
	pthread_cond_t non_full;
	pthread_cond_t non_empty;
	int num_threads;
}threadpool;


threadpool* threadpool_init(int ,ringbuffer* );
void* thread_work(void* );
void thread_wait(void );
void destroy_pool(threadpool* );
// void* print_thread_info(void *);
buffArg* getJob();


#endif