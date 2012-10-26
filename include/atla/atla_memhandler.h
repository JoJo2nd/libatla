/********************************************************************

	filename: 	atla_memhandler.h	
	
	Copyright (c) 6:8:2012 James Moran
	
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.
	
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	
	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.
	
	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.
	
	3. This notice may not be removed or altered from any source
	distribution.

*********************************************************************/
#pragma once

#ifndef ATLA_MEMHANDLER_H__
#define ATLA_MEMHANDLER_H__

#include "atla/atla_config.h"

#ifdef __cplusplus
extern "C" {
#endif//

typedef void* (ATLA_CALLBACK *atMallocProc)(atsize_t size, void* user);
typedef void  (ATLA_CALLBACK *atFreeProc)(void* ptr, void* user);

typedef struct atMemoryHandler
{
    atMallocProc    memAlloc_;
    atFreeProc      memFree_;
    void*           memUser_;
} atMemoryHandler_t;

#ifdef __cplusplus
} //extern "C"
#endif//

#endif // ATLA_MEMHANDLER_H__
