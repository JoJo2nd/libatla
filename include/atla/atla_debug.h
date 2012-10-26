/********************************************************************

	filename: 	atla_debug.h	
	
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

#ifndef ATLA_DEBUG_H__
#define ATLA_DEBUG_H__

#include "atla_config.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif//

#ifdef ATAL_USE_ASSERT

#   define atAssertFail(msg)
#   define atAssert(cond) assert(cond)

#else

#   define atAssertFail(msg)
#   define atAssert(cond)

#endif

#define atCompileTimeAssert(cond, msg)

#ifdef __cplusplus
} //extern "C"
#endif//

#endif // ATLA_DEBUG_H__