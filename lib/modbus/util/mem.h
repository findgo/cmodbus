/**
 * @file mem.h
 * @brief 内存分配api重定向
 * @author mo
 * @version v0.1.0
 * @date 2018-12-01
 *
 */
#ifndef __MEM_H__
#define __MEM_H__

#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief   malloc memory
* @param   size size of need
* @return  pointer the memory
*      - nil failed
*/
void *KMalloc(size_t size);

/**
 * @brief   free memory
 * @param   ptr pointer the memory you want to free
 * @return  None
 */
void KFree(void *ptr);

/**
 * @brief   在内存的动态存储区中分配nmemb个长度为size的连续空间，函数返回一个指向分配起始地址的指针,并初始化该内存空间为0
 * @param   nmemb 块个数
 * @param   size 块大小
 * @return  pointer the memory
 *      - nil failed
 */
void *KCalloc(size_t nmemb, size_t size);

/**
 * @brief  重新调成内存空间,不改变原有数据
 * @param   ptr pointer the memory header you want to change
 * @param   size new size of memory
 * @return  pointer the memory
 *      - nil failed
 */
void *KRealloc(void *ptr, size_t size);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif

