
libatla
=======

A small C library to serialise and de-serialise C PODs (and probably C++ classes) into a binary format.

# Usage
Currently, subject to change :)

Defining data
```C
// File: alldata.struct.inl
// Not required to all be here, but here for ease

enum BRversion { 
  vesrion_0, 
  vesrion_1, // Added extended character codes for font lookup
  vesrion_next
};

#define version_latest (version_next-1)

ATLA_BEGIN(chardata_t)
ATLA_ADD_TYPE_D(version_0, int, x)
ATLA_ADD_TYPE_D(version_0, int, y)
ATLA_ADD_TYPE_D(version_0, int, width)
ATLA_ADD_TYPE_D(version_0, int, height)
ATLA_ADD_TYPE_D(version_0, int, baselineoffset)
ATLA_ADD_TYPE_D(version_0, float, u0)
ATLA_ADD_TYPE_D(version_0, float, v0)
ATLA_ADD_TYPE_D(version_0, float, u1)
ATLA_ADD_TYPE_D(version_0, float, v1)
ATLA_END(chardata_t)

ATLA_BEGIN(fontdata_t)
ATLA_ADD_TYPE_D(version_0, uint32_t, fontSize);
ATLA_ADD_TYPE_D(version_0, uint32_t, charCount);
ATLA_ADD_TYPE_D(version_1, uint32_t, nCodepoints);
ATLA_ADD_TYPE_DPTR(version_0, uint32_t, codepoints, nCodepoints);
ATLA_ADD_TYPE_SPTR(version_0, chardata_t, metrics, charCount);
ATLA_ADD_TYPE_D(version_0, uint32_t, texDataLen);
ATLA_ADD_TYPE_DPTR(version_0, uint8_t, texData, texDataLen);
ATLA_END(fontdata_t)

ATLA_BEGIN(fontcollection_t)
ATLA_ADD_TYPE_D(version_zero, uint32_t, nFonts);
ATLA_ADD_TYPE_SPTR(version_zero, fontdata_t, fonts, nFonts);
ATLA_END(fontcollection_t)
```
Then when including 
```C
#include "atla/atla.h"
#include "atla/atla_type_begin.h"
#include "alldata.struct.inl"
#include "atla/atla_type_end.h"

// In a .c file
#include "atla/atla_def_begin.h"
#include "alldata.struct.inl"
#include "atla/atla_def_end.h"
```

To serialize out to disk
```C
void saveToDisk(char const* path) {
  fontcollection_t final_fcoll;
  /*
    ...
    Code to fill in final_fcol
    ...
  */
  atAtlaContext_t atla_context;
  atAtlaSerializer_t atla_ser;
  atMemoryHandler_t  mem = {.alloc = atla_malloc,
                           .ralloc = atla_realloc,
                           .free = atla_free,
                           .user = NULL};
  atioaccess_t io = {.readProc = atla_read,
                     .writeProc = atla_write,
                     .seekProc = atla_seek,
                     .tellProc = atla_tell,
                     .user = fopen(path, "wb")};
  atCreateAtlaContext(&atla_context, &mem);
  atSerializeWriteBegin(&atla_ser, &atla_context, "user_tag", &io, version_latest);
  atSerializeWriteRoot(
    &atla_ser, &final_fcoll, atla_serialize_fontcollection_t);
  atSerializeWriteFinalize(&atla_ser);
  fclose(io.user);
}
```
To serialize from disk
```C
void loadFromDisk(char const* filepath) 
{
  atAtlaSerializer_t atla_ser;
  atMemoryHandler_t  mem = {.alloc = atla_malloc,
                           .ralloc = atla_realloc,
                           .free = atla_free,
                           .user = NULL};
  atioaccess_t io = {.readProc = atla_read,
                     .writeProc = atla_write,
                     .seekProc = atla_seek,
                     .tellProc = atla_tell,
                     .user = fopen(path, "wb")};
  io.user = fopen(filepath, "rb");
  atCreateAtlaContext(&atla_context, &mem);
  // Custom function to register all types
  // More on this later...
  register_all_types(&atla_ctx);
  atSerializeReadBegin(&atla_ser, globalAtlaContext(), &io, version_latest);
  uint32_t total_mem = atUtilCalcReadMemRequirements(&atla_ser);
  font_collection = (fontcollection_t*)malloc(total_mem + sizeof(fontcollection_t));
  int mem_fixup_ok = atUtilAssignReadMem(&atla_ser, font_collection+1, total_mem);
  atSerializeReadRoot(&atla_ser, font_collection, atla_serialize_fontcollection_t);
  atSerializeReadFinalize(&atla_ser);
  fclose((FILE*)io.user);
 }
```