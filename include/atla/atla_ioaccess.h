/********************************************************************

	filename: 	atla_iostream.h	
	
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

#ifndef ATLA_IOSTREAM_H__
#define ATLA_IOSTREAM_H__

#include "atla/atla_config.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif//

typedef enum atSeekOffset
{
    eSeekOffset_Begin = SEEK_SET,
    eSeekOffset_Current = SEEK_CUR,
    eSeekOffset_End = SEEK_END
} atSeekOffset;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

typedef atuint32 (ATLA_CALLBACK *atIOReadProc)(void* pBuffer, atuint32 size, void* user);
typedef atuint32 (ATLA_CALLBACK *atIOWriteProc)(const void* pBuffer, atuint32 size, void* user);
typedef atuint64 (ATLA_CALLBACK *atIOSeekProc)(atuint64 offset, atSeekOffset from, void* user);
typedef atuint64 (ATLA_CALLBACK *atIOTellProc)(void* user);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

typedef struct atIOAccess
{
    atIOReadProc    readProc_;
    atIOWriteProc   writeProc_;
    atIOSeekProc    seekProc_;
    atIOTellProc    tellProc_;
    void*           user_;    
} atIOAccess_t;

#ifdef __cplusplus
} //extern "C"
#endif//

#endif // ATLA_IOSTREAM_H__
