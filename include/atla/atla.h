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

/********************************************************************
 * atla file format
 *
 * + atla header
 * + atla type & object library
 * -+ alta object listings [xN]
 * --+ object id
 * --+ other meta data
 * + serialized object data
 * -+ binary data described by type layout
 * --+ steam of raw types (int, float, etc) or object references
 *********************************************************************/

#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif //

#if defined(_WIN32)
#if defined(libatla_EXPORTS)
#define ATLA_EXPORT __declspec(dllexport)
#else
#define ATLA_EXPORT __declspec(dllimport)
#endif
#else // non windows
#define ATLA_EXPORT
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

// #define ATLA_USE_ASSERT (0)

#if !defined(ATLA_USE_ASSERT)
#define ATLA_USE_ASSERT (0)
#endif

#if ATLA_USE_ASSERT
#define atla_assert(cond, msg, ...)
#else
#define atla_assert(cond, msg, ...)
#endif

#ifdef COMPILE_LIB_ATLA
#define ATLA_ENSURE_PRIVATE_HEADER()
#else
#define ATLA_ENSURE_PRIVATE_HEADER() atCompileTimeAssert(0 && "This header shouldn't be included outside of libatla")
#endif

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

enum atla_ErrorCode {
  ATLA_EC_END_OF_LIST = 1,
  ATLA_EOK = 0,
  ATLA_NOMEM = -1,
  ATLA_EC_OUT_OF_MEMORY = -1,
  ATLA_BADFILE = -2,
  ATLA_DUPLICATE_DATA = -3,
  ATLA_NOTSUPPORTED = -4,
  ATLA_EPOINTERNOTFOUND = -5,
  ATLA_EBADSCHEMAID = -6,
  ATLA_ENESTEDSCHEMANOTFOUND = -7,
  ATLA_ETYPEMISMATCH = -8,
  ATLA_ENOTFOUND = -9,
  ATLA_EOUTOFRANGE = -10,
  ATLA_INCORRECT_TYPE_VERSION = -11,
  ATLA_IO_UNSUPPORTED = -12,
  ATLA_EC_OBJECT_REF_POOL_EXHAUSTED = -13,
  ATLA_EC_UNKNOWN_OBJ = -14,
  ATLA_EC_OBJECT_LEN_OVERRUN = -15,
  ATLA_EC_ALREADY = -16,
  ATLA_EC_UNKNOWN_FILE_FORMAT = -17,
  ATLA_EC_NOT_IMPLEMENTED = -18,
  ATLA_EC_TYPE_NOT_KNOWN = -19,
};

typedef enum atla_ErrorCode atErrorCode;
typedef enum atla_ErrorCode atla_ErrorCode;

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
typedef char               atchar;
typedef atchar             atbyte;
typedef atuint8            atbool;
typedef atuint64           atUUID_t;


/*
 * V2 interface
 */

typedef enum atla_Type {
  atla_type_none = 0,

  atla_type_int8,
  atla_type_char,
  atla_type_int16,
  atla_type_int32,
  atla_type_int64,

  atla_type_uint8,
  atla_type_uint16,
  atla_type_uint32,
  atla_type_uint64,

  atla_type_float,
  atla_type_double,

  atla_type_composite,

  atla_type_custom_type_id_first,
} atla_Type;

#define ATLA_TYPE_ID_MASK (0xFFFFFFF0)
#define ATLA_TYPE_ID_SHIFT (8)
#define ATLA_INVALID_TYPE_ID (0)

typedef enum atla_SeekOffset { atla_SeekOffset_Begin = SEEK_SET, atla_SeekOffset_Current = SEEK_CUR, atla_SeekOffset_End = SEEK_END } atla_SeekOffset;

typedef void(ATLA_CALLBACK* atla_IOReadProc)(void* pBuffer, atsize_t size, void* user);
typedef void(ATLA_CALLBACK* atla_IOWriteProc)(void const* pBuffer, atsize_t size, void* user);
typedef atsize_t(ATLA_CALLBACK* atla_IOSeekProc)(int64_t offset, atla_SeekOffset from, void* user);
typedef atsize_t(ATLA_CALLBACK* atla_IOTellProc)(void* user);

typedef int(ATLA_CALLBACK* atla_GetTypeInfo)(int type_id, atsize_t* runtimesize, atuint32* version, atsize_t* disksize);
typedef void*(ATLA_CALLBACK* atla_GetTypePointerOffset)(void* base, int type_id, int member_id);

