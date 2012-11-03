/********************************************************************

	filename: 	atla_config.h	
	
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

#ifndef ATLA_CONFIG_H__
#define ATLA_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif//

#if defined (_DEBUG)
#   define ATLA_DEBUG
#else 
#   define ATLA_RELEASE
#endif

#ifdef ATLA_DEBUG
#   define ATAL_USE_ASSERT
#endif

#define ATLA_CALLBACK   __cdecl
#define ATLA_API        __cdecl

//#define ATLA_USE_ASSERT (0)

#if !defined (ATLA_USE_ASSERT)
#   define ATLA_USE_ASSERT (1)
#endif 

#ifdef COMPILE_LIB_ATLA
#   define ATLA_ENSURE_PRIVATE_HEADER() 
#else
#   define ATLA_ENSURE_PRIVATE_HEADER()   atCompileTimeAssert(0 && "This header shouldn't be included outside of libatla")
#endif

#ifdef __cplusplus
} //extern "C"
#endif//

#include "atla_types.h"

#endif // ATLA_CONFIG_H__
