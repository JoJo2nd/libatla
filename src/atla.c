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

        2. Altered source versions must be plainly marked as such, and must not
be
        misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.

*********************************************************************/

#include "atla/atla.h"
#include "smd.h"
#include <string.h>
#include <assert.h>

#define atla_FileBlockHeader_disksize (sizeof(atuint32) * 2 + sizeof(atuint64))

#define atla_FileHeader_version (1)
#define atla_FileHeader_disksize (atla_FileBlockHeader_disksize)
#define atla_FileHeader_fourCC (smd_make_fourCC('A', 'T', 'L', 'A'))

struct atla_FileObjectEntry {
  atuint32 typeID;
  atuint32 objectID;
  atuint32 count;
};
#define atla_FileObjectEntry_disksize (sizeof(atuint32) * 3)

struct atla_FileObject {
  atla_FileBlockHeader_t blkHeader;
  atuint32               typeID;
  atuint32               objectID;
  atuint32               version;
  atuint32               count;
};
#define atla_FileObject_version (1)
#define atla_FileObject_disksize (atla_FileBlockHeader_disksize + sizeof(atuint32) * 4)
#define atla_FileObject_fourCC (smd_make_fourCC('O', 'b', 'j', 'D'))

struct atla_FileObjectList {
  atla_FileBlockHeader_t blkHeader;
  atuint32               count;
};
#define atla_FileObjectList_version (1)
#define atla_FileObjectList_disksize (atla_FileBlockHeader_disksize + sizeof(atuint32))
#define atla_FileObjectList_fourCC (smd_make_fourCC('O', 'b', 'j', 'L'))

struct atla_FileObjectRef {
  atuint32 objectID;
  atuint32 compositeElementID;
};

struct atla_ObjectRef {
  atuint32                   objectID;
  atuint32                   typeID;
  atuint32                   version;
  atsize_t                   count;
  atsize_t                   runtimeSize;
  atsize_t                   diskSize;
  atbyte*                    dataPtr;
  atsize_t                   dataLen;
  atuint32                   flags;
  struct atla_ObjectRef*     next;
  struct alta_Context const* ownerCtx;
};

typedef struct atla_ObjectRefPool    atla_ObjectRefPool_t;
typedef struct atla_PointerIndexPair atla_PointerIndexPair_t;
struct atla_PointerIndexPair {
  uintptr_t ptrBegin, ptrEnd;
  atuint32  index;
};

struct atla_ObjectRefPool {
  uintptr_t                low;
  uintptr_t                high;
  atla_ObjectRef_t*        objRefs;
  atla_PointerIndexPair_t* ptrs;
  atuint32                 nextIndex, indexLimit;
  atla_ObjectRefPool_t*    next;
};

atsize_t BaseTypeSizeLookup[] = {
  0,                // atla_type_none = 0,
  sizeof(atint8),   // atla_type_int8,
  sizeof(atchar),   // atla_type_char,
  sizeof(atint16),  // atla_type_int16,
  sizeof(atint32),  // atla_type_int32,
  sizeof(atint64),  // atla_type_int64,
  sizeof(atuint8),  // atla_type_uint8,
  sizeof(atuint16), // atla_type_uint16,
  sizeof(atuint32), // atla_type_uint32,
  sizeof(atuint64), // atla_type_uint64,
  sizeof(float),    // atla_type_float,
  sizeof(double),   // atla_type_double,
  0,                // atla_type_composite,
  0,                // atla_type_custom_type_id_first,
};

atsize_t atla_GetObjectRefPoolRequiredSize(atuint32 max_objects) {
  return (sizeof(atla_ObjectRefPool_t) + (max_objects * ((sizeof(atla_ObjectRef_t) + sizeof(atla_PointerIndexPair_t)))) + 15) & ~15;
}

