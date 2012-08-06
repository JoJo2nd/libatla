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
	
	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.
	
	3. This notice may not be removed or altered from any source
	distribution.

*********************************************************************/

#ifndef ATLA_IOSTREAM_H__
#define ATLA_IOSTREAM_H__

namespace atla
{
    enum atSeekOffset
    {
        eSeekOffset_Begin,
        eSeekOffset_Current,
        eSeekOffset_End
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    class atIOStream
    {
    public:
        virtual atuint32    		read( void* pBuffer, atuint32 size ) = 0;
        virtual atuint32			write( const void* pBuffer, atuint32 size ) = 0;
        virtual atuint32    		seek( atuint32 offset, atSeekOffset offset ) = 0;
        virtual atuint32    		tell() = 0;
    };
}

#endif // ATLA_IOSTREAM_H__