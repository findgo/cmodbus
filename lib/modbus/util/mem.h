

#ifndef __MEM_H__
#define __MEM_H__

#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

void *KMalloc(size_t size);

void KFree(void *ptr);

void *KCalloc(size_t nmemb, size_t size);

void *KRealloc(void *ptr, size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif

