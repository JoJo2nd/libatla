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
#include <string.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ATLA_VERSION_MAJOR (0)
#define ATLA_VERSION_MINOR (1)
#define ATLA_VERSION_REV (1)

#define at_itoptr(x) ((void*)((uintptr_t)(x)))

atAtlaRuntimeTypeInfo_t atla_runtime_type_list;

atAtlaRuntimeTypeInfo_t* atla_type_reg(atAtlaRuntimeTypeInfo_t* head,
                                       atAtlaRuntimeTypeInfo_t* next) {
  atAtlaRuntimeTypeInfo_t* tail = head->next;
  head->next = next;
  return tail;
}

static int addr_comp(void const* a, void const* b) {
  atAddressIDPair_t const *c = a, *d = b;
  if ((uintptr_t)c->address < (uintptr_t)d->address) return -1;
  if ((uintptr_t)c->address > (uintptr_t)d->address) return 1;
  return 0;
}

uint64_t ATLA_API atGetAtlaVersion() {
  return (((atuint64)(ATLA_VERSION_MAJOR & 0xFFFF) << 48) |
          ((atuint64)(ATLA_VERSION_MAJOR & 0xFFFF) << 32) | ATLA_VERSION_REV);
}

void atSerializeWriteBegin(atAtlaSerializer_t* serializer,
                           atAtlaContext_t*    ctx,
                           char const*         usertag,
                           atioaccess_t*       io,
                           uint32_t            version) {
  atMemoryHandler_t* mem = &ctx->mem;
  serializer->ctx = ctx;
  serializer->mem = &ctx->mem;
  serializer->io = io;
  serializer->reading = 0;
  serializer->version = version;
  serializer->nextID = 1;
  serializer->depth = 0;
  serializer->objectListLen = 0;
  serializer->objectListRes = 16;
  serializer->objectList =
    mem->alloc(sizeof(atAtlaTypeData_t) * serializer->objectListRes, mem->user);
  serializer->idList = mem->alloc(
    sizeof(atAddressIDPair_t) * serializer->objectListRes, mem->user);
  strncpy(serializer->userTag, usertag, ATLA_USER_TAG_LEN);
}

void atSerializeWriteRoot(atAtlaSerializer_t*    serializer,
                          void*                  data,
                          atSerializeTypeProc_t* proc) {
  atSerializeWrite(serializer, &serializer->userTag, 1, ATLA_USER_TAG_LEN);
  atSerializeWrite(serializer, "ATLA", 1, 4);
  atSerializeWrite(serializer, &serializer->version, sizeof(uint32_t), 1);
  (*proc)(serializer, data);
}

void atSerializeWriteProcessPending(atAtlaSerializer_t* serializer) {
  // walk the list of objects awaiting writes, find first that hasn't be done
  // and do it
  // continue until nothing is left to do...Note that more objects may be added
  // mid iteration of this list
  for (uint32_t i = 0; i < serializer->objectListLen; ++i) {
    if ((serializer->objectList[i].flags & at_wflag_processed) == 0) {
      // IDs are 1 based because zero is reserved for null pointers
      serializer->objectList[i].foffset =
        serializer->io->tellProc(serializer->io->user) + sizeof(uint32_t);
      serializer->objectList[i].flags |= at_wflag_processed;
      if (!serializer->objectList[i].proc.ptr) {
        // atomic data type, count first then data
        atSerializeWrite(
          serializer, &serializer->objectList[i].count, sizeof(uint32_t), 1);
        atSerializeWrite(serializer,
                         serializer->objectList[i].data,
                         serializer->objectList[i].size,
                         serializer->objectList[i].count);
      } else {
        // non-atomic data type
        uint32_t n = serializer->objectList[i].count;
        uint32_t s = serializer->objectList[i].size;
        atSerializeWrite(serializer, &n, sizeof(uint32_t), 1);
        for (uint32_t j = 0; j < n; ++j) {
          (*serializer->objectList[i].proc.ptr)(
            serializer, (uint8_t*)serializer->objectList[i].data + s * j);
        }
      }
    }
  }
}

