#include "../../header/threads.h"
#include <iostream>
#include <libgen.h>

int NUM_THREADS;

using namespace std;
file_list* recfiles;


pthread_mutex_t rec_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buff_lock = PTHREAD_MUTEX_INITIALIZER;

volatile int threads_alive;
pthread_cond_t non_empty;
pthread_cond_t non_full;

#define BUFF 1024
#define PERMS 0777

threadpool* tp;

/*
*	Join all threads with the main thread.
*	Release any space allocated.
*/

void destroy_pool(threadpool* pool){

	thread_wait();

	threads_alive = 0;

	tp->ring->len = NUM_THREADS;
	
	pthread_cond_broadcast(&(tp->non_empty));
	
	for(int i = 0 ; i < NUM_THREADS; i++){
		pthread_join(pool->threads[i]->thread,0);
		delete pool->threads[i];
	}

	delete recfiles;

	free(tp->savepath);

	delete[] pool->threads;

	delete pool->ring;

	delete pool;

}


/*
*	Initialize the threadpool with num_threads and pass the ringbuffer.
*
*/

threadpool* threadpool_init(int num_threads,ringbuffer* ring){
	NUM_THREADS = num_threads;
	threads_alive = 1;
	tp = new threadpool;

	if (tp == NULL){
		cout << "error on threadpool init.No memory to allocate" << endl;
		exit(1);
	}

	tp->threads = new thread_info*[num_threads];
	tp->num_threads = num_threads;

	tp->working = 0;
	tp->alive = 0;
	tp->ring = ring;
	tp->savepath = NULL;
	recfiles = new file_list();
	pthread_mutex_init(&(tp->access),NULL);
	pthread_cond_init(&(tp->all_idle),NULL);
	pthread_cond_init(&(tp->non_empty),NULL);
	pthread_cond_init(&(tp->non_full),NULL);
	for(int i = 0 ; i < num_threads ; i++){
		tp->threads[i] = new thread_info;
		tp->threads[i]->pool = tp;
		tp->threads[i]->id = i;
		pthread_create(&(tp->threads[i]->thread),NULL,&thread_work,tp->threads[i]);
	}

	//while(tp->alive != num_threads);//wait for all threads to come online

	return tp;
}



buffArg* getJob(){
	return tp->ring->retrieve();

}

void* fileListJob(void* arg){
	buffArg* obj = (buffArg*)arg;

	uint32_t ip = htonl(obj->ip);
	int port = htons(obj->portnum);

	tasklist* tlist = new tasklist;
	tlist->next = NULL;
	int task_size = 0;


	int sockfd = -1;
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == 0){
		cout << "Cannot create socket" << endl;
		exit(1);
	} 

	char sendmsg[BUFF];
	memset(sendmsg,0,BUFF);

	char response[BUFF];
	memset(response,0,BUFF);

	struct sockaddr_in client;
	client.sin_family = AF_INET;
	memcpy(&client.sin_addr,&ip, sizeof(ip));
    client.sin_port = port;

    /*
    *	Connect to client and send a GET_FILE_LIST request
    *
    */

	while(connect(sockfd,(struct sockaddr*) &client,sizeof(client)) < 0){
    	//perror("Connect fail on file list job");
    }

    strcat(sendmsg,"GET_FILE_LIST");
    write(sockfd,sendmsg,BUFF);
    


    memset(sendmsg,0,BUFF);

    read(sockfd,response,BUFF);
    char* token = NULL;
    token = strtok(response," ");
    token = strtok(NULL," ");
    int size = atoi(token);

    memset(response,0,sizeof(response));


    unsigned long v=0;

    tasklist* head = tlist;

    /*
    *	For each file in the list get
    *	the pathname and the version
    */

    char fbuff[BUFF];
    buffArg* newtask = new buffArg;
	newtask->ip = obj->ip;
	newtask->portnum = obj->portnum;
    for(int i =0 ; i < size ;i++){

    	memset(fbuff,0,BUFF);


    	if(read(sockfd,fbuff,128) < 0)//get pathname
    		break;
    	memset(newtask->pathname,0,128);
    	strcpy(newtask->pathname,fbuff);

    	read(sockfd,&v,sizeof(unsigned long));

    	newtask->version = v;
    	tp->ring->place(newtask);
    	
    	// tlist->data = newtask;
    	// tlist->next = NULL;
    	// tlist->next = new tasklist;
    	// tlist = tlist->next;
    	// tlist->next = NULL;
    	// task_size++;
    }

  //   tlist = head;
  //   tasklist* temp = NULL;
  //   while(tlist != NULL && task_size != 0){
		// temp = tlist;
		// if(tlist->data != NULL && tlist->data->pathname != NULL) tp->ring->place(tlist->data); //finaly add all tasks to the ringbuffer
  //   	tlist = tlist->next;
  //   	// delete temp->data;
  //   	// delete temp;
  //   }