typedef struct atla_ioaccess {
  atla_IOReadProc  read;
  atla_IOWriteProc write;
  void*            user;
} atla_ioaccess_t;


typedef struct atla_FileBlockHeader atla_FileBlockHeader_t;
typedef struct atla_FileHeader      atla_FileHeader_t;
typedef struct atla_Context         atla_Context_t;
typedef struct atla_ObjectRef       atla_ObjectRef_t;
typedef struct atla_ObjectRefPool   atla_ObjectRefPool_t;

struct atla_FileBlockHeader {
  union {
    char     fourCCBytes[4];
    atuint32 fourCC;
  };
  atuint32 version;
  atuint64 size;
};

struct atla_FileHeader {
  atla_FileBlockHeader_t blkHeader;
};

typedef struct atla_ObjectLocation {
  atuint32 objectID;
  atuint32 typeID;
  atuint32 version;
  atsize_t count;
  atbyte*  dataPtr;
} atla_ObjectLocation_t;

typedef struct atla_ObjectPointer {
  atuint32 objectID;
  atuint32 count;
  atuint32 memberID;
} atla_ObjectPointer_t;

struct atla_Context {
  atla_ioaccess_t           io;
  atla_GetTypeInfo          typeInfoFn;
  atla_GetTypePointerOffset typePointerOffsetFn;
  atla_FileHeader_t         header;
  atla_ObjectRefPool_t *    objectPool, *activePool, *sentinelPool;
  atbyte*                   ioBuffer;
  atsize_t                  ioBufferLimit, ioBufferUsed;
  atuint32                  nextObjectID;
  atuint32                  objectCount;
  atla_ObjectRef_t*         currentObj;
  atsize_t                  bytesWritten, bytesToWrite;
  atuint32                  status;
  struct {
    atla_ObjectLocation_t* objectList;
    atuint32               currentObject;
  } read;
  struct {
    atla_ObjectRef_t *objectList, *objectListEnd;
  } write;
};

atsize_t atla_GetObjectRefPoolRequiredSize(atuint32 max_objects);

int atla_InitializeAtlaContext(atla_Context_t* ctx, atla_GetTypeInfo fn1, atla_GetTypePointerOffset fn2);
int atla_AssignIOAccess(atla_Context_t* ctx, atla_ioaccess_t* io);
int atla_AssignIOBuffer(atla_Context_t* ctx, atbyte* io_buffer, atsize_t io_buffer_size);
int atla_AssignObjectRefPool(atla_Context_t* ctx, void* ptr, atsize_t size);

int atla_fwrite_begin(atla_Context_t* ctx);
int atla_fwrite_begin_object_list(atla_Context_t* ctx);
int atla_fwrite_add_object_to_list(atla_Context_t* ctx, atuint32 type_id, atsize_t count, void* ptr, atsize_t len, atla_ObjectRef_t** obj_out);
int atla_fwrite_end_object_list(atla_Context_t* ctx);
atla_ObjectRef_t const* atla_fwrite_find_object_ref_by_ptr(atla_Context_t* ctx, void* ptr);
int                     atla_fwrite_next_object(atla_Context_t* ctx, atla_ObjectLocation_t* oref);
/*
 * The following functions that take pointers must ensure that the pointer value lives within ptr+len > &v >= ptr
 */
int atla_fwrite_i8(atla_Context_t* ctx, atint8 val);
int atla_fwrite_i8p(atla_Context_t* ctx, atint8 const* val, atsize_t count);
int atla_fwrite_u8(atla_Context_t* ctx, atuint8 val);
int atla_fwrite_u8p(atla_Context_t* ctx, atuint8 const* val, atsize_t count);
int atla_fwrite_i16(atla_Context_t* ctx, atint16 val);
int atla_fwrite_i16p(atla_Context_t* ctx, atint16 const* val, atsize_t count);
int atla_fwrite_u16(atla_Context_t* ctx, atuint16 val);
int atla_fwrite_u16p(atla_Context_t* ctx, atuint16 const* val, atsize_t count);
int atla_fwrite_i(atla_Context_t* ctx, atint val);
int atla_fwrite_ip(atla_Context_t* ctx, atint const* val, atsize_t count);
int atla_fwrite_u(atla_Context_t* ctx, atuint val);
int atla_fwrite_up(atla_Context_t* ctx, atuint const* val, atsize_t count);
int atla_fwrite_i32(atla_Context_t* ctx, atint32 val);
int atla_fwrite_i32p(atla_Context_t* ctx, atint32 const* val, atsize_t count);
int atla_fwrite_u32(atla_Context_t* ctx, atuint32 val);
int atla_fwrite_u32p(atla_Context_t* ctx, atuint32 const* val, atsize_t count);
int atla_fwrite_i64(atla_Context_t* ctx, atint64 val);
int atla_fwrite_i64p(atla_Context_t* ctx, atint64 const* val, atsize_t count);
int atla_fwrite_u64(atla_Context_t* ctx, atuint64 val);
int atla_fwrite_u64p(atla_Context_t* ctx, atuint64 const* val, atsize_t count);
int atla_fwrite_f32(atla_Context_t* ctx, float val);
int atla_fwrite_f32p(atla_Context_t* ctx, float const* val, atsize_t count);
int atla_fwrite_f64(atla_Context_t* ctx, double val);
int atla_fwrite_f64p(atla_Context_t* ctx, double const* val, atsize_t count);
/*
 * When serializing a memory value outsize of the current object (possibly, note we can point to ourselves), a new object reference is needed
 */
