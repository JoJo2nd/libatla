/********************************************************************

	filename: 	atla_datablob.c	
	
	Copyright (c) 17:10:2012 James Moran
	
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

#include "atla/atla_memhandler.h"
#include "atla/atla_datablob.h"
#include "atla/atla.h"
#include "atla/atla_context.h"
#include <memory.h>
#include <string.h>

#define ATLA_IOREAD(io, buf, size)      io->readProc_(buf, size, io->user_)
#define ATLA_IOSEEK(io, offset, from)   io->seekProc_(offset, from, io->user_)
#define ATLA_IOWRITE(io, buf, size)     io->writeProc_(buf, size, io->user_)
#define ATLA_IOTELL(io)                 io->tellProc_(io->user_)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ATLA_API atInitDataBlob(atAtlaDataBlob_t*, atMemoryHandler_t*);
atErrorCode ATLA_API atAddSchemaToBlob(atAtlaDataBlob_t*, atDataSchema_t*);
atErrorCode ATLA_API atSerialiseSchema(atIOAccess_t*, atDataSchema_t*);
atErrorCode ATLA_API atSerialiseNestedSchema(atAtlaDataBlob_t* ctx, atUUID_t schemaID, atuint32 count, atuint8* eledataptr, atIOAccess_t* iostream);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atAtlaDataBlob_t* ATLA_API atOpenAtlaDataBlob(atAtlaContext_t* ctx, atIOAccess_t* ioaccess, atuint32 flags)
{
    atMemoryHandler_t* mem = ctx->memCtx_;
    atAtlaDataBlob_t* datablob = NULL;
    void* tmpmem, *tempend;
    atuint32 totalsize;
    atDataSchema_t* schemap;

    if ((flags & ATLA_READ) && ioaccess->readProc_ == NULL) return NULL;
    if ((flags & ATLA_WRITE) && ioaccess->writeProc_ == NULL) return NULL;
    if ((flags & ATLA_WRITE) && (flags & ATLA_READ)) return NULL;

    datablob = mem->memAlloc_(sizeof(atAtlaDataBlob_t), mem->memUser_);
    atInitDataBlob(datablob, mem);
    datablob->ctx_ = ctx;
    datablob->iostream_ = *ioaccess;

    if (flags & ATLA_WRITE)
    {
        datablob->props_ = ATLA_WRITE;

        ATLA_IOREAD(ioaccess, &datablob->header_, sizeof(datablob->header_));
        totalsize = datablob->header_.dataStartOffset_-datablob->header_.headerSize_;
        tmpmem = mem->memAlloc_(totalsize, mem->memUser_);
        datablob->deserialeseInfo_.headerMemSize_ = totalsize;
        datablob->deserialeseInfo_.headerMem_ = tmpmem;
        ATLA_IOREAD(ioaccess, tmpmem, totalsize);
        datablob->deserialeseInfo_.objects_ = ((atuint8*)tmpmem) + (datablob->header_.tocOffset_ - datablob->header_.headerSize_);
        datablob->deserialeseInfo_.objectCount_ = (totalsize-datablob->header_.tocOffset_)/datablob->header_.tocSize_;
        datablob->deserialeseInfo_.schema_ = ((atuint8*)tmpmem) + (datablob->header_.schemaOffset_ - datablob->header_.headerSize_);
        datablob->deserialiseInfo_.schemaCount_ = 0;
        tempend = datablob->deserialiseInfo_.objects;
        while (tmpmem <= tempend)
        {
            schemap = tmpmem;
            ++datablob->deserialiseInfo_.schemaCount_;
            tmpmem += datablob->header.schemaSize_;
            tmpmem += schemap->elementCount_*datablob->header.schemaElementSize_;
        }
    }
    else if (flags & ATLA_READ)
    {
        datablob->props_ = ATLA_READ;
    }

    return datablob;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ATLA_API atCloseAtlaDataBlob(atAtlaDataBlob_t* datablob)
{
    atMemoryHandler_t* mem = &datablob->memHandler_;
    atDataBlob_t* tdbp = NULL, *tmptdbp = NULL;
    atUsedSchema_t* utdp = NULL, *tmputdp = NULL;
    
    for (tdbp = datablob->dataBlobsHead_; tdbp; tdbp = tmptdbp)
    {
        tmptdbp = tdbp->next_;
        mem->memFree_(tdbp, mem->memUser_);
    }

    for (utdp = datablob->usedSchemaHead_; utdp; utdp = tmputdp)
    {
        tmputdp = utdp->next_;
        mem->memFree_(tdbp, mem->memUser_);
    }

    mem->memFree_(datablob, mem->memUser_);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atuint ATLA_API atDeserialiseDataBlob(atAtlaDataBlob_t* ctx, atIOAccess_t* iostream)
{
    // TODO:
    return ATLA_NOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atuint ATLA_API atSerialiseDataBlob(atAtlaDataBlob_t* blob)
{
    atuint elementIdx = 0;
    atuint typeeleIdx = 0;
    atuint8* rootdataptr = NULL, *eledataptr = NULL;
    atUsedSchema_t* utdp = NULL;
    atDataBlob_t* tdbp = NULL;
    atDataSchema_t* typeSchema = NULL;
    atSchemaElement_t* eleDesc = NULL;
    atSerialisedObjectHeader_t objectHeader = {0};
    atuint32 elementSize = 0;
    atuint32 pointerRefID = ATLA_INVALIDPOINTERREF, currentObjectID = 0;
    atIOAccess_t* ios = &blob->iostream_;
    atuint64 tellof;
    atSerialisedTOC_t toc;

    ATLA_IOSEEK(ios, 0, eSeekOffset_Begin);
    ATLA_IOWRITE(ios, &blob->header_, sizeof(blob->header_));

    blob->header_.schemaOffset_ = ATLA_IOTELL(ios);

    //Walk the Type Desc structs and write them
    for (utdp = blob->usedSchemaHead_; utdp; utdp = utdp->next_)
    {
         blob->statusCode_ = atSerialiseSchema(ios, utdp->schema_);
         if (blob->statusCode_ != ATLA_EOK) return blob->statusCode_;
    }

    tellof = ATLA_IOTELL(ios);
    blob->header_.tocOffset_ = tellof;

    //Walk the Data pointers and serialise the data IDs make space for offsets
    for (tdbp = blob->dataBlobsHead_, currentObjectID = 0; tdbp; tdbp = tdbp->next_, ++currentObjectID)
    {
        toc.objectID_ = tdbp->objectID_;
        toc.fileoffset_ = tellof;
        if (tdbp->schema_)
        {
            tellof += tdbp->schema_->serialisedSize_;
        }
        else
        {
            tellof += tdbp->elementSize_*tdbp->elementCount_;
        }
        ATLA_IOWRITE(ios, &toc, sizeof(atSerialisedTOC_t));
    }

    blob->header_.dataStartOffset_ = ATLA_IOTELL(ios);

    //Walk the Data pointers and serialise the data
    for (tdbp = blob->dataBlobsHead_, currentObjectID = 0; tdbp; tdbp = tdbp->next_, ++currentObjectID)
    {
        rootdataptr = tdbp->pointer_;       
        
        if (tdbp->schema_)
        {
            //Write the object header.

            objectHeader.objectID_ = tdbp->objectID_;
            objectHeader.count_ = tdbp->elementCount_;
            objectHeader.typeID_ = tdbp->schema_->id_;

            ATLA_IOWRITE(ios, &objectHeader, sizeof(objectHeader));

            typeSchema = tdbp->schema_;
            for (elementIdx = 0; elementIdx < tdbp->elementCount_; ++elementIdx)
            {
                for (typeeleIdx = 0; typeeleIdx < typeSchema->elementCount_; ++typeeleIdx)
                {
                    eleDesc = typeSchema->elementArray_ + typeeleIdx;
                    eledataptr = rootdataptr+eleDesc->offset_;
                    if (eleDesc->nestedID_ != 0)
                    {
                        blob->statusCode_ = atSerialiseNestedSchema(blob, eleDesc->nestedID_, eleDesc->arrayCount_, eledataptr, ios);
                        if (blob->statusCode_ != ATLA_EOK) return blob->statusCode_;
                    }
                    else
                    {
                        ATLA_IOWRITE(ios, eledataptr, eleDesc->size_*eleDesc->arrayCount_);
                    }
                }

                rootdataptr += typeSchema->typeSize_;
            }
        }
        else
        {
            //Deal with typeless data blocks
            objectHeader.objectID_ = tdbp->objectID_;
            objectHeader.count_ = tdbp->elementCount_ | 0x80000000;
            objectHeader.typeID_ = tdbp->elementSize_;

            ATLA_IOWRITE(ios, rootdataptr,  tdbp->elementSize_*tdbp->elementCount_);
        }

    }

    //Write updated headers
    ATLA_IOSEEK(ios, 0, eSeekOffset_Begin);
    ATLA_IOWRITE(ios, &blob->header_, sizeof(blob->header_));

    return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atSerialiseSchema(atIOAccess_t* io, atDataSchema_t* typedesc)
{
    atuint i;
    atuint elementOffset = 0;
    atSerialisedSchema_t stypedef = {0};
    atSerialisedSchemaElement_t typeeledef = {0};
    atSchemaElement_t* elements = typedesc->elementArray_;

    stypedef.typeID_ = typedesc->id_;
    stypedef.dataSize_ = typedesc->serialisedSize_;
    stypedef.elementCount_ = typedesc->elementCount_;
    ATLA_IOWRITE(io, &stypedef, sizeof(stypedef));
    for (i = 0; i < stypedef.elementCount_; ++i)
    {
        memset(&typeeledef, 0, sizeof(atSerialisedSchemaElement_t));
        typeeledef.elementID_ = elements[i].id_;
        typeeledef.size_ = elements[i].size_;
        typeeledef.offset_ = elementOffset;
        typeeledef.arraycount_ = elements[i].arrayCount_;
        typeeledef.typeID_  = elements[i].nestedID_;
        typeeledef.flags_ |= elements[i].arrayCount_ > 1 ? ATLA_ARRAY_TYPE : 0;

        ATLA_IOWRITE(io, &typeeledef, sizeof(atSerialisedSchemaElement_t));
    }

    return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atAddDataToBlob(atAtlaDataBlob_t* blob, const atchar* objectname, const atchar* schemaname, atuint32 count, void* data)
{
    atDataBlob_t* tdbp = NULL, *tmptdbp = NULL, *newtdbp = NULL;
    atSchemaElement_t* eleDesc = NULL;
    atuint stringscratchsize = 0;
    atUUID_t schemaID;
    atDataSchema_t* schema;
    
    schemaID = atBuildAtlaStringUUID(schemaname, strlen(schemaname));
    schema = atContextGetDataSchema(blob->ctx_, schemaID);
    if (!schema) return ATLA_EBADSCHEMAID;

    blob->statusCode_ = atAddSchemaToBlob(blob, schema);
    if (blob->statusCode_ != ATLA_EOK) return blob->statusCode_;

    for (tdbp = blob->dataBlobsHead_; tdbp; tdbp = tdbp->next_)
    {
        // Adding the same memory pointer in this way is not supported. 
        // In the future, Atla may handle this in some clever way
        if (data >= tdbp->pointer_ && data < (void*)(((atuint8*)tdbp->pointer_)+(schema->typeSize_*tdbp->elementCount_))) return ATLA_DUPLICATE_DATA;
    }

    newtdbp = blob->memHandler_.memAlloc_(sizeof(atDataBlob_t), blob->memHandler_.memUser_);
    if (!newtdbp) return ATLA_NOMEM;

    newtdbp->objectID_ = atBuildAtlaStringUUID(objectname, strlen(objectname));
    newtdbp->schema_ = schema;
    newtdbp->pointer_ = data;
    newtdbp->elementCount_ = count;
    newtdbp->elementSize_ = 0;

    newtdbp->prev_ = NULL;
    newtdbp->next_ = blob->dataBlobsHead_;
    if (blob->dataBlobsHead_) blob->dataBlobsHead_->prev_ = newtdbp;
    blob->dataBlobsHead_ = newtdbp;

    return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atAddTypelessDataToBlob(atAtlaDataBlob_t* blob, const atchar* objectname, atuint32 size, atuint32 count, void* data)
{
    atDataBlob_t* tdbp = NULL, *newtdbp = NULL;

    for (tdbp = blob->dataBlobsHead_; tdbp; tdbp = tdbp->next_)
    {
        // Adding the same memory pointer in this way is not supported. 
        // In the future, Atla may handle this in some clever way
        if (data >= tdbp->pointer_ && data < (void*)(((atuint8*)tdbp->pointer_)+(size*count))) return ATLA_DUPLICATE_DATA;
    }

    newtdbp = blob->memHandler_.memAlloc_(sizeof(atDataBlob_t), blob->memHandler_.memUser_);
    if (!newtdbp) return ATLA_NOMEM;

    newtdbp->objectID_ = atBuildAtlaStringUUID(objectname, strlen(objectname));
    newtdbp->schema_ = 0;
    newtdbp->pointer_ = data;
    newtdbp->elementCount_ = count;
    newtdbp->elementSize_ = size;

    newtdbp->prev_ = NULL;
    newtdbp->next_ = blob->dataBlobsHead_;
    if (blob->dataBlobsHead_) blob->dataBlobsHead_->prev_ = newtdbp;
    blob->dataBlobsHead_ = newtdbp;

    return ATLA_EOK;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ATLA_API atInitDataBlob(atAtlaDataBlob_t* datablob, atMemoryHandler_t* mem)
{
    datablob->header_.fourCC_ = ATLA_FOURCC;
    datablob->header_.version_ = atGetAtlaVersion();

    datablob->header_.headerSize_ = sizeof(atAtlaFileHeader_t);
    datablob->header_.schemaSize_ = sizeof(atSerialisedSchema_t);
    datablob->header_.schemaElementSize_ = sizeof(atSerialisedSchemaElement_t);
    datablob->header_.tocSize_ = sizeof(atSerialisedTOC_t);
    datablob->header_.objectHdrSize_ = sizeof(atSerialisedObjectHeader_t);
    datablob->header_.schemaOffset_ = 0;
    datablob->header_.tocOffset_ = 0;
    datablob->header_.dataStartOffset_ = 0;

    memset(&datablob->iostream_, 0, sizeof(datablob->iostream_));
    datablob->memHandler_ = *mem;
    datablob->statusCode_ = ATLA_EOK;
    datablob->props_ = 0;
    datablob->dataBlobsHead_ = NULL;
    datablob->usedSchemaHead_ = NULL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atAddSchemaToBlob(atAtlaDataBlob_t* ctx, atDataSchema_t* schema)
{
    atUsedSchema_t* utdp = NULL, *tmputdp = NULL, *newutdp;

    for (utdp = ctx->usedSchemaHead_; utdp; utdp = utdp->next_)
    {
        if (utdp->schema_->id_ == schema->id_) return ATLA_EOK;
    }

    newutdp = ctx->memHandler_.memAlloc_(sizeof(atUsedSchema_t), ctx->memHandler_.memUser_);
    if (!newutdp) return ATLA_NOMEM;

    newutdp->schema_ = schema;

    tmputdp = ctx->usedSchemaHead_;
    if (tmputdp) tmputdp->prev_ = newutdp;
    newutdp->prev_ = NULL;
    newutdp->next_ = tmputdp;
    ctx->usedSchemaHead_ = newutdp;

    return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atSerialiseNestedSchema(atAtlaDataBlob_t* blob, atUUID_t schemaID, atuint32 count, atuint8* rootdataptr, atIOAccess_t* iostream) 
{
    atuint typeeleIdx;
    atuint rootele;
    atDataSchema_t* schema;
    atSchemaElement_t* eleDesc;
    atuint8* eledataptr;

    schema = atContextGetDataSchema(blob->ctx_, schemaID);
    if (!schema) return ATLA_EBADSCHEMAID;

    for (rootele = 0; rootele < count; ++rootele)
    {
        for (typeeleIdx = 0; typeeleIdx < schema->elementCount_; ++typeeleIdx)
        {
            eleDesc = schema->elementArray_ + typeeleIdx;
            eledataptr = rootdataptr+eleDesc->offset_;
            if (eleDesc->nestedID_ != 0)
            {
                blob->statusCode_ = atSerialiseNestedSchema(blob, eleDesc->nestedID_, eleDesc->arrayCount_, eledataptr, iostream);
                if (blob->statusCode_ != ATLA_EOK) return blob->statusCode_;
            }
            else
            {
                ATLA_IOWRITE(iostream, eledataptr, eleDesc->size_*eleDesc->arrayCount_);
            }
        }
        rootdataptr += schema->typeSize_;
    }
    return ATLA_EOK;

}

#undef ATLA_IOREAD
#undef ATLA_IOSEEK
#undef ATLA_IOWRITE
#undef ATLA_IOTELL