int atla_AssignObjectRefPool(atla_Context_t* ctx, void* ptr, atsize_t size) {
  atla_ObjectRefPool_t* new_pool = ptr;
  new_pool->low = ~((uintptr_t)0);
  new_pool->high = 0;
  new_pool->next = NULL;
  if (ctx->sentinelPool != NULL) ctx->sentinelPool->next = new_pool;
  ctx->sentinelPool = new_pool;
  if (ctx->objectPool == NULL) {
    ctx->objectPool = ctx->activePool = new_pool;
  }
  size -= sizeof(atla_ObjectRefPool_t);
  new_pool->indexLimit = size / (sizeof(atla_ObjectRef_t) + sizeof(uintptr_t));
  new_pool->objRefs = (atla_ObjectRef_t*)(new_pool + 1);
  new_pool->ptrs = (atla_PointerIndexPair_t*)(new_pool->objRefs + new_pool->indexLimit);
  new_pool->nextIndex = 0;
  return ATLA_EOK;
}

static atla_ObjectRef_t* allocObjectRef(atla_Context_t* ctx, void const* ptr, atsize_t len) {
  if (ctx->activePool->nextIndex == ctx->activePool->indexLimit) {
    // try next pool
    if (ctx->activePool->next == NULL) return NULL;
    ctx->activePool = ctx->activePool->next;
  }
  atla_ObjectRef_t* ret = ctx->activePool->objRefs + ctx->activePool->nextIndex;
  // insertion sort
  if (ctx->activePool->nextIndex == 0) {
    ctx->activePool->ptrs[0].ptrBegin = (uintptr_t)ptr;
    ctx->activePool->ptrs[0].ptrEnd = (uintptr_t)ptr + len;
    ctx->activePool->ptrs[0].index = 0;
  } else {
    atsize_t at = ctx->activePool->nextIndex;
    for (atsize_t i = 0, n = ctx->activePool->nextIndex; i < n; ++i) {
      if (ctx->activePool->ptrs[i].ptrBegin > (uintptr_t)ptr) {
        memmove(&ctx->activePool->ptrs[i + 1], &ctx->activePool->ptrs[i], sizeof(atla_PointerIndexPair_t) * (n - i));
        at = i;
        break;
      }
    }
    ctx->activePool->ptrs[at].ptrBegin = (uintptr_t)ptr;
    ctx->activePool->ptrs[at].ptrEnd = (uintptr_t)ptr + len;
    ctx->activePool->ptrs[at].index = ctx->activePool->nextIndex;
  }
  ctx->activePool->nextIndex++;
  if ((uintptr_t)ptr < ctx->activePool->low) ctx->activePool->low = (uintptr_t)ptr;
  if ((uintptr_t)ptr + len > ctx->activePool->high) ctx->activePool->high = (uintptr_t)ptr + len;
  return ret;
}

static void alta_FlushIOBuffer(atla_Context_t* ctx) {
  // TODO:
}

static int atla_SendToIOBuffer(atla_Context_t* ctx, void const* v, atsize_t c) {
  // TODO: check, object size is respected
  // TODO: store in io buffer, and flush if needed.
  ctx->io.write(v, c, ctx->io.user);
  return ATLA_EOK;
}

static int atla_read_from_io_buffer(atla_Context_t* ctx, void* v, atsize_t c) {
  ctx->io.read(v, c, ctx->io.user);
  return ATLA_EOK;
}

int atla_InitializeAtlaContext(atla_Context_t* ctx, atla_GetTypeInfo fn1, atla_GetTypePointerOffset fn2) {
  if (ctx->status & 1) return ATLA_EC_ALREADY;
  memset(ctx, 0, sizeof(*ctx));
  ctx->status = 1;
  ctx->typeInfoFn = fn1;
  ctx->typePointerOffsetFn = fn2;
  return ATLA_EOK;
}

int atla_AssignIOAccess(atla_Context_t* ctx, atla_ioaccess_t* io) {
  ctx->io = *io;
  return ATLA_EOK;
}

int atla_AssignIOBuffer(atla_Context_t* ctx, atbyte* io_buffer, atsize_t io_buffer_size) {
  ctx->ioBuffer = io_buffer;
  ctx->ioBufferLimit = io_buffer_size;
  ctx->ioBufferUsed = 0;
  return ATLA_EOK;
}

