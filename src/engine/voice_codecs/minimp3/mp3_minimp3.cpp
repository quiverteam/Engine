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
	mp3dec_t	m_decoder;
	mp3dec_frame_info_t m_info;
	IAudioStreamEvent* m_pHandler;
	char m_buffer[16000];
	int m_bufferpos = 0;
	int m_filepos = 0;
};

CMiniMP3AudioStream::CMiniMP3AudioStream()
{
}

bool CMiniMP3AudioStream::Init( IAudioStreamEvent* pHandler )
{
	m_pHandler = pHandler;

	memset( m_buffer, NULL, sizeof( m_buffer ) );
	m_pHandler->StreamRequestData( m_buffer, sizeof( m_buffer ), 0 );
	mp3dec_init( &m_decoder );
	
	if ( mp3dec_decode_frame( &m_decoder, ( const uint8_t* )m_buffer, sizeof( m_buffer ), NULL, &m_info ) > 0 )
		return true;

	return false;
}


CMiniMP3AudioStream::~CMiniMP3AudioStream()
{

}

// IAudioStream functions
int	CMiniMP3AudioStream::Decode( void* pBuffer, unsigned int bufferSize )
{
	if ( m_bufferpos >= 15000 )
	{
		memset( m_buffer, NULL, sizeof( m_buffer ) );
		if ( m_pHandler->StreamRequestData( m_buffer, sizeof( m_buffer ), m_filepos ) < sizeof( m_buffer ) )
			return 0;

		m_bufferpos = 0;
	}

	int samples = mp3dec_decode_frame( &m_decoder, ( const uint8_t* )m_buffer + ( size_t )m_bufferpos, sizeof( m_buffer ), ( short* )pBuffer, &m_info );
	m_bufferpos += m_info.frame_bytes;
	m_filepos += m_info.frame_bytes;

	return samples*2*m_info.channels;
}


int CMiniMP3AudioStream::GetOutputBits()
{
	return m_info.bitrate_kbps;
}


int CMiniMP3AudioStream::GetOutputRate()
{
	return m_info.hz;
}


int CMiniMP3AudioStream::GetOutputChannels()
{
	return m_info.channels;
}


unsigned int CMiniMP3AudioStream::GetPosition()
{
	return 0;
}

// NOTE: Only supports seeking forward right now
void CMiniMP3AudioStream::SetPosition( unsigned int position )
{

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