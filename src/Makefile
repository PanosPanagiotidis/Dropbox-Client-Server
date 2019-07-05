CC=g++
CPPFLAGS = -g3 -std=c++11

all:dropbox_server dropbox_client

OBJDIR = obj

dropbox_server: ./obj/dropbox_server.o ./obj/tuple_list.o
	$(CC) $(CPPFLAGS) -o dropbox_server.out dropbox_server.o tuple_list.o

dropbox_client: ./obj/dropbox_client.o ./obj/tuple_list.o ./obj/threads.o ./obj/file_list.o
	$(CC) $(CPPFLAGS) -o dropbox_client.out ./obj/dropbox_client.o ./obj/tuple_list.o ./obj/threads.o ./obj/file_list.o -lpthread

./obj/dropbox_client.o: ./dropbox_client.cpp
	$(CC) $(CPPFLAGS) -c ./dropbox_client.cpp

./obj/dropbox_server.o: ./dropbox_server.cpp
	$(CC) $(CPPFLAGS) -c ./dropbox_server.cpp

./obj/tuple_list.o:	./tuple_list.cpp
	$(CC) $(CPPFLAGS) -c ./tuple_list.cpp

./obj/file_list.o:	./file_list.cpp
	$(CC) $(CPPFLAGS) -c ./file_list.cpp

./obj/threads.o:	./threads.cpp
	$(CC) $(CPPFLAGS) -c ./threads.cpp -lpthread

run_server:
	clear && ./dropbox_server.out 11234

run_client1:
	clear && dropbox_client.out -d test -p 11234 -w 4 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 1

run_client2:
	clear && dropbox_client.out -d test2 -p 11234 -w 4 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 2

run_client3:
	clear && dropbox_client.out -d test3 -p 11234 -w 4 -b 8 -sp 11234 -sip linuxvm01.di.uoa.gr -i 3

debug_server:
	clear && gdb --args dropbox_server.out 11234

debug_client1:
	gdb --args dropbox_client.out -d test -p 11234 -w 4 -b 4 -sp 11234 -sip linuxvm01.di.uoa.gr -i 1

debug_client2:
	gdb --args dropbox_client.out -d test2 -p 11234 -w 4 -b 4 -sp 11234 -sip linuxvm01.di.uoa.gr -i 2

debug_client3:
	gdb --args dropbox_client.out -d test3 -p 11234 -w 4 -b 4 -sp 11234 -sip linuxvm01.di.uoa.gr -i 3

val_client1:
	clear && valgrind --show-leak-kinds=all --leak-check=full --track-origins=yes --trace-children=yes ./dropbox_client.out -d test -p 11234 -w 1 -b 8 -sp 11234 -sip linux01di.uoa.gr -i 1

val_client2:
	clear && valgrind ./dropbox_client.out -d test2 -p 11234 -w 1 -b 8 -sp 11234 -sip linux01.di.uoa.gr -i 2

clean:
	rm -rf *.o
	rm -rf *.out
