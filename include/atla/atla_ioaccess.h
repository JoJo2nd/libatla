/********************************************************************

        filename: 	atla_iostream.h

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

#include "atla/atla_config.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif //

typedef enum atSeekOffset {
  eSeekOffset_Begin = SEEK_SET,
  eSeekOffset_Current = SEEK_CUR,
  eSeekOffset_End = SEEK_END
} atSeekOffset;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

typedef void(ATLA_CALLBACK* atIOReadProc)(void*    pBuffer,
                                          uint32_t size,
                                          void*    user);
typedef void(ATLA_CALLBACK* atIOWriteProc)(void const* pBuffer,
                                           uint32_t    size,
                                           void*       user);
typedef uint32_t(ATLA_CALLBACK* atIOSeekProc)(uint32_t     offset,
                                              atSeekOffset from,
                                              void*        user);
typedef int64_t(ATLA_CALLBACK* atIOTellProc)(void* user);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

typedef struct atioaccess {
  atIOReadProc  readProc;
  atIOWriteProc writeProc;
  atIOSeekProc  seekProc;
  atIOTellProc  tellProc;
  void*         user;
} atioaccess_t;
