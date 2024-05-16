# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy_utils.o: proxy_utils.c proxy_utils.h
	$(CC) $(CFLAGS) -c proxy_utils.c

integer_queue.o: integer_queue.c integer_queue.h csapp.h
	$(CC) $(CFLAGS) -c integer_queue.c

cache.o: cache.c cache.h proxy_utils.h csapp.h
	$(CC) $(CFLAGS) -c cache.c

http_request_parser.o: http_request_parser.c http_request_parser.h proxy_utils.h csapp.h
	$(CC) $(CFLAGS) -c http_request_parser.c

proxy.o: proxy.c http_request_parser.h cache.h integer_queue.h proxy_utils.h csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o http_request_parser.o cache.o integer_queue.o proxy_utils.o csapp.o
	$(CC) $(CFLAGS) proxy.o http_request_parser.o cache.o integer_queue.o proxy_utils.o csapp.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o ./tiny/*.o ./tiny/tiny proxy core *.tar *.zip *.gzip *.bzip *.gz test
	rm -rf ./.noproxy ./.proxy
