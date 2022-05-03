
libatla
=======

A small C library to serialise and de-serialise C PODs into a binary format.

# Usage
Currently all subject to change :)

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