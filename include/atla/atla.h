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

        2. Altered source versions must be plainly marked as such, and must not
be
        misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.

*********************************************************************/

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#if defined(_WIN32)
#  if defined(libatla_EXPORTS)
#    define ATLA_EXPORT __declspec(dllexport)
#  else
#    define ATLA_EXPORT __declspec(dllimport)
#  endif
#else // non windows
#  define ATLA_EXPORT
#endif

#if defined(_DEBUG)
#define ATLA_DEBUG
#else
#define ATLA_RELEASE
#endif

#ifdef ATLA_DEBUG
#define ATLA_USE_ASSERT (1)
#endif

#define ATLA_CALLBACK __cdecl
#define ATLA_API __cdecl

//#define ATLA_USE_ASSERT (0)

#if !defined(ATLA_USE_ASSERT)
#define ATLA_USE_ASSERT (0)
#endif

#if ATLA_USE_ASSERT
# define atla_assert(cond, msg, ...)
#else
# define atla_assert(cond, msg, ...)
#endif

#ifdef COMPILE_LIB_ATLA
#define ATLA_ENSURE_PRIVATE_HEADER()
#else
#define ATLA_ENSURE_PRIVATE_HEADER()                                           \
  atCompileTimeAssert(0 &&                                                     \
                      "This header shouldn't be included outside of libatla")
#endif

typedef struct ht_hash_elem ht_elem_t;

struct ht_hash_table {
  uint32_t (*hash_key)(void const* key);
  int (*compare_key)(void const* a, void const* b);
  void (*value_free)(void const* k, void* v);
  void* (*mralloc)(void* ptr, size_t size, void* user);
  void (*mfree)(void* ptr, void* user);

  ht_elem_t* __restrict buffer;
  uint32_t* __restrict hashes;
  void*    user;
  int      num_elems;
  int      capacity;
  int      resize_threshold;
  uint32_t mask;
};

typedef struct ht_hash_table ht_hash_table_t;

typedef enum atTypeID {
  atAtomicType = 1,
  atCString,
  atUserType,

  atTypeInvalid = -1
} atTypeID;

//////////////////////////////////////////////////////////////////////////
// IO access /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

typedef enum atCreateFlags { ATLA_READ = 1, ATLA_WRITE = 1 << 1 } atCreateFlags;

#define ATLA_INVALIDPOINTERREF (~0U)

typedef enum atErrorCode {
  ATLA_EOK = 0,
  ATLA_NOMEM = -1,
  ATLA_BADFILE = -2,
  ATLA_DUPLICATE_DATA = -3,
  ATLA_NOTSUPPORTED = -4,
  ATLA_EPOINTERNOTFOUND = -5,
  ATLA_EBADSCHEMAID = -6,
  ATLA_ENESTEDSCHEMANOTFOUND = -7,
  ATLA_ETYPEMISMATCH = -8,
  ATLA_ENOTFOUND = -9,
  ATLA_EOUTOFRANGE = -10
} atErrorCode;

typedef size_t             atsize_t;
typedef unsigned long long atuint64;
typedef signed long long   atint64;
typedef unsigned long      atuint32;
typedef signed long        atint32;
typedef unsigned int       atuint;
typedef signed int         atint;
typedef unsigned short     atuint16;
typedef signed short       atint16;
typedef signed char        atint8;
typedef unsigned char      atuint8;
typedef atuint8            atbyte;
typedef char               atchar;
typedef atuint8            atbool;
typedef atuint64           atUUID_t;

