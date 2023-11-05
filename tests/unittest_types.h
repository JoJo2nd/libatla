/********************************************************************

	filename: 	unittest_types.h	
	
	Copyright (c) 4:11:2012 James Moran
	
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

#pragma once

#ifndef UNITTEST_TYPES_H__
#define UNITTEST_TYPES_H__

#include "atla/atla.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
} //extern "C"
#endif

#endif // UNITTEST_TYPES_H__