void atSerializeWriteFinalize(atAtlaSerializer_t* serializer) {
  // sort object list by object ID (for easy look up next time)
  // write the footer
  atioaccess_t* io = serializer->io;
  uint32_t      total_str_len = 0;
  for (uint32_t i = 0, n = serializer->objectListLen; i < n; ++i) {
    serializer->objectList[i].data = NULL;
    char const* name_str = serializer->objectList[i].name.ptr;
    if (name_str && !(serializer->objectList[i].flags & at_rflag_hasname)) {
      //uint32_t offset = total_str_len;
      uint32_t name_str_len = (uint32_t)(strlen(name_str) + 1);
      io->writeProc(name_str, name_str_len, io->user);
      serializer->objectList[i].flags |= at_rflag_hasname;
      for (uint32_t j = i; j < n; ++j) {
        if (serializer->objectList[j].name.ptr == name_str) {
          serializer->objectList[j].flags |= at_rflag_hasname;
          serializer->objectList[j].name.offset = total_str_len;
          serializer->objectList[j].proc.offset = 0;
        }
      }
      total_str_len += name_str_len;
    }
  }

  io->writeProc(serializer->objectList,
                sizeof(atAtlaTypeData_t) * serializer->objectListLen,
                io->user);
  io->writeProc(&serializer->objectListLen, sizeof(uint32_t), io->user);
  io->writeProc(&total_str_len, sizeof(uint32_t), io->user);
}

void atSerializeWrite(atAtlaSerializer_t* serializer,
                      void*               src,
                      uint32_t            element_size,
                      uint32_t            element_count) {
  atioaccess_t* io = serializer->io;
  io->writeProc(src, element_size * element_count, io->user);
}

uint32_t atSerializeWritePendingBlob(atAtlaSerializer_t* serializer,
                                     void*               data,
                                     uint32_t            element_size,
                                     uint32_t            count) {
  // Big TODO: handle pointer offsets with already serialized memory blobs
  // (track with id plus offset??)

  if (!data) return 0;
  atMemoryHandler_t* mem = serializer->mem;
  atAtlaTypeData_t   new_blob = {.name.ptr = NULL,
                               .proc.ptr = NULL,
                               .size = element_size,
                               .count = count,
                               .data = data,
                               .id = serializer->nextID,
                               .foffset = 0,
                               .flags = 0};
  atAddressIDPair_t* foundid = bsearch(&new_blob,
                                       serializer->idList,
                                       serializer->objectListLen,
                                       sizeof(atAddressIDPair_t),
                                       addr_comp);
  if (foundid) return serializer->objectList[foundid->index].id;

  if (serializer->objectListLen + 1 >= serializer->objectListRes) {
    serializer->objectListRes *= 2;
    serializer->objectList =
      mem->ralloc(serializer->objectList,
                  sizeof(atAtlaTypeData_t) * serializer->objectListRes,
                  mem->user);
    serializer->idList =
      mem->ralloc(serializer->idList,
                  sizeof(atAddressIDPair_t) * serializer->objectListRes,
                  mem->user);
  }
  serializer->objectList[serializer->objectListLen] = new_blob;
  serializer->idList[serializer->objectListLen].index =
    serializer->objectListLen;
  serializer->idList[serializer->objectListLen++].address = data;
  ++serializer->nextID;
  qsort(serializer->idList,
        serializer->objectListLen,
        sizeof(atAddressIDPair_t),
        addr_comp);
  return new_blob.id;
}

uint32_t atSerializeWritePendingType(atAtlaSerializer_t*    serializer,
                                     void*                  data,
                                     char const*            name,
                                     atSerializeTypeProc_t* proc,
                                     uint32_t               element_size,
                                     uint32_t               count) {
  // Big TODO: handle pointer offsets with already serialized memory blobs
  // (track with id plus offset??)

  if (!data) return 0;
  atMemoryHandler_t* mem = serializer->mem;
  atAtlaTypeData_t   new_blob = {.name.ptr = name,
                               .proc.ptr = proc,
                               .size = element_size,
                               .count = count,
                               .data = data,
                               .id = serializer->nextID,
                               .foffset = 0,
                               .flags = 0};
  atAddressIDPair_t* foundid = bsearch(&new_blob,
                                       serializer->idList,
                                       serializer->objectListLen,
                                       sizeof(atAddressIDPair_t),
                                       addr_comp);
  if (foundid) return serializer->objectList[foundid->index].id;

  if (serializer->objectListLen + 1 >= serializer->objectListRes) {
    serializer->objectListRes *= 2;
    serializer->objectList =
      mem->ralloc(serializer->objectList,
                  sizeof(atAtlaTypeData_t) * serializer->objectListRes,
                  mem->user);
    serializer->idList =
      mem->ralloc(serializer->idList,
                  sizeof(atAddressIDPair_t) * serializer->objectListRes,
                  mem->user);
  }
  serializer->objectList[serializer->objectListLen] = new_blob;
  serializer->idList[serializer->objectListLen].index =
    serializer->objectListLen;
  serializer->idList[serializer->objectListLen++].address = data;
  ++serializer->nextID;
  qsort(serializer->idList,
        serializer->objectListLen,
        sizeof(atAddressIDPair_t),
        addr_comp);
  return new_blob.id;
}

