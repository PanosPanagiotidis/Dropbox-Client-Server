#ifndef FILE_LIST_H
#define FILE_LIST_H

#include <iostream>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <dirent.h>
#include <sys/file.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

typedef struct fnode{
	struct fnode* next;
	char path[128];
	unsigned long timestamp;
	char type;
}fnode;

class file_list{
	public:
		char* dir;
		int len;
		fnode* head;
		fnode* list;
		file_list(char* );
		file_list();
		~file_list();
		void build_list(char* );
		void append(char* ,unsigned long );
		fnode* search(char* );
		void print();
		void print_list();
};

#endif