.PHONY: clean zip

default: all

all:
	gcc -std=gnu99 -O3 -Wall -o pcc_server pcc_server.c -pthread
	gcc -std=gnu99 -O3 -Wall -o pcc_client pcc_client.c

clean:
	rm -f pcc_server pcc_client

zip:
	zip ex5_203521984 pcc_server.c pcc_client.c makefile