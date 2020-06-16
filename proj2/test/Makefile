default: server
	gcc -o client client.c
server:
	gcc -o server server.c
dist:
	tar -cvzf 705124638.tar.gz server.c client.c Makefile README
clean:
	rm -f server client *.file cout
work: workserver
	gcc -o wclient workingclient.c
workserver:
	gcc -o wserver workingserver.c
wclean:
	rm -f wserver wclient *.file cout
test:
	./server 8080 & ./client localhost 8080 short.txt > cshort
	diff short.txt 1.file > errors.txt
	./test_format.py cshort
	#./client localhost 8080 hii.txt >
	# final
	if [ -s errors.txt ]; \
	then \
		echo -e "\n\n *** :( smoke test failed *** "; \
	else \
		echo -e "\npassed bois"; \
	fi; \