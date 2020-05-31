default: server
	gcc -o client client.c
server:
	gcc -o server server.c
dist:
	tar -cvzf 705124638.tar.gz server.c client.c Makefile README
clean:
	rm -f server client *.file