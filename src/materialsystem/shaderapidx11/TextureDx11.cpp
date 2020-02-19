#include "TextureDx11.h"

CTextureDx11::CTextureDx11()
{
	m_nFlags	 = 0;
	m_Count		 = 0;
	m_CountIndex	 = 0;
	m_nTimesBoundMax = 0;
	m_nTimesBoundThisFrame = 0;
	m_Format	       = IMAGE_FORMAT_RGBA8888;
}