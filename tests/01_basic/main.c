
#include "atla/atla.h"

#include <memory.h>
#include <assert.h>

typedef
struct mem_io {
    size_t reserve;
    size_t tell;
    char* buffer;
} mem_io_t;

void mem_io_write(void const* src, atsize_t size, void* user) {
    mem_io_t* f = user;
    if (f->tell + size >= f->reserve) {
        f->reserve = ((f->reserve + size) + 4095) & ~4095;
        f->buffer = realloc(f->buffer, f->reserve);
    }
    memcpy(f->buffer+f->tell, src, size);
    f->tell += size;
}

void mem_io_read(void* dst, atsize_t size, void* user) {
    mem_io_t* f = user;
    size_t to_read = min(f->reserve - f->tell, size);
    memcpy(dst, f->buffer+f->tell, to_read);
    f->tell += size;
    //return to_read;
}

enum { DataType_TypeID = atla_type_custom_type_id_first };

typedef 
struct DataTypeV1 {
    float       vec[3];
    int         health;
    uint8_t     strenght;
    uint8_t     defence;
    uint8_t     luck;
} DataTypeV1;
static size_t DataTypeV1_bytecount = sizeof(float)*3 + sizeof(int) + sizeof(uint8_t);

typedef 
struct DataTypeV2 {
    float       vec[3];
    int         health;
    uint8_t     strenght;
    uint8_t     defence;
    uint8_t     magic;
    uint8_t     luck;
    uint8_t     charm;
} DataTypeV2;

typedef 
struct DataTypeV3 {
    float       vec[3];
    int         health;
    uint8_t     strenght;
    uint8_t     defence;
    uint8_t     magic;
    uint8_t     charm;
} DataTypeV3;

void LoadDataTypeV1(atla_Context_t* ctx, int ver, DataTypeV1* data) {
    atla_fread_f32p(ctx, data->vec, 3);
    atla_fread_i(ctx, &data->health);
    atla_fread_u8(ctx, &data->strenght);
    atla_fread_i8(ctx, &data->defence);
    atla_fread_i8(ctx, &data->luck);
}
void LoadDataTypeV2(atla_Context_t* ctx, int ver, DataTypeV2* data) {

}
void LoadDataTypeV3(atla_Context_t* ctx, int ver, DataTypeV3* data) {

}

void WriteDataTypeV1(atla_Context_t* ctx, int ver, DataTypeV1 const* data) {
    atla_fwrite_f32p(ctx, data->vec, 3);
    atla_fwrite_i(ctx, data->health);
    atla_fwrite_u8(ctx, data->strenght);
    atla_fwrite_i8(ctx, data->defence);
    atla_fwrite_i8(ctx, data->luck);
}
void WriteDataTypeV2(atla_Context_t* ctx, int ver, DataTypeV2 const* data) {
    atla_fwrite_f32p(ctx, data->vec, 3);
    atla_fwrite_i(ctx, data->health);
    atla_fwrite_u8(ctx, data->strenght);
    atla_fwrite_i8(ctx, data->defence);
    atla_fwrite_i8(ctx, data->magic);
    atla_fwrite_i8(ctx, data->luck);
    atla_fwrite_i8(ctx, data->charm);
}
void WriteDataTypeV3(atla_Context_t* ctx, int ver, DataTypeV3 const* data) {
    atla_fwrite_f32p(ctx, data->vec, 3);
    atla_fwrite_i(ctx, data->health);
    atla_fwrite_u8(ctx, data->strenght);
    atla_fwrite_i8(ctx, data->defence);
    atla_fwrite_i8(ctx, data->magic);
    atla_fwrite_i8(ctx, data->charm);
}

void* AllocateTypeStorage(int typeid, atsize_t count) {
    switch (typeid) {
    case DataType_TypeID: return malloc(sizeof(DataTypeV1)*count);
    default: return NULL;
    }
}

