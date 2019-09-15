ALL:echo-server

INC = -I../../include \
      -I../../build/include

LIB = -L../../build/lib \
      -levent-2.2 \


echo-server:echo-server.c
	gcc -Wall -g $(INC) $(LIB) -o $@ $<
