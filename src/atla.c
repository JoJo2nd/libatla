/********************************************************************

	filename: 	atla.c	
	
	Copyright (c) 17:10:2012 James Moran
	
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

#include "atla/atla_config.h"
#include "atla/atla_md5.h"
#include <string.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ATLA_VERSION_MAJOR    (0)
#define ATLA_VERSION_MINOR    (1)
#define ATLA_VERSION_REV      (1)

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atuint64 ATLA_API atGetAtlaVersion()
{
    return (((atuint64)(ATLA_VERSION_MAJOR & 0xFFFF) << 48) | ((atuint64)(ATLA_VERSION_MAJOR & 0xFFFF) << 32) | ATLA_VERSION_REV);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atUUID_t ATLA_API atBuildAtlaStringUUID(const atchar* str, atuint32 strlen)
{
    atUUID_t uid = 0;
    at_md5_ctxt md5 = {0};

    atInitMD5(&md5);
    atLoopMD5(&md5, str, strlen);
    atPadMD5(&md5);
    atResultMD5ToUUID(&uid, &md5);

    return uid;
}

