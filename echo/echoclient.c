#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio; //I/O package

    if(argc != 3){
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port); // 서버에 연결하고 연결된 소켓의 파일 디스크립터를 반환
    Rio_readinitb(&rio, clientfd);  // Rio 구조체를 초기화하고 clientfd와 연결

    while(Fgets(buf, MAXLINE, stdin) != NULL){ // 표준 입력에서 한줄 씩 읽기
        Rio_writen(clientfd, buf, strlen(buf)); // 읽고 서버에 전송
        Rio_readlineb(&rio, buf, MAXLINE); // 서버로부터 응답 받기
        Fputs(buf, stdout); // 서버의 응답을 표준 출력에 출력
    }
    Close(clientfd); //클라이언트 소켓 닫기
    exit(0);
}