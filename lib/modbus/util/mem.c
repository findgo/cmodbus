
#include "mem.h"
#include "heap.h"


void *KMalloc(size_t size) {
    return pvPortMalloc(size);
    //return malloc(size);
}

void KFree(void *ptr) {
    vPortFree(ptr);
    //free(ptr);
}

void *KCalloc(size_t nmemb, size_t size) {
    void *addr = NULL;

    addr = (void *) pvPortMalloc(size * nmemb);
    memset(addr, 0, size * nmemb);

    return addr;
    //return calloc(nmemb, size);
}

void *KRealloc(void *ptr, size_t size) {
    void *addr = NULL;

    addr = (void *) pvPortMalloc(size);
    if (addr) {
        memset(addr, 0, size);
        memcpy(addr, ptr, size);
        vPortFree(ptr);
    }

    return addr;
    //return realloc(ptr, size);
}

