/** @file *//********************************************************************************************************

                                                      TGAFile.h

						                    Copyright 2003, John J. Bolton
	--------------------------------------------------------------------------------------------------------------

	$Header: //depot/Libraries/TGAFile/TGAFile.h#7 $

	$NoKeywords: $

 ********************************************************************************************************************/

#pragma once


#include "Misc/Types.h"
#include <string>
#include <boost/noncopyable.hpp>

struct _iobuf;		// Forward declaration for C stdio FILE


/********************************************************************************************************************/
/*																													*/
/********************************************************************************************************************/

//! A class for reading TGA files
//
//! The TGA files are read according to this specification:
//! "Truevision TGA File Format Specification Version 2.0", Technical manual version 2.2, January 1991
//! 

class TgaFile : boost::noncopyable
{
public:

	//! The image format.
	enum ImageType
	{
		IMAGE_NOIMAGE			= 0,
		IMAGE_COLORMAPPED		= 1,
		IMAGE_TRUECOLOR			= 2,
		IMAGE_GREYSCALE			= 3,
		IMAGE_RLECOLORMAPPED	= 9,
		IMAGE_RLETRUECOLOR		= 10,
		IMAGE_RLEGREYSCALE		= 11
	};

	//! The order the pixels (as specified by the location in the image of the first pixel in the file).
	enum PixelOrder
	{
		ORDER_BOTTOMLEFT		= 0,
		ORDER_BOTTOMRIGHT		= 1,
		ORDER_TOPLEFT			= 2,
		ORDER_TOPRIGHT			= 3
	};

	//! Information about the alpha channel data.
	enum AlphaType
	{
		ALPHA_UNSPECIFIED			= -1,	// Note: Not part of the specification
		ALPHA_NONE					= 0,
		ALPHA_UNDEFINED				= 1,
		ALPHA_PAD					= 2,
		ALPHA_PRESENT				= 3,
		ALPHA_PREMULTIPLIED			= 4
	};

	//! Format version used by the file
	enum Version
	{
		VERSION_1,
		VERSION_2
	};

	//! Constructor
	TgaFile( char const * sFilename );

	// Destructor
	virtual ~TgaFile();

	//! Loads the image data into the given buffers, and returns false if it fails
	bool Read( void * pImage, void * pColorMap = 0 );

	//! Loads the image data into the given buffers in the desired order, and returns false if it fails
	bool Read( void * pImage, PixelOrder order, void * pColorMap = 0 );

	std::string		m_Id;					//!< Image name or ID
	ImageType		m_ImageType;			//!< Image type
	int16			m_FirstColorMapEntry;	//!< Index of first entry
	int16			m_ColorMapSize;			//!< Number of entries in the color map
	int16			m_XOrigin;				//!< X origin of the bottom-left corner of the image
	int16			m_YOrigin;				//!< Y origin of the bottom-left corner of the image
	int16			m_Width;				//!< Width of the image
	int16			m_Height;				//!< Height of the image
	int8			m_Depth;				//!< Pixel depth (including alpha)
	int8			m_ColorMapEntrySize;	//!< Number of bits in a color map entry
	int				m_AlphaDepth;			//!< Number of bits in the alpha channel
	PixelOrder		m_PixelOrder;			//!< Order of pixels in the image
	AlphaType		m_AlphaType;			//!< Information about the alpha channel
	Version			m_Version;				//!< File format version
	long			m_ImageDataSize;		//!< Size of the image data in bytes.

private:

	//! Reads the footer info (if it exists).
	Version ReadFooter( uint32 * pExtensionOffset, uint32 * pDeveloperOffset );

	//! Reorders pixel data assumed to be loaded from this file
	void Reorder( uint8 * pData, PixelOrder order ); 

	int8			m_ColorMapType;			// 1 if the image uses a color map, 0 otherwise
	struct _iobuf *	m_Fp;					// FILE pointer to image file
};
