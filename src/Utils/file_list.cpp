#include "../../header/file_list.h"

using namespace std;


/*
*	Either init a file list with a directory to build a list based off of the files in the dir.
*	Or init a file list by appending files to it.
*
*/

void file_list::print(){
	fnode* head = this->head;
	while(head!= NULL){
		cout << "pathname is " << head->path<<endl;
	}
}


file_list::file_list(char* dir){
	this->dir = strdup(dir);
	this->list = NULL;
	this->len = 0;
	this->head = NULL;
	this->build_list(this->dir);
}

file_list::file_list(){
	this->dir = NULL;
	this->list = NULL;
	this->head = NULL;
	this->len = 0;
}

file_list::~file_list(){
	fnode* head = this->head;

	while(head != NULL){
		fnode* del = head;
		head = head->next;
		// free(del->path);
		delete del;
	}

	if(this->dir != NULL) free(this->dir);
}

fnode* file_list::search(char* path){

	fnode* temp = NULL;
	temp = this->head;
	while(temp!= NULL){
		if(!strcmp(path,temp->path)){

			return temp;	
		} 
		temp = temp->next;
	}


	return NULL;
}

void file_list::append(char* pathname,unsigned long version){

	if(this->list == NULL){
		this->list = new fnode;	
		this->head = this->list;
	}else{
		this->list->next = new fnode;
		this->list = this->list->next;
	}
	if(this->len == 0){
		this->head = this->list;
	}
	this->len++;
	fnode* temp = this->list;
	memset(temp->path,0,128);
	strcpy(temp->path,pathname);
	temp->timestamp = version;
	temp->next = NULL;
	temp->type ='F';

}

void file_list::build_list(char* top){
	DIR* main_dir;
	struct dirent *dir;
	char* inputdir = strdup(top);
	if((main_dir = opendir(inputdir)) == NULL){
		perror("Dir does not exist");
		exit(1);
	}

	char* filename = NULL;
	char* newpath = NULL;	
	int filesindir = 0;


	while((dir = readdir(main_dir)) != NULL){
		struct stat fstat;
		if(!(strcmp(dir->d_name,".")) || !(strcmp(dir->d_name,"..") )) continue;
		if(dir->d_ino == 0) continue;
		filesindir++;

		filename = (char*)calloc(strlen(dir->d_name)+strlen(inputdir)+3,sizeof(char));
		strcat(filename,inputdir);

		if(inputdir[strlen(inputdir)-1] !='/')
			strcat(filename,"/");

		strcat(filename,dir->d_name);

		if(stat(filename,&fstat) == -1){
			cout << "Unable to stat file in sender" << dir->d_name<<endl;
			exit(1);
		}


	if(S_ISDIR(fstat.st_mode)){//check for directory
			
				//If it is a directory call dir_reader with newpath
				newpath = (char*)calloc(strlen(inputdir)+2+strlen(filename),sizeof(char));
				memset(newpath,0,sizeof(newpath));
				strcpy(newpath,filename);
				this->build_list(newpath);

				free(filename);
				free(newpath);
				continue;
			
			}else{
				stat(filename,&fstat);
				//add file to list
				if(this->list == NULL){//empty
					this->list = new fnode;
					this->head = this->list;
				}else{
					this->list->next = new fnode;
					this->list = this->list->next;

				}

				this->list->timestamp = fstat.st_ctime;
				this->list->timestamp = 2;
				this->list->next = NULL;
				this->list->type = 'F';
				memset(this->list->path,0,128);
				strcpy(this->list->path,filename);
				this->len++;
				
				
				free(filename);


			}

	}

	closedir(main_dir);
	free(inputdir);

}


void file_list::print_list(){
	fnode* temp = this->head;
	while(temp != NULL){
		cout << temp->path << endl;
		cout << temp->timestamp << endl;
		temp = temp->next;
	}
}