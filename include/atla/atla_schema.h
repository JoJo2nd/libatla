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
	
	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.
	
	3. This notice may not be removed or altered from any source
	distribution.

*********************************************************************/

#pragma once

#ifndef ATLA_DATADEFTYPES_H__
#define ATLA_DATADEFTYPES_H__

#include "atla/atla_config.h"
#include "atla/atla_utils.h"
#include "atla/atla_debug.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif//

/* For future versions? */
typedef void* (ATLA_CALLBACK *atDataTypeAllocProc)(atUUID_t /*typeID*/, void* /*user*/);
typedef void* (ATLA_CALLBACK *atDataTypeDefaultProc)(atUUID_t /*typeID*/, void* /*dst*/, void* /*user*/);

typedef struct atDataDesc
{
    atUUID_t        typeID_;
    atUUID_t        dataID_;
    atuint          count_;
} atDataDesc_t;

typedef struct atSchemaElement
{
    atUUID_t                id_;
    atchar*                 name_;
    atuint32                size_;
    atuint32                offset_;
    atuint32                arrayCount_;
    atUUID_t                nestedID_;
} atSchemaElement_t;

typedef struct atDataSchema
{
    ATLA_LINKED_LIST_HEADER(atDataSchema);
    atUUID_t                id_;
    atchar*                 name_;
    atuint32                typeSize_;
    atuint32                serialisedSize_;
    atuint32                elementCount_;
    atSchemaElement_t*      elementArray_;
} atDataSchema_t;


#define ATLA_ELESIZE(y, x) (sizeof(((y*)0)->x))
#define ATLA_ELEOFFSET(y, x) ((atuint32)&(((y*)0)->x))
#define ATLA_CALC_COUNT(y, x) ((sizeof(((y*)0)->x))/(sizeof(((((y*)0)->x))[0])))
#define ATLA_ELESIZEA(y, x) (sizeof(((y*)0)->x[0]))

#define ATLA_INIT_SCHEMA(stype)\
    schema_##stype.id_ =  atBuildAtlaStringUUID(#stype,strlen(#stype));\
    schema_##stype.name_ = #stype;\
    schema_##stype.typeSize_ = sizeof(stype);\
    schema_##stype.serialisedSize_ = 0;\

