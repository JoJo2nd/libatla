/********************************************************************

	filename: 	atla_datablob.h	
	
	Copyright (c) 16:10:2012 James Moran
	
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

#ifndef ATLA_DATABLOB_H__
#define ATLA_DATABLOB_H__

#ifdef __cplusplus
extern "C" {
#endif//

#include "atla/atla_config.h"
#include "atla/atla_utils.h"
#include "atla/atla_schema.h"
#include "atla/atla_serialisationtypes.h"
#include "atla/atla_memhandler.h"
#include "atla/atla_ioaccess.h"

typedef struct atAtlaContext atAtlaContext_t;

typedef struct atUsedSchema
{
    ATLA_LINKED_LIST_HEADER(atUsedSchema);
    atDataSchema_t*    schema_;
} atUsedSchema_t;

typedef struct atDataBlob
{
    ATLA_LINKED_LIST_HEADER(atDataBlob);
    atUUID_t            objectID_;
    atDataSchema_t*     schema_;
    void*               pointer_;
    atuint32            elementCount_;
    atuint32            elementSize_; /* Only valid if schema is NULL */
} atDataBlob_t;

typedef struct atAtlaDataBlob
{
    atAtlaContext_t*    ctx_;
    atAtlaFileHeader_t  header_;
    atuint32            statusCode_;
    atuint32            props_;
    atMemoryHandler_t   memHandler_;
    atIOAccess_t        iostream_;
    atUsedSchema_t*     usedSchemaHead_;
    atDataBlob_t*       dataBlobsHead_;
    struct atDeserialiseInfo
    {
        atuint32                        headerMemSize_;
        void*                           headerMem_;
        atuint32                        schemaCount_;
        atSerialisedSchema_t*           schema_;
        atuint32                        objectCount_;
        atSerialisedObjectHeader_t*     objects_;
    } deserialeseInfo_;
} atAtlaDataBlob_t;

#ifdef __cplusplus
}//extern "C"
#endif//

#endif // ATLA_DATABLOB_H__