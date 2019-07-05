CC=g++
CPPFLAGS = -g3 -std=c++11

all:dropbox_server dropbox_client

dropbox_server: dropbox_server.o tuple_list.o
	$(CC) $(CPPFLAGS) -o dropbox_server.out dropbox_server.o tuple_list.o

dropbox_client: dropbox_client.o tuple_list.o threads.o tuple_list.o file_list.o
	$(CC) $(CPPFLAGS) -o dropbox_client.out dropbox_client.o tuple_list.o threads.o file_list.o -lpthread

dropbox_client.o: ./src/Dropbox_Client/dropbox_client.cpp
	$(CC) $(CPPFLAGS) -c ./src/Dropbox_Client/dropbox_client.cpp

dropbox_server.o: ./src/Dropbox_Server/dropbox_server.cpp
	$(CC) $(CPPFLAGS) -c ./src/Dropbox_Server/dropbox_server.cpp

tuple_list.o:	./src/Utils/tuple_list.cpp
	$(CC) $(CPPFLAGS) -c ./src/Utils/tuple_list.cpp

file_list.o:	./src/Utils/file_list.cpp
	$(CC) $(CPPFLAGS) -c ./src/Utils/file_list.cpp

threads.o:	./src/Utils/threads.cpp
	$(CC) $(CPPFLAGS) -c ./src/Utils/threads.cpp -lpthread

run_server:
	clear && ./dropbox_server.out -p 11234

run_client1:
	clear && dropbox_client.out -d datasets/ -p 11234 -w 4 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 1

run_client2:
	clear && dropbox_client.out -d datasets/ -p 11234 -w 6 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 2

run_client3:
	clear && dropbox_client.out -d datasets/c3 -p 11234 -w 4 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 3

debug_server:
	clear && gdb --args dropbox_server.out -p 11234

debug_client1:
	gdb --args dropbox_client.out -d datasets -p 11234 -w 6 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 1

debug_client2:
	gdb --args dropbox_client.out -d datasets -p 11234 -w 6 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 2

debug_client3:
	gdb --args dropbox_client.out -d datasets -p 11234 -w 4 -b 4 -sp 11234 -sip linuxvm01.di.uoa.gr -i 3

val_client1:
	clear && valgrind --tool=helgrind ./dropbox_client.out -d datasets/ -p 11234 -w 6 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 1

val_client2:
	clear && valgrind --tool=helgrind ./dropbox_client.out -d datasets/ -p 11234 -w 6 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 2

clean:
	rm -rf *.o
	rm -rf *.out
