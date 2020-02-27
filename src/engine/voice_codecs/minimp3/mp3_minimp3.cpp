#include "interface.h"
#include "vaudio/ivaudio.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"

class CMiniMP3AudioStream : public IAudioStream
{
public:
	CMiniMP3AudioStream();
	bool Init( IAudioStreamEvent* pHandler );
	~CMiniMP3AudioStream();

	// IAudioStream functions
	virtual int	Decode( void* pBuffer, unsigned int bufferSize );
	virtual int GetOutputBits();
	virtual int GetOutputRate();
	virtual int GetOutputChannels();
	virtual unsigned int GetPosition();
	virtual void SetPosition( unsigned int position );
private:
	mp3dec_t	m_Decoder;
	mp3dec_frame_info_t m_Info;
	IAudioStreamEvent* m_pHandler;
	char m_Buffer[16000];
	uint m_BufferPos = 0;
	uint m_FilePos = 0;
	int m_EndOfBufferPos = -1;
};

CMiniMP3AudioStream::CMiniMP3AudioStream()
{
}

bool CMiniMP3AudioStream::Init( IAudioStreamEvent* pHandler )
{
	m_pHandler = pHandler;

	memset( m_Buffer, NULL, sizeof( m_Buffer ) );
	m_pHandler->StreamRequestData( m_Buffer, sizeof( m_Buffer ), 0 );
	mp3dec_init( &m_Decoder );
	
	if ( mp3dec_decode_frame( &m_Decoder, ( const uint8_t* )m_Buffer, sizeof( m_Buffer ), NULL, &m_Info ) > 0 )
		return true;

	return false;
}


CMiniMP3AudioStream::~CMiniMP3AudioStream()
{

}

// IAudioStream functions
int	CMiniMP3AudioStream::Decode( void* pBuffer, unsigned int bufferSize )
{
	if ( m_BufferPos >= sizeof( m_Buffer ) - m_Info.frame_bytes )
	{
		memset( m_Buffer, NULL, sizeof( m_Buffer ) );

		int ReadBytes = m_pHandler->StreamRequestData( m_Buffer, sizeof( m_Buffer ), m_FilePos );
		if ( ReadBytes < sizeof( m_Buffer ) )
		{
			m_EndOfBufferPos = ReadBytes;
		}

		m_BufferPos = 0;
	}
	if ( m_EndOfBufferPos >= 0 && (int)m_BufferPos >= m_EndOfBufferPos )
	{
		return 0;
	}

	int samples = mp3dec_decode_frame( &m_Decoder, ( const uint8_t* )m_Buffer + ( size_t )m_BufferPos, sizeof( m_Buffer ), ( short* )pBuffer, &m_Info );
	m_BufferPos += m_Info.frame_bytes;
	m_FilePos += m_Info.frame_bytes;

	return samples*2*m_Info.channels;
}


int CMiniMP3AudioStream::GetOutputBits()
{
	return m_Info.bitrate_kbps;
}


int CMiniMP3AudioStream::GetOutputRate()
{
	return m_Info.hz;
}


int CMiniMP3AudioStream::GetOutputChannels()
{
	return m_Info.channels;
}


unsigned int CMiniMP3AudioStream::GetPosition()
{
	return m_FilePos;
}

void CMiniMP3AudioStream::SetPosition( unsigned int position )
{
	m_FilePos = position;
	m_BufferPos = m_Info.frame_bytes;

	int ReadBytes = m_pHandler->StreamRequestData( m_Buffer, sizeof( m_Buffer ), m_FilePos - m_BufferPos );
	if ( ReadBytes < sizeof( m_Buffer ) )
	{
		m_EndOfBufferPos = ReadBytes;
	}

	//Due to bit-reservoir we decode a previous frame first, otherwise the frame we want will fail to decode
	short TmpBuffer[MINIMP3_MAX_SAMPLES_PER_FRAME];
	mp3dec_init( &m_Decoder );
	mp3dec_decode_frame( &m_Decoder, ( const uint8_t* )m_Buffer, sizeof( m_Buffer ), TmpBuffer, &m_Info );
}


class CVAudio : public IVAudio
{
public:
	CVAudio()
	{
	}

	~CVAudio()
	{
	}

	IAudioStream* CreateMP3StreamDecoder( IAudioStreamEvent* pEventHandler )
	{
		CMiniMP3AudioStream* pMP3 = new CMiniMP3AudioStream;
		if ( !pMP3->Init( pEventHandler ) )
		{
			delete pMP3;
			return NULL;
		}
		return pMP3;
	}

	void DestroyMP3StreamDecoder( IAudioStream* pDecoder )
	{
		delete pDecoder;
	}
};

EXPOSE_INTERFACE( CVAudio, IVAudio, VAUDIO_INTERFACE_VERSION );