close(sockfd);
}

void* getFilejob(void* arg){
	buffArg* obj = (buffArg*)arg;

	char fullpath[BUFF];
	memset(fullpath,0,BUFF);

	struct stat fstat;
	
	uint32_t ip = htonl(obj->ip);
	int port = htons(obj->portnum);
	unsigned long version;

	pthread_mutex_lock(&rec_lock);
	fnode* temp = recfiles->search(obj->pathname);
	pthread_mutex_unlock(&rec_lock);
	
	struct sockaddr_in client;
	client.sin_family = AF_INET;
	memcpy(&client.sin_addr,&ip, sizeof(ip));
    client.sin_port = port;

    int sockfd = -1;
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == 0){
		cout << "Cannot create socket" << endl;
		exit(1);
	} 

	if(connect(sockfd,(struct sockaddr*) &client,sizeof(client)) < 0){
    	perror("connect thread fail getfile");
    	exit(1);
    }


	strcat(fullpath,tp->savepath);
	strcat(fullpath,"/");
	strcat(fullpath,obj->pathname);

	char sendmsg[BUFF];
	memset(sendmsg,0,BUFF);
	char response[BUFF];
	memset(response,0,BUFF);

	strcat(sendmsg,"GET_FILE ");


	write(sockfd,sendmsg,BUFF);
	memset(sendmsg,0,sizeof(sendmsg));

	if(temp == NULL){//file does not exist in mirror dir
		version = 1;
	}else{//IT EXISTS	
		version = temp->timestamp;
	}	

	write(sockfd,obj->pathname,128);//send filepath in remote


	write(sockfd,&version,sizeof(version));//write local version

	memset(response,0,sizeof(response));
	read(sockfd,response,BUFF);//read fileuptodate or FILESIZE


	if(!strcmp(response,"FILE_UP_TO_DATE")){
		cout << "File " << obj->pathname << " up to date " << endl;
		close(sockfd);
		return NULL;
	}

	memset(response,0,sizeof(response));

	unsigned long remote_version;
	read(sockfd,&remote_version,sizeof(remote_version));

	int filesize = 0;
	read(sockfd,&filesize,sizeof(filesize));

	char savepath[BUFF];
	memset(savepath,0,BUFF);
	char* temppath = NULL;
	temppath = strdup(obj->pathname);
	strcat(savepath,tp->savepath);

	
	strcat(savepath,"/");
	strcat(savepath,obj->pathname);

	free(temppath);
	strcat(savepath,"/");

	char* ret = NULL;
	ret = strrchr(obj->pathname,'/');
	ret++;

	strcat(savepath,ret);
	char* dname = NULL;
	dname = dirname(savepath);
	char* bname = NULL;
	bname = basename(savepath);
	
	pthread_mutex_lock(&tp->access);
	char* token = NULL;
	token = strtok(dname,"/");
	char dirpath[BUFF];
	memset(dirpath,0,BUFF);

	while(token != NULL){
		strcat(dirpath,token);
		strcat(dirpath,"/");
		mkdir(dirpath,PERMS);


		token = strtok(NULL,"/");
	}

	strcat(dirpath,bname);

	int fd = open(dirpath,O_CREAT|O_APPEND|O_RDWR,PERMS);
	pthread_mutex_unlock(&tp->access);
	int total = 0;

	int toreceivebytes = filesize;

	int bytes = 0;
	int nread = 0;
	char filebuff[BUFF];

	memset(filebuff,0,BUFF);

	while(toreceivebytes > 0)//while there still bytes left to read
	{	
		if(toreceivebytes < BUFF)//set bytes as max = buffsize or less
			bytes = toreceivebytes;
		else
			bytes = BUFF;

		int received = 0;

		nread = 0;
		for(received = 0 ; received < bytes ; received+=nread)//keep reading to fill buffer
			nread = read(sockfd,filebuff+received,bytes - received);

		
		write(fd,filebuff,bytes);
		memset(filebuff,0,BUFF);

		toreceivebytes = toreceivebytes - bytes;

	}


	close(fd);
	close(sockfd);

	pthread_mutex_lock(&rec_lock);
	recfiles->append(obj->pathname,remote_version);
	pthread_mutex_unlock(&rec_lock);

	cout << "Received " << obj->pathname << endl;

}


/*
*	Main thread function.While queue length is 0 wait on the non_empty condition.
*	Based on the task retrieved from the ringbuffer either request file list or a file.
*
*/

void* thread_work(void* arg){
	threadpool* pool = tp;

	// pthread_mutex_lock(&pool->access);
	// pool->alive+=1;
	// pthread_mutex_unlock(&pool->access);


			while(threads_alive == 1){

				pthread_mutex_lock(&pool->access);

				while(pool->ring->len == 0){
					pthread_cond_wait(&(tp->non_empty),&pool->access);
				}

				pthread_mutex_unlock(&pool->access);

				if(threads_alive == 0){
					pthread_mutex_lock(&pool->access);
					pool->ring->len--;
					pthread_mutex_unlock(&pool->access);
					break;
				}
				pthread_mutex_lock(&pool->access);

				pool->working++;

				pthread_mutex_unlock(&pool->access);

				void *(*func_buff)(void*);
				void* args;

				pthread_mutex_lock(&(tp->access));
				buffArg* task = pool->ring->retrieve();

				buffArg* ntask = new buffArg;
				ntask->ip = task->ip;
				ntask->portnum = task->portnum;
				ntask->version = task->version;
				strcpy(ntask->pathname,task->pathname);
				pthread_mutex_unlock(&(tp->access));
				


				if(ntask != NULL){
					//call function based on task/buffarg
					if((ntask->version == 0 )&& (!strcmp(ntask->pathname,"NULL"))){
						//getfile list
						func_buff = &fileListJob;
						args = ntask;
					
					}else if ((ntask->version != 0) && (strcmp(ntask->pathname,"NULL"))){
						//get file
						//cout << "get file " << endl;
						func_buff = &getFilejob;
						args = ntask;
					}

					func_buff(args);
					delete ntask;
				}

				pthread_mutex_lock(&pool->access);
				pool->working--;

				if(pool->working == 0){
					pthread_cond_signal(&pool->all_idle);
				}

				pthread_mutex_unlock(&pool->access);

		}
	
	// pthread_mutex_lock(&pool->access);
	// pool->alive--;
	// pthread_mutex_unlock(&pool->access);
	pthread_exit(0);
	return NULL;
}




void thread_wait(){
	pthread_mutex_lock(&tp->access);
	while(tp->ring->len > 0 || tp->working > 0){
		pthread_cond_wait(&tp->all_idle,&tp->access);
	}
	pthread_mutex_unlock(&tp->access);
}



/*
*	Init ringbuffer
*
*/

ringbuffer::ringbuffer(int buffersize){
	this->max = buffersize;
	this->len = 0;
	this->start = 0;
	this->end = -1;
	this->args = new buffArg*[buffersize];

	for(int i = 0 ;i < buffersize ;i++){
		this->args[i] = new buffArg;
		this->args[i]->ip = 0;
		this->args[i]->portnum = 0;
		this->args[i]->version = 0;
		memset(this->args[i]->pathname,0,128);
	}
}

/*
*	Place item in ringbuffer
*
*/

void ringbuffer::place(buffArg* item){
	pthread_mutex_lock(&tp->access);
	while(this->len >= this->max){
		pthread_cond_wait(&(tp->non_full),&tp->access);
	}


	this->end = (this->end+1)%this->max;

	//this->args[this->end] = item;
	this->args[this->end]->ip = item->ip;
	this->args[this->end]->portnum = item->portnum;
	this->args[this->end]->version = item->version;
	memset(this->args[this->end]->pathname,0,128);
	strcpy(this->args[this->end]->pathname,item->pathname);

	this->len++;

	
	pthread_cond_signal(&(tp->non_empty));
	pthread_mutex_unlock(&tp->access);
	
	
}

buffArg* ringbuffer::retrieve(){
	
	
	while(this->len <= 0){
		pthread_cond_wait(&(tp->non_empty),&tp->access);
	}

	buffArg* obj = this->args[this->start];
	// obj->ip = this->args[this->start]->ip;
	// obj->portnum = this->args[this->start]->portnum;
	// obj->version = this->args[this->start]->version;
	// strncpy(obj->pathname,this->args[this->start]->pathname,strlen(this->args[this->start]->pathname)+1);
	// this->args[this->start]->ip = -1;
	// this->args[this->start]->portnum = -1;
	// this->args[this->start]->version = 0;
	// memset(this->args[this->start]->pathname,0,128);
	this->start = (this->start+1)%(this->max);
	this->len--;


	pthread_cond_signal(&(tp->non_full));
	return obj;

}

ringbuffer::~ringbuffer(){
	for(int i = 0 ;i < this->max ;i++){
		delete this->args[i];
	}

	delete[] this->args;
}

void ringbuffer::print(){
	for(int i = 0 ;i < this->max ;i++){
		if(this->args[i] != NULL){
			cout << " not null " << this->args[i]->pathname << endl;
		}	
	}
}

buffArg::buffArg(){
	this->ip=0;
	this->portnum = 0;
	this->version = 0;
	memset(this->pathname,0,128);
}