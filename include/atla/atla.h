/********************************************************************

	filename: 	atla.h	
	
	Copyright (c) 15:10:2012 James Moran
	
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

#ifndef ATLA_H__
#define ATLA_H__

/*
 * Main Include for alta lib
 **/

#include "atla_config.h"

#ifdef __cplusplus
extern "C" {
#endif//

typedef struct atAtlaDataBlob_t;

atAtlaDataBlob_t* ATLA_API atCreateAtlaDataBlob(atMemoryHandler_t*);
void ATLA_API atDestroyAtlaDataBlob(atAtlaDataBlob_t*);
atuint ATLA_API atReadDataBlob(atAtlaDataBlob_t*, atIOStream_t*);
atuint ATLA_API atWriteDataBlob(atAtlaDataBlob_t*, atIOStream_t*);
atuint ATLA_API atAddTypeBlob(atAtlaDataBlob_t*, const atchar* /*objectname*/, atDataTypeDesc* /*typedef*/, void* /*data*/);
atuint ATLA_API atGetDataBlobCount(atAtlaDataBlob_t*);
void* ATLA_API atGetDataBlobByIndex(atAtlaDataBlob_t*, atuint);
void* ATLA_API atGetTypeBlobDataByUUID(atAtlaDataBlob_t*, const atchar* /*objectname*/);

#ifdef __cplusplus
} //extern "C"
#endif//

#endif // ATLA_H__