CC=g++
#Это еще один комментарий. Он поясняет, что в переменной CFLAGS лежат флаги, которые передаются компилятору
#g++ -I/usr/include/libftdi1 -lftdi1 -std=c++0x main.cpp powerhub.cpp ads1x15c.cpp -o main
CFLAGS=-c -Wall -I/usr/include/libftdi1 -std=c++0x -pthread
LDFLAGS=-lftdi1 -lpthread

all: main

main: main.o powerhub.o ads1x15c.o snmp.o
	$(CC) $(LDFLAGS) main.o powerhub.o ads1x15c.o snmp.o -o main

main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp

powerhub.o: powerhub.cpp
	$(CC) $(CFLAGS) powerhub.cpp

ads1x15c.o: ads1x15c.cpp
	$(CC) $(CFLAGS) ads1x15c.cpp

snmp.o: snmp.cpp
	$(CC) $(CFLAGS) snmp.cpp

