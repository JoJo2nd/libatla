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
    --allocCount;
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
    return fseek(user, offset, from);
}

atuint64 ATLA_CALLBACK atIOTell(void* user)
{
    return ftell(user);
}

typedef struct SimplePOD
{
    atuint64 i64;
    double   d64;
    float    r32;
    atuint32 i32;
    atuint16 i16;
    atuint8  i8;
    atchar   c8[2];
} SimplePOD_t;

ATLA_BEGIN_SCHEMA(SimplePOD_t, 7)
    ATLA_SCHEMA_ELEMENT(SimplePOD_t, i64)
    ATLA_SCHEMA_ELEMENT(SimplePOD_t, d64)
    ATLA_SCHEMA_ELEMENT(SimplePOD_t, r32)
    ATLA_SCHEMA_ELEMENT(SimplePOD_t, i32)
    ATLA_SCHEMA_ELEMENT(SimplePOD_t, i16)
    ATLA_SCHEMA_ELEMENT(SimplePOD_t, i8)
    ATLA_SCHEMA_ELEMENTA(SimplePOD_t, c8)
ATLA_END_SCHEMA(SimplePOD_t);

typedef struct NestedPOD
{
    float       r32;
    atuint32    i32;
    SimplePOD_t n1;
} NestedPOD_t;

ATLA_BEGIN_SCHEMA(NestedPOD_t, 3)
    ATLA_SCHEMA_ELEMENT(NestedPOD_t, r32)
    ATLA_SCHEMA_ELEMENT(NestedPOD_t, i32)
    ATLA_SCHEMA_ELEMENT_NESTED(NestedPOD_t, n1, SimplePOD_t)
ATLA_END_SCHEMA(NestedPOD_t);

typedef struct DoubleNestedPOD
{
    atuint          i32;
    NestedPOD_t     n1;
    NestedPOD_t     n2[8];
    SimplePOD_t     n3;
} DoubleNestedPOD_t;

ATLA_BEGIN_SCHEMA(DoubleNestedPOD_t, 4)
    ATLA_SCHEMA_ELEMENT(DoubleNestedPOD_t, i32)
    ATLA_SCHEMA_ELEMENT_NESTED(DoubleNestedPOD_t, n1, NestedPOD_t);
    ATLA_SCHEMA_ELEMENT_NESTEDA(DoubleNestedPOD_t, n2, NestedPOD_t);
    ATLA_SCHEMA_ELEMENT_NESTED(DoubleNestedPOD_t, n3, SimplePOD_t);
ATLA_END_SCHEMA(DoubleNestedPOD_t);


int main(int argc, char *argv[])
{
    atUUID_t id = {0};
    atuint i = 0;
    atAtlaContext_t* atla;
    atAtlaDataBlob_t* atladb;
    atMemoryHandler_t atlamem;
    atIOAccess_t ioaccess;
    atErrorCode ec = ATLA_EOK;
    SimplePOD_t a = {0};
    DoubleNestedPOD_t b = {0};
    NestedPOD_t c = {0};
    atuint d[100];

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

    for (i = 0; i < 100; ++i)
    {
        d[i] = i;
    }

    atlamem.memAlloc_ = atMalloc;
    atlamem.memFree_ = atFree;
    atlamem.memUser_ = NULL;

    ioaccess.readProc_ = atIORead;
    ioaccess.seekProc_ = atIOSeek;
    ioaccess.writeProc_ = atIOWrite;
    ioaccess.tellProc_ = atIOTell;
    ioaccess.user_ = fopen("atla_test.dat", "wb");

    atla = atCreateAtlaContext(&atlamem);
    ec = atAddDataSchema(atla, ATLA_GET_TYPE_SCHEMA_PTR(SimplePOD_t));
    assert(ec == ATLA_EOK);
    ec = atAddDataSchema(atla, ATLA_GET_TYPE_SCHEMA_PTR(NestedPOD_t));
    assert(ec == ATLA_EOK);
    ec = atAddDataSchema(atla, ATLA_GET_TYPE_SCHEMA_PTR(DoubleNestedPOD_t));
    assert(ec == ATLA_EOK);

    atladb = atOpenAtlaDataBlob(atla, &ioaccess, ATLA_WRITE);

    ec = atAddDataToBlob(atladb, "write_test_1", "SimplePOD_t", 1, &a);
    assert(ec == ATLA_EOK);
    ec = atAddDataToBlob(atladb, "write_test_2", "DoubleNestedPOD_t", 1, &b);
    assert(ec == ATLA_EOK);
    ec = atAddDataToBlob(atladb, "write_test_3", "NestedPOD_t", 1, &c);
    assert(ec == ATLA_EOK);
    ec = atAddTypelessDataToBlob(atladb, "write_test_4", sizeof(atuint), sizeof d / sizeof *d, d);
    assert(ec == ATLA_EOK);

    ec = atSerialiseDataBlob(atladb);
    assert(ec == ATLA_EOK);

    atCloseAtlaDataBlob(atladb);

    fclose(ioaccess.user_);

    ioaccess.user_ = fopen("atla_test.dat", "rb");

    atladb = atOpenAtlaDataBlob(atla, &ioaccess, ATLA_READ);


    atCloseAtlaDataBlob(atladb);

    ec = atDestroyAtlaContext(atla);
    assert(ec == ATLA_EOK);

    assert(allocCount == 0);

    return id & 0xFFFFFFFF;
}