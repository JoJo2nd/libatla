/********************************************************************

	filename: 	atla.c	
	
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

#include "atla/atla.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ATLA_VERSION_MAJOR    (0)
#define ATLA_VERSION_MINOR    (1)
#define ATLA_VERSION_REV      (1)

static int addr_comp(void const *a, void const* b) {
	atAtlaTypeData_t const *c = a, *d = b;
	return (uintptr_t)c->data - (uintptr_t)d->data;
}

uint64_t ATLA_API atGetAtlaVersion()
{
    return (((atuint64)(ATLA_VERSION_MAJOR & 0xFFFF) << 48) | ((atuint64)(ATLA_VERSION_MAJOR & 0xFFFF) << 32) | ATLA_VERSION_REV);
}

void atSerializeWriteBegin(atAtlaSerializer_t* serializer, atMemoryHandler_t* mem, atioaccess_t* io, uint32_t version) {
	serializer->mem = mem;
	serializer->io = io;
	serializer->reading = 0;
	serializer->version = version;
	serializer->nextID = 1;
	serializer->depth = 0;
	serializer->objectListLen = 0;
	serializer->objectListRes = 16;
	serializer->objectList = mem->alloc(sizeof(atAtlaTypeData_t)*serializer->objectListRes, mem->user);
}

void atSerializeWriteRoot(atAtlaSerializer_t* serializer, void* data, atSerializeTypeProc_t* proc) {
	(*proc)(serializer, data);
}

void atSerializeWriteProcessPending(atAtlaSerializer_t* serializer) {
	//TOOD: walk the list of objects awaiting writes, find first that hasn't be done and do it
	// continue until nothing is left to do...
}

void atSerializeWriteFinalize(atAtlaSerializer_t* serializer) {
	//TOOD: sort object list by object ID (for easy look up next time)
	//TODO: write the footer
}

void atSerializeWrite(atAtlaSerializer_t* serializer, void* src, uint32_t element_size, uint32_t element_count) {
	atioaccess_t *io = serializer->io;
	io->writeProc(src, element_size, io->user);
}

uint32_t atSerializeWritePendingBlob(atAtlaSerializer_t* serializer, void* data, uint32_t element_size, uint32_t count) {
	// Big TODO: handle pointer offsets with already serialized memory blobs (track with id plus offset??)

	if (!data) return 0;
	atMemoryHandler_t *mem = serializer->mem;
	atAtlaTypeData_t new_blob = {	.name=NULL, .proc=NULL, .size=element_size, .count=count, .data=data, .id=serializer->nextID, .processed=0 };
	atAtlaTypeData_t* found = bsearch(&new_blob, serializer->objectList, serializer->objectListLen, sizeof(atMemoryHandler_t), addr_comp);
	if (found) return found->id;

	if (serializer->objectListLen+1 >= serializer->objectListRes) {
		serializer->objectListRes *= 2;
		serializer->objectList = mem->ralloc(serializer->objectList, sizeof(atAtlaTypeData_t)*serializer->objectListRes, mem->user);
	}
	serializer->objectList[serializer->objectListLen++] = new_blob;
	++serializer->nextID;
	qsort(serializer->objectList, serializer->objectListLen, sizeof(atAtlaTypeData_t), addr_comp);
	return new_blob.id;
}

uint32_t atSerializeWritePendingType(atAtlaSerializer_t* serializer, void* data, char const *name, atSerializeTypeProc_t* proc, uint32_t count) {
	// Big TODO: handle pointer offsets with already serialized memory blobs (track with id plus offset??)

	if (!data) return 0;
	atMemoryHandler_t *mem = serializer->mem;
	atAtlaTypeData_t new_blob = {	.name=name, .proc=proc, .size=0, .count=count, .data=data, .id=serializer->nextID, .processed=0 };
	atAtlaTypeData_t* found = bsearch(&new_blob, serializer->objectList, serializer->objectListLen, sizeof(atMemoryHandler_t), addr_comp);
	if (found) return found->id;

	if (serializer->objectListLen+1 >= serializer->objectListRes) {
		serializer->objectListRes *= 2;
		serializer->objectList = mem->ralloc(serializer->objectList, sizeof(atAtlaTypeData_t)*serializer->objectListRes, mem->user);
	}
	serializer->objectList[serializer->objectListLen++] = new_blob;
	++serializer->nextID;
	qsort(serializer->objectList, serializer->objectListLen, sizeof(atAtlaTypeData_t), addr_comp);
	return new_blob.id;	
}

void atSerializeRead(atAtlaSerializer_t* serializer, void* dest, uint32_t element_size, uint32_t element_count) {
	atioaccess_t *io = serializer->io;
	io->readProc(dest, element_size, io->user);	
}

void atSerializeSkip(atAtlaSerializer_t* serializer, uint32_t element_size, uint32_t element_count) {
	atioaccess_t *io = serializer->io;
	io->seekProc(element_size, eSeekOffset_Current, io->user);	
}

void* atSerializeReadGetBlobLocation(atAtlaSerializer_t* serializer, uint32_t blob_id) {
	if (blob_id == 0) return NULL;
	return serializer->objectList[blob_id].data;
}


void* atSerializeReadTypeLocation(atAtlaSerializer_t* serializer, uint32_t type_id, char const *name, atSerializeTypeProc_t* proc) {
	//TODO
	return NULL;
}
