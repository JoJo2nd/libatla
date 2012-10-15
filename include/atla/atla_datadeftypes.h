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

typedef void* ATLA_API (*atDataTypeAlloc)(atUUID_t /*typeID*/, void* /*user*/);

#endif // ATLA_DATADEFTYPES_H__