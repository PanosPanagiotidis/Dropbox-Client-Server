#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include "../../header/tuple_list.h"
#include "../../header/threads.h"
#include "../../header/file_list.h"

#define TIMEOUT 50000


pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ring_lock = PTHREAD_MUTEX_INITIALIZER;

char dropfolder[100];
ringbuffer* ring;
file_list* flist;


#define PERMS 0777
#define BUFF 1024

volatile int keepalive = 1;



void catch_interrupt(int signo,siginfo_t *info,void* context)
{
	thread_wait();
	keepalive = 0;
	return;
}

void getArgs(int argc, char* argv[],int* portnum, char** dir_name,int* threads,int* buffersize,int* serverport,char** server_ip,char** id)
{
	for(int i = 0 ; i < argc ; i++)
	{	
		if(!strcmp(argv[i],"-d"))	// dir name
		{	
			*dir_name = strdup(argv[i+1]);
		}
		else if(!strcmp(argv[i],"-p"))	//port for client
		{
			*portnum = atoi(argv[i+1]);
		}
		else if(!strcmp(argv[i],"-w")) // number of threads
		{
			*threads = atoi(argv[i+1]);
		}
		else if(!strcmp(argv[i],"-b")) // circular buffer size
		{
			*buffersize = atoi(argv[i+1]);
		}
		else if(!strcmp(argv[i],"-sp")) // port server is running
		{
			*serverport = atoi(argv[i+1]);
		}
		else if(!strcmp(argv[i],"-sip")) //ip server is on
		{
			*server_ip = strdup(argv[i+1]);
		}
		else if(!strcmp(argv[i],"-i")) //client id.needed to create local mirror directory
		{
			*id = strdup(argv[i+1]);
		}

	}
}


/*
*	USER_ON command from server.
*
*/

void log_new_user(client_list* list,char* buff){
	cout << "new user logged on " << endl;

	client_data data;
	char* ptr = NULL;
	char* token = NULL;
	token = strtok(buff," ");
	token = strtok(NULL," ");

	data.ip_addr = ntohl(strtoul(token,&ptr,10));

	token = strtok(NULL," ");
	data.portnum = ntohs(atoi(token));

	pthread_mutex_lock(&list_lock);
	list->append(&data);
	pthread_mutex_unlock(&list_lock);

	buffArg* arg = new buffArg;

	arg->version = 0;
	arg->ip = data.ip_addr;
	arg->portnum = data.portnum;
	strcpy(arg->pathname,"NULL");

	ring->place(arg);

	delete arg;
}

/*
*	GET_FILE_LIST request from other clients
*
*/

void send_file_list(int sockfd){
	fnode* temp = flist->head;
	char msg[BUFF];
	memset(msg,0,BUFF);


	sprintf(msg, "CLIENT_LIST %d", flist->len);
	write(sockfd,msg,BUFF);

	while(temp != NULL){

		write(sockfd,temp->path,128);

		write(sockfd,&temp->timestamp,sizeof(unsigned long));

		temp = temp->next;

	}
	close(sockfd);
}

/*
*	GET_FILE request from other clients.
*
*/

void send_file(int sockfd){
	
	fnode *temp = flist->head;
	char filebuff[128];
	memset(filebuff,0,128);

	read(sockfd,filebuff,128);
	fnode* file = flist->search(filebuff);

	struct stat fstat;

	lstat(file->path,&fstat);
	int fsize = fstat.st_size;

	unsigned long timestamp = file->timestamp;

	long unsigned version;
	read(sockfd,&version,sizeof(version));


	if(timestamp == version){
		char utd[BUFF];
		memset(utd,0,BUFF);
		strcat(utd,"FILE_UP_TO_DATE");
		write(sockfd,utd,BUFF);
		close(sockfd);
		return;
	}
	//check version

	int fd = open(file->path,O_RDONLY,PERMS);
	if(fd < 0 ){
		perror("not found");
		exit(1);
	}


	char msg[BUFF] = "FILE_SIZE";
	write(sockfd,msg,BUFF);	
	write(sockfd,&timestamp,sizeof(timestamp)); //write the local version
	write(sockfd,&fsize,sizeof(fsize));//write the file size



	int bytes = 0;

	/*
	* Write all contents from fd to socket
	*/

	memset(filebuff,0,BUFF);

	int tosentbytes = fsize;

	while(tosentbytes > 0)
		{	
			//Check how many bytes i can send up to buffsize
			if(tosentbytes < BUFF)
				bytes = tosentbytes;
			else
				bytes = BUFF;

			//Alloc bytes space
			char buffer[BUFF];
			memset(buffer,0,BUFF);

			//Read from the file
			read(fd,buffer,bytes);

			int sentbytes;
			int nwrite = 0;
			//write on pipe
			for(sentbytes = 0; sentbytes < bytes ; sentbytes+=nwrite)
				nwrite = write(sockfd,buffer+sentbytes,bytes-sentbytes);

			tosentbytes = tosentbytes - bytes;

		}



	close(fd);
	close(sockfd);
}