int atla_fwrite_begin(atla_Context_t* ctx) {
  if (!ctx->io.write) return ATLA_IO_UNSUPPORTED;
  ctx->header.blkHeader.fourCCBytes[0] = 'A';
  ctx->header.blkHeader.fourCCBytes[1] = 'T';
  ctx->header.blkHeader.fourCCBytes[2] = 'L';
  ctx->header.blkHeader.fourCCBytes[3] = 'A';
  smd_assert(ctx->header.blkHeader.fourCC == smd_make_fourCC('A', 'T', 'L', 'A'));
  ctx->header.blkHeader.size = 16;
  ctx->header.blkHeader.version = 1;
  ctx->nextObjectID = 1;
  ctx->objectCount = 0;
  return ATLA_EOK;
}

int atla_fwrite_begin_object_list(atla_Context_t* ctx) {
  ctx->objectCount = 0;
  ctx->write.objectList = NULL;
  ctx->write.objectListEnd = NULL;
  return ATLA_EOK;
}

int atla_fwrite_add_object_to_list(atla_Context_t* ctx, atuint32 type_id, atsize_t count, void* ptr, atsize_t len, atla_ObjectRef_t** obj_out) {
  atla_ObjectRef_t* obj = allocObjectRef(ctx, ptr, len);
  *obj_out = obj;
  if (obj == NULL) return ATLA_EC_OBJECT_REF_POOL_EXHAUSTED;
  obj->objectID = ctx->nextObjectID++;
  obj->typeID = type_id;
  if (type_id < atla_type_composite) {
    obj->runtimeSize = obj->diskSize = BaseTypeSizeLookup[type_id];
    obj->version = 1;
  } else {
    ctx->typeInfoFn(type_id, &obj->runtimeSize, &obj->version, &obj->diskSize);
  }
  obj->count = count;
  obj->ownerCtx = ctx;
  obj->dataPtr = ptr;
  obj->dataLen = len;
  obj->flags = 0;
  obj->next = NULL;
  ctx->objectCount++;
  if (ctx->write.objectListEnd) ctx->write.objectListEnd->next = obj;
  ctx->write.objectListEnd = obj;
  if (!ctx->write.objectList) ctx->write.objectList = obj;
  // TODO: error check for overlapping pointers?
  return ATLA_EOK;
}

int atla_fwrite_end_object_list(atla_Context_t* ctx) {
  // write the header
  atla_fwrite_u32(ctx, ctx->header.blkHeader.fourCC);
  atla_fwrite_u32(ctx, ctx->header.blkHeader.version);
  atla_fwrite_u32(ctx, ctx->header.blkHeader.size);
  // calculate the file object list block size
  atla_fwrite_u32(ctx, atla_FileObjectList_fourCC);
  atla_fwrite_u32(ctx, atla_FileObjectList_version);
  atla_fwrite_u32(ctx, atla_FileObjectList_disksize + (atla_FileObjectEntry_disksize * ctx->objectCount));
  // write the file object list
  atla_ObjectRefPool_t* pool = ctx->objectPool;
  atsize_t              current_pool_index = 0;
  for (atsize_t i = 0, n = ctx->objectCount; i < n; ++i) {
    smd_assert(pool);
    atla_fwrite_u32(ctx, pool->objRefs[current_pool_index].typeID);
    atla_fwrite_u32(ctx, pool->objRefs[current_pool_index].objectID);
    atla_fwrite_u64(ctx, pool->objRefs[current_pool_index].count);
    if (++current_pool_index >= pool->nextIndex) {
      current_pool_index = 0;
      pool = pool->next;
    }
  }

  ctx->currentObj = ctx->write.objectList;
  return ATLA_EOK;
}

static int objref_by_pointer_comp(void const* lhs_, void const* rhs_) {
  atla_PointerIndexPair_t const* s_ptr = lhs_; // search pointer
  atla_PointerIndexPair_t const* r_ptr = rhs_; // range pointer
  // not 100% sure this is required.
  // if (s_ptr->ptrBegin != s_ptr->ptrEnd) {
  //  s_ptr = rhs_;
  //  r_ptr = lhs_;
  //}
  if (s_ptr->ptrBegin < r_ptr->ptrBegin)
    return -1;
  else if (s_ptr->ptrBegin > r_ptr->ptrEnd)
    return 1;
  else
    return 0;
}

