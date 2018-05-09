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

#if defined ATLA_DATA_DEF

#   define ATLA_ADD_TYPE_D(ver, type, field) type field ;
#   define ATLA_REM_TYPE_D(vera, verr, type, field) 

#   define ATLA_ADD_TYPE_S(ver, type, field) type field ;
#   define ATLA_REM_TYPE_S(vera, verr, type, field)

#else defined ATLA_DATA_WRITE

// TODO: Static arrays. NOTE: Changing the size of a static array should bump the version #
// TODO: Object references i.e. pointers.
// TODO: Dynamic arrays of atomic types (int, flaot, char, etc).
// TODOL Dynamic arrays of non-atomic types (e.g. structs)

#   define ATLAI_RW_TYPED(dptr, type, count) \
    if (serializer->reading) {\
        atSerializeRead(serializer, &(dptr), sizeof(type), (count)); \
    } else { \
        atSerializeWrite(serializer, &(dptr), sizeof(type), (count));\
    }

#   define ATLAI_RW_TYPES(dptr, type) \
    extern void atla_serialize_##type (atAtlaSerializer_t*, type*); \
    atla_serialize_##type (serializer, &(dptr));

#   define ATLAI_SKIP_TYPED(type, count) \
    if (serializer->reading) {\
        atSerializeSkip(serializer, sizeof(type), (count)); \
    } 

#   define ATLA_ADD_TYPE_D(ver, type, field) \
    if (serializer->version >= ptr->(field)) {\
        ATLAI_RW_TYPED(ptr->(field), type, 1) \
    }
#   define ATLA_REM_TYPE_D(vera, verr, type, field) \
    if (serializer->version >= (vera) && serializer->version < (verr)) { \
        ATLAI_SKIP_TYPED(type, 1) \
    }

#   define ATLA_ADD_TYPE_S(ver, type, field) \
    if (serializer->version >= ptr->(field)) {\
        ATLAI_RW_TYPES(ptr->(field), type) \
    }
#   define ATLA_REM_TYPE_S(vera, verr, type, field) \
    if (serializer->version >= (vera) && serializer->version < (verr)) { \
        type dummy; \
        ATLAI_RW_TYPES(dummy, type) \
    }

// Undef these as they aren't needed outside of this header.
#undef ATLAI_RW_TYPED
#undef ATLAI_SKIP_TYPED

#else
#   error Unexpected mode
#endif
