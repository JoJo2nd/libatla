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

#define ATLA_IOREAD(io, buf, size)      (io)->readProc_(buf, size, (io)->user_)
#define ATLA_IOSEEK(io, offset, from)   (io)->seekProc_(offset, from, (io)->user_)
#define ATLA_IOWRITE(io, buf, size)     (io)->writeProc_(buf, size, (io)->user_)
#define ATLA_IOTELL(io)                 (io)->tellProc_((io)->user_)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atAtlaDataBlob_t* ATLA_API atOpenAtlaDataBlob(atAtlaContext_t* ctx, atIOAccess_t* ioaccess, atuint32 flags)
{
    atMemoryHandler_t* mem = ctx->memCtx_;
    atAtlaDataBlob_t* datablob = NULL;
    atuint8* tmpmem, *tempend;
    atuint32 totalsize;
    atSerialisedSchema_t* schemap;

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
    }
    else if (flags & ATLA_READ)
    {
        datablob->props_ = ATLA_READ;

        ATLA_IOREAD(ioaccess, &datablob->header_, sizeof(datablob->header_));
        totalsize = datablob->header_.dataStartOffset_-datablob->header_.headerSize_;
        tmpmem = mem->memAlloc_(totalsize, mem->memUser_);
        datablob->deserialiseInfo_.headerMemSize_ = totalsize;
        datablob->deserialiseInfo_.headerMem_ = tmpmem;
        ATLA_IOREAD(ioaccess, tmpmem, totalsize);
        datablob->deserialiseInfo_.objects_ = (atSerialisedTOC_t*)(((atuint8*)tmpmem) + (datablob->header_.tocOffset_ - datablob->header_.headerSize_));
        datablob->deserialiseInfo_.objectCount_ = (datablob->header_.dataStartOffset_-datablob->header_.tocOffset_)/datablob->header_.tocSize_;
        datablob->deserialiseInfo_.schema_ = (atSerialisedSchema_t*)(((atuint8*)tmpmem) + (datablob->header_.schemaOffset_ - datablob->header_.headerSize_));
        datablob->deserialiseInfo_.schemaCount_ = 0;
        tempend = (atuint8*)datablob->deserialiseInfo_.objects_;
        while (tmpmem < tempend)
        {
            schemap = (atSerialisedSchema_t*)tmpmem;
            ++datablob->deserialiseInfo_.schemaCount_;
            tmpmem += datablob->header_.schemaSize_;
            tmpmem += schemap->elementCount_*datablob->header_.schemaElementSize_;
            //TODO: validate that the elements are in offset order, make serialisation simpler
            // If not in order, easy enough to sort them and make that the case.
        }
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
        mem->memFree_(utdp, mem->memUser_);
    }
    mem->memFree_(datablob->deserialiseInfo_.headerMem_, mem->memUser_);

    mem->memFree_(datablob, mem->memUser_);
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

    blob->header_.schemaOffset_ = (atuint32)ATLA_IOTELL(ios);

    //Walk the Type Desc structs and write them
    for (utdp = blob->usedSchemaHead_; utdp; utdp = utdp->next_)
    {
         blob->statusCode_ = atSerialiseSchema(ios, utdp->schema_);
         if (blob->statusCode_ != ATLA_EOK) return blob->statusCode_;
    }

    tellof = ATLA_IOTELL(ios);
    blob->header_.tocOffset_ = (atuint32)tellof;

    //account for TOC storage
    for (tdbp = blob->dataBlobsHead_; tdbp; tdbp = tdbp->next_)
    {
        tellof += sizeof(atSerialisedTOC_t);
    }

    //Walk the Data pointers and serialise the data IDs make space for offsets
    for (tdbp = blob->dataBlobsHead_, currentObjectID = 0; tdbp; tdbp = tdbp->next_, ++currentObjectID)
    {
        toc.objectID_ = tdbp->objectID_;
        toc.fileoffset_ = tellof;
        toc.count_ = tdbp->elementCount_;
        if (tdbp->schema_)
        {
            toc.typeID_ = tdbp->schema_->id_;
            toc.size_ = tdbp->schema_->serialisedSize_;
            tellof += tdbp->schema_->serialisedSize_*tdbp->elementCount_;
        }
        else
        {
            toc.typeID_ = 0;
            toc.size_ = tdbp->elementSize_;
            tellof += tdbp->elementSize_*tdbp->elementCount_;
        }
        ATLA_IOWRITE(ios, &toc, sizeof(atSerialisedTOC_t));
    }

    blob->header_.dataStartOffset_ = (atuint32)ATLA_IOTELL(ios);

    //Walk the Data pointers and serialise the data
    for (tdbp = blob->dataBlobsHead_, currentObjectID = 0; tdbp; tdbp = tdbp->next_, ++currentObjectID)
    {
        rootdataptr = tdbp->pointer_;       
        
        if (tdbp->schema_)
        {
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

        elementOffset += elements[i].size_;
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
    datablob->header_.objectHdrSize_ = 0;
    datablob->header_.schemaOffset_ = 0;
    datablob->header_.tocOffset_ = 0;
    datablob->header_.dataStartOffset_ = 0;

    memset(&datablob->iostream_, 0, sizeof(datablob->iostream_));
    datablob->memHandler_ = *mem;
    datablob->statusCode_ = ATLA_EOK;
    datablob->props_ = 0;
    datablob->dataBlobsHead_ = NULL;
    datablob->usedSchemaHead_ = NULL;

    memset(&datablob->deserialiseInfo_, 0, sizeof(datablob->deserialiseInfo_));
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

atErrorCode ATLA_API atGetSerialisedSchema(atAtlaDataBlob_t* ctx, atUUID_t schemaID, atSerialisedSchema_t** outschema, atSerialisedSchemaElement_t** outeles)
{
    atSerialisedSchema_t* schema;
    atuint i, imax;

    atAssert(ctx && outschema && outeles);

    imax = ctx->deserialiseInfo_.schemaCount_;
    schema = ctx->deserialiseInfo_.schema_; 

    for (i = 0; i < imax; ++i)
    {
        if (schema->typeID_ == schemaID)
        {
            *outschema = schema;
            *outeles = (atSerialisedSchemaElement_t*)(schema+1);
            return ATLA_EOK;
        }
        schema = (atSerialisedSchema_t*)(((atuint8*)schema)+sizeof(atSerialisedSchema_t)+(schema->elementCount_*sizeof(atSerialisedSchemaElement_t)));
    }

    return ATLA_EBADSCHEMAID;
}

void* atGetOutputOffset(atDataSchema_t* schema, atUUID_t id, void* ptr, atUUID_t* typeID)
{
    atuint i;

    for (i = 0; i < id; ++i)
    {
        if (schema->elementArray_[i].id_ == id)
        {
            *typeID = schema->elementArray_[i].nestedID_;
            return (atuint8*)ptr + schema->elementArray_[i].offset_;
        }
    }

    return NULL;
}

atErrorCode ATLA_API atResolveAndDeserialseData(atAtlaDataBlob_t* blob, atUUID_t typeID, atuint count, void* outptr)
{
    /*
     * The assumption is made here that io stream is in the correct place.
     */
    atSerialisedSchema_t* dataSchema;
    atSerialisedSchemaElement_t* dataSchemaElements;
    atDataSchema_t* schema;
    atIOAccess_t* ios = &blob->iostream_;
    atuint dataEleCount, eleCount;
    atuint object, dataEle;
    atuint datasize;
    atuint8* rootptr;
    void* eleptr; 
    atUUID_t schemaTypeID;

    schema = atContextGetDataSchema(blob->ctx_, typeID);
    if (!schema) return ATLA_EBADSCHEMAID;

    blob->statusCode_ = atGetSerialisedSchema(blob, typeID, &dataSchema, &dataSchemaElements);
    if (blob->statusCode_ != ATLA_EOK) return blob->statusCode_;

    dataEleCount = dataSchema->elementCount_;
    eleCount = schema->elementCount_;

    rootptr = (atuint8*)outptr;
    for (object = 0; object < count; ++object)
    {
        for (dataEle = 0; dataEle < dataEleCount; ++dataEle)
        {
            datasize = dataSchemaElements[dataEle].size_*dataSchemaElements[dataEle].arraycount_;
            // find correct offset into outptr
            eleptr = atGetOutputOffset(schema, dataSchemaElements[dataEle].elementID_, rootptr, &schemaTypeID);

            // if found offset to write to...
            if (eleptr)
            {
                // ...read in the data if the element desc still exists in the 
                // type schema
                if (dataSchemaElements[dataEle].typeID_ == 0)
                {
                    ATLA_IOREAD(ios, eleptr, datasize);
                }
                else if (dataSchemaElements[dataEle].typeID_ == schemaTypeID)
                {
                    blob->statusCode_ = atResolveAndDeserialseData(blob, schemaTypeID, dataSchemaElements[dataEle].arraycount_, eleptr);
                    if (blob->statusCode_ != ATLA_EOK) return ATLA_EOK;
                }
                else
                {
                    /* Can't resolve this case*/
                    return ATLA_ETYPEMISMATCH;
                }
            }
            else
            {
                // Seek past element
                ATLA_IOSEEK(ios, datasize, eSeekOffset_Current);
            }
        }

        rootptr += schema->typeSize_;
    }

    return ATLA_EOK;
}

atuint ATLA_API atGetDataCount(atAtlaDataBlob_t* blob)
{
    return blob->deserialiseInfo_.objectCount_;
}

atErrorCode ATLA_API atGetDataDescByIndex(atAtlaDataBlob_t* blob, atuint idx, atDataDesc_t* outdesc)
{
    if (idx >= blob->deserialiseInfo_.objectCount_) return ATLA_EOUTOFRANGE;

    outdesc->count_ = blob->deserialiseInfo_.objects_[idx].count_;
    outdesc->dataID_ = blob->deserialiseInfo_.objects_[idx].objectID_;
    outdesc->typeID_ = blob->deserialiseInfo_.objects_[idx].typeID_;

    return ATLA_EOK;
}

atErrorCode ATLA_API atGetDataDescByName(atAtlaDataBlob_t* blob, const atchar* objectname, atDataDesc_t* outdesc)
{
    atuint i, iend;
    atUUID_t id;

    id = atBuildAtlaStringUUID(objectname, strlen(objectname));

    for (i = 0, iend = blob->deserialiseInfo_.objectCount_; i < iend; ++i)
    {
        if (blob->deserialiseInfo_.objects_[i].objectID_ == id)
        {
            return atGetDataDescByIndex(blob, i, outdesc);
        }
    }

    return ATLA_ENOTFOUND;
}

atErrorCode ATLA_API atDeserialiseDataByIndex(atAtlaDataBlob_t* blob, atuint idx, void* output)
{
    atUUID_t typeID;
    atuint count; 

    if (idx >= blob->deserialiseInfo_.objectCount_) return ATLA_EOUTOFRANGE;

    count = blob->deserialiseInfo_.objects_[idx].count_;
    typeID = blob->deserialiseInfo_.objects_[idx].typeID_;

    ATLA_IOSEEK(&blob->iostream_, blob->deserialiseInfo_.objects_[idx].fileoffset_, eSeekOffset_Begin);
    if (typeID != 0)
    {
        blob->statusCode_ = atResolveAndDeserialseData(blob, typeID, count, output);
        if (blob->statusCode_ != ATLA_EOK) return blob->statusCode_;
    }
    else
    {
        //Typeless data, assumes no padding
        ATLA_IOREAD(&blob->iostream_, output, count*blob->deserialiseInfo_.objects_[idx].size_);
    }

    return ATLA_EOK;
}

atErrorCode ATLA_API atDeserialiseDataByName(atAtlaDataBlob_t* blob, const atchar* objectname, void* output)
{
    atuint i, iend;
    atUUID_t id;

    id = atBuildAtlaStringUUID(objectname, strlen(objectname));

    for (i = 0, iend = blob->deserialiseInfo_.objectCount_; i < iend; ++i)
    {
        if (blob->deserialiseInfo_.objects_[i].objectID_ == id)
        {
            return atDeserialiseDataByIndex(blob, i, output);
        }
    }

    return ATLA_ENOTFOUND;
}

#undef ATLA_IOREAD
#undef ATLA_IOSEEK
#undef ATLA_IOWRITE
#undef ATLA_IOTELL

