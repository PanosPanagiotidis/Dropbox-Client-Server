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

using namespace std;

#define TIMEOUT 50000 //For poll
#define BUFF 1024	//Size for char buffers


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

/*
*	Send client list to client_addr.
*
*
*/

void get_clients(int sockfd,struct sockaddr_in client_addr,client_list* list){
	cout << "Received clients request " << endl;
	node* temp = NULL;
	char msg[BUFF] = "NULL";
	char response[BUFF]="";


	client_data data;
	data.ip_addr = ntohl(client_addr.sin_addr.s_addr);
	data.portnum = ntohs(client_addr.sin_port);
	

	if((list->size == 1) && (list->search(data) != NULL)){
		write(sockfd, msg, BUFF);
		return;
	}



	struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
	struct in_addr ipAddr = pV4Addr->sin_addr;
	char str[INET_ADDRSTRLEN];
	inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );


	int size = list->size;
	if(list->search(data) != NULL) size--;

	strcat(response,"CLIENT_LIST");
	write(sockfd,response,strlen(response)+1);
	write(sockfd,&size,sizeof(size));
	bzero(response,sizeof(response));

	temp = list->head;
	while(temp != NULL){
		if((temp->d.ip_addr == ntohl(client_addr.sin_addr.s_addr))){ 
			// if saved ip is the same as the client do nothing
		}else{

			unsigned long ip = htonl(temp->d.ip_addr);
			int port = htons(temp->d.portnum);

			write(sockfd,&ip,sizeof(ip));
			write(sockfd,&port,sizeof(port));
		}
		temp = temp->next;
	}
}

/*
*	Connect to all logged clients and inform of new arrival.
*
*
*/

void notify_clients(client_list* list,client_data data){
	node* temp = list->head;
	struct sockaddr_in client;
	struct sockaddr* clientptr = (struct sockaddr*) &client;
	int sock;
	int client_port;
	char* client_ip;
	char message[BUFF] ="";
	char convs[BUFF] = "";
	memset(convs,0,BUFF);
	memset(message,0,BUFF);


	while(temp != NULL){
		if(temp->d.ip_addr == data.ip_addr){
			temp = temp->next;
			continue;
		}
		client.sin_family = AF_INET;
		uint32_t ip = htonl(temp->d.ip_addr);
		memcpy(&client.sin_addr,&ip,sizeof(ip));
		client.sin_port = htons(temp->d.portnum);

		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("socket");
			exit(-2);
		}



		if (connect(sock, clientptr, sizeof(client)) < 0)
		{
			perror("notify connect");
            exit(1);
        }
		
		strcat(message,"USER_ON ");
		sprintf(convs,"%u",htonl(data.ip_addr));
		strcat(message,convs);
		strcat(message," ");

		sprintf(convs,"%u",htons(data.portnum));
		strcat(message,convs);


		write(sock, message, BUFF);
		memset(message,0,BUFF);
		memset(convs,0,BUFF);
		temp = temp->next;
		close(sock);
	}
	
}

/*
*	Log incoming client.
*
*
*/

void log_client(char* buff,struct sockaddr_in client_addr,client_list* list,int sockfd){
		struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
		struct in_addr ipAddr = pV4Addr->sin_addr;
		char str[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );

		cout << "accepted connection from " << str <<  endl;

		int count = 0;
		char* ptr = NULL;
		ptr = buff;
		while((ptr = strchr(ptr,' ')) != NULL){
			count++;
			ptr++;
		}

		if(count != 2){
			char msg[] = "invalid request";
			write(sockfd,msg,strlen(msg)+1);
			return;
		}

		char* token = NULL;
		ptr = NULL;
		token = strtok(buff," ");

		token = strtok(NULL," ");

		unsigned long nwlong = strtoul(token,&ptr,10);

		token = strtok(NULL," ");

		int portnum = atoi(token);


		client_data data;

		data.ip_addr = ntohl(nwlong);
		data.portnum = ntohs(portnum);

		notify_clients(list,data);
		list->append(&data);
	
}