atla_ObjectRef_t const* atla_fwrite_find_object_ref_by_ptr(atla_Context_t* ctx, void* ptr) {
  atla_ObjectRefPool_t* search_pool = NULL;
  for (atla_ObjectRefPool_t* i = ctx->objectPool; i; i = i->next) {
    if (ptr >= i->low && ptr <= i->high) {
      search_pool = i;
      break;
    }
  }
  if (search_pool == NULL) return NULL;
  // TODO: make faster?
  for (atuint32 i = 0, n = search_pool->nextIndex; i < n; ++i) {
    if (ptr >= search_pool->ptrs[i].ptrBegin && ptr < search_pool->ptrs[i].ptrEnd) return &search_pool->objRefs[i];
  }
  return NULL;
}

int atla_fwrite_next_object(atla_Context_t* ctx, atla_ObjectLocation_t* oloc) {
  atla_ObjectRef_t* ret;
  do {
    ret = ctx->currentObj;
    if (!ret) return ATLA_EC_END_OF_LIST;
    // mark the objects as started (flag) ?
    ctx->currentObj = ret->next;
    // store total size
    ctx->bytesToWrite = ret->diskSize;
    // reset byte count
    ctx->bytesWritten = 0;
    // Write header
    atla_fwrite_u32(ctx, atla_FileObject_fourCC);
    atla_fwrite_u32(ctx, atla_FileObject_version);
    atla_fwrite_u32(ctx, atla_FileObject_disksize + ret->diskSize);
    atla_fwrite_u32(ctx, ret->typeID);
    atla_fwrite_u32(ctx, ret->objectID);
    atla_fwrite_u32(ctx, ret->version);
    atla_fwrite_u64(ctx, ret->count);

    if (ret->typeID < atla_type_composite) {
      // raw type, we can handle this
      atla_fwrite_u8p(ctx, ret->dataPtr, ret->count * BaseTypeSizeLookup[ret->typeID]);
      atla_fwrite_end_object(ctx);
    } else {
      oloc->count = ret->count;
      oloc->objectID = ret->objectID;
      oloc->typeID = ret->typeID;
      oloc->version = ret->version;
      oloc->dataPtr = ret->dataPtr;
    }
  } while (ret->typeID < atla_type_composite);
  return ATLA_EOK;
}

/*int atla_fwrite_begin_object(atla_Context_t* ctx, atla_ObjectRef_t const* obj, atsize_t expected_byte_count) {
  // validate obj belongs to ctx
  if (obj->ownerCtx != ctx) return ATLA_EC_UNKNOWN_OBJ;
  // mark the objects as started (flag)
  ctx->currentObj = obj;
  // store total size
  ctx->bytesToWrite = expected_byte_count;
  // reset byte count
  ctx->bytesWritten = 0;
  // Write header
  atla_fwrite_u32(ctx, atla_FileObject_fourCC);
  atla_fwrite_u32(ctx, atla_FileObject_version);
  atla_fwrite_u32(ctx, atla_FileObject_disksize + expected_byte_count);
  atla_fwrite_u32(ctx, obj->typeID);
  atla_fwrite_u32(ctx, obj->objectID);
  atla_fwrite_u32(ctx, obj->version);
  atla_fwrite_u64(ctx, obj->count);
  return ATLA_EOK;
}*/

int atla_fwrite_end_object(atla_Context_t* ctx) {
  // mark the object as ended
  // ctx->currentObj = ctx->currentObj->next;
  // validate bytes written
  return ctx->bytesWritten <= ctx->bytesToWrite ? ATLA_EOK : ATLA_EC_OBJECT_LEN_OVERRUN;
}

int atla_fwrite_end(atla_Context_t* ctx) {
  // flush IO buffer if needed.
  alta_FlushIOBuffer(ctx);
  return ATLA_EOK;
}

