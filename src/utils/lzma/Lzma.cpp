//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
//	LZMA Codec.
//
//	LZMA SDK 19.00 Copyright (c) 1999-2019 Igor Pavlov (2019-02-21)
//	http://www.7-zip.org/
//
//=====================================================================================//

#include "CPP/Common/MyWindows.h"
#include "CPP/Common/MyInitGuid.h"
#include "CPP/Common/MyTypes.h"
#include "CPP/7zip/Common/FileStreams.h"
#include "CPP/7zip/Compress/LZMADecoder.h"
#include "CPP/7zip/Compress/LZMAEncoder.h"
#include "../../public/tier1/lzmaDecoder.h"

// lzma buffers will have a 13 byte trivial header
// [0]		reserved
// [1..4]	dictionary size, little endian
// [5..8]	uncompressed size, little endian low word
// [9..12]	uncompressed size, little endian high word
// [13..]	lzma compressed data
#define LZMA_ORIGINAL_HEADER_SIZE	13

class CInStreamRam: public ISequentialInStream, public CMyUnknownImp
{
	const Byte *Data;
	size_t Size;
	size_t Pos;

public:
	MY_UNKNOWN_IMP
	void Init(const Byte *data, size_t size)
	{
		Data = data;
		Size = size;
		Pos = 0;
	}

	STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CInStreamRam::Read(void *data, UInt32 size, UInt32 *processedSize)
{
	UInt32 remain = Size - Pos;
	if (size > remain)
		size = remain;
	for (UInt32 i = 0; i < size; i++)
		((Byte *)data)[i] = Data[Pos + i];
	Pos += size;
	if (processedSize != NULL)
		*processedSize = size;
	return S_OK;
}
  
class COutStreamRam: public ISequentialOutStream, public CMyUnknownImp
{
	size_t Size;

public:
	Byte *Data;
	size_t Pos;
	bool Overflow;

	void Init(Byte *data, size_t size)
	{
		Data = data;
		Size = size;
		Pos = 0;
		Overflow = false;
	}

	void SetPos(size_t pos)
	{
		Overflow = false;
		Pos = pos;
	}

	MY_UNKNOWN_IMP
	HRESULT WriteByte(Byte b)
	{
		if (Pos >= Size)
		{
			Overflow = true;
			return E_FAIL;
		}
		Data[Pos++] = b;
		return S_OK;
	}

	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP COutStreamRam::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
	UInt32 i;
	for (i = 0; i < size && Pos < Size; i++)
		Data[Pos++] = ((const Byte *)data)[i];
	if (processedSize != NULL)
		*processedSize = i;
	if (i != size)
	{
		Overflow = true;
		return E_FAIL;
	}
	return S_OK;
}

#define SZE_OK				0  
#define SZE_FAIL			1
#define SZE_OUTOFMEMORY		2
#define SZE_OUT_OVERFLOW	3

int LzmaEncode(
const Byte	*inBuffer,
size_t		inSize, 
Byte		*outBuffer,
size_t		outSize,
size_t		*outSizeProcessed, 
UInt32		dictionarySize )
{
	*outSizeProcessed = 0;

	const size_t kLzmaPropsSize = 5;
	const size_t kMinDestSize = LZMA_ORIGINAL_HEADER_SIZE;
	if ( outSize < kMinDestSize )
		return SZE_OUT_OVERFLOW;

	NCompress::NLzma::CEncoder *encoderSpec = new NCompress::NLzma::CEncoder;
	CMyComPtr<ICompressCoder> encoder = encoderSpec;

	// defaults
	UInt32 posStateBits = 2;
	UInt32 litContextBits = 3;
	UInt32 litPosBits = 0;
	UInt32 algorithm = 2;
	UInt32 numFastBytes = 64;
	wchar_t *mf = L"BT4";

	PROPID propIDs[] = 
	{ 
		NCoderPropID::kDictionarySize,
		NCoderPropID::kPosStateBits,
		NCoderPropID::kLitContextBits,
		NCoderPropID::kLitPosBits,
		NCoderPropID::kAlgorithm,
		NCoderPropID::kNumFastBytes,
		NCoderPropID::kMatchFinder,
		NCoderPropID::kEndMarker,
	};
	const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);
	PROPVARIANT properties[kNumProps];

	// set all properties
	properties[0].vt = VT_UI4;
	properties[0].ulVal = UInt32(dictionarySize);
	properties[1].vt = VT_UI4;
	properties[1].ulVal = UInt32(posStateBits);
	properties[2].vt = VT_UI4;
	properties[2].ulVal = UInt32(litContextBits);
	properties[3].vt = VT_UI4;
	properties[3].ulVal = UInt32(litPosBits);
	properties[4].vt = VT_UI4;
	properties[4].ulVal = UInt32(algorithm);
	properties[5].vt = VT_UI4;
	properties[5].ulVal = UInt32(numFastBytes);
	properties[6].vt = VT_BSTR;
	properties[6].bstrVal = (BSTR)(const wchar_t *)mf;
	properties[7].vt = VT_BOOL;
	properties[7].boolVal = VARIANT_FALSE;
	if ( encoderSpec->SetCoderProperties(propIDs, properties, kNumProps) != S_OK )
		return 1;
  
