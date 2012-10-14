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
	
	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.
	
	3. This notice may not be removed or altered from any source
	distribution.

*********************************************************************/
#pragma once

#ifndef ATLA_TYPES_H__
#define ATLA_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif//

typedef size_t              atsize_t;
typedef unsigned long long  atuint64;
typedef signed long long    atint64;
typedef unsigned long       atuint32;
typedef signed long         atint32;
typedef unsigned int        atuint;
typedef signed int          atint;
typedef unsigned short      atuint16;
typedef signed short        atint16;
typedef signed char         atint8;
typedef unsigned char       atuint8;
typedef atuint8             atbyte;
typedef atint8              atchar;
typedef bool                atbool;


#ifdef __cplusplus
} //extern "C"
#endif//

#endif // ATLA_TYPES_H__