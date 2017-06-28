/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved
*/


/*
 * Implementation of pvPortMalloc() and vPortFree() that relies on the
 * compilers own malloc() and free() implementations.
 *
 * This file can only be used if the linker is configured to to generate
 * a heap memory area.
 *
 * See heap_1.c, heap_2.c and heap_4.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */

#include <stdlib.h>
#include "mem_mange.h"

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
    void *pvReturn;
    halIntState_t bintstate;
    
	ENTER_SAFE_ATOM_CODE(bintstate)
	{
		pvReturn = malloc( xWantedSize );
		traceMALLOC( pvReturn, xWantedSize );
	}
	EXIT_SAFE_ATOM_CODE(bintstate);

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			vApplicationMallocFailedHook();
		}
	}
	#endif

	return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
    halIntState_t bintstate;
    
	if( pv )
	{
        ENTER_SAFE_ATOM_CODE(bintstate)
		{
			free( pv );
			traceFREE( pv, 0 );
		}
        EXIT_SAFE_ATOM_CODE(bintstate);
	}
}