void atSerializeRead(atAtlaSerializer_t* serializer,
                     void*               dest,
                     uint32_t            element_size,
                     uint32_t            element_count) {
  atioaccess_t* io = serializer->io;
  io->readProc(dest, element_size*element_count, io->user);
}

void atSerializeSkip(atAtlaSerializer_t* serializer,
                     uint32_t            element_size,
                     uint32_t            element_count) {
  atioaccess_t* io = serializer->io;
  io->seekProc(element_size*element_count, eSeekOffset_Current, io->user);
}

void* atSerializeReadGetBlobLocation(atAtlaSerializer_t* serializer,
                                     uint32_t            blob_id) {
  if (blob_id == 0) return NULL;
  return serializer->objectList[blob_id - 1].rmem.ptr;
}


void* atSerializeReadTypeLocation(atAtlaSerializer_t*    serializer,
                                  uint32_t               type_id,
                                  char const*            name,
                                  atSerializeTypeProc_t* proc) {
  if (type_id == 0) return NULL;
  serializer->objectList[type_id - 1].proc.ptr = proc;
  return serializer->objectList[type_id - 1].rmem.ptr;
}

void atSerializeReadBegin(atAtlaSerializer_t* serializer,
                          atAtlaContext_t*    ctx,
                          atioaccess_t*       io,
                          uint32_t            version) {
  uint32_t           string_table_len;
  char               atla_tag[5] = {0};
  atMemoryHandler_t* mem = &ctx->mem;
  memset(serializer, 0, sizeof(atAtlaSerializer_t));
  serializer->ctx = ctx;
  serializer->io = io;
  serializer->mem = &ctx->mem;
  serializer->reading = 1;
  serializer->version = version;
  io->readProc(serializer->userTag, ATLA_USER_TAG_LEN, io->user);
  io->readProc(atla_tag, 4, io->user);
  io->readProc(&serializer->version, sizeof(serializer->version), io->user);
  io->seekProc(-((int64_t)sizeof(uint32_t) * 2), eSeekOffset_End, io->user);
  io->readProc(&serializer->objectListLen, sizeof(uint32_t), io->user);
  io->readProc(&string_table_len, sizeof(uint32_t), io->user);
  serializer->objectList =
    mem->alloc(sizeof(atAtlaTypeData_t) * serializer->objectListLen, mem->user);
  serializer->objectListRes = serializer->objectListLen;
  serializer->rStrings = mem->alloc(string_table_len, mem->user);
  io->seekProc(-(int64_t)(serializer->objectListLen * sizeof(atAtlaTypeData_t) +
                 string_table_len + sizeof(uint32_t) * 2),
               eSeekOffset_Current,
               io->user);
  io->readProc(serializer->rStrings, string_table_len, io->user);
  io->readProc(serializer->objectList,
               serializer->objectListLen * sizeof(atAtlaTypeData_t),
               io->user);

  for (uint32_t i = 0, n = serializer->objectListLen; i < n; ++i) {
    serializer->objectList[i].name.ptr =
      serializer->objectList[i].flags & at_rflag_hasname
        ? serializer->rStrings + serializer->objectList[i].name.offset
        : NULL;
    if (serializer->objectList[i].name.ptr) {
      uintptr_t tidx = ((uintptr_t)ht_table_find(
        &serializer->ctx->typeLUT, serializer->objectList[i].name.ptr));
      atla_assert(strcmp(serializer->ctx->types[tidx].name,
                    serializer->objectList[i].name.ptr) == 0, "Name mismatch");
      atla_assert((uint32_t)serializer->ctx->types[tidx].size == serializer->ctx->types[tidx].size,
        "int overflow");
      serializer->objectList[i].size = (uint32_t)serializer->ctx->types[tidx].size;
    }
  }
}

void atSerializeReadRoot(atAtlaSerializer_t*    serializer,
                         void*                  dest,
                         atSerializeTypeProc_t* proc) {
  atioaccess_t* io = serializer->io;
  // check that all objects have memory allocated for them.
  for (uint32_t i = 0, n = serializer->objectListLen; i < n; ++i) {
    if (serializer->objectList[i].rmem.ptr == 0) {
      atla_assert(
        false,
        "Object in the object list hasn't been assigned memory to read into");
      return;
    }
  }

  io->seekProc(
    ATLA_USER_TAG_LEN + sizeof(uint32_t) + 4, eSeekOffset_Begin, io->user);
  (*proc)(serializer, dest);

  int work_to_do = 0;
  do {
    for (uint32_t i = 0, n = serializer->objectListLen; i < n; ++i) {
      atAtlaTypeData_t* tdata = &serializer->objectList[i];
      if (tdata->name.ptr) {
        if (tdata->proc.ptr) {
          io->seekProc(tdata->foffset, eSeekOffset_Begin, io->user);
          for (uint32_t j = 0, m = tdata->count; j < m; ++j) {
            (*tdata->proc.ptr)(serializer, (uint8_t*)tdata->rmem.ptr + (tdata->size * j));
          }
        } else {
          work_to_do = 1;
        }
      } else {
        io->seekProc(tdata->foffset, eSeekOffset_Begin, io->user);
        io->readProc(tdata->rmem.ptr, tdata->size * tdata->count, io->user);
      }
    }
  } while (work_to_do);
}

