/********************************************************************

	filename: 	atla_serialiser.h	
	
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

#ifndef ATLA_SERIALISER_H__
#define ATLA_SERIALISER_H__

#include "atla_config.h"
#include "atla_debug.h"

#define CREATE_SERIALISE_CLASS( x ) \
    typedef struct IDGetter_local \
    { \
        static hUint32 GetCRCID() { static hUint32 id_ = Heart::hCRC32::StringCRC( #x ); return id_; } \
    } IDGetter;

#define SERIALISE_ELEMENT( x ) \
    {\
        CREATE_SERIALISE_CLASS( x );\
        ser->WriteData< IDGetter >( x );\
    }

#define SERIALISE_ELEMENT_ENUM_AS_INT( x ) \
    {\
        CREATE_SERIALISE_CLASS( x );\
        ser->WriteData< IDGetter >( (hUint32)x );\
    }

#define SERIALISE_ELEMENT_PTR_AS_INT( x ) \
    {\
        CREATE_SERIALISE_CLASS( x );\
        ser->WriteData< IDGetter >( (hUint32)x );\
    }

#define SERIALISE_ELEMENT_RESOURCE_CRC( x ) \
    {\
        ser->SetEnableMD5FileGen( hFalse );\
        CREATE_SERIALISE_CLASS( x );\
        ser->WriteData< IDGetter >( (hUint32)x );\
        ser->SetEnableMD5FileGen( hTrue );\
    }

#define SERIALISE_ELEMENT_COUNT( x, count ) \
    {\
        CREATE_SERIALISE_CLASS( x );\
        ser->WriteData< IDGetter >( x, count );\
    }

#define DESERIALISE_ELEMENT( x ) \
    {\
        CREATE_SERIALISE_CLASS( x );\
        ser->ReadData< IDGetter >( x );\
    }

#define DESERIALISE_ELEMENT_INT_AS_ENUM( x ) \
    {\
        CREATE_SERIALISE_CLASS( x );\
        ser->ReadData< IDGetter >( *((hUint32*)& x) );\
    }

#define DESERIALISE_ELEMENT_INT_AS_PTR( x ) \
    {\
        CREATE_SERIALISE_CLASS( x );\
        ser->ReadData< IDGetter >( *((hUint32*)& x) );\
    }

#define DESERIALISE_ELEMENT_RESOURCE_CRC( x ) \
    {\
        CREATE_SERIALISE_CLASS( x );\
        ser->ReadData< IDGetter >( *((hUint32*)& x) );\
    }

#define HEART_ALLOW_SERIALISE_FRIEND() \
    template< typename _Ty > \
    friend void SerialiseMethod( Heart::hSerialiser*, const _Ty& ); \
    template< typename _Ty > \
    friend void DeserialiseMethod( hSerialiser*, _Ty& ); \

namespace Heart
{
    class hSerialiser;

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    template< typename _Ty >
    void SerialiseMethod( Heart::hSerialiser* ser, const _Ty& data )
    {
        atCompileTimeAssert( false, "Serialise Method not defined for type" );
    }

    template< typename _Ty >
    void DeserialiseMethod( Heart::hSerialiser* ser, _Ty& data )
    {
        atCompileTimeAssert( false, "Deserialise Method not defined for type" );
    }



    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

#pragma pack ( push, 1 )

    struct hSerialisedElementHeader
    {
        enum Info
        {
            Flag_Ptr        = 1 << 0,
            Flag_Array      = 1 << 1,
            Flag_Mask       = Flag_Ptr | Flag_Array,

            Type_1byte  = 1 << 2,
            Type_2byte  = 1 << 3,
            Type_4byte  = 1 << 4,
            Type_8byte  = 1 << 5,
            Type_User   = 1 << 6,
            Type_Mask   = Type_1byte | Type_2byte | Type_4byte | Type_8byte | Type_User,
        };
        hUint32    count_;
        hUint32    crcID_;
        hByte      typeID_;
        hByte      flags_; 
    };

#pragma pack ( pop )

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    class hSerialiser
    {
    public:

        hSerialiser() 
            : file_(NULL)
        {
        }
        ~hSerialiser()
        {
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        
        void SetEnableMD5FileGen( hBool val ) { file_->SetEnableMD5Gen( val ); }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        template< typename _Ty >
        void Serialise( hISerialiseStream* filehandle, const _Ty& data )
        {
            hcAssert( filehandle );
            file_ = filehandle;

            CREATE_SERIALISE_CLASS( root_item );
            file_->Seek( 0 );

            WriteData< IDGetter >( data );
        }

        template< typename _Ty >
        void Deserialise( hISerialiseStream* filehandle, _Ty& outdata )
        {
            hcAssert( filehandle );
            file_ = filehandle;

            CREATE_SERIALISE_CLASS( root_item );
            file_->Seek( 0 );

            ReadData< IDGetter >( outdata );
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        
        void WriteUserTypeInfo( hUint32 crcid, hUint32 count, hByte flags )
        {
            hSerialisedElementHeader header;
            header.typeID_ = hSerialisedElementHeader::Type_User;
            header.crcID_ = crcid;
            header.flags_ = hSerialisedElementHeader::Flag_Mask & flags;
            header.count_ = count;

            file_->Write( &header, sizeof(header) );
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        template< typename _MetaTy, typename _Ty >
        void WriteData( const _Ty& data )
        {
            typedef hSerialiserElementWorker< hIsArray< _Ty >::RawType > WorkerType;

            WorkerType::DoSerialise< hIsArray< _Ty >, _MetaTy >( this, data );
        }

        template< typename _MetaTy, typename _Ty >
        void WriteData( const _Ty* data, hUint32 count )
        {
            typedef hSerialiserElementWorker< hIsArray< _Ty* >::RawType > WorkerType;

            WorkerType::DoSerialise< hIsArray< _Ty* >, _MetaTy >( this, data, count );
        }

        template< typename _MetaTy, typename _Ty >
        void ReadData( _Ty& data )
        {
            typedef hSerialiserElementWorker< hIsArray< _Ty >::RawType > WorkerType;
            typedef _MetaTy NameInfo;

            hSerialisedElementHeader header = FindElement( NameInfo::GetCRCID() );
            //Until versions come along
            hcAssert( header.crcID_ == NameInfo::GetCRCID() );
            hcAssert( header.count_ == hIsArray< _Ty >::ElementCount );

            WorkerType::DoDeserialise< hIsArray< _Ty >, _MetaTy >( this, data );
        }

        template< typename _MetaTy, typename _Ty >
        void ReadData( _Ty*& data )
        {
            typedef hSerialiserElementWorker< hIsArray< _Ty* >::RawType > WorkerType;
            typedef _MetaTy NameInfo;

            hSerialisedElementHeader header = FindElement( NameInfo::GetCRCID() );
            //Until versions come along
            hcAssert( header.crcID_ == NameInfo::GetCRCID() );
            hcAssert( header.flags_ & hSerialisedElementHeader::Flag_Ptr );

            switch ( header.count_ )
            {
            case 0:
                data = NULL;
                return;
            case 1:
                data = hNEW(GetGlobalHeap(), _Ty);//need placement_new( malloc_func() )
                break;
            default:
                data = hNEW_ARRAY(GetGlobalHeap(), _Ty, header.count_);//need placement_new( malloc_func() )
                break;
            }

            WorkerType::DoDeserialise< hIsArray< _Ty* >, _MetaTy >( this, data, header.count_ );
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        template< typename _Ty >
        void Write( const _Ty& data, hUint32 count, hUint32 crcid, hByte type )
        {
            hSerialisedElementHeader header;
            header.typeID_ = hSerialisedElementHeader::Type_Mask & type;
            header.crcID_ = crcid;
            header.flags_ = hSerialisedElementHeader::Flag_Mask & type;
            header.count_ = count;

            file_->Write( &header, sizeof(header) );
            file_->Write( &data, sizeof(_Ty)*count );
        }

        template< typename _Ty >
        void Write( const _Ty* data, hUint32 count, hUint32 crcid, hByte type )
        {
            hSerialisedElementHeader header;
            header.typeID_ = hSerialisedElementHeader::Type_Mask & type;
            header.crcID_ = crcid;
            header.flags_ = hSerialisedElementHeader::Flag_Mask & type;
            header.count_ = count;

            file_->Write( &header, sizeof(header) );
            file_->Write( data, sizeof(_Ty)*count );
        }

        template< typename _Ty >
        void Read( _Ty& odata, hUint32 count )
        {
            file_->Read( &odata, sizeof(_Ty)*count );
        }

        template< typename _Ty >
        void Read( _Ty* odata, hUint32 count )
        {
            file_->Read( odata, sizeof(_Ty)*count );
        }

        hSerialisedElementHeader FindElement( hUint32 /*crcid*/ )
        {
            //TODO:
            hSerialisedElementHeader header;

            file_->Read( &header, sizeof(header) );

            return header;
        }

    private:

        atIOStream* file_;
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    #define _SERIALISE_WORKER_TYPE_SPECIALISATION( ty, count, typeflag ) \
    template<> \
    struct hSerialiserElementWorker< ty > \
    { \
     \
        template< typename _ArrayQType, typename _MetaName > \
        static void DoSerialise( hSerialiser* ser, const ty & data ) \
        { \
            typedef _MetaName NameInfo; \
            ser->Write( data, 1, NameInfo::GetCRCID(), 0 ); \
        } \
        template< typename _ArrayQType, typename _MetaName > \
        static void DoSerialise( hSerialiser* ser, const ty (&data)[_ArrayQType::ElementCount] ) \
        { \
            typedef _MetaName NameInfo; \
            static const hUint32 arraycount = _ArrayQType::ElementCount; \
            ser->Write( data, arraycount, NameInfo::GetCRCID(), hSerialisedElementHeader::Flag_Array ); \
        } \
        template< typename _ArrayQType, typename _MetaName > \
        static void DoDeserialise( hSerialiser* ser, ty& data ) \
        { \
            ser->Read( data, 1 ); \
        } \
        template< typename _ArrayQType, typename _MetaName > \
        static void DoDeserialise( hSerialiser* ser, ty (&data)[_ArrayQType::ElementCount] ) \
        { \
            static const hUint32 arraycount = _ArrayQType::ElementCount; \
            ser->Read( data, arraycount ); \
        } \
    }; \
    template<> \
    struct hSerialiserElementWorker< ty* > \
    { \
        template< typename _ArrayQType, typename _MetaName > \
        static void DoSerialise( hSerialiser* ser, const ty* data, hUint32 count = 1 ) \
        { \
            typedef _MetaName NameInfo; \
            if ( data == NULL ) count = 0; \
            ser->Write( data, count, NameInfo::GetCRCID(), hSerialisedElementHeader::Flag_Ptr ); \
        } \
        template< typename _ArrayQType, typename _MetaName > \
        static void DoDeserialise( hSerialiser* ser, ty* data, hUint32 count ) \
        { \
            ser->Read( data, count ); \
        } \
    };

    #define SERIALISE_WORKER_TYPE_SPECIALISATION( ty, typeflag ) \
        _SERIALISE_WORKER_TYPE_SPECIALISATION( ty, arrayElements, typeflag )

    template< typename _Ty > 
    struct hSerialiserElementWorker
    {
        template< typename _ArrayQType, typename _MetaName >
        static void DoSerialise( hSerialiser* ser, const _Ty& data )
        {
            typedef _MetaName NameInfo;

            ser->WriteUserTypeInfo( NameInfo::GetCRCID(), 1, 0 );
            SerialiseMethod/*< _Ty >*/( ser, data );
        }
        template< typename _ArrayQType, typename _MetaName >
        static void DoSerialise( hSerialiser* ser, const _Ty (&data)[_ArrayQType::ElementCount] )
        {
            typedef _MetaName NameInfo;
            static const hUint32 arraycount = _ArrayQType::ElementCount;

            ser->WriteUserTypeInfo( NameInfo::GetCRCID(), arraycount, hSerialisedElementHeader::Flag_Array );
            for ( hUint32 i = 0; i < arraycount; ++i )
            {
                SerialiseMethod/*< _Ty >*/( ser, data[i] );
            }
        }

        template< typename _ArrayQType, typename _MetaName >
        static void DoDeserialise( hSerialiser* ser, _Ty& data )
        {
            DeserialiseMethod/*< _Ty >*/( ser, data );
        }
        template< typename _ArrayQType, typename _MetaName >
        static void DoDeserialise( hSerialiser* ser, _Ty (&data)[_ArrayQType::ElementCount] )
        {
            typedef _MetaName NameInfo;
            static const hUint32 arraycount = _ArrayQType::ElementCount;

            for ( hUint32 i = 0; i < arraycount; ++i )
            {
                DeserialiseMethod/*< _Ty >*/( ser, data[i] );
            }
        }
    };

    template< typename _Ty > 
    struct hSerialiserElementWorker< _Ty* >
    {
        template< typename _ArrayQType, typename _MetaName >
        static void DoSerialise( hSerialiser* ser, const _Ty* data, hUint32 count = 1 )
        {
            typedef _MetaName NameInfo;
            if ( data == NULL )
            {
                count = 0;
            }

            ser->WriteUserTypeInfo( NameInfo::GetCRCID(), count, hSerialisedElementHeader::Flag_Ptr );
            for ( hUint32 i = 0; i < count; ++i )
            {
                SerialiseMethod< _Ty >( ser, data[i] );
            }
        }
        template< typename _ArrayQType, typename _MetaName >
        static void DoDeserialise( hSerialiser* ser, _Ty* data, hUint32 count )
        {
            for ( hUint32 i = 0; i < count; ++i )
            {
                DeserialiseMethod< _Ty >( ser, data[i] );
            }
        }
    };

    // Atomic type specializations 
    SERIALISE_WORKER_TYPE_SPECIALISATION( hUint64, hSerialisedElementHeader::Type_8byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hInt64,  hSerialisedElementHeader::Type_8byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hUint32, hSerialisedElementHeader::Type_4byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hInt32,  hSerialisedElementHeader::Type_4byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hUint16, hSerialisedElementHeader::Type_2byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hInt16,  hSerialisedElementHeader::Type_2byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hByte,   hSerialisedElementHeader::Type_1byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hChar,   hSerialisedElementHeader::Type_1byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hFloat,  hSerialisedElementHeader::Type_4byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hDouble, hSerialisedElementHeader::Type_8byte );
    SERIALISE_WORKER_TYPE_SPECIALISATION( hBool,   hSerialisedElementHeader::Type_1byte );
}

#endif // ATLA_SERIALISER_H__