void remove_client(int sockfd,client_list* list,struct sockaddr_in client_addr){
	cout << "Received log off msg" << endl;
	struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client_addr;
	struct in_addr ipAddr = pV4Addr->sin_addr;

	uint32_t ip =0;
	int port = 0;

	read(sockfd,&ip,sizeof(ip));
	read(sockfd,&port,sizeof(port));


	client_data data;
	data.ip_addr = ntohl(ip);
	data.portnum = ntohs(port);
	node* res = list->search(data);
	char msg[BUFF];
	memset(msg,0,BUFF);


	if(res == NULL){
		strcat(msg,"ERROR_IP_PORT_NOT_FOUND_IN_LIST");
		write(sockfd,msg,BUFF);
		return;
	}

	
	strcat(msg,"REMOVED_FROM_LIST");
	write(sockfd,msg,BUFF);

	node* temp = list->head;

	struct sockaddr_in client;
	struct sockaddr* clientptr = (struct sockaddr*) &client;
	char log_off[BUFF];
	memset(log_off,0,BUFF);
	strcat(log_off,"LOG_OFF");


	while(temp!= NULL){

		client.sin_family = AF_INET;
		uint32_t clip = htonl(temp->d.ip_addr);
		memcpy(&client.sin_addr,&clip,sizeof(clip));
		client.sin_port = htons(temp->d.portnum);

		int sock;
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("socket");
			exit(-2);
		}


		if (connect(sock, clientptr, sizeof(client)) < 0)
		{
			perror(" remove connect");
            exit(1);
        }

        write(sock,log_off,BUFF);
        write(sock,&ip,sizeof(ip));
        write(sock,&port,sizeof(port));
        close(sock);



		temp = temp->next;
	}

	list->remove(data);

}


int main(int argc,char* argv[]){
	client_list *list = new client_list();

	if(argc != 3){
		cout << "Wrong number of arguments.Try ./dropbox_server.out $PORT_NUM" << endl;
		exit(1);
	}

	int port = atoi(argv[2]);

	if(port == 0){
		cout << "Incorrect port num" << endl;
		exit(1);
	}


	/*
	*	Create socket
	*/

	int sockfd = -1;
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == 0){
		cout << "Cannot create socket" << endl;
		exit(1);
	} 


	int addr = bind_port(sockfd,port);

	if(addr < 0 ){
		cout << "bind error " << endl;
		exit(1);
	}


	if(listen(sockfd,10) < 0){
		cout << "Listen error " << endl;
		exit(1);
	}

	cout << "Awaiting requests on " << port<< endl;


	//get requests

	struct pollfd reqs[1];
	reqs[0].fd = sockfd;
	reqs[0].events = POLLIN;
	int sock_acc;

	while(1){

		int retval;
		cout << "Waiting for new connection " << endl;
		retval = poll(reqs,1,TIMEOUT); //timeout in milliseconds
		if(retval < 0){
			cout << "Timeout on request" << endl;
			exit(1);
		}

		if(reqs[0].revents == POLLIN){
			struct sockaddr_in client_addr;
			unsigned int len = sizeof(client_addr);

			sock_acc = accept(sockfd,(struct sockaddr *)&client_addr,&len);

			char buff[1024] ="";
			while(read(sock_acc,buff,sizeof(buff)) > 0){
				if(buff[strlen(buff)-1] == '\n') buff[strlen(buff)-1] = '\0';
				
				if(strstr(buff,"LOG_ON ")) log_client(buff,client_addr,list,sock_acc);
				if(strstr(buff,"GET_CLIENTS")) get_clients(sock_acc,client_addr,list);
				if(strstr(buff,"LOG_OFF")) remove_client(sock_acc,list,client_addr);
				if(strstr(buff,"stop")) break;
			}
			close(sock_acc);
		}



	}
	shutdown(sockfd,2);
	close(sockfd);

}