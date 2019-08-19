/**
 * @file   heap.h
 * @brief  堆api接口
 * @author mo
 * @date   2018-12-01
 */

#ifndef __HEAP_H_
#define __HEAP_H_

#include "heap_def.h"



//! byte alignment mask
#if portBYTE_ALIGNMENT == 32
#define portBYTE_ALIGNMENT_MASK     ( 0x001f )
#elif portBYTE_ALIGNMENT == 16
#define portBYTE_ALIGNMENT_MASK     ( 0x000f )
#elif portBYTE_ALIGNMENT == 8
#define portBYTE_ALIGNMENT_MASK     ( 0x0007 )
#elif portBYTE_ALIGNMENT == 4
#define portBYTE_ALIGNMENT_MASK     ( 0x0003 )
#elif portBYTE_ALIGNMENT == 2
#define portBYTE_ALIGNMENT_MASK     ( 0x0001 )
#elif portBYTE_ALIGNMENT == 1
#define portBYTE_ALIGNMENT_MASK     ( 0x0000 )
#else
#error "Invalid portBYTE_ALIGNMENT definition"
#endif

/**
* @brief   malloc memory
* @param   size size of need
* @return  pointer the memory
*      - nil failed
*/
void *pvPortMalloc(size_t xWantedSize);

/**
 * @brief   free memory
 * @param   ptr pointer the memory you want to free
 * @return  None
 */
void vPortFree(void *pv);

/**
 * @brief   get free memory
 * @return  size of free memory
 */
size_t xPortGetFreeHeapSize(void);

#if (configUSE_MALLOC_FAILED_HOOK == 1)
/**
 * @brief   application malloc failed hook
 * @return  None
 */
extern void vApplicationMallocFailedHook( void );
#endif


#endif