void user_log_off(int sockfd,client_list* list){
	cout << "User logged off " << endl;
	uint32_t ip;
	int port;

	read(sockfd,&ip,sizeof(ip));
	read(sockfd,&port,sizeof(port));

	client_data data;
	data.ip_addr = ntohl(ip);
	data.portnum = ntohs(port);


	pthread_mutex_lock(&list_lock);
	int res = list->remove(data);
	pthread_mutex_unlock(&list_lock); 

}


void freeArgs(char** dir_name,char** server_ip,char** id){
	free(*server_ip);
	free(*dir_name);
	free(*id);
}

int bind_port(int sockfd,int port){
	int flag = 1;
	int retval = setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(int));

	if(retval == -1){
		perror("setsockopt");
		exit(1);
	}

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);
	int bound = bind(sockfd,(struct sockaddr*) &server,sizeof(server));
	return bound;
}


int main(int argc,char* argv[]){

	if (argc != 15){
		cout << "Wrong number of arguments" << endl;
		exit(1);
	}

	struct sigaction int_action;
	int_action.sa_sigaction = catch_interrupt;
	sigemptyset(&int_action.sa_mask);
	int_action.sa_flags = SA_SIGINFO;
	sigaction(SIGINT,&int_action,NULL);

	

	char client_name[1024];
	gethostname(client_name,sizeof(client_name));
	struct hostent* client = gethostbyname(client_name);	//get machine ip address
	if(client == NULL) exit(1);

 	char* IP = inet_ntoa(*((struct in_addr*) client->h_addr_list[0]));

	struct in_addr caddr;
	inet_aton(IP,&caddr);

	struct sockaddr_in server;
	int serverlen;

	int portnum;
	char* dir_name = NULL;
	int threads;
	int buffersize;
	int serverport;
	char* server_ip = NULL;
	char* id = NULL;

	getArgs(argc,argv,&portnum,&dir_name,&threads,&buffersize,&serverport,&server_ip,&id);

	/*
	*	Create mirror folders to store content from other clients
	*/
	memset(dropfolder,0,100);
	strcat(dropfolder,"mirror_");
	strcat(dropfolder,id);
	mkdir(dropfolder,PERMS);


	flist = new file_list(dir_name);
	ring = new ringbuffer(buffersize);

	int count = 0;
	fnode* head = flist->head;
	while(head != NULL){
		count++;
		head = head->next;
	}


	threadpool* tp = threadpool_init(threads,ring);
	tp->savepath = strdup(dropfolder);
	


	client_list *list = new client_list();

	struct hostent* rem = NULL;
	rem =  gethostbyname(server_ip);
	if(rem == NULL){
		freeArgs(&dir_name,&server_ip,&id);
		cout << "Could not get server" << endl;
		exit(1);
	}

	server.sin_family = AF_INET;
	memcpy(&server.sin_addr,rem->h_addr, rem->h_length);
    server.sin_port = htons(serverport);
    serverlen = sizeof(server);

    int sock_server = -1;
    sock_server = socket(AF_INET,SOCK_STREAM,0);

    if (sock_server < 0){
    	perror("socket");
    	exit(1);
    }

    if(connect(sock_server,(struct sockaddr*) &server,sizeof(server)) < 0){
    	perror("connect");
    	exit(1);
    }

    char client_ip[BUFF];
    char client_port[BUFF];

    sprintf(client_ip,"%u",caddr.s_addr);
    sprintf(client_port,"%u",htons(portnum));

    /*
    *	Connect to server to inform of us logging into the system
    *
    *
    */

    char log[BUFF] = "LOG_ON ";
    char log_message[BUFF] ="";

    strcat(log_message,log);
    strcat(log_message,client_ip);
    strcat(log_message," ");
    strcat(log_message,client_port);

    write(sock_server,log_message,BUFF);


	//get clients

	char gc[BUFF] = "GET_CLIENTS";
	char response[BUFF];
	memset(response,0,sizeof(response));
	cout << "Sending message to server for clients" << endl;
	write(sock_server,gc,BUFF);//get clients

	read(sock_server, response, sizeof(response)); //NULL or CLIENT_LIST 
	
	//If response is NULL,there are no other clients to connect to
	
	if(!strstr(response,"NULL")){
		client_data data;
		int new_clients ;
		read(sock_server,&new_clients,sizeof(new_clients));
		unsigned long ip;
		int port;

		for(int i = 0 ;i < new_clients ;i++){
			read(sock_server,&ip,sizeof(ip));
			data.ip_addr = ntohl(ip);
			read(sock_server,&port,sizeof(port));
			data.portnum = ntohs(port);

			pthread_mutex_lock(&list_lock);
			list->append(&data);
			pthread_mutex_unlock(&list_lock);

		}

	}

	close(sock_server);

	// cout << "file list len is " << flist->len << endl;
	// cout << "client list size is " << list->size << endl;
	node* temp = NULL;
	temp = list->head;

	/*
	*	Add every other client to the ringbuffer and let the threads take over.
	*/

	while(temp != NULL && list->size != 0){
		buffArg* arg = new buffArg;

		arg->ip = temp->d.ip_addr;
		arg->portnum = temp->d.portnum;
		memset(arg->pathname,0,128);
		strcpy(arg->pathname,"NULL");
		arg->version = 0;

		tp->ring->place(arg);

		temp = temp->next;
	}


	int sockfd = -1;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "Cannot create socket" << endl;
		exit(1);
	}

	int addr = bind_port(sockfd, portnum);

	if (addr < 0)
	{
		cout << "bind error " << endl;
		exit(1);
	}

	//max 10 requests at a time
	if(listen(sockfd,100) < 0){
		cout << "Listen error " << endl;
		exit(1);
	}



	cout << "Awaiting requests on " << portnum<< endl;


	struct pollfd reqs[1];
	reqs[0].fd = sockfd;
	reqs[0].events = POLLIN;
	int sock_acc;
	char buff[BUFF];
	memset(buff,0,BUFF);
	cout << "Awaiting new connections" << endl;
	while (keepalive == 1)
	{
		int retval;
		retval = poll(reqs, 1, TIMEOUT); //timeout in milliseconds


		if (reqs[0].revents == POLLIN)
		{
			struct sockaddr_in client_addr;
			unsigned int len = sizeof(client_addr);

			sock_acc = accept(sockfd, (struct sockaddr *)&client_addr, &len);
			read(sock_acc, buff, BUFF);

			if (strstr(buff, "USER_ON ")) log_new_user(list,buff);
			if (strstr(buff, "GET_FILE_LIST")) send_file_list(sock_acc);
			if (strstr(buff, "GET_FILE ")) send_file(sock_acc);
			if (strstr(buff, "LOG_OFF")) user_log_off(sock_acc,list);


			memset(buff,0,BUFF);
			close(sock_acc);
		}

	}

	/*
	*	Finally connect to server one last time,inform of us leaving the system and clean up 
	*	any memory reserved
	*/


    uint32_t ip = caddr.s_addr;
    int port = htons(portnum);

    sock_server = socket(AF_INET,SOCK_STREAM,0);
    if (sock_server < 0){
    	perror("socket");
    	exit(1);
    }

    if(connect(sock_server,(struct sockaddr*) &server,sizeof(server)) < 0){
    	perror("connect");
    	exit(1);
    }

    char log_off[BUFF];
    memset(log_off,0,BUFF);

    strcat(log_off,"LOG_OFF");
    
    write(sock_server,log_off,BUFF);
    write(sock_server,&ip,sizeof(ip));
    write(sock_server,&port,sizeof(port));


    memset(log_off,0,BUFF);
    read(sock_server,log_off,BUFF);

    freeArgs(&dir_name,&server_ip,&id);

    destroy_pool(tp);
    delete flist;
    delete list;

    cout << log_off << endl;
    

}