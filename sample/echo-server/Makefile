ALL:echo-server echo-client

INC = -I../../include/ \
      -I/root/ftp/learning-libevent/build/include/ \

echo-server:echo-server.c
	gcc -Wall -g $(INC) -levent-2.2 -o $@ $<

echo-client:echo-client.c
	gcc -Wall -g -static $(LIB) -o $@ $<

.PHONY :
clean:
	rm -rf echo-server echo-client