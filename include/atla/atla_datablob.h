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

typedef struct atUsedTypeData
{
    ATLA_LINKED_LIST_HEADER();
    atDataTypeDesc*     typeDesc_;
};

typedef struct atTypeDataBlob
{
    ATLA_LINKED_LIST_HEADER();
    atDataTypeDesc*     typeDesc_;
    void*               pointer_;
    atuint32            dataSize_;
    atuint32            objectID_;
};

typedef struct atAtlaDataBlob
{
    atAtlaFileHeader_t  header_;
    atuint32            statusCode_;
    atMemoryHandler_t*  memHandler_;
    atIOStream_t*       iostream_;
    atUsedTypeData*     usedTypesDescHead_;
    atTypeDataBlob*     typeBlobsHead_;
}atAtlaDataBlob_t;

#endif // ATLA_DATABLOB_H__