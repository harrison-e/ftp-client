CC=gcc
CFLAGS=-std=c11 -w

4700ftp: ftp_client.o
	$(CC) $(CFLAGS) -o $@ $^
	 
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -f 4700ftp *.o
