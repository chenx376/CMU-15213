/*
 * Yue Zhao (yzhao6)
 ******************************************************************************
 *                                proxcy.c                                    *
 *                        A simple caching proxy server.                      *
 *                  18-600: Foundation of Computor Systems (Lab 8)            *
 *                                                                            *
 *  ************************************************************************  *
 *                               DOCUMENTATION                                *
 *                                                                            *
 * Based on the example of tiny and echo， I construct the basic structure of *
 * proxy. It listens to coming connections. After established a connection,   *
 * my proxy reads the request from cliend and parse the uri. Then it delivers *
 * the request to server the response to client.                              *
 * By creating multiple threads, the proxy can deal with cocurrent requests.  *
 * I use a double linked list to construct the cache, new cache line will be  *
 * add to end of the list; thus the start of the list is the cache line to be *
 * evicted. After read one cache, I move it to the end of the list.           *
 * I write my own wrapper functions to hand read/write error.                 *
 ******************************************************************************
 */
#include <stdio.h>
#include "csapp.h"
#include "cache.h"
#include "cache.c"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_close = "Connection: close\r\n";
static const char *proxy_conn_close = "Proxy-Connection: close\r\n";

/* Global Variable for lock */
sem_t mutex;
sem_t w;
int readcnt;

/* Helper functions */
void parse_uri(char *uri, char *hostname, char *path, char *port);
void serve(int fd, char* uri, char *hostname, 
    char *path, char *port);
void *doit(void *ptr);
void Rio_writen_revise(int fd, void *usrbuf, size_t n);
ssize_t Rio_readlineb_revise(rio_t *rp, void *usrbuf, size_t maxlen);

int main(int argc, char **argv)
{
    int listenfd;
    int *connfd_ptr;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t  tid;

    Sem_init(&mutex, 0, 1);
    Sem_init(&w, 0, 1);

    readcnt = 0;

    if (argc != 2) 
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    Signal(SIGPIPE, SIG_IGN);
    listenfd = Open_listenfd(argv[1]);
    cache_init();

    while (1) 
    {
        clientlen = sizeof(clientaddr);
        connfd_ptr = Malloc(sizeof(int));
        *connfd_ptr = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, doit, connfd_ptr);
    }
    cache_free();
    return 0;
}
/*
 * doit - handle one HTTP request/response transaction, revise from tiny.c
 */
void *doit(void *ptr)
{
    int fd = *((int*)ptr);
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char path[MAXLINE], hostname[MAXLINE], port[MAXLINE];
    rio_t rio;

    Pthread_detach(Pthread_self());
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb_revise(&rio, buf, MAXLINE))
    {
        return NULL;
    }

    sscanf(buf, "%s %s %s", method, uri, version);
    parse_uri(uri, hostname, path, port);


    P(&mutex);
    readcnt++;
    if (readcnt == 1)
    {
        P(&w);
    }
    V(&mutex);

    cache_t *hit = find_fit(uri);
    /* Hit in cache */
    if (hit != NULL)
    { 
        Rio_writen_revise(fd, hit->content, hit->size);  
    }
    
    P(&mutex);
    readcnt--;
    if (readcnt == 0)
    {
        V(&w);
    }
    V(&mutex);

    /* not in cache*/
    if (hit == NULL)
    {
        serve(fd, uri, hostname, path, port);
    }
    Free(ptr);
    Close(fd);

    return NULL;
}
/*
 * parse_uri - parse URI into hostname and path and port number
 *             revise from tiny.c.
 */
void parse_uri(char *uri, char *hostname, char *path, char *port)
{
    char *hostname_start = strstr(uri, "//");
    if (hostname_start != NULL)
    {
        strcpy(hostname, hostname_start + 2);
    }
    else
    {
        strcpy(hostname, uri);
    }

    char *path_start = index(hostname, '/');
    if (path_start == 0)
    {
        strcpy(path, "/index.html");
    }
    else
    {
        strcpy(path, path_start);
        *path_start = '\0';
    }    

    char *port_start = index(hostname, ':');
    if (port_start == 0)
    {
        strcpy(port, "80");
    } else {
        strcpy(port, port_start + 1);
        *port_start='\0';
    }
    return;
}
void serve(int fd, char* uri, char *hostname, 
    char *path, char *port) {

    int connfd_server_proxy;
    rio_t rio;
    char buf[MAXLINE];

    char buffer[MAX_OBJECT_SIZE];

    connfd_server_proxy = Open_clientfd(hostname, port);
    if (connfd_server_proxy < 0)
    {
        return;
    }    
    Rio_readinitb(&rio, connfd_server_proxy);
    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    sprintf(buf, "%sHost: %s\r\n", buf, hostname);
    sprintf(buf, "%s%s", buf, user_agent_hdr);
    sprintf(buf, "%s%s", buf, conn_close);
    sprintf(buf, "%s%s\r\n", buf, proxy_conn_close);

    Rio_writen_revise(connfd_server_proxy, buf, strlen(buf));

    int read_length;
    int sum = 0;
    while ((read_length = Rio_readlineb_revise(&rio, buf, MAXLINE)) != 0)
    {
        Rio_writen_revise(fd, buf, read_length);
        if ((sum + read_length) < MAX_OBJECT_SIZE)
        {
            memcpy(buffer + sum, buf, read_length);
        }
        sum += read_length;
    }
    if (sum < MAX_OBJECT_SIZE)
    {
        P(&w);
        while(sum + c->size > MAX_CACHE_SIZE)
        {
            delete_uri();
        }
        add_uri(uri, buffer, sum);
        V(&w);
    }    
    Close(connfd_server_proxy);
    return;  

}
/*
 * Rio_writen_revise: revise wrapper class from csapp.c, prevent
 *      termination from EPIPE.
 */
void Rio_writen_revise(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n && errno != EPIPE)
    {
        unix_error("Rio_writen error");
    }
}
/*
 * Rio_readlineb_revise: revise wrapper class from csapp.c, prevent
 *      termination from ECONNRESET.
 */
ssize_t Rio_readlineb_revise(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0 && errno != ECONNRESET)
    {
        unix_error("Rio_readlineb error");
    }
    return rc;
}
