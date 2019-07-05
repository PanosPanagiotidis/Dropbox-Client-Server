#ifndef TUPLE_LIST_H
#define TUPLE_LIST_H

#include <sys/types.h>
#include <stdint.h>

/*
*	Contains all data referring to a client.
*
*/

typedef struct client_data{
		uint32_t ip_addr;
		uint16_t portnum;
		client_data();
		int operator == (const client_data );
}client_data;



typedef struct node{
	client_data d;
	struct node* next;
	node();
}node;


/*
*	A client list made of node structs.Each node represents a client.
*
*/

class client_list {
	public:
		node* clients;
		int size;
		node* head;
		node* tail;
		client_list();
		~client_list();
		node* search(client_data );
		void append(client_data* );
		int remove(client_data );
		void print();	// mostly for debug reasons
};


#endif