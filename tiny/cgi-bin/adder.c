/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0, n2 = 0;

  /*Extract the two arguments*/
  if((buf = getenv("QUERY_STRING")) != NULL){
    p = strchr(buf, '&');
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p+1);
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }


  /*Make the response body */
  //content = File pointer -> 파일에 응답 body 작성
  sprintf(content, "QUERY_STRING=%s", buf); // 받은 인자 출력
  sprintf(content,"Welcome to add.com: ");
  sprintf(content,"%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1,n2, n1+n2); // 연산 결과 출력.
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /*Generate the HTTP response*/
  /*adder.c는 부모로부터 fork()된 자식프로세스로부터 excute되었기 때문에
  부모는 자식이 생성한 컨텐츠의 종류와 크기를 알지 못한다. 따라서 응답 헤더와 헤더를 종료하는 빈줄까지 생성해야한다.*/
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout); //출력버퍼를 비운다.

  exit(0);
}
/* $end adder */