#define ATLA_INIT_SCHEMA_ELEMENT(ele, stype, ename)\
    elements_##stype[ele].id_ = atBuildAtlaStringUUID(#ename,strlen(#ename));\
    elements_##stype[ele].name_ = #ename;\
    elements_##stype[ele].size_ = ATLA_ELESIZE(stype,ename);\
    elements_##stype[ele].offset_ = ATLA_ELEOFFSET(stype,ename);\
    elements_##stype[ele].arrayCount_ = 1;\
    elements_##stype[ele].nestedID_ = 0;\
    atAssert(ele < (sizeof(elements_##stype)/sizeof(elements_##stype[0]))); ++ele;

#define ATLA_INIT_SCHEMA_ELEMENTA(ele, stype, ename)\
    elements_##stype[ele].id_ = atBuildAtlaStringUUID(#ename,strlen(#ename));\
    elements_##stype[ele].name_ = #ename;\
    elements_##stype[ele].size_ = ATLA_ELESIZEA(stype,ename);\
    elements_##stype[ele].offset_ = ATLA_ELEOFFSET(stype,ename);\
    elements_##stype[ele].arrayCount_ = ATLA_CALC_COUNT(stype, ename);\
    elements_##stype[ele].nestedID_ = 0;\
    atAssert(ele < (sizeof(elements_##stype)/sizeof(elements_##stype[0]))); ++ele;

#define ATLA_INIT_SCHEMA_ELEMENT_NESTED(ele, stype, ename, nname)\
    elements_##stype[ele].id_ = atBuildAtlaStringUUID(#ename,strlen(#ename));\
    elements_##stype[ele].name_ = #ename;\
    elements_##stype[ele].size_ = ATLA_ELESIZE(stype,ename);\
    elements_##stype[ele].offset_ = ATLA_ELEOFFSET(stype,ename);\
    elements_##stype[ele].arrayCount_ = 1;\
    elements_##stype[ele].nestedID_ = atBuildAtlaStringUUID(#nname,strlen(#nname));\
    atAssert(ele < (sizeof(elements_##stype)/sizeof(elements_##stype[0]))); ++ele;

#define ATLA_INIT_SCHEMA_ELEMENT_NESTEDA(ele, stype, ename, nname)\
    elements_##stype[ele].id_ = atBuildAtlaStringUUID(#ename,strlen(#ename));\
    elements_##stype[ele].name_ = #ename;\
    elements_##stype[ele].size_ = ATLA_ELESIZEA(stype,ename);\
    elements_##stype[ele].offset_ = ATLA_ELEOFFSET(stype,ename);\
    elements_##stype[ele].arrayCount_ = ATLA_CALC_COUNT(stype, ename);\
    elements_##stype[ele].nestedID_ = atBuildAtlaStringUUID(#nname,strlen(#nname));\
    atAssert(ele < (sizeof(elements_##stype)/sizeof(elements_##stype[0]))); ++ele;

#define ATLA_DEFINE_SCHEMA_FUNC_BEGIN(stype, elementsCount)\
    atSchemaElement_t elements_##stype [elementsCount] = {0};\
    atDataSchema_t schema_##stype = {NULL, NULL,\
        {0},\
        #stype,sizeof(stype),0,\
        elementsCount,\
        elements_##stype\
    };\
    atDataSchema_t* atla_helper_get_type_schema_##stype() {\
        static atuint first_time = 0;\
        int i = 0;\
        if (first_time == 1) return &schema_##stype;\
        ATLA_INIT_SCHEMA(stype);

#define ATLA_DEFINE_SCHEMA_FUNC_END(stype) \
    first_time = 1;\
    return &schema_##stype; }
    

#ifdef __cplusplus
} //extern "C"
#endif//

#ifdef __cplusplus

#   define ATLA_BEGIN_SCHEMA(sname, elementCount) \
    atSchemaElement_t elements_##sname [elementCount] = {

#   define ATLA_SCHEMA_ELEMENT(stype, ename) \
    {atBuildAtlaStringUUID(##sname, sizeof(##sname)), #ename, ATLA_ELESIZE(stype,ename), ATLA_ELEOFFSET(stype,ename), 1, {0}},

#   define ATLA_SCHEMA_ELEMENTA(stype, ename) \
    {atBuildAtlaStringUUID(##sname, sizeof(##sname)), #ename, ATLA_ELESIZE(stype,ename), ATLA_ELEOFFSET(stype,ename), ATLA_CALC_COUNT(stype, ename), {0}},

#   define ATLA_SCHEMA_ELEMENT_NESTED(stype, ename) \
    {atBuildAtlaStringUUID(##sname, sizeof(##sname)), #ename, ATLA_ELESIZE(stype,ename), ATLA_ELEOFFSET(stype,ename), ATLA_CALC_COUNT(stype, ename), {0}},

#   define ATLA_CPP_END_SCHEMA(sname) };\
    atDataSchema_t schema_##sname = {NULL, NULL,\
        atBuildAtlaStringUUID(##sname, sizeof(##sname)),\
        #sname,sizeof(sname),0,\
        (sizeof(elements_##sname)/sizeof(elements_##sname[0])),\
        elements_##sname\
    }

#define ATLA_GET_TYPE_SCHEMA_PTR(stype) &schema_##sname;

#else 

#   define ATLA_BEGIN_SCHEMA(sname, elementCount)               ATLA_DEFINE_SCHEMA_FUNC_BEGIN(sname, elementCount)
#   define ATLA_SCHEMA_ELEMENT(stype, ename)                    ATLA_INIT_SCHEMA_ELEMENT(i, stype, ename)
#   define ATLA_SCHEMA_ELEMENTA(stype, ename)                   ATLA_INIT_SCHEMA_ELEMENTA(i, stype, ename)
#   define ATLA_SCHEMA_ELEMENT_NESTED(stype, ename, nname)      ATLA_INIT_SCHEMA_ELEMENT_NESTED(i, stype, ename, nname)
#   define ATLA_SCHEMA_ELEMENT_NESTEDA(stype, ename, nname)     ATLA_INIT_SCHEMA_ELEMENT_NESTEDA(i, stype, ename, nname)
#   define ATLA_END_SCHEMA(sname)                               ATLA_DEFINE_SCHEMA_FUNC_END(sname)
#   define ATLA_GET_TYPE_SCHEMA_PTR(stype)                      atla_helper_get_type_schema_##stype()

#endif


#endif // ATLA_DATADEFTYPES_H__
