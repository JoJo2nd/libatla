/********************************************************************

	filename: 	atla_unittest.c	
	
	Copyright (c) 25:10:2012 James Moran
	
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
#include "unittest_types.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

static atuint allocCount = 0;

void* ATLA_CALLBACK atMalloc(atsize_t size, void* user)
{
    ++allocCount;
    return malloc(size);
}

void  ATLA_CALLBACK atFree(void* ptr, void* user)
{
    if (ptr)
    {
        --allocCount;
    }
    free(ptr);
}

atuint32 ATLA_CALLBACK atIORead(void* pBuffer, atuint32 size, void* user)
{
    return fread(pBuffer, 1, size, user);
}

atuint32 ATLA_CALLBACK atIOWrite(const void* pBuffer, atuint32 size, void* user)
{
    return fwrite(pBuffer, 1, size, user);
}

atuint64 ATLA_CALLBACK atIOSeek(atuint64 offset, atSeekOffset from, void* user)
{
    return fseek(user, (long)offset, from);
}

atuint64 ATLA_CALLBACK atIOTell(void* user)
{
    return ftell(user);
}

typedef int (*unitTestProc)(atAtlaContext_t*);

int typelessDataTest(atAtlaContext_t* atla)
{
    atuint i;
    atAtlaDataBlob_t* atladb;
    atIOAccess_t ioaccess;
    atErrorCode ec;
    atuint d[100], d2[100];

    for (i = 0; i < 100; ++i)
    {
        d[i] = i;
    }

    ioaccess.readProc_ = atIORead;
    ioaccess.seekProc_ = atIOSeek;
    ioaccess.writeProc_ = atIOWrite;
    ioaccess.tellProc_ = atIOTell;
    ioaccess.user_ = fopen("atla_"__FUNCTION__".dat", "wb");

    atladb = atOpenAtlaDataBlob(atla, &ioaccess, ATLA_WRITE);
    if (!atladb) return -1;
    ec = atAddTypelessDataToBlob(atladb, "write_test", sizeof(atuint), sizeof d / sizeof *d, d);
    if (ec != ATLA_EOK) return -1;
    ec = atSerialiseDataBlob(atladb);
    if (ec != ATLA_EOK) return -1;
    atCloseAtlaDataBlob(atladb);
    fclose(ioaccess.user_);

    ioaccess.user_ = fopen("atla_"__FUNCTION__".dat", "rb");
    atladb = atOpenAtlaDataBlob(atla, &ioaccess, ATLA_READ);
    if (!atladb) return -1;
    ec = atDeserialiseDataByName(atladb, "write_test", &d2);
    if (ec != ATLA_EOK) return -1;
    if (memcmp(&d, &d2, sizeof(d)) != 0) return -1;
    atCloseAtlaDataBlob(atladb);

    return 0;
}

int bulkSchemaDataTest(atAtlaContext_t* atla)
{
    atAtlaDataBlob_t* atladb;
    atIOAccess_t ioaccess;
    atErrorCode ec;
    atuint i;
    SimplePOD_t a = {0}, a2 = {0};
    DoubleNestedPOD_t b = {0}, b2 = {0};
    NestedPOD_t c = {0}, c2 = {0};

    a.i64 = 64;
    a.d64 = 64.0;
    a.r32 = 32.f;
    a.i32 = 32;
    a.i16 = 16;
    a.i8 = 8;
    a.c8[0] = 81;
    a.c8[1] = 82;

    c.i32 = 32;
    c.r32 = 32.f;
    c.n1 = a;

    b.i32 = 32;
    b.n1 = c;
    b.n3 = a;
    for (i = 0; i < 8; ++i)
    {
        b.n2[i] = c;
    }


    ioaccess.readProc_ = atIORead;
    ioaccess.seekProc_ = atIOSeek;
    ioaccess.writeProc_ = atIOWrite;
    ioaccess.tellProc_ = atIOTell;
    ioaccess.user_ = fopen("atla_"__FUNCTION__".dat", "wb");

    atladb = atOpenAtlaDataBlob(atla, &ioaccess, ATLA_WRITE);
    if (!atladb) return -1;
    ec = atAddDataToBlob(atladb, "write_test_1", "SimplePOD_t", 1, &a);
    if (ec != ATLA_EOK) return -1;
    ec = atAddDataToBlob(atladb, "write_test_2", "DoubleNestedPOD_t", 1, &b);
    if (ec != ATLA_EOK) return -1;
    ec = atAddDataToBlob(atladb, "write_test_3", "NestedPOD_t", 1, &c);
    if (ec != ATLA_EOK) return -1;
    ec = atSerialiseDataBlob(atladb);
    if (ec != ATLA_EOK) return -1;
    atCloseAtlaDataBlob(atladb);
    fclose(ioaccess.user_);

    ioaccess.user_ = fopen("atla_"__FUNCTION__".dat", "rb");
    atladb = atOpenAtlaDataBlob(atla, &ioaccess, ATLA_READ);
    ec = atDeserialiseDataByName(atladb, "write_test_1", &a2);
    if (ec != ATLA_EOK) return -1;
    if (memcmp(&a, &a2, sizeof(a)) != 0) return -1;
    ec = atDeserialiseDataByName(atladb, "write_test_2", &b2);
    if (ec != ATLA_EOK) return -1;
    if (memcmp(&b, &b2, sizeof(b)) != 0) return -1;
    ec = atDeserialiseDataByName(atladb, "write_test_3", &c2);
    if (ec != ATLA_EOK) return -1;
    if (memcmp(&c, &c2, sizeof(c)) != 0) return -1;

    atCloseAtlaDataBlob(atladb);
    return 0;
}

struct {
    const char*     name;
    unitTestProc    func;
} unitTest[] = {
    {"typelessData", typelessDataTest},
    {"bulkSchemaData", bulkSchemaDataTest},
};

int main(int argc, char *argv[])
{
    atuint i;
    atAtlaContext_t* atla;
    atMemoryHandler_t atlamem;
    atErrorCode ec = ATLA_EOK;

    atlamem.memAlloc_ = atMalloc;
    atlamem.memFree_ = atFree;
    atlamem.memUser_ = NULL;

    atla = atCreateAtlaContext(&atlamem);
    ec = atAddDataSchema(atla, ATLA_GET_TYPE_SCHEMA_PTR(SimplePOD_t));
    assert(ec == ATLA_EOK);
    ec = atAddDataSchema(atla, ATLA_GET_TYPE_SCHEMA_PTR(NestedPOD_t));
    assert(ec == ATLA_EOK);
    ec = atAddDataSchema(atla, ATLA_GET_TYPE_SCHEMA_PTR(DoubleNestedPOD_t));
    assert(ec == ATLA_EOK);

    for (i = 0; i < sizeof(unitTest)/sizeof(unitTest[0]); ++i)
    {
        printf("unit test %s: %s\n", unitTest[i].name, unitTest[i].func(atla) == 0 ? "Passed" : "Failed");
    }

    ec = atDestroyAtlaContext(atla);
    assert(ec == ATLA_EOK);

    if (allocCount != 0)
    {
        printf("Memory leaked by ATLA");
    }

    return 0;
}