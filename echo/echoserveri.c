#include "csapp.h"

void echo(int connfd);

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

int main(int argc, char **argv){
    int listendfd, connfd; // 듣기 식별자, 연결 식별자
    socklen_t clientlen; // 클라이언트 요청 길이

    /*sockaddr_storage : 모든 종류의 소켓 주소를 저장할 수 있는 충분히 큰 구조체이다.*/
    /*clientaddr는 accept로 보내지는 소켓 주소 구조체. accept가 리턴하기 전에 clientaddr에는 연결의 다른 쪽 끝의 클라이언트 소켓 주소로 채워짐.*/
    struct sockaddr_storage clientaddr; /*Enough space for any address*/

    /*client array of hostname and port*/
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    //port  = argv[1]
    listendfd = Open_listenfd(argv[1]); // 요청을 받고 듣기 식별자 리턴

    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listendfd, (SA*)&clientaddr, &clientlen); //(SA*) Socket Adress로 형변환
        Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE,
        client_port, MAXLINE, 0); // 소켓 구조체에 대응되는 호스트와 서비스 이름 스트링으로 변환
        echo(connfd);
        Close(connfd);
    }

    exit(0);
    
}