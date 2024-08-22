/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;//listenfd : 듣기 선택자 , connfd : 연결 식별자
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]); // file stream에 err 출력. buffer와 상관없이 빠르게 출력하기 위해서 fprintf 사용.
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); //argv[1] : port
  while (1) { // 계속해서 client의 입력 요청을 받는다.
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, 
                    &clientlen);  // (SA *) : SocketAddress. Accept: 클라이언트 요청을 기다림.
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0); // 클라이언트 주소로부터 host나 service name을 알아냄.
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit -> 1개의 HTTP transection 처리
    Close(connfd);  // line:netp:tiny:close
  }
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /*Read request line and headers*/
  Rio_readinitb(&rio, fd); // connect file descripter and read buffer
  Rio_readlineb(&rio, buf, MAXLINE); // read MAXLINE-1 in rio and copy to buf. return add \0 end of the buf
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // 문자열을 지정된 formet("%s,%s,%s")으로 맞춰 저장.
  if(strcasecmp(method,"GET")){ //strcasecmp : = return 0 , str1< str2 return x<0 , str1 > str2 return x>0
    clienterror(fd, method,"501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio); //요청 헤더를 읽고 무시

  /*Parse URI from GET request*/
  is_static = parse_uri(uri, filename, cgiargs);
  if(stat(filename, &sbuf)< 0){
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if(is_static){/*Serve static content*/
    // 읽기 권한을 갖고 있는지 검증
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){//S_ISREG : 일반 파일 검사 , S_IRUSR : 파일 권한 비트 검사(사용자 읽기)
      clienterror(fd, filename, "403", "Forbidden",
        "Tiny couldn't read the file");

      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  else{/*Serve dynamic content*/
    //파일이 실행 가능한지 검증
    if(!(S_ISREG(sbuf.st_mode))||!(S_IXUSR & sbuf.st_mode)){ // S_IXUSR : excute 권한 허용 여부
      clienterror(fd, filename, "403", "Forbidden", 
      "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")){
    rio_readlineb(rp,buf, MAXLINE);
    printf("%s", buf);
  }
  
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if(!strstr(uri, "cgi-bin")){ /*Static content*/
    strcpy(cgiargs, ""); //static은 cgi를 쓰지않기 때문에 ""로 초기화
    strcpy(filename, ".");
    strcat(filename, uri);
    if(uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else{/*Dynamic content*/
    ptr = index(uri, '?'); //file과 argument 분리하는 ?의 위치 추출
    if(ptr){
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");

    //상대리눅스파일 이름으로 변환
    strcpy(filename, ".");
    strcat(filename,uri);

    return 0;
  }

}

void serve_static(int fd, char *filename, int filesize)
{ 
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  printf("thisis static \n");

  /*Send responese headers to client*/
  get_filetype(filename, filetype);

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);

  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /*Send response body to client*/
  srcfd = Open(filename, O_RDONLY, 0); //filename 파일을 열고 식별자를 반환한다.
  
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd,0); // 요청한 파일을 가상메모리 영역으로 매핑한다.
  Close(srcfd);  //메모리 누수 방지를 위해서 file close
  Rio_writen(fd, srcp, filesize); //client 연결 식별자로 복사
  Munmap(srcp , filesize); // 가상 메모리 주소 반환
}

void get_filetype(char *filename, char *filetype)
{
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if(strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");

}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /*Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if(Fork() == 0){/*Child*/
    /* Real server would set all CGI vars here*/
    setenv("QUERY_STRING", cgiargs, 1); //CGI 환경 변수 설정
    Dup2(fd, STDOUT_FILENO); /*Redirect Stdout to client*/
    Execve(filename, emptylist, environ); /* Run CGI program*/
  }

  Wait(NULL); /*Parent waits for and reaps child*/
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /*Build the HTTP response body*/
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /*Print the HTTP response*/
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));

}
