#ifndef CACHE_H
#define CACHE_H
#include "csapp.h"

/* Struct for a cache line */
typedef struct cache_line cache_t;
struct cache_line
{
  
  int size;
  char uri[MAXLINE];
  char *content;
  cache_t *next;
  cache_t *prev;
};

/* Struct for the cache */
typedef struct cache_all cache;
struct cache_all
{
    cache_t *head;
    cache_t *tail;
    int size;
};

/* Variable for a cache */
cache* c;

/* Helper functions */
void cache_init();
void add_uri(char *uri, char *buf, int size);
cache_t *find_fit(char *uri);
void delete_uri();
void cache_free();

#endif