int atla_fwrite_i8(atla_Context_t* ctx, atint8 val) {
  return atla_fwrite_i8p(ctx, &val, 1);
}
int atla_fwrite_i8p(atla_Context_t* ctx, atint8 const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_u8(atla_Context_t* ctx, atuint8 val) {
  return atla_fwrite_u8p(ctx, &val, 1);
}
int atla_fwrite_u8p(atla_Context_t* ctx, atuint8 const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_i16(atla_Context_t* ctx, atint16 val) {
  return atla_fwrite_i16p(ctx, &val, 1);
}
int atla_fwrite_i16p(atla_Context_t* ctx, atint16 const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_u16(atla_Context_t* ctx, atuint16 val) {
  return atla_fwrite_u16p(ctx, &val, 1);
}
int atla_fwrite_u16p(atla_Context_t* ctx, atuint16 const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_i(atla_Context_t* ctx, atint val) {
  return atla_fwrite_ip(ctx, &val, 1);
}
int atla_fwrite_ip(atla_Context_t* ctx, atint const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_u(atla_Context_t* ctx, atuint val) {
  return atla_fwrite_up(ctx, &val, 1);
}
int atla_fwrite_up(atla_Context_t* ctx, atuint const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_i32(atla_Context_t* ctx, atint32 val) {
  return atla_fwrite_i32p(ctx, &val, 1);
}
int atla_fwrite_i32p(atla_Context_t* ctx, atint32 const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_u32(atla_Context_t* ctx, atuint32 val) {
  return atla_fwrite_u32p(ctx, &val, 1);
}
int atla_fwrite_u32p(atla_Context_t* ctx, atuint32 const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_i64(atla_Context_t* ctx, atint64 val) {
  return atla_fwrite_i64p(ctx, &val, 1);
}
int atla_fwrite_i64p(atla_Context_t* ctx, atint64 const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_u64(atla_Context_t* ctx, atuint64 val) {
  return atla_fwrite_u64p(ctx, &val, 1);
}
int atla_fwrite_u64p(atla_Context_t* ctx, atuint64 const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_f32(atla_Context_t* ctx, float val) {
  return atla_fwrite_f32p(ctx, &val, 1);
}
int atla_fwrite_f32p(atla_Context_t* ctx, float const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}
int atla_fwrite_f64(atla_Context_t* ctx, double val) {
  return atla_fwrite_f64p(ctx, &val, 1);
}
int atla_fwrite_f64p(atla_Context_t* ctx, double const* val, atsize_t count) {
  return atla_SendToIOBuffer(ctx, val, sizeof(*val) * count);
}

int atla_fwrite_obj_pointer(atla_Context_t* ctx, atla_ObjectPointer_t optr) {
  atla_fwrite_u32(ctx, optr.objectID);
  atla_fwrite_u32(ctx, optr.memberID);
  atla_fwrite_u32(ctx, optr.count);
  return ATLA_EOK;
}

int atla_fwrite_pointer_to_obj_pointer(atla_Context_t* ctx, void* ptr, atla_ObjectPointer_t* optr) {
  if (ptr == NULL) {
    optr->count = 0;
    optr->objectID = 0;
    optr->memberID = 0;
    return ATLA_EOK;
  }
  atla_ObjectRef_t const* oref = atla_fwrite_find_object_ref_by_ptr(ctx, ptr);
  if (!oref) return ATLA_EC_UNKNOWN_OBJ;
  optr->objectID = oref->objectID;
  optr->memberID = 0;
  optr->count = 0;
  if (oref->dataPtr != ptr) {
    // handle pointing within range
    uintptr_t byte_offset = (uintptr_t)ptr - (uintptr_t)oref->dataPtr;
    atsize_t  rt_size = oref->runtimeSize;
    if (byte_offset % rt_size != 0) {
      // Inter member. TODO: Implelment
      return ATLA_EC_NOT_IMPLEMENTED;
    } else {
      optr->count = byte_offset / rt_size;
    }
  }
  return ATLA_EOK;
}

int atla_fread_begin(atla_Context_t* ctx) {
  if (!ctx->io.read) return ATLA_IO_UNSUPPORTED;
  // read the header
  atla_fread_u32(ctx, &ctx->header.blkHeader.fourCC);
  atla_fread_u32(ctx, &ctx->header.blkHeader.version);
  atla_fread_u64(ctx, &ctx->header.blkHeader.size);
  if (ctx->header.blkHeader.fourCC != atla_FileHeader_fourCC) return ATLA_EC_UNKNOWN_FILE_FORMAT;
  return ATLA_EOK;
}

atsize_t atla_fread_get_object_count(atla_Context_t* ctx) {
  atuint32 fourCC, version_num, disk_size, object_list_size;

  // calculate the file object list block size
  atla_fread_u32(ctx, &fourCC);      // smd_make_fourCC('O', 'b', 'j', 'L'));
  atla_fread_u32(ctx, &version_num); // atla_FileObjectList_version);
  atla_fread_u32(ctx, &disk_size);   // atla_FileObjectList_disksize +
                                     // (atla_FileObjectEntry_disksize * ctx->objectCount));
  if (fourCC != atla_FileObjectList_fourCC) return ATLA_EC_UNKNOWN_FILE_FORMAT;
  if (version_num != atla_FileObjectList_version) return ATLA_EC_UNKNOWN_FILE_FORMAT;
  object_list_size = (disk_size - atla_FileObjectList_disksize) / atla_FileObjectEntry_disksize;
  ctx->objectCount = object_list_size;
  return ctx->objectCount;
}

// Assign memory for object locations found during read.
// read the object list and fill location array for reading.
int atla_fread_assign_object_locations(atla_Context_t* ctx, atla_ObjectLocation_t* arr, atsize_t count) {
  if (count < ctx->objectCount) return ATLA_EC_OUT_OF_MEMORY;
  ctx->read.objectList = arr;
  for (atuint32 i = 0; i < ctx->objectCount; ++i) {
    atla_fread_u32(ctx, &ctx->read.objectList[i].typeID);
    atla_fread_u32(ctx, &ctx->read.objectList[i].objectID);
    atla_fread_u64(ctx, &ctx->read.objectList[i].count);
  }

  return ATLA_EOK;
}

// Validate that object pointers are not empty,
int atla_fread_begin_objects(atla_Context_t* ctx) {
  for (atuint32 i = 0; i < ctx->objectCount; ++i) {
    if (ctx->read.objectList[i].dataPtr == NULL) return ATLA_EC_OUT_OF_MEMORY;
  }
  ctx->read.currentObject = 0;
  return ATLA_EOK;
}

// Begin reading an object from file. Returns the location data (type id, object
// id, array count, memory pointer).
int atla_fread_next_object(atla_Context_t* ctx, atla_ObjectLocation_t* ol) {
  atuint32 fourCC, hdr_version_num, disk_size;
  atuint32 typeID, objectID, version;
  atsize_t count;
  do {
    if (ctx->read.currentObject >= ctx->objectCount) {
      return ATLA_EC_END_OF_LIST;
    }
    *ol = ctx->read.objectList[ctx->read.currentObject];
    // read header
    atla_fread_u32(ctx, &fourCC);
    atla_fread_u32(ctx, &hdr_version_num);
    atla_fread_u32(ctx, &disk_size); // atla_FileObject_disksize + expected_byte_count);
    atla_fread_u32(ctx, &typeID);
    atla_fread_u32(ctx, &objectID);
    atla_fread_u32(ctx, &version);
    atla_fread_u64(ctx, &count);
    if (ol->typeID != typeID) return ATLA_EC_UNKNOWN_FILE_FORMAT;
    if (ol->objectID != objectID) return ATLA_EC_UNKNOWN_FILE_FORMAT;
    if (ol->count != count) return ATLA_EC_UNKNOWN_FILE_FORMAT;
    if (ol->typeID < atla_type_composite) {
      atla_fread_u8p(ctx, ol->dataPtr, ol->count * BaseTypeSizeLookup[ol->typeID]);
    }
    ctx->read.currentObject++;
  } while (ol->typeID < atla_type_composite);
  ol->version = version;
  return ATLA_EOK;
}

int atla_fread_skip_obj(atla_Context_t* ctx) {
  return ATLA_EC_NOT_IMPLEMENTED;
}
// Clean up and data for reading. Releases resources assigned after
// atla_fread_begin() was called, allowing them to be freed.
int atla_fread_end(atla_Context_t* ctx) {
  ctx->read.objectList = NULL;
  return ATLA_EOK;
}

int atla_fread_i8(atla_Context_t* ctx, atint8* val) {
  return atla_fread_i8p(ctx, val, 1);
}
int atla_fread_i8p(atla_Context_t* ctx, atint8* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atint8) * count);
}
int atla_fread_u8(atla_Context_t* ctx, atuint8* val) {
  return atla_fread_u8p(ctx, val, 1);
}
int atla_fread_u8p(atla_Context_t* ctx, atuint8* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atuint8) * count);
}
int atla_fread_i16(atla_Context_t* ctx, atint16* val) {
  return atla_fread_i16p(ctx, val, 1);
}
int atla_fread_i16p(atla_Context_t* ctx, atint16* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atint16) * count);
}
int atla_fread_u16(atla_Context_t* ctx, atuint16* val) {
  return atla_fread_u16p(ctx, val, 1);
}
int atla_fread_u16p(atla_Context_t* ctx, atuint16* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atint16) * count);
}
int atla_fread_i(atla_Context_t* ctx, atint* val) {
  return atla_fread_ip(ctx, val, 1);
}
int atla_fread_ip(atla_Context_t* ctx, atint* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atint) * count);
}
int atla_fread_u(atla_Context_t* ctx, atuint* val) {
  return atla_fread_up(ctx, val, 1);
}
int atla_fread_up(atla_Context_t* ctx, atuint* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atuint) * count);
}
int atla_fread_i32(atla_Context_t* ctx, atint32* val) {
  return atla_fread_i32p(ctx, val, 1);
}
int atla_fread_i32p(atla_Context_t* ctx, atint32* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atint32) * count);
}
int atla_fread_u32(atla_Context_t* ctx, atuint32* val) {
  return atla_fread_u32p(ctx, val, 1);
}
int atla_fread_u32p(atla_Context_t* ctx, atuint32* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atuint32) * count);
}
int atla_fread_i64(atla_Context_t* ctx, atint64* val) {
  return atla_fread_i64p(ctx, val, 1);
}
int atla_fread_i64p(atla_Context_t* ctx, atint64* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atint64) * count);
}
int atla_fread_u64(atla_Context_t* ctx, atuint64* val) {
  return atla_fread_u64p(ctx, val, 1);
}
int atla_fread_u64p(atla_Context_t* ctx, atuint64* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(atuint64) * count);
}
int atla_fread_f32(atla_Context_t* ctx, float* val) {
  return atla_fread_f32p(ctx, val, 1);
}
int atla_fread_f32p(atla_Context_t* ctx, float* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(float) * count);
}
int atla_fread_f64(atla_Context_t* ctx, double* val) {
  return atla_fread_f64p(ctx, val, 1);
}
int atla_fread_f64p(atla_Context_t* ctx, double* val, atsize_t count) {
  return atla_read_from_io_buffer(ctx, val, sizeof(double) * count);
}

int atla_fread_obj_pointer(atla_Context_t* ctx, atla_ObjectPointer_t* optr) {
  atla_fread_u32(ctx, &optr->objectID);
  atla_fread_u32(ctx, &optr->memberID);
  atla_fread_u32(ctx, &optr->count);
  return ATLA_EOK;
}
void* atla_fread_obj_pointer_to_pointer(atla_Context_t* ctx, atla_ObjectPointer_t optr) {
  atsize_t rtSize, dSize;
  atuint32 ver;
  for (int i = 0, n = ctx->objectCount; i < n; ++i) {
    if (ctx->read.objectList[i].objectID == optr.objectID) {
      if (ctx->read.objectList[i].typeID < atla_type_composite) {
        rtSize = BaseTypeSizeLookup[ctx->read.objectList[i].typeID];
      } else {
        ctx->typeInfoFn(ctx->read.objectList[i].typeID, &rtSize, &ver, &dSize);
      }
      return (void*)((uintptr_t)ctx->read.objectList[i].dataPtr + (rtSize * optr.count));
    }
  }
  return NULL;
}