	COutStreamRam *outStreamSpec = new COutStreamRam;
	if ( outStreamSpec == 0 )
		return SZE_OUTOFMEMORY;
	CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;

	CInStreamRam *inStreamSpec = new CInStreamRam;
	if ( inStreamSpec == 0 )
		return SZE_OUTOFMEMORY;
	CMyComPtr<ISequentialInStream> inStream = inStreamSpec;

	outStreamSpec->Init( outBuffer, outSize );

	if ( encoderSpec->WriteCoderProperties(outStream) != S_OK )
		return SZE_OUT_OVERFLOW;
	if ( outStreamSpec->Pos != kLzmaPropsSize )
		return 1;
  
	int i;
	for ( i = 0; i < 8; i++ )
	{
		UInt64 t = (UInt64)(inSize);
		if ( outStreamSpec->WriteByte((Byte)((t) >> (8 * i))) != S_OK )
			return SZE_OUT_OVERFLOW;
	}

	UInt32 minSize = 0;
	int mainResult = SZE_OK;
	size_t startPos = outStreamSpec->Pos;
	outStreamSpec->SetPos( startPos );
	inStreamSpec->Init( inBuffer, inSize );

	HRESULT lzmaResult = encoder->Code( inStream, outStream, 0, 0, 0 );

	if ( lzmaResult == E_OUTOFMEMORY )
	{
		mainResult = SZE_OUTOFMEMORY;
	} 
	else if ( outStreamSpec->Pos <= minSize )
	{
		minSize = outStreamSpec->Pos;
	}

	if ( outStreamSpec->Overflow )
	{
		mainResult = SZE_OUT_OVERFLOW;
	}
	else if ( lzmaResult != S_OK )
	{
		mainResult = SZE_FAIL;
	} 

	*outSizeProcessed = outStreamSpec->Pos;

	return mainResult;
}

//-----------------------------------------------------------------------------
// Encoding glue. Returns non-null Compressed buffer if successful.
// Caller must free.
//-----------------------------------------------------------------------------
unsigned char *LZMA_Compress( 
unsigned char	*pInput,
unsigned int	inputSize, 
unsigned int	*pOutputSize, 
unsigned int	dictionarySize )
{
	*pOutputSize = 0;

	if ( inputSize <= sizeof( lzma_header_t ) )
	{
		// pointless
		return NULL;
	}

	dictionarySize = ( 1 << dictionarySize );

	// using same work buffer calcs as the SDK 105% + 64K
	unsigned outSize = inputSize/20 * 21 + (1<<16);
	unsigned char *pOutputBuffer = (unsigned char*)malloc( outSize );
	if ( !pOutputBuffer )
	{
		return NULL;
	}

	// compress, skipping past our header
	unsigned int compressedSize;
	int result = LzmaEncode( pInput, inputSize, pOutputBuffer + sizeof( lzma_header_t ), outSize - sizeof( lzma_header_t ), &compressedSize, dictionarySize );
	if ( result != SZE_OK )
	{
		free( pOutputBuffer );
		return NULL;
	}

	if ( compressedSize - LZMA_ORIGINAL_HEADER_SIZE + sizeof( lzma_header_t ) >= inputSize )
	{
		// compression got worse or stayed the same
		free( pOutputBuffer );
		return NULL;
	}

	// construct our header, strip theirs
	lzma_header_t *pHeader = (lzma_header_t *)pOutputBuffer;
	pHeader->id = LZMA_ID;
	pHeader->actualSize = inputSize;
	pHeader->lzmaSize = compressedSize - LZMA_ORIGINAL_HEADER_SIZE;
	memcpy( pHeader->properties, pOutputBuffer + sizeof( lzma_header_t ), 5 );

	// shift the compressed data into place
	memcpy( pOutputBuffer + sizeof( lzma_header_t ), pOutputBuffer + sizeof( lzma_header_t ) + LZMA_ORIGINAL_HEADER_SIZE, compressedSize - LZMA_ORIGINAL_HEADER_SIZE );

	// final output size is our header plus compressed bits
	*pOutputSize = sizeof( lzma_header_t ) + compressedSize - LZMA_ORIGINAL_HEADER_SIZE;

	return pOutputBuffer;
}

bool LZMA_IsCompressed( unsigned char *pInput )
{
	lzma_header_t *pHeader = (lzma_header_t *)pInput;
	if ( pHeader && pHeader->id == LZMA_ID )
	{
		return true;
	}

	// unrecognized
	return false;
}

unsigned int LZMA_GetActualSize( unsigned char *pInput )
{
	lzma_header_t *pHeader = (lzma_header_t *)pInput;
	if ( pHeader && pHeader->id == LZMA_ID )
	{
		return pHeader->actualSize;
	}

	// unrecognized
	return 0;
}