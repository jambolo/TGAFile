/** @file *//********************************************************************************************************

                                                     TGAFile.cpp

						                    Copyright 2003, John J. Bolton
	--------------------------------------------------------------------------------------------------------------

	$Header: //depot/Libraries/TGAFile/TGAFile.cpp#9 $

	$NoKeywords: $

 ********************************************************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "TGAFile.h"

#include "misc/max.h"
#include "misc/exceptions.h"
#include <cstdio>
#include <cassert>

using namespace std;


// Various file offsets and data sizes

static int const	SIGNATURE_SIZE			= 18;					// Size of the signature

static int const	OFFSET_IMAGE_DATA		= 18;					// Offset from the end of the id to the start
																	// of the image data
static int const	OFFSET_ATTRIBUTE_TYPE	= 494;					// Offset to the attribute type in the
																	// extension area
static int const	OFFSET_SIGNATURE		= SIGNATURE_SIZE;		// Offset to the signature from the end of the
																	// file
static int const	OFFSET_FOOTER			= OFFSET_SIGNATURE+8;	// Offset to the footer data from the end of
																	// the file

																	
/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/


//! Opens the file and loads the image information. If there is an error the file is closed and
//! a ConstructorFailedException is thrown.
//!
//! @param	sFilename	Name of the file to open
//!
//! @warning	A ConstructorFailedException may be thrown

TgaFile::TgaFile( char const * sFilename )
{
	size_t itemsRead;

	m_ImageType		= IMAGE_NOIMAGE;
	m_ColorMapType	= 0;
	m_AlphaType		= ALPHA_UNSPECIFIED;
	m_Width			= 0;
	m_Height		= 0;
	m_Depth			= 0;
	m_ImageDataSize	= 0;

	// Open the file

	m_Fp = fopen( sFilename, "rb" );
	if ( !m_Fp )
	{
		throw ConstructorFailedException();
	}

	unsigned __int32 extensionOffset, developerOffest;

	// Look for the footer to determine the format version and get the offsets
	// to the extension and developer areas.
	
	m_Version = ReadFooter( &extensionOffset, &developerOffest );

	// Get the alpha type data

	if ( m_Version == VERSION_2 && extensionOffset != 0 )
	{
		fseek( m_Fp, extensionOffset + OFFSET_ATTRIBUTE_TYPE, SEEK_SET );

		unsigned __int8 attributeType;

		itemsRead = fread( &attributeType, sizeof attributeType, 1, m_Fp );
		if ( itemsRead != 1 )
		{
			fclose( m_Fp );
			throw ConstructorFailedException();
		}

		m_AlphaType = static_cast< AlphaType >( attributeType );
	}

	// Restore the pointer to the beginning of the file

	fseek( m_Fp, 0, SEEK_SET );

	// Read the ID field length and adjust the ID string length

	unsigned __int8 idFieldLength;
	itemsRead = fread( &idFieldLength, sizeof idFieldLength, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	m_Id.resize( idFieldLength, 0 );

	// Read color map type

	assert( sizeof m_ColorMapType == 1 );
	itemsRead = fread( &m_ColorMapType, sizeof m_ColorMapType, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	// Get the image type (only 1, 2, 9, and 10 are supported)

	unsigned __int8 imageType;
	itemsRead = fread( &imageType, sizeof imageType, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	if ( imageType != IMAGE_COLORMAPPED		&&
		 imageType != IMAGE_TRUECOLOR		&&
		 imageType != IMAGE_RLECOLORMAPPED	&&
		 imageType != IMAGE_RLETRUECOLOR )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}
	m_ImageType = static_cast< ImageType >( imageType );

	// Read the color map specification

	assert( sizeof m_FirstColorMapEntry == 2 );
	itemsRead = fread( &m_FirstColorMapEntry, sizeof m_FirstColorMapEntry, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	assert( sizeof m_ColorMapSize == 2 );
	itemsRead = fread( &m_ColorMapSize, sizeof m_ColorMapSize, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	assert( sizeof m_ColorMapEntrySize == 1 );
	itemsRead = fread( &m_ColorMapEntrySize, sizeof m_ColorMapEntrySize, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	// Read the image specification

	assert( sizeof m_XOrigin == 2 );
	itemsRead = fread( &m_XOrigin, sizeof m_XOrigin, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	assert( sizeof m_YOrigin == 2 );
	itemsRead = fread( &m_YOrigin, sizeof m_YOrigin, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	assert( sizeof m_Width == 2 );
	itemsRead = fread( &m_Width, sizeof m_Width, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	assert( sizeof m_Height == 2 );
	itemsRead = fread( &m_Height, sizeof m_Height, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	assert( sizeof m_Depth == 1 );
	itemsRead = fread( &m_Depth, sizeof m_Depth, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	unsigned __int8 imageDescriptor;
	itemsRead = fread( &imageDescriptor, sizeof imageDescriptor, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		fclose( m_Fp );
		throw ConstructorFailedException();
	}

	m_AlphaDepth = imageDescriptor & 0xf;
	m_PixelOrder = static_cast< PixelOrder >( ( imageDescriptor >> 4 ) & 0x3 );

	// Read the image ID if there is one

	if ( m_Id.size() > 0 )
	{
		itemsRead = fread( &m_Id[0], sizeof( char ), m_Id.size(), m_Fp );
		if ( itemsRead != m_Id.size() )
		{
			fclose( m_Fp );
			throw ConstructorFailedException();
		}
	}

	// Get the size of the image data

	size_t imageDataOffset = OFFSET_IMAGE_DATA + m_Id.size();

	if ( m_ColorMapType == 1)
	{
		imageDataOffset += m_ColorMapSize * ( ( m_ColorMapEntrySize + 7 ) / 8 );
	}

	long imageDataEnd;
	
	if ( m_Version == VERSION_2 )
	{
		if ( extensionOffset != 0 )
		{
			if ( developerOffest != 0 )
				imageDataEnd = min( (int)extensionOffset, (int)developerOffest );
			else
				imageDataEnd = extensionOffset;
		}
		else if ( developerOffest != 0 )
		{
			imageDataEnd = developerOffest;
		}
		else
		{
			fseek( m_Fp, 0, SEEK_END );
			imageDataEnd = ftell( m_Fp ) - OFFSET_FOOTER;
		}
	}
	else
	{
		fseek( m_Fp, 0, SEEK_END );
		imageDataEnd = ftell( m_Fp );
	}

	m_ImageDataSize = imageDataEnd - (long)imageDataOffset;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! If the file is open, it is closed.
//!

TgaFile::~TgaFile()
{
	if ( m_Fp )
	{
		fclose( m_Fp );
	}
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! @param	pImage		Location to put image data. The location must be large enough to hold @c m_ImageDataSize
//!						bytes.
//! @param	pColorMap	Location to put palette data, or 0. If a palette buffer is specified, it must be large
//!						to hold the entire palette (which is <tt>m_ColorMapSize * ( ( m_ColorMapEntrySize+7 ) / 8 )
//!						</tt> bytes).
//!
//! @return				@c true, if the data is read successfully

bool TgaFile::Read( void * pImage, void * pColorMap/* = 0*/ )
{
	// Skip the header and id

	if ( fseek( m_Fp, OFFSET_IMAGE_DATA + (int)m_Id.size(), SEEK_SET ) != 0 )
	{
		return false;
	}

	// Read the color map

	if ( m_ColorMapType == 1 &&
		 ( m_ImageType == IMAGE_COLORMAPPED ||
		   m_ImageType == IMAGE_RLECOLORMAPPED ) )
	{
		int const colorMapSize = m_ColorMapSize * ( ( m_ColorMapEntrySize + 7 ) / 8 );

		if ( pColorMap )
		{
			size_t itemsRead = fread( pColorMap, colorMapSize, 1, m_Fp );
			if ( itemsRead != 1 )
			{
				return false;
			}
		}
		else
		{
			if ( fseek( m_Fp, colorMapSize, SEEK_CUR ) != 0 )
			{
				return false;
			}
		}
	}

	// Load the image data

	if ( pImage && m_ImageType != IMAGE_NOIMAGE )
	{
		size_t itemsRead = fread( pImage, m_ImageDataSize, 1, m_Fp );
		if ( itemsRead != 1 )
		{
			return false;
		}
	}

	return true;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! @param	pImage		Location to put image data. The location must be large enough to hold @c m_ImageDataSize
//!						bytes.
//! @param	order		Desired pixel order
//! @param	pColorMap	Location to put palette data, or 0. If a palette buffer is specified, it must be large
//!						to hold the entire palette (which is <tt>m_ColorMapSize * ( ( m_ColorMapEntrySize+7 ) / 8 )
//!						</tt> bytes).
//!
//! @return				@c true, if the data is read successfully

bool TgaFile::Read( void * pImage, PixelOrder order, void * pColorMap/* = 0*/ )
{
	bool	ok;

	// Load the image data

	ok = Read( pImage, pColorMap );
	if ( !ok )
	{
		return false;
	}

	// Make sure it's in the right order

	Reorder( (uint8 *)pImage, order );

	return true;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! The function returns VERSION_1 if the signature does not match the version 2 signature. If the file format is
//! version 2 but the signature is not correct, the file format will be identified as version 1. The extension and
//! directory offsets are set only if the format is version 1.
//!
//! @param	pExtensionOffset	Location to put extension offset.
//! @param	pDirectoryOffset	Location to put directory offset.
//!
//! @return		Format version

TgaFile::Version TgaFile::ReadFooter( unsigned __int32 * pExtensionOffset, unsigned __int32 * pDirectoryOffset )
{
	size_t itemsRead;

	// Determine the format version. If the signature is not found, then it is
	// a version 1 file.

	fseek( m_Fp, -OFFSET_SIGNATURE, SEEK_END );	// The signature is the last 18 bytes at the end of the file

	char signature[SIGNATURE_SIZE];

	itemsRead = fread( signature, SIGNATURE_SIZE, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		return VERSION_1;
	}

	signature[ sizeof( signature ) - 1 ] = 0;	// Ensure that it is terminated
	if ( strcmp( signature, "TRUEVISION-XFILE." ) != 0 )
	{
		return VERSION_1;
	}

	// Version 2 - return the extension and directory offsets. The offsets
	// are required for version 2, so if they can't be read, then treat this
	// file as a version 1 file, instead of throwing an exception for now.

	fseek( m_Fp, -OFFSET_FOOTER, SEEK_END );

	assert( sizeof *pExtensionOffset == 4 );
	itemsRead = fread( pExtensionOffset, sizeof *pExtensionOffset, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		return VERSION_1;
	}

	assert( sizeof *pDirectoryOffset == 4 );
	itemsRead = fread( pDirectoryOffset, sizeof *pDirectoryOffset, 1, m_Fp );
	if ( itemsRead != 1 )
	{
		return VERSION_1;
	}

	return VERSION_2;
}


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

namespace
{


template< int N >
void SwapPixel( uint8 * p0, uint8 * p1 )
{
	for ( int i = 0; i < ( N + 7 ) / 8 ; i++ )
	{
		swap( p0[ i ], p1[ i ] );
	}
}


template<>
void SwapPixel< 8 >( uint8 * p0, uint8 * p1 )
{
	swap( *p0, *p1 );
}


template<>
void SwapPixel< 16 >( uint8 * p0, uint8 * p1 )
{
	swap( *(uint16*)p0, *(uint16*)p1 );
}


template<>
void SwapPixel< 24 >( uint8 * p0, uint8 * p1 )
{
	swap( p0[ 0 ], p1[ 0 ] );
	swap( p0[ 1 ], p1[ 1 ] );
	swap( p0[ 2 ], p1[ 2 ] );
}


template<>
void SwapPixel< 32 >( uint8 * p0, uint8 * p1 )
{
	swap( *(uint32*)p0, *(uint32*)p1 );
}


template< int N >
void SwapV( uint8 * pData, int w, int h )
{
	int const	rowSize	= w * N;

	for ( int i = 0; i < h/2; i++ )
	{
		uint8 *	p0	= pData + i * rowSize;
		uint8 *	p1	= pData + ( h - 1 - i ) * rowSize;

		for ( int j = 0; j < w; j++ )
		{
			SwapPixel< N >( p0, p1 );
			p0 += N;
			p1 += N;
		}
	}
}


template< int N >
void SwapH( uint8 * pData, int w, int h )
{
	int const	rowSize	= w * N;

	for ( int i = 0; i < h; i++ )
	{
		uint8 *	p0	= pData + i * rowSize;
		uint8 *	p1	= pData + i * rowSize + ( rowSize - N );

		for ( int j = 0; j < w/2; j++ )
		{
			SwapPixel< N >( p0, p1 );
			p0 += N;
			p1 -= N;
		}
	}
}


template< int N >
void SwapHV( uint8 * pData, int w, int h )
{
	uint8 *		p0	= pData;
	uint8 *		p1	= pData + ( w * h - 1 ) * N;

	for ( int i = 0; i < w * h / 2; i++ )
	{
		SwapPixel< N >( p0, p1 );
		p0 += N;
		p1 -= N;
	}
}


template< int N >
void ReorderN( uint8 * pData, int w, int h, TgaFile::PixelOrder oldOrder, TgaFile::PixelOrder newOrder )
{
	switch ( oldOrder ^ newOrder )
	{
	case 0:
	{
		// Nothing to do

		break;
	}

	case 1 << 0:
	{
		SwapH< N >( pData, w, h );
		break;
	}

	case 1 << 1:
	{
		SwapV< N >( pData, w, h );
		break;
	}

	case ( 1 << 1 ) | ( 1 << 0 ):
	{
		SwapHV< N >( pData, w, h );
		break;
	}

	default:
		assert( false );
		break;
	}
}

} // anonymous namespace


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

void TgaFile::Reorder( uint8 * pData, PixelOrder order )
{
	switch ( m_Depth )
	{
	case 8:
		ReorderN< 8 >( pData, m_Width, m_Height, m_PixelOrder, order );
		break;

	case 16:
		ReorderN< 16 >( pData, m_Width, m_Height, m_PixelOrder, order );
		break;

	case 24:
		ReorderN< 24 >( pData, m_Width, m_Height, m_PixelOrder, order );
		break;

	case 32:
		ReorderN< 32 >( pData, m_Width, m_Height, m_PixelOrder, order );
		break;

	default:
		assert( false );
		break;
	}

}