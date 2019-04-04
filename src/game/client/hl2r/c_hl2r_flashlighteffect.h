//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FLASHLIGHTEFFECT_H
#define FLASHLIGHTEFFECT_H
#ifdef _WIN32
#pragma once
#endif

struct dlight_t;


class CFlashlightEffect
{
public:

	CFlashlightEffect(int nEntIndex = 0);
	~CFlashlightEffect();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance);
	void TurnOn();
	void TurnOff();
	bool IsOn( void ) { return m_bIsOn;	}

	ClientShadowHandle_t GetFlashlightHandle( void ) { return m_FlashlightHandle; }
	void SetFlashlightHandle( ClientShadowHandle_t Handle ) { m_FlashlightHandle = Handle;	}
	
protected:

	void LightOff();
	void LightOffOld();
	void LightOffNew();

	void UpdateLightNew(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp);
	void UpdateLightOld(const Vector &vecPos, const Vector &vecDir, int nDistance);

	bool m_bIsOn;
	int m_nEntIndex;
	ClientShadowHandle_t m_FlashlightHandle;

	// Vehicle headlight dynamic light pointer
	dlight_t *m_pPointLight;
	float m_flCurrentPullBackDist;

	// Texture for flashlight
	CTextureReference m_FlashlightTexture;

private:

	// --------------------------------------
	// uberlight
	FlashlightState_t	m_FlashlightState;
	UberlightState_t	m_UberlightState;

	//bool m_bUberlightEnabled;
	float m_fNearEdge;
	float m_fFarEdge;
	float m_fCutOn;
	float m_fCutOff;
	float m_fShearx;
	float m_fSheary;
	float m_fWidth;
	float m_fWedge;
	float m_fHeight;
	float m_fHedge;
	float m_fRoundness;
	//Vector m_vecRenderBoundsMin, m_vecRenderBoundsMax;
};

class CHeadlightEffect : public CFlashlightEffect
{
public:
	
	CHeadlightEffect();
	~CHeadlightEffect();

	virtual void UpdateLight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance );
};

#endif // FLASHLIGHTEFFECT_H
