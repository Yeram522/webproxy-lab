#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void handle_client(int connfd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void forward_request(rio_t *client_rio, int serverfd, char *method, char *path, char *version, char *hostname);

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    printf("Proxy server is running on port %s\n", argv[1]);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        handle_client(connfd);
        Close(connfd);
    }
    return 0;
}

void handle_client(int connfd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    rio_t client_rio, server_rio;

    Rio_readinitb(&client_rio, connfd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE)) 
        return;

    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET")) {
        clienterror(connfd, method, "501", "Not Implemented", "Proxy does not implement this method");
        return;
    }

    parse_uri(uri, hostname, port, path);

    int serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0) {
        clienterror(connfd, uri, "502", "Bad Gateway", "Cannot connect to server");
        return;
    }

    Rio_readinitb(&server_rio, serverfd);

    forward_request(&client_rio, serverfd, method, path, version, hostname);

    // Forward response to client
    size_t n;
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0) {
        Rio_writen(connfd, buf, n);
    }

    Close(serverfd);
}

void parse_uri(char *uri, char *hostname, char *port, char *path) {
    char *hoststart = strstr(uri, "//");
    if (hoststart == NULL) {
        hoststart = uri;
    } else {
        hoststart += 2;
    }

    char *hostend = strchr(hoststart, '/');
    if (hostend == NULL) {
        strcpy(path, "/");
        hostend = hoststart + strlen(hoststart);
    } else {
        strcpy(path, hostend);
        *hostend = '\0';
    }

    char *portstart = strchr(hoststart, ':');
    if (portstart == NULL) {
        strcpy(port, "80");  // Default HTTP port
    } else {
        *portstart = '\0';
        strcpy(port, portstart + 1);
    }

    strcpy(hostname, hoststart);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    // Build the HTTP response body
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy Web server</em>\r\n", body);

    // Print the HTTP response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void forward_request(rio_t *client_rio, int serverfd, char *method, char *path, char *version, char *hostname) {
    char buf[MAXLINE];

    // Forward request line
    sprintf(buf, "%s %s %s\r\n", method, path, version);
    Rio_writen(serverfd, buf, strlen(buf));

    // Forward request headers
    sprintf(buf, "Host: %s\r\n", hostname);
    Rio_writen(serverfd, buf, strlen(buf));
    Rio_writen(serverfd, user_agent_hdr, strlen(user_agent_hdr));

    sprintf(buf, "Connection: close\r\n");
    Rio_writen(serverfd, buf, strlen(buf));
    sprintf(buf, "Proxy-Connection: close\r\n");
    Rio_writen(serverfd, buf, strlen(buf));

    // Forward remaining headers
    Rio_readlineb(client_rio, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        if (strncasecmp(buf, "Host:", 5) && 
            strncasecmp(buf, "User-Agent:", 11) && 
            strncasecmp(buf, "Connection:", 11) && 
            strncasecmp(buf, "Proxy-Connection:", 17)) {
            Rio_writen(serverfd, buf, strlen(buf));
        }
        Rio_readlineb(client_rio, buf, MAXLINE);
    }
    Rio_writen(serverfd, "\r\n", 2);
}