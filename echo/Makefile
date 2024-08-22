CC = gcc
CFLAGS = -g -Wall -I ..
# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

# 상위 폴더의 csapp.o 경로 지정
CSAPP_O = ../csapp

all: echoclient echoserver

echoserver: echoserver.c $(CSAPP_O)
	$(CC) $(CFLAGS) -o echoserver echoserver.c $(CSAPP_O) $(LIB)

echoclient: echoclient.c $(CSAPP_O)
	$(CC) $(CFLAGS) -o echoclient echoclient.c $(CSAPP_O) $(LIB)

# csapp.o에 대한 의존성 추가
$(CSAPP_O):
	$(MAKE) -C .. csapp.o

clean:
	rm -f *.o echoserver echoclient *~