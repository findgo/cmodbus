/**
  ******************************************************************************
  * @file   mem_macros.h
  * @author   jgb
  * @version 
  * @date    
  * @brief    本文档宏用于控制堆的配置，
  ******************************************************************************
  * @attention      v1.1   	jgb		20170512重构 
  ******************************************************************************
  */
  /*  本堆管理全部移植于freertos 的堆管理，几乎未做任务修改，只修改了临界区的保护
  *  heap_1  只实现分配，不实现释放
  *  heap_2  实现分配和释放，但不支持碎片管理
  *  heap_3  对库文件malloc free做了临界保护
  *  heap_4  实现分配和释放,而且支持碎片管理
  *  heap_5 没研究，但也移植了
  *
  *  一般情况下对于单片机只要用heap_1或heap_1就行了，只分配不释放用heap_1也行
  *
  */
#ifndef __MEM_MACROS_H_
#define __MEM_MACROS_H_

#include <stdint.h>
#include "common_signal.h"

/* set support dynamic alloca compatibility. */
#define configSUPPORT_DYNAMIC_ALLOCATION    1
/* Default total heap size 17K*/
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 4 * 1024 ) )
/* set byte alignment */
#define portBYTE_ALIGNMENT			4

#define portPOINTER_SIZE_TYPE   uint32_t

/* set use malloc failed hook */
#define configUSE_MALLOC_FAILED_HOOK 0

/* for debug prinft malloc and free trace */
#define traceMALLOC( pvAddress, uiSize )
#define traceFREE( pvAddress, uiSize )
#define mtCOVERAGE_TEST_MARKER()
#define configASSERT( x )

typedef long BaseType_t;

#define pdFALSE			( ( BaseType_t ) 0 )
#define pdTRUE			( ( BaseType_t ) 1 )

#endif