void atSerializeReadFinalize(atAtlaSerializer_t* serializer) {
  atMemoryHandler_t* mem = serializer->mem;
  mem->free(serializer->rStrings, mem->user);
  mem->free(serializer->objectList, mem->user);
  memset(serializer, 0, sizeof(atAtlaSerializer_t));
}

#define FVN_OFFSET_BASIS (0xcbf29ce484222325)
#define FVN_PRIME (0x100000001b3)

static uint32_t hash_key(void const* key) {
  // murmur hash would be good here. Temp use FVN-1a
  size_t hash = FVN_OFFSET_BASIS;
  for (char const* c = (char const*)key; *c; ++c) {
    hash = (hash ^ *c) * FVN_PRIME;
  }
  return (uint32_t)hash;
}

static int compare_key(void const* lhs, void const* rhs) {
  return strcmp((char const*)lhs, (char const*)rhs);
}

static void value_free(void const* key, void* value) {}

void ATLA_API atCreateAtlaContext(atAtlaContext_t*         ctx,
                                  atMemoryHandler_t const* mem_handler) {
  ctx->mem = *mem_handler;
  ctx->typesCount = 0;
  uint32_t block_count = 16;
  ctx->types =
    (*ctx->mem.alloc)(sizeof(atAtlaTInfo_t) * block_count, ctx->mem.user);
  ht_table_init(&ctx->typeLUT,
                hash_key,
                compare_key,
                value_free,
                ctx->mem.ralloc,
                ctx->mem.free,
                ctx->mem.user);
}

void ATLA_API atContextRegisterType(atAtlaContext_t* ctx,
                                    char const*      name,
                                    size_t           size) {
  uint32_t prev_types_count = ctx->typesCount;
  ++ctx->typesCount;
  if (((ctx->typesCount + 15) & ~15) > ((prev_types_count + 15) & ~15)) {
    uint32_t block_count = ((ctx->typesCount + 15) & ~15);
    ctx->types = (*ctx->mem.ralloc)(
      ctx->types, sizeof(atAtlaTInfo_t) * block_count, ctx->mem.user);
  }
  ht_table_insert(&ctx->typeLUT, name, at_itoptr(prev_types_count));
  ctx->types[prev_types_count].name = name;
  ctx->types[prev_types_count].size = size;
}

void ATLA_API atDestroyAtlaContext(atAtlaContext_t* ctx) {
  ht_table_destroy(&ctx->typeLUT);
  (*ctx->mem.free)(ctx->types, ctx->mem.user);
}

size_t atUtilCalcReadMemRequirements(atAtlaSerializer_t* ser) {
  size_t total_mem = 0;
  ht_hash_table_t* ht = &ser->ctx->typeLUT;
  atAtlaTInfo_t* tlist = ser->ctx->types;
  for (uint32_t i = 0, n = ser->objectListLen; i < n; ++i) {
    char const* type_name = ser->objectList[i].name.ptr;
    if (type_name == NULL) {
      total_mem += ser->objectList[i].size * ser->objectList[i].count;
    } else {
      uintptr_t type_idx = (uintptr_t)ht_table_find(ht, type_name);
      total_mem += tlist[type_idx].size * ser->objectList[i].count;
    }
  }
  return total_mem;
}

int atUtilAssignReadMem(atAtlaSerializer_t* ser, void* mem, size_t len) {
  uint8_t*    memp = (uint8_t*)(mem);
  uint8_t* memend = memp + len;
  ht_hash_table_t* ht = &ser->ctx->typeLUT;
  atAtlaTInfo_t* tlist = ser->ctx->types;
  for (uint32_t i = 0, n = ser->objectListLen; i < n; ++i) {
    if (memp >= memend) return 0;
    char const* type_name = ser->objectList[i].name.ptr;
    if (type_name == NULL) {
      ser->objectList[i].rmem.ptr = memp;
      memp += ser->objectList[i].size * ser->objectList[i].count;
    } else {
      ser->objectList[i].rmem.ptr = memp;
      uintptr_t type_idx = (uintptr_t)ht_table_find(ht, type_name);
      memp += tlist[type_idx].size * ser->objectList[i].count;
    }
  }
  return 1;
}
