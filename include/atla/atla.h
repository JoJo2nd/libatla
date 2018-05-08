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

#include "atla/atla_config.h"
#include "atla/atla_memhandler.h"
#include "atla/atla_ioaccess.h"
#include "atla/atla_schema.h"

#ifdef __cplusplus
extern "C" {
#endif//

typedef struct atIOAccess atIOAccess_t;
typedef struct atAtlaContext atAtlaContext_t;
typedef struct atAtlaDataBlob atAtlaDataBlob_t;
typedef struct atDataSchema atDataSchema_t;
typedef struct atMemoryHandler atMemoryHandler_t;
typedef struct atDataDesc atDataDesc_t;

atuint64 ATLA_API atGetAtlaVersion();
atUUID_t ATLA_API atBuildAtlaStringUUID(const atchar* /*str*/, atuint32 /*strlen*/);

atAtlaContext_t* ATLA_API atCreateAtlaContext(atMemoryHandler_t*);
atErrorCode ATLA_API atAddDataSchema(atAtlaContext_t*, atDataSchema_t*);
atErrorCode ATLA_API atDestroyAtlaContext(atAtlaContext_t*);

atAtlaDataBlob_t* ATLA_API atOpenAtlaDataBlob(atAtlaContext_t*, atIOAccess_t*, atuint32);
void ATLA_API atCloseAtlaDataBlob(atAtlaDataBlob_t*);
atErrorCode ATLA_API atSerialiseDataBlob(atAtlaDataBlob_t*);
atErrorCode ATLA_API atAddDataToBlob(atAtlaDataBlob_t*, const atchar* /*objectname*/, const atchar* /*schemaname*/, atuint32 /*count*/, void* /*data*/);
atErrorCode ATLA_API atAddTypelessDataToBlob(atAtlaDataBlob_t*, const atchar* /*objectname*/, atuint32 /*size*/, atuint32 /*count*/, void* /*data*/);

//TODO:
atuint ATLA_API atGetDataCount(atAtlaDataBlob_t*);
atErrorCode ATLA_API atGetDataDescByIndex(atAtlaDataBlob_t*, atuint, atDataDesc_t*);
atErrorCode ATLA_API atGetDataDescByName(atAtlaDataBlob_t*, const atchar* /*objectname*/, atDataDesc_t*);
atErrorCode ATLA_API atDeserialiseDataByIndex(atAtlaDataBlob_t*, atuint, void* /*output*/);
atErrorCode ATLA_API atDeserialiseDataByName(atAtlaDataBlob_t*, const atchar* /*objectname*/, void* /*output*/);

#ifdef __cplusplus
} //extern "C"
#endif//

#endif // ATLA_H__