int atla_fwrite_obj_pointer(atla_Context_t* ctx, atla_ObjectPointer_t optr);
int atla_fwrite_pointer_to_obj_pointer(atla_Context_t* ctx, void* ptr, atla_ObjectPointer_t* optr);
// int atla_file_ObjectWrite_skip(atla_Context_t* ctx, atla_Type type, atsize_t count);

int atla_fwrite_end_object(atla_Context_t* ctx);
int atla_fwrite_end(atla_Context_t* ctx);

int      atla_fread_begin(atla_Context_t* ctx);
atsize_t atla_fread_get_object_count(atla_Context_t* ctx);
// Assign memory for object locations found during read.
int atla_fread_assign_object_locations(atla_Context_t* ctx, atla_ObjectLocation_t* arr, atsize_t count);
// Validate that object pointers are not empty, read the object list and fill location array for reading.
int atla_fread_begin_objects(atla_Context_t* ctx);
// Begin reading an object from file. Returns the location data (type id, object id, array count, memory pointer).
int atla_fread_next_object(atla_Context_t* ctx, atla_ObjectLocation_t* ol);
int atla_fread_skip_obj(atla_Context_t* ctx);
// Clean up and data for reading. Releases resources assigned after atla_fread_begin() was called, allowing them to be freed.
int atla_fread_end(atla_Context_t* ctx);

int atla_fread_i8(atla_Context_t* ctx, atint8* val);
int atla_fread_i8p(atla_Context_t* ctx, atint8* val, atsize_t count);
int atla_fread_u8(atla_Context_t* ctx, atuint8* val);
int atla_fread_u8p(atla_Context_t* ctx, atuint8* val, atsize_t count);
int atla_fread_i16(atla_Context_t* ctx, atint16* val);
int atla_fread_i16p(atla_Context_t* ctx, atint16* val, atsize_t count);
int atla_fread_u16(atla_Context_t* ctx, atuint16* val);
int atla_fread_u16p(atla_Context_t* ctx, atuint16* val, atsize_t count);
int atla_fread_i(atla_Context_t* ctx, atint* val);
int atla_fread_ip(atla_Context_t* ctx, atint* val, atsize_t count);
int atla_fread_u(atla_Context_t* ctx, atuint* val);
int atla_fread_up(atla_Context_t* ctx, atuint* val, atsize_t count);
int atla_fread_i32(atla_Context_t* ctx, atint32* val);
int atla_fread_i32p(atla_Context_t* ctx, atint32* val, atsize_t count);
int atla_fread_u32(atla_Context_t* ctx, atuint32* val);
int atla_fread_u32p(atla_Context_t* ctx, atuint32* val, atsize_t count);
int atla_fread_i64(atla_Context_t* ctx, atint64* val);
int atla_fread_i64p(atla_Context_t* ctx, atint64* val, atsize_t count);
int atla_fread_u64(atla_Context_t* ctx, atuint64* val);
int atla_fread_u64p(atla_Context_t* ctx, atuint64* val, atsize_t count);
int atla_fread_f32(atla_Context_t* ctx, float* val);
int atla_fread_f32p(atla_Context_t* ctx, float* val, atsize_t count);
int atla_fread_f64(atla_Context_t* ctx, double* val);
int atla_fread_f64p(atla_Context_t* ctx, double* val, atsize_t count);

int   atla_fread_obj_pointer(atla_Context_t* ctx, atla_ObjectPointer_t* optr);
void* atla_fread_obj_pointer_to_pointer(atla_Context_t* ctx, atla_ObjectPointer_t optr);

#ifdef __cplusplus
} // extern "C"
#endif //
