/********************************************************************

        filename: 	atla_datadeftypes.h

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

#undef ATLA_ADD_TYPE_D
#undef ATLA_ADD_TYPE_DPTR
#undef ATLA_REM_TYPE_D
#undef ATLA_ADD_TYPE_S
#undef ATLA_ADD_TYPE_SPTR
#undef ATLA_REM_TYPE_S
#undef ATLA_BEGIN
#undef ATLA_END

#if ATLA_DATA_DEF
// "checking data def"

#define ATLA_ADD_TYPE_D(ver, type, field) type field;
#define ATLA_ADD_TYPE_DPTR(ver, type, field, count) type* field;
#define ATLA_REM_TYPE_D(vera, verr, type, field)
#define ATLA_ADD_TYPE_S(ver, type, field) type field;
#define ATLA_ADD_TYPE_SPTR(ver, type, field, coun) type* field;
#define ATLA_REM_TYPE_S(vera, verr, type, field)
#define ATLA_BEGIN(type) struct type {
#define ATLA_END(type)                                                         \
  }                                                                            \
  ;                                                                            \
  void atla_serialize_##type(atAtlaSerializer_t*, void*);

#elif ATLA_DATA_WRITE

// TODO: Static arrays. NOTE: Changing the size of a static array should bump
// the version #
// TODO: Object references i.e. pointers.
// TODO: Dynamic arrays of atomic types (int, flaot, char, etc).
// TODOL Dynamic arrays of non-atomic types (e.g. structs)

#define ATLAI_RW_TYPED(dptr, type, count)                                      \
  if (serializer->reading) {                                                   \
    atSerializeRead(serializer, &(dptr), sizeof(type), (count));               \
  } else {                                                                     \
    atSerializeWrite(serializer, &(dptr), sizeof(type), (count));              \
  }

#define ATLAI_RW_TYPES(dptr, type)                                             \
  extern void atla_serialize_##type(atAtlaSerializer_t*, void*);               \
  atla_serialize_##type(serializer, &(dptr));

#define ATLAI_SKIP_TYPED(type, count)                                          \
  if (serializer->reading) {                                                   \
    atSerializeSkip(serializer, sizeof(type), (count));                        \
  }

#define ATLA_ADD_TYPE_D(ver, type, field)                                      \
  if (serializer->version >= ver) {                                            \
    if (serializer->reading) {                                                 \
      atSerializeRead(serializer, &(ptr->field), sizeof(type), 1);             \
    } else {                                                                   \
      atSerializeWrite(serializer, &(ptr->field), sizeof(type), 1);            \
    }                                                                          \
  }
#define ATLA_ADD_TYPE_DPTR(ver, type, field, count)                            \
  if (serializer->version >= ver) {                                            \
    if (serializer->reading) {                                                 \
      uint32_t a;                                                              \
      atSerializeRead(serializer, &a, sizeof(a), 1);                           \
      ptr->field = (type*)atSerializeReadGetBlobLocation(serializer, a);       \
    } else {                                                                   \
      uint32_t id = atSerializeWritePendingBlob(                               \
        serializer, ptr->field, sizeof(*ptr->field), ptr->count);              \
      atSerializeWrite(serializer, &id, sizeof(id), 1);                        \
    }                                                                          \
  }

#define ATLA_REM_TYPE_D(vera, verr, type, field)                               \
  if (serializer->version >= (vera) && serializer->version < (verr)) {         \
    ATLAI_SKIP_TYPED(type, 1)                                                  \
  }

#define ATLA_ADD_TYPE_S(ver, type, field)                                      \
  if (serializer->version >= ver) {                                            \
    ATLAI_RW_TYPES(ptr->(field), type)                                         \
  }
#define ATLA_ADD_TYPE_SPTR(ver, type, field, count)                            \
  if (serializer->version >= ver) {                                            \
    extern void atla_serialize_##type(atAtlaSerializer_t*, void*);             \
    if (serializer->reading) {                                                 \
      uint32_t a;                                                              \
      atSerializeRead(serializer, &a, sizeof(a), 1);                           \
      ptr->field = (type*)atSerializeReadTypeLocation(                         \
        serializer, a, #type, atla_serialize_##type);                          \
    } else {                                                                   \
      uint32_t id = atSerializeWritePendingType(serializer,                    \
                                                ptr->field,                    \
                                                #type,                         \
                                                atla_serialize_##type,         \
                                                sizeof(type),                  \
                                                ptr->count);                   \
      atSerializeWrite(serializer, &id, sizeof(id), 1);                        \
    }                                                                          \
  }
#define ATLA_REM_TYPE_S(vera, verr, type, field)                               \
  if (serializer->version >= (vera) && serializer->version < (verr)) {         \
    type dummy;                                                                \
    ATLAI_RW_TYPES(dummy, type)                                                \
  }

#define ATLA_BEGIN(type)                                                       \
  void atla_serialize_##type(atAtlaSerializer_t* serializer, void* vptr) {     \
    extern char const* atla_##type##_typename();                               \
    char const*        type_name = atla_##type##_typename();                   \
    struct type*       ptr = (struct type*)vptr;                               \
    serializer->depth++;
#define ATLA_END(type)                                                         \
  if (serializer->depth == 1 && !serializer->reading)                          \
    atSerializeWriteProcessPending(serializer);                                \
  --serializer->depth;                                                         \
  }                                                                            \
  char const* atla_##type##_typename() { return #type; }                       \
  void atla_##type##_reg(atAtlaContext_t* c) {                                 \
    atContextRegisterType(c, atla_##type##_typename(), sizeof(type));          \
  }


// Undef these as they aren't needed outside of this header.
#undef ATLAI_RW_TYPED
#undef ATLAI_SKIP_TYPED

#endif
