httpd : httpd.o
	cc -o -pthread  httpd httpd.o
httpd.o: httpd.c
	cc -c httpd.c