#ifdef __cplusplus
extern "C" {
#endif //

/*
 * Main Include for alta lib
 **/

typedef void*(ATLA_CALLBACK* atMallocProc)(atsize_t size, void* user);
typedef void*(ATLA_CALLBACK* atReallocProc)(void*    ptr,
                                            atsize_t size,
                                            void*    user);
typedef void(ATLA_CALLBACK* atFreeProc)(void* ptr, void* user);

struct atMemoryHandler {
  atMallocProc  alloc;
  atReallocProc ralloc;
  atFreeProc    free;
  void*         user;
};

typedef struct atMemoryHandler atMemoryHandler_t;

typedef enum atSeekOffset {
  eSeekOffset_Begin = SEEK_SET,
  eSeekOffset_Current = SEEK_CUR,
  eSeekOffset_End = SEEK_END
} atSeekOffset;

typedef void(ATLA_CALLBACK* atIOReadProc)(void*    pBuffer,
                                          uint32_t size,
                                          void*    user);
typedef void(ATLA_CALLBACK* atIOWriteProc)(void const* pBuffer,
                                           uint32_t    size,
                                           void*       user);
typedef uint32_t(ATLA_CALLBACK* atIOSeekProc)(int64_t     offset,
                                              atSeekOffset from,
                                              void*        user);
typedef int64_t(ATLA_CALLBACK* atIOTellProc)(void* user);

typedef struct atioaccess_t {
  atIOReadProc  readProc;
  atIOWriteProc writeProc;
  atIOSeekProc  seekProc;
  atIOTellProc  tellProc;
  void*         user;
} atioaccess_t;

#define ATLA_USER_TAG_LEN (32)

typedef uint64_t  at_loc_t;
typedef uintptr_t at_handle_t;

#define at_invalid_handle (0)

typedef struct atAtlaSerializer atAtlaSerializer_t;

typedef void(atSerializeTypeProc_t)(atAtlaSerializer_t*, uint32_t version, void*);

#define at_rflag_root (0x1)
#define at_wflag_processed (0x2)
#define at_rflag_hasname (0x4)

typedef struct atAtlaTInfo {
  char const* name;
  size_t      size;
} atAtlaTInfo_t;

struct atAtlaContext_t {
  atMemoryHandler_t mem;
  ht_hash_table_t   typeLUT;
  atAtlaTInfo_t*    types;
  uint32_t          typesCount;
};
typedef struct atAtlaContext_t atAtlaContext_t;

typedef struct atAtlaFileHeader_t {
  char fourcc[4];
  union {
    struct {
      uint16_t major, minor, revision, patch;
    } fileFormat;
    size_t fileFormatVersion;
  };
  size_t objectCount;
  size_t stringTableLen;
} atAtlaFileHeader_t;

typedef struct atAtlaFileFooter_t {
  //?
} atAtlaFileFooter_t;

struct atAtlaTypeData {
  union {
    at_loc_t    offset;
    char const* ptr;
  } name; // if NULL then this is a native type (e.g. int, float, char, etc)
  union {
    at_loc_t               offset;
    atSerializeTypeProc_t* ptr;
  } proc; // NULL, if name is NULL
  void*    data;
  uint32_t size;
  uint32_t count;
  uint32_t id;
  at_loc_t foffset; // location with the atla file
  // extra members needed for reads
  union {
    at_loc_t offset;
    void*    ptr;
  } rmem; // always null for write, on read filled in by user
  uint32_t flags;
  uint32_t version; // per type versioning
};
typedef struct atAtlaTypeData atAtlaTypeData_t;

typedef struct atAddressIDPair {
  void*    address;
  uint32_t index;
} atAddressIDPair_t;

struct atAtlaSerializer {
  atAtlaContext_t*   ctx;
  atMemoryHandler_t* mem;
  atioaccess_t*      io;
  uint32_t           reading;
  uint32_t           version;
  uint32_t           nextID;
  uint32_t           depth;
  uint32_t           objectListLen, objectListRes;
  atAtlaFileHeader_t fileHeader;
  atAtlaTypeData_t*  objectList;
  atAddressIDPair_t* idList;
  char*              rStrings;
  char               userTag[ATLA_USER_TAG_LEN];
};

typedef struct atAtlaRuntimeTypeInfo {
  char const*                   name;
  size_t                        size;
  struct atAtlaRuntimeTypeInfo* next;
} atAtlaRuntimeTypeInfo_t;

typedef struct atTypeVariant_t {
  union {
    int8_t   asInt8;
    int16_t  asInt16;
    int32_t  asInt32;
    uint8_t  asUint8;
    uint16_t asUint16;
    uint32_t asUint32;
    float    asFloat;
    double   asDouble;
  };
  uint8_t term;
} atTypeVariant_t;

enum {
  at_rinfo_none = 0,
  at_rinfo_deprecated = 0x01,
  at_rinfo_pointer = 0x02,
  at_rinfo_dynamic_array = 0x04,
  at_rinfo_fixed_array = 0x08,
};

enum {
  at_rinfo_type_none = 0,

  at_rinfo_type_int8,
  at_rinfo_type_char,
  at_rinfo_type_int16,
  at_rinfo_type_int32,
  at_rinfo_type_int64,

  at_rinfo_type_uint8,
  at_rinfo_type_uint16,
  at_rinfo_type_uint32,
  at_rinfo_type_uint64,

  at_rinfo_type_float,
  at_rinfo_type_double,

  at_rinfo_type_user,
};

typedef struct atAtlaReflInfo_t {
  char const* name;
  size_t      arrayCount;
  uint32_t    id;
  uint32_t    flags;
  uint32_t    verAdd, verRem;
  uint32_t    type;
  ptrdiff_t   offset;
  struct {
    struct atAtlaReflInfo_t const* reflData;
  } typeExtra;
  atTypeVariant_t* defaultVal;
  atTypeVariant_t* minVal;
  atTypeVariant_t* maxVal;
  uint32_t memberCount;
  struct atAtlaReflInfo_t* members;
} atAtlaReflInfo_t;

extern atAtlaRuntimeTypeInfo_t atla_runtime_type_list;

ATLA_EXPORT void ATLA_API atSerializeWriteBegin(atAtlaSerializer_t* serializer,
                           atAtlaContext_t*    ctx,
                           char const*         usertag,
                           atioaccess_t*       context,
                           uint32_t            version);
ATLA_EXPORT void ATLA_API atSerializeWriteRoot(atAtlaSerializer_t*    serializer,
                          void*                  data,
                          atSerializeTypeProc_t* proc);
ATLA_EXPORT void ATLA_API atSerializeWriteProcessPending(atAtlaSerializer_t* serializer);
ATLA_EXPORT void ATLA_API atSerializeWriteFinalize(atAtlaSerializer_t* serializer);

// Hidden interface functions???
ATLA_EXPORT void ATLA_API atSerializeWrite(atAtlaSerializer_t* serializer,
                      void*               src,
                      uint32_t            element_size,
                      uint32_t            element_count);
ATLA_EXPORT uint32_t ATLA_API atSerializeWritePendingBlob(atAtlaSerializer_t* serializer,
                                     void*               data,
                                     uint32_t            element_size,
                                     uint32_t            count);
ATLA_EXPORT uint32_t ATLA_API atSerializeWritePendingType(atAtlaSerializer_t* serializer,
                                     void*                  data,
                                     char const*            name,
                                     atSerializeTypeProc_t* proc,
                                     uint32_t               type_ver,
                                     uint32_t               element_size,
                                     uint32_t               count);

// uint32_t atSerializeObjRef(atAtlaSerializer_t* serializer, void* src, char
// const* type_name);

// libatla reading interface

ATLA_EXPORT void ATLA_API atSerializeReadBegin(atAtlaSerializer_t* serializer,
                          atAtlaContext_t*    ctx,
                          atioaccess_t*       context,
                          uint32_t            version);

ATLA_EXPORT void ATLA_API atSerializeReadRoot(atAtlaSerializer_t*    serializer,
                         void*                  dest,
                         atSerializeTypeProc_t* proc,
                         uint32_t type_ver);

ATLA_EXPORT void ATLA_API atSerializeReadFinalize(atAtlaSerializer_t* serializer);

// Hidden interface functions???
ATLA_EXPORT void ATLA_API atSerializeRead(atAtlaSerializer_t* serializer,
                     void*               dest,
                     uint32_t            element_size,
                     uint32_t            element_count);
ATLA_EXPORT void ATLA_API atSerializeSkip(atAtlaSerializer_t* serializer,
                     uint32_t            element_size,
                     uint32_t            element_count);
ATLA_EXPORT void* ATLA_API atSerializeReadGetBlobLocation(atAtlaSerializer_t* serializer,
                                     uint32_t            blob_id);

ATLA_EXPORT void* ATLA_API atSerializeReadTypeLocation(atAtlaSerializer_t*    serializer,
                                  uint32_t               type_id,
                                  char const*            name,
                                  atSerializeTypeProc_t* proc);

// void* atSerializeReadObjRef(atAtlaSerializer_t* serializer);


ATLA_EXPORT uint64_t ATLA_API atGetAtlaVersion();
ATLA_EXPORT void ATLA_API atCreateAtlaContext(atAtlaContext_t*         ctx,
                                  atMemoryHandler_t const* mem_handler);
ATLA_EXPORT void ATLA_API atDestroyAtlaContext(atAtlaContext_t*);
ATLA_EXPORT void ATLA_API atContextRegisterType(atAtlaContext_t* ctx,
                                    char const*      name,
                                    size_t           size);

#define atContextReg(ctx, type)                                                \
  void atla_##type##_reg (atAtlaContext_t* c);                    \
  atla_##type##_reg ((ctx))

ATLA_EXPORT size_t ATLA_API atUtilCalcReadMemRequirements(atAtlaSerializer_t* ser);
ATLA_EXPORT int ATLA_API atUtilAssignReadMem(atAtlaSerializer_t* ser, void* mem, size_t len);
ATLA_EXPORT int ATLA_API atUtilAllocAssignReadMem(atAtlaSerializer_t* ser);

ATLA_EXPORT void ATLA_API atCreateFileIOContext(atioaccess_t* io, char const* path, char const* access);

ATLA_EXPORT void ATLA_API atDestroyFileIOContext(atioaccess_t* io);

/*

        Atla -



   ///
   // New additional ideas for Atla
   
   *) Remove unused code files - one header, one source file?
   *) Allow split & multiple lua files for declaring & defining types. I think each lua file registers (up to) two functions
      one for declaring, one for defining. These are stored in the lua reg table and called later once all files are parsed
   *) Fix mid type pointer offsets not working.
   *) Allow per type versioning. 
   *) Remove per field version checks - exchange this for whole type up versioning, user defines how to handle up version for each 
      increment, no skipping versions allowed - could be done with strings of C code given in Lua. 
      Also, no adding pointered types (because requires allocating mid-read?) this should be enforced by auto gen'd post fix check
      on up versioning code given by user.
   *) No mid read allocs. User can ask how much memory needed and then provide it. Same for writes
   *) Enforce forward only reading.
   *) Decide how to protect from fuzzing (initial idea, signed block reader abstraction so parsing can ignore the problem?).
*/

#ifdef __cplusplus
} // extern "C"
#endif //
