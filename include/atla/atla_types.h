/********************************************************************

        filename: 	atla_types.h

        Copyright (c) 6:8:2012 James Moran

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
#pragma once

#include <stdlib.h>
#include <stdint.h>

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
typedef atuint8            atbyte;
typedef char               atchar;
typedef atuint8            atbool;
typedef atuint64           atUUID_t;

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ATLA_INVALIDPOINTERREF (~0U)

//////////////////////////////////////////////////////////////////////////
// error codes ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

typedef enum atErrorCode {
  ATLA_EOK = 0,
  ATLA_NOMEM = -1,
  ATLA_BADFILE = -2,
  ATLA_DUPLICATE_DATA = -3,
  ATLA_NOTSUPPORTED = -4,
  ATLA_EPOINTERNOTFOUND = -5,
  ATLA_EBADSCHEMAID = -6,
  ATLA_ENESTEDSCHEMANOTFOUND = -7,
  ATLA_ETYPEMISMATCH = -8,
  ATLA_ENOTFOUND = -9,
  ATLA_EOUTOFRANGE = -10
} atErrorCode;