int GetTypeInfo(atuint32  type_id,
                atsize_t* runtimesize,
                atuint32* version,
                atsize_t* disksize) {
    switch (type_id) {
    case DataType_TypeID: 
        *runtimesize = sizeof(DataTypeV1);
        *version = 1;
        *disksize = DataTypeV1_bytecount;
        break;
    default: return ATLA_EC_TYPE_NOT_KNOWN;
    }
    return ATLA_EOK;
}
void* GetTypePointerOffset(void* base, int type_id, int member_id) {
    return (void*)((uintptr_t)base + 0);
}

int main(int argc, char** argv) {
    mem_io_t out_data = {0};
    static const int atla_io_scratch_buffer_len = 4096;
    void* atla_io_scratch_buffer = malloc(atla_io_scratch_buffer_len);
    size_t atla_object_pool_size = atla_GetObjectRefPoolRequiredSize(1024);
    void* atla_object_pool = malloc(atla_object_pool_size);
    atla_ioaccess_t io = {
        .read = mem_io_read,
        .write = mem_io_write,
        .user = &out_data
    };
    atla_Context_t actx;
    atla_InitializeAtlaContext(&actx, GetTypeInfo, GetTypePointerOffset);
    atla_AssignIOAccess(&actx, &io);
    atla_AssignObjectRefPool(&actx, atla_object_pool, atla_object_pool_size);
    atla_AssignIOBuffer(&actx, atla_io_scratch_buffer, atla_io_scratch_buffer_len);

    DataTypeV1 v1out, v1in;
    atla_ObjectRef_t* v1_or;
    memset(&v1out, 0, sizeof(v1out));
    memset(&v1in, 0, sizeof(v1in));
    v1out.vec[0] = 42.24f;
    v1out.vec[1] = 3.1415f;
    v1out.vec[2] = 8335.04321f;
    v1out.health = 100;
    v1out.strenght = 53;
    v1out.defence = 48;
    v1out.luck = 24;

    atla_fwrite_begin(&actx);
    atla_fwrite_begin_object_list(&actx);
    atla_fwrite_add_object_to_list(&actx, DataType_TypeID, 1, &v1out, sizeof(v1out), &v1_or);
    atla_fwrite_end_object_list(&actx);
    {
        atla_ObjectLocation_t cur_obj;
        while (atla_fwrite_next_object(&actx, &cur_obj) == ATLA_EOK) {
          switch (cur_obj.typeID) {
          case DataType_TypeID: {
            WriteDataTypeV1(&actx, cur_obj.version, (DataTypeV1*)cur_obj.dataPtr);
          } break;
          default: {
            assert(0);
          } break;
          }
        }
    }
    atla_fwrite_end_object(&actx);
    atla_fwrite_end(&actx);

    mem_io_t in_data = {
        .reserve = out_data.tell,
        .tell = 0,
        .buffer = out_data.buffer
    };
    io.user = &in_data;
    atla_AssignIOAccess(&actx, &io);
    atla_fread_begin(&actx);
    atsize_t object_count = atla_fread_get_object_count(&actx);
    atla_ObjectLocation_t* objects = malloc(object_count * sizeof(atla_ObjectLocation_t));
    atla_fread_assign_object_locations(&actx, objects, object_count);
    assert(object_count == 1);
    for (atsize_t i=0; i < object_count; ++i) {
        assert(objects[i].typeID == DataType_TypeID);
        objects[i].dataPtr = &v1in;
        //AllocateTypeStorage(objects[i].typeID, objects[i].count);
    }
    atla_fread_begin_objects(&actx);
    atla_ObjectLocation_t cur_obj;
    while (atla_fread_next_object(&actx, &cur_obj) == ATLA_EOK) {
        switch(cur_obj.typeID) {
        case DataType_TypeID: {
            LoadDataTypeV1(&actx, cur_obj.version, (DataTypeV1*)cur_obj.dataPtr);
        } break;
        default: {
            atla_fread_skip_obj(&actx);
        } break;
        }
    }
    atla_fread_end(&actx);

    assert(memcmp(&v1out, &v1in, sizeof(DataTypeV1)) == 0);

    return 0;
}