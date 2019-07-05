#include "../../header/tuple_list.h"
#include <iostream>

using namespace std;

/*
* Constructor for client_data structs.
*/

client_data::client_data(){
	this->ip_addr = -1;
	this->portnum = -1;
}

/*
*	Not used now.
*/

int client_data::operator == (const client_data d){
	if((this->ip_addr == d.ip_addr)) return 1;
	return 0;
}

/*
* Constructor for node structs.
*/

node::node(){
	this->next = NULL;
}

/*
* Constructor for client_list.
*/

client_list::client_list(){
	this->clients = NULL;
	this->tail = NULL;
	this->size = 0;
}

/*
* Destructor for client_list.
*/

client_list::~client_list(){
	node* head = this->head;

	while(head != NULL){

		node* del = head;
		head = head->next;
		delete del;
	}

}

/*
*	Append data to client list.
*
*/

void client_list::append(client_data* data){

	node* temp = NULL;

	client_data d;
	d.ip_addr = data->ip_addr;
	d.portnum = data->portnum;

	temp = this->search(d);
	if(temp != NULL){
		cout << "Duplicate entry" << endl;
		return;
	}

	if(this->size == 0){
		this->clients = new node();
		temp = this->clients;
		this->head = temp;
	}else{
		this->clients->next = new node();
		temp = this->clients->next;
		this->clients = temp;
	}
	
	this->tail = temp;
	this->size++;
	temp->d.ip_addr = data->ip_addr;
	temp->d.portnum = data->portnum;
	temp->next = NULL;

}

/*
*	Search list for st client.
*
*/

node* client_list::search(client_data st){
	node* temp = this->head;

	if (this->size == 0) return NULL;

	while(temp != NULL){
		if(temp->d.ip_addr == st.ip_addr){
			return temp;
		} 

		temp = temp->next;
	}

	return NULL;
}

/*
*	Remove client d from the list.
*
*
*/ 
int client_list::remove(client_data d){
	node* temp = this->head;
	node* prev = this->head;

	if(this->search(d) == NULL) return 0;


	while(temp != NULL){

		if(temp->d.ip_addr == d.ip_addr){

			if(temp == this->head){

				this->head = this->head->next;

			}else if (temp == this->tail){

				prev->next = NULL;
				this->tail = prev;

			}else{

				prev->next = temp->next;
			}

			this->size--;
			delete temp;
			if(this->size == 0) this->head = NULL;
			return 1;
		}

		prev = temp;
		temp = temp->next;


	}
	return 0;
}

void client_list::print(){
	node* temp = this->clients;
	while(temp != NULL){
		cout << "ip is " << temp->d.ip_addr << endl;
		cout << "Port is " << temp->d.portnum << endl;
		temp = temp->next;
	}
}