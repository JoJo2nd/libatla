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

#ifdef __cplusplus
extern "C" {
#endif//

/*
 * Main Include for alta lib
 **/

#include "atla_config.h"
#include "atla_memhandler.h"
#include "atla_context.h"
#include "atla/atla_ioaccess.h"

typedef uint64_t at_loc_t;

typedef struct atAtlaSerializer atAtlaSerializer_t;

typedef void (atSerializeTypeProc_t)(atAtlaSerializer_t*, void*);

struct atAtlaTypeData {
	union {
		at_loc_t offset;
		char const* ptr;
	} name; // if NULL then this is a native type (e.g. int, float, char, etc)
	union {
		at_loc_t offset;
		atSerializeTypeProc_t* ptr;
	} proc;// NULL, if name is NULL
	void* data;
	uint32_t size;
	uint32_t count;
	uint32_t id;
	at_loc_t processed;
};
typedef struct atAtlaTypeData atAtlaTypeData_t; 

struct atAtlaSerializer {
  atMemoryHandler_t*  mem;
	atioaccess_t *io;
	uint32_t reading;
	uint32_t version;
	uint32_t nextID;
	uint32_t depth;
	uint32_t objectListLen, objectListRes;
	atAtlaTypeData_t *objectList;
};

void atSerializeWriteBegin(atAtlaSerializer_t* serializer, atMemoryHandler_t* mem, atioaccess_t* context, uint32_t version);
void atSerializeWriteRoot(atAtlaSerializer_t* serializer, void* data, atSerializeTypeProc_t* proc);
void atSerializeWriteProcessPending(atAtlaSerializer_t* serializer);
void atSerializeWriteFinalize(atAtlaSerializer_t* serializer);

void atSerializeWrite(atAtlaSerializer_t* serializer, void* src, uint32_t element_size, uint32_t element_count);
uint32_t atSerializeWritePendingBlob(atAtlaSerializer_t* serializer, void* data, uint32_t element_size, uint32_t count);
uint32_t atSerializeWritePendingType(atAtlaSerializer_t* serializer, void* data, char const *name, atSerializeTypeProc_t* proc, uint32_t count);

//uint32_t atSerializeObjRef(atAtlaSerializer_t* serializer, void* src, char const* type_name);

void atSerializeRead(atAtlaSerializer_t* serializer, void* dest, uint32_t element_size, uint32_t element_count);
void atSerializeSkip(atAtlaSerializer_t* serializer, uint32_t element_size, uint32_t element_count);
void* atSerializeReadGetBlobLocation(atAtlaSerializer_t* serializer, uint32_t blob_id);
void* atSerializeReadTypeLocation(atAtlaSerializer_t* serializer, uint32_t type_id, char const *name, atSerializeTypeProc_t* proc);

//void* atSerializeReadObjRef(atAtlaSerializer_t* serializer);



uint64_t ATLA_API atGetAtlaVersion();

atAtlaContext_t* ATLA_API atCreateAtlaContext(uint32_t data_version, atMemoryHandler_t* mem_handler);
atErrorCode ATLA_API atDestroyAtlaContext(atAtlaContext_t*);

/*

	Atla - 
	1) Should be a self describing format. i.e. Format can be read by other tools & data types within a serialised blob are 'understandable'. Could be optional?
	2) Minimal code should be written by user. May result in a funky meta lang but avoids errors. Look at the LBP method and repeat includes to define
		 define struct & serialise
	3) Should provide a runtime reflection interface. Struct data described by arrays of elements(?) Must be optional!
	4) Must be backward compat
	5) Prasing should be simple and not use rewind seeking

	Current thoughts:
	* Pointers are stored as object IDs in a binary blob. Hash map on write to avoid rewriting the same data
	* Footer describes all formats in blob (type ids & descriptors are saved on write?)
	* Footer note object counts for pre-alloction.
	* Borrow from LBP method. Single global version number, linear add to data, no remove (only macro to ommit data from future writes)

*/

#ifdef __cplusplus
} //extern "C"
#endif//

