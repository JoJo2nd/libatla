/********************************************************************

	filename: 	atla_serialisationtypes.h	
	
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

#ifndef ATLA_SERIALISATIONTYPES_H__
#define ATLA_SERIALISATIONTYPES_H__

#ifdef __cplusplus
extern "C" {
#endif//

#pragma pack(push, 1)

#define ATLA_FOURCC (atuint32)(('A'<<24)|('T'<<16)|('L'<<8)|('A'))
#define ATLA_ARRAY_TYPE     (1)
#define ATLA_POINTER_TYPE   (1<<1)
#define ATLA_ATOMIC_TYPE    (1<<2)
#define ATLA_C_STRING_TYPE  (1<<3)
#define ATLA_USER_TYPE      (1<<4)
#define ATLA_DATA_REFERENCE (1<<5)

typedef struct atAtlaFileHeader
{
    atuint32    fourCC_; /* <- ATLA */
    atuint64    version_;
    atuint32    headerSize_;
    atuint32    schemaSize_;
    atuint32    schemaElementSize_;
    atuint32    tocSize_;
    atuint32    objectHdrSize_;
    atuint64    schemaOffset_;
    atuint64    tocOffset_;
    atuint64    dataStartOffset_;
} atAtlaFileHeader_t;

typedef struct atSerialisedSchema
{
    atUUID_t    typeID_;
    atuint32    dataSize_;          
    atuint32    elementCount_;      /* array of atSerialisedTypeElementDef[elementCount_] follows this. 0 is for atomic types*/
} atSerialisedSchema_t;

typedef struct atSerialisedSchemaElement
{
    atUUID_t    elementID_;
    atuint32    size_;
    atuint32    offset_;
    atuint32    arraycount_;/* set to 1 for non array types */
    atUUID_t    typeID_;/* UUID of type, only valid if ATLA_USER_TYPE is set in flags_*/
    atuint8     flags_;
} atSerialisedSchemaElement_t;

typedef struct atSerialisedTOC
{
    atUUID_t objectID_;
    atuint64 fileoffset_;
} atSerialisedTOC_t;

typedef struct atSerialisedObjectHeader
{
    atUUID_t    objectID_;
    atUUID_t    typeID_;
    atuint32    count_;     /* if top bit of count_ is set, this is a raw block and 
                            typeID is not an ID but a size. Bad dual meaning but 
                            we want atSerialisedObjectHeader to be small*/
} atSerialisedObjectHeader_t;

#pragma pack(pop)

#ifdef __cplusplus
} //extern "C"
#endif//

#endif // ATLA_SERIALISATIONTYPES_H__
