#ifndef C_LIGHTS_H
#define C_LIGHTS_H

#include "c_baseentity.h"

class C_EnvLight : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_EnvLight, C_BaseEntity );
	DECLARE_NETWORKCLASS();

	C_EnvLight();
	~C_EnvLight();

	virtual void OnDataChanged( DataUpdateType_t type );

	bool IsCascadedShadowMappingEnabled() const;

	void GetShadowMappingConstants( QAngle &angSunAngles, Vector &vecLight, Vector &vecAmbient ) const
	{
		angSunAngles = m_angSunAngles;
		vecLight = m_vecLight;
		vecAmbient = m_vecAmbient;
	}

	struct ShadowConfig_t
	{
		float flOrthoSize;
		float flForwardOffset;
		float flUVOffsetX;
		float flViewDepthBiasHack;
	} shadowConfigs[2];

	template <typename T, size_t C> void CopyShadowConfigData( T (&dest)[C] )
	{
		V_memcpy( dest, shadowConfigs, sizeof( shadowConfigs ) );
	}

private:
	QAngle m_angSunAngles;
	Vector m_vecLight;
	Vector m_vecAmbient;
	bool m_bCascadedShadowMappingEnabled;
};

extern C_EnvLight *g_pCSMEnvLight;

#endif
