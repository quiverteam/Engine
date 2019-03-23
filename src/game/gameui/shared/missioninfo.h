#ifndef _MISSION_INFO_H_
#define _MISSION_INFO_H_

#pragma once

#include "igamesystem.h"

abstract_class IMissionInfo
{
public:
	virtual void	Initialize() = 0;
	virtual KeyValues *GetCampaignDetails(int index) = 0;
	virtual KeyValues *GetCampaignDetails(const char *name) = 0;
	virtual int	GetCampaignCount() = 0;
};

#define MISSION_INFORMER_VERSION "MISSION_INFO_001"

class CMissionInfo : public IMissionInfo
{
public:
	~CMissionInfo();
	void Initialize();
	KeyValues *GetCampaignDetails(int index) { return m_Campaigns[index]; }
	KeyValues *GetCampaignDetails(const char *name);
	int	GetCampaignCount();

private:
	CUtlVector<KeyValues *> m_Campaigns;
};

extern IMissionInfo *missioninformer;
#endif