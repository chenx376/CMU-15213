#ifndef CACHE_C
#define CACHE_C

#include "cache.h"

/* cache_init: initialize the cache */
void cache_init()
{
    c = (cache *)malloc(sizeof(cache));
    c->head = NULL;
    c->tail = NULL;
    c->size = 0;
}

/* add_uri: Add the new cache line to the end of cache */
void add_uri(char *uri, char *buf, int size)
{
    /* Set up the new cache line */
    cache_t * new_line = (cache_t *)malloc(sizeof(cache_t));
    new_line->content = malloc(size);
    memcpy(new_line->content, buf, size);
    new_line->size = size;
    strcpy(new_line->uri, uri);

    /* add to cache */
    /* No line in the cache */
    if (c->head == NULL)
    {
        c->head = new_line;
        c->tail = new_line;
        new_line->prev = NULL;
        new_line->next = NULL;
    }
    /* Add after tail */
    else
    {
        c->tail->next = new_line;
        new_line->prev = c->tail;
        new_line->next = NULL;
        c->tail = c->tail->next; 
    }    
    c->size += size;
}
/* find_fit: if exist, find the cached content, move the cache line
 *           to the end of cache, else return NULL.
 */
cache_t *find_fit(char *uri)
{
    cache_t *curr = c->head;
    while(curr != NULL)
    {
        // strcmp: return 0 when equal
        if (!strcmp(uri, curr->uri))
        {
            /* if curr is head, need to move head pointer */
            if (c->head == curr)
            {
                /* More than one cache line, move curr(head) after tail */
                if (c->tail != curr)
                {    
                    c->head = c->head->next;
                    curr->next->prev = NULL;
                    c->tail->next = curr;
                    curr->prev = c->tail;
                    curr->next = NULL;
                    c->tail = curr;
                }
            }
            /* curr is not head */
            else
            {
                /* if curr is tail, no need to move it */
                if (c->tail != curr)
                {   
                    curr->next->prev = curr->prev;
                    curr->prev->next = curr->next;
                    curr->prev = c->tail;
                    c->tail->next = curr;
                    curr->next = NULL;
                    c->tail = curr;
                }
            }    
            return curr;
        }
        curr = curr->next;  
    }
    return NULL;  
}

/** delete_uri: Delete the first cache line in cache*/
void delete_uri()
{
    cache_t *temp = c->tail;
    /* Only one cache line in cache */
    if (c->head->next == NULL)
    {
        c->head = NULL;
        c->tail = NULL;

    }
    /* Delete the first one */
    else
    {
        c->head->next->prev = NULL;
        c->head = c->head->next;

    }
    /** Free space in the deleted one */
    free(temp->content);
    free(temp);   
}
/* cache_free: free the whole cache */
void cache_free()
{
    cache_t *curr;
    curr = c->head;
    cache_t *temp;
    while(curr != NULL)
    {
        temp = curr;
        curr = curr->next;
        free(temp->content);
        free(temp);
    }
    free(c);
}
#endif