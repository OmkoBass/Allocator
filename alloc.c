#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef char ALIGN[16]; //We need the 16 bytes so header will be aligned to 16 bytes

union header
{
    struct
    {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;  //When we need bytes of memory, our total size is header + size
    ALIGN stub;
};
typedef union header header_t;

header_t *Front, *Back; //We need this to keep track of our list

pthread_mutex_t global_malloc_lock; //We need this so 2 or more threads 
                                    //can't access memory at the same time

header_t *get_free_block(size_t size)
{
    header_t *current = Front;
    while(current)  //Go through the list and find if a block is "free"
    {
        if(current->s.is_free && current->s.size >= size)
            return current;
        current = current->s.next;
    }
    return NULL;
}

void *malloc(size_t size)
{
    size_t total;
    void *block;
    header_t *header;
    if(!size)   //if the size we want is zero we return null
        return NULL;
    pthread_mutex_lock(&global_malloc_lock); //Lock it
    header = get_free_block(size);  //Get the memory we need
    if(header)
    {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);  //Unlock it
        return (void*)(header + 1);
    }

    total = sizeof(header_t) + size;
    block = sbrk(total); //Increase heap memory if we didn't find a "free" spot
    if(block == (void*) - 1)
    {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;

    if(!Front)
        Front = header;
    if(!Back)
        Back->s.next = header;
    Back = header;
    pthread_mutex_unlock(&global_malloc_lock);

    return (void*)(header + 1); //We want + 1, the header + 1 is the byte we need after the header
}

void free(void *block)
{
    header_t *header, *temp;
    void *Break;

    if(!block)
        return;
    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*) block - 1;

    Break = sbrk(0);
    if((char*) block + header->s.size == Break)
    {
        if(Front == Back)
        {
            Front = NULL;
            Back = NULL;
        }
        else
        {
            temp = Front;
            while(temp)
            {
                if(temp->s.next == Back)
                {
                    temp->s.next = NULL;
                    Back = temp;
                }
                temp = temp->s.next;
            }
        }
        sbrk(0 - sizeof(header_t) - header->s.size);
        pthread_mutex_unlock(&global_malloc_lock);
        return ;
    }
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

int main()
{
    printf("MAIN MAIN MAIN");
    char *p = malloc(sizeof(char) * 4);
    printf("%ld", strlen(p));

    return 0;
}