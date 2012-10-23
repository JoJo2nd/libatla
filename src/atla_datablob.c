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
atErrorCode ATLA_API atSerialiseSchema(atIOStream_t*, atDataSchema_t*);
atErrorCode ATLA_API atResolvePointer(atAtlaDataBlob_t*, void*, atuint32*);
atErrorCode ATLA_API atSerialiseNestedSchema(atAtlaDataBlob_t* ctx, atDataSchema_t* dataDesc, atuint8* eledataptr, atIOStream_t* iostream);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atAtlaDataBlob_t* ATLA_API atCreateAtlaDataBlob(atAtlaContext_t* ctx)
{
    atMemoryHandler_t* mem = ctx->memCtx_;
    atAtlaDataBlob_t* datablob = mem->tempAlloc_(sizeof(atAtlaDataBlob_t), mem->memUser_);
    datablob->ctx_ = ctx;
    atInitDataBlob(datablob, mem);
    return datablob;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ATLA_API atDestroyAtlaDataBlob(atAtlaDataBlob_t* datablob)
{
    atMemoryHandler_t* mem = &datablob->memHandler_;
    atDataBlob_t* tdbp = NULL, *tmptdbp = NULL;
    atUsedSchema_t* utdp = NULL, *tmputdp = NULL;
    
    for (tdbp = datablob->dataBlobsHead_; tdbp; tdbp = tmptdbp)
    {
        tmptdbp = tdbp->next_;
        mem->tempFree_(tdbp, mem->tempUser_);
    }

    for (utdp = datablob->usedSchemaHead_; utdp; utdp = tmputdp)
    {
        tmputdp = utdp->next_;
        mem->tempFree_(tdbp, mem->tempUser_);
    }

    mem->tempFree_(datablob, mem->memUser_);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atuint ATLA_API atDeserialiseDataBlob(atAtlaDataBlob_t* ctx, atIOStream_t* iostream)
{
    // TODO:
    return ATLA_NOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atuint ATLA_API atSerialiseDataBlob(atAtlaDataBlob_t* ctx, atIOStream_t* iostream)
{
    atuint elementIdx = 0;
    atuint typeeleIdx = 0;
    atuint8* rootdataptr = NULL, *eledataptr = NULL;
    atUsedSchema_t* utdp = NULL;
    atDataBlob_t* tdbp = NULL;
    atDataSchema_t* dataDesc = NULL;
    atSchemaElement_t* eleDesc = NULL;
    atSerialisedObjectHeader_t objectHeader = {0};
    atuint32 elementSize = 0;
    atuint32 pointerRefID = ATLA_INVALIDPOINTERREF, currentObjectID = 0;

    ATLA_IOSEEK(iostream, 0, eSeekOffset_Begin);
    ATLA_IOWRITE(iostream, &ctx->header_, sizeof(ctx->header_));

    //Walk the Type Desc structs and write them
    for (utdp = ctx->usedSchemaHead_; utdp; utdp = utdp->next_)
    {
        atSerialiseSchema(iostream, utdp->schema_);
    }

    //Walk the Data pointers and serialise the data
    for (tdbp = ctx->dataBlobsHead_, currentObjectID = 0; tdbp; tdbp = ctx->dataBlobsHead_, ++currentObjectID)
    {
        rootdataptr = tdbp->pointer_;       
        //Write the object header.
        objectHeader.objectID_ = tdbp->objectID_;
        objectHeader.referenceID_ = currentObjectID;
        objectHeader.diskSize_ = 0;//TODO:
        objectHeader.count_ = tdbp->elementCount_;
        if (tdbp->schema_)
            objectHeader.typeID_ = tdbp->schema_->id_;
        else
            memset(&objectHeader.typeID_, 0, sizeof(atUUID_t));

        ATLA_IOWRITE(iostream, &objectHeader, sizeof(objectHeader));

        dataDesc = tdbp->schema_;
        for (elementIdx = 0; elementIdx < tdbp->elementCount_; ++elementIdx)
        {
            for (typeeleIdx = 0; typeeleIdx < dataDesc->elementCount_; ++typeeleIdx)
            {
                eleDesc = dataDesc->elementArray_ + typeeleIdx;
                eledataptr = rootdataptr+eleDesc->offset_;
                if (eleDesc->pointer_)
                {
                    ctx->statusCode_ = atResolvePointer(ctx, *((void**)eledataptr), &pointerRefID);
                    if (ctx->statusCode_ != ATLA_EOK)
                        return ctx->statusCode_;
                    ATLA_IOWRITE(iostream, &pointerRefID, sizeof(pointerRefID));
                }
                else if (eleDesc->nested_)
                {
                    ctx->statusCode_ = atSerialiseNestedSchema(ctx, dataDesc, eledataptr, iostream);
                    if (ctx->statusCode_ != ATLA_EOK)
                        return ctx->statusCode_;
                }
                else
                {
                    ATLA_IOWRITE(iostream, eledataptr, eleDesc->size_*eleDesc->arrayCount_);
                }
            }

            rootdataptr += tdbp->typeSizeBytes_;
        }
    }

    return ATLA_NOTSUPPORTED;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atSerialiseSchema(atIOStream_t* io, atDataSchema_t* typedesc)
{
    atuint i;
    atuint elementOffset = 0;
    atSerialisedSchema_t stypedef = {0};
    atSerialisedSchemaElement_t typeeledef = {0};
    atSchemaElement_t* elements = typedesc->elementArray_;

    stypedef.typeID_ = typedesc->id_;
    stypedef.elementCount_ = typedesc->elementCount_;
    ATLA_IOWRITE(io, &stypedef, sizeof(stypedef));
    for (i = 0; i < stypedef.elementCount_; ++i)
    {
        //ATLA_IOWRITE
        typeeledef.elementID_ = elements[i].id_;
        typeeledef.size_ = elements[i].size_;
        typeeledef.offset_ = elementOffset;
        typeeledef.arraycount_ = elements[i].arrayCount_;
        if (elements[i].nested_)
            typeeledef.typeID_  = elements[i].nested_->id_;
        else
            memset(&typeeledef.typeID_, 0, sizeof(atUUID_t));
        typeeledef.flags_ = elements[i].atomicType_ ? ATLA_ATOMIC_TYPE : 0;
        typeeledef.flags_ |= elements[i].arrayCount_ > 1 ? ATLA_ARRAY_TYPE : 0;
        typeeledef.flags_ |= elements[i].nested_ ? ATLA_USER_TYPE : 0;
        typeeledef.flags_ |= elements[i].pointer_ ? ATLA_DATA_REFERENCE : 0;
    }

    return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atAddTypeDataToBlob(atAtlaDataBlob_t* blob, const atchar* objectname, const atchar* schemaname, atuint32 count, void* data)
{
/*
    atDataBlob_t* tdbp = NULL, *tmptdbp = NULL, *newtdbp = NULL;
    atSchemaElement_t* eleDesc = NULL;
    atuint coutnIdx, eleIdx;
    atuint stringscratchsize = 0;

    atAddTypeDescToBlob(ctx, typedesc);

    for (tdbp = ctx->dataBlobsHead_; tdbp; tdbp = tdbp->next_)
    {
        // Already added this pointer, don't need to worry
        if (data == tdbp->pointer_) return ATLA_EOK;
        // Adding the same memory pointer in this way is not supported. 
        // In the future, Atla will resolve cases where
        // a pointer points to the start of another object in the AtlaDataBlob
        if (data > tdbp->pointer_ && data < (void*)(((atuint8*)tdbp->pointer_)+(tdbp->typeSizeBytes_*tdbp->elementCount_))) return ATLA_DUPLICATE_DATA;
    }

    newtdbp = ctx->memHandler_.tempAlloc_(sizeof(atDataBlob_t), ctx->memHandler_.memUser_);
    if (!newtdbp) return ATLA_NOMEM;

    newtdbp->objectID_ = atBuildAtlaStringUUID(objectname, strlen(objectname));
    newtdbp->schema_ = typedesc;
    newtdbp->pointer_ = data;
    newtdbp->typeSizeBytes_ = typedesc->typeSize_;
    newtdbp->elementCount_ = count;

    newtdbp->prev_ = NULL;
    newtdbp->next_ = ctx->dataBlobsHead_;
    ctx->dataBlobsHead_->prev_ = newtdbp;
    ctx->dataBlobsHead_ = newtdbp;
*/
    //TODO: Follow any pointers and add them too
/*    for (coutnIdx = 0; coutnIdx < count; ++coutnIdx)
    {
        for (eleIdx = 0; eleIdx < typedesc->elementCount_; ++eleIdx)
        {
            eleDesc = typedesc->elementArray_+eleIdx;
            if (eleDesc->pointer_)
            {
                atAddTypeDataToBlob(ctx, eleDesc->)
            }
            else if ()
        }
    }
*/
    return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void ATLA_API atInitDataBlob(atAtlaDataBlob_t* datablob, atMemoryHandler_t* mem)
{
    datablob->header_.fourCC_ = ATLA_FOURCC;
    datablob->header_.headerSize_ = sizeof(atAtlaFileHeader_t);
    datablob->header_.version_ = atGetAtlaVersion();
    memset(&datablob->iostream_, 0, sizeof(datablob->iostream_));
    datablob->memHandler_ = *mem;
    datablob->statusCode_ = ATLA_EOK;
    datablob->dataBlobsHead_ = NULL;
    datablob->usedSchemaHead_ = NULL;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atAddSchemaToBlob(atAtlaDataBlob_t* ctx, atDataSchema_t* typedesc)
{
    atUsedSchema_t* utdp = NULL, *tmputdp = NULL, *newutdp;

    for (utdp = ctx->usedSchemaHead_; utdp; utdp = utdp->next_)
    {
        if (strncmp(utdp->schema_->id_.uuid_, typedesc->id_.uuid_, sizeof(atUUID_t)) == 0) return ATLA_EOK;
    }

    newutdp = ctx->memHandler_.tempAlloc_(sizeof(atUsedSchema_t), ctx->memHandler_.memUser_);
    if (!newutdp) return ATLA_NOMEM;

    tmputdp = ctx->usedSchemaHead_;
    tmputdp->prev_ = newutdp;
    newutdp->prev_ = NULL;
    newutdp->next_ = tmputdp;
    ctx->usedSchemaHead_ = newutdp;

    return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode atSerialiseNestedSchema(atAtlaDataBlob_t* ctx, atDataSchema_t* dataDesc, atuint8* rootdataptr, atIOStream_t* iostream) 
{
    atuint typeeleIdx;
    atSchemaElement_t* eleDesc;
    atuint8* eledataptr;
    atuint32 pointerRefID;

    for (typeeleIdx = 0; typeeleIdx < dataDesc->elementCount_; ++typeeleIdx)
    {
        eleDesc = dataDesc->elementArray_ + typeeleIdx;
        eledataptr = rootdataptr+eleDesc->offset_;
        if (eleDesc->pointer_)
        {
            ctx->statusCode_ = atResolvePointer(ctx, *((void**)eledataptr), &pointerRefID);
            if (ctx->statusCode_ != ATLA_EOK)
                return ctx->statusCode_;
            ATLA_IOWRITE(iostream, &pointerRefID, sizeof(pointerRefID));
        }
        else if (eleDesc->nested_)
        {
            ctx->statusCode_ = atSerialiseNestedSchema(ctx, dataDesc, eledataptr, iostream);
            if (ctx->statusCode_ != ATLA_EOK)
                return ctx->statusCode_;
        }
        else
        {
            ATLA_IOWRITE(iostream, eledataptr, eleDesc->size_*eleDesc->arrayCount_);
        }
    }
    return ATLA_EOK;

}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atResolvePointer(atAtlaDataBlob_t* ctx, void* ptr, atuint32* outid)
{
    atDataBlob_t* tdbp = NULL;
    atuint refid = 0;

    if (!ptr)
    {
        *outid = ATLA_INVALIDPOINTERREF;
        return ATLA_EOK;
    }

    for (tdbp = ctx->dataBlobsHead_, refid = 0; tdbp; tdbp = tdbp->next_, ++refid)
    {
        if (ptr == tdbp->pointer_)
        {
            *outid = refid;
        }
        return ATLA_EOK;
    }

    return ATLA_EPOINTERNOTFOUND;
}

#undef ATLA_IOREAD
#undef ATLA_IOSEEK
#undef ATLA_IOWRITE
#undef ATLA_IOTELL

