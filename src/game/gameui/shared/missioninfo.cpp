#include "cbase.h"
#include "missioninfo.h"
#include "filesystem.h"
#include "tier0/memdbgon.h"

void CMissionInfo::Initialize()
{
	FileFindHandle_t handle;
	// change folder name to campaign?
	const char *filename = filesystem->FindFirstEx("missions/*.txt", "GAME", &handle);
	while(filename) {

		char file[FILENAME_MAX];
		Q_snprintf(file, sizeof(file), "missions/%s", filename);
		KeyValues *keyvalues = new KeyValues("");

		if(!keyvalues->LoadFromFile(filesystem, file, "GAME")) 
		{
			filename = filesystem->FindNext(handle);
			keyvalues->deleteThis();
			continue;
		}

		m_Campaigns.AddToTail(keyvalues);

		filename = filesystem->FindNext(handle);
	}
	filesystem->FindClose(handle);
}

KeyValues *CMissionInfo::GetCampaignDetails(const char *name)
{
	for (int i = 0; i < m_Campaigns.Count(); i++)
	{
		const char* CampaignName = m_Campaigns[i]->GetString("Name");
		if  (stricmp( CampaignName, name) == 0 )
		{
			return m_Campaigns[i];
		}
	}
	return NULL;
}
int CMissionInfo::GetCampaignCount()
{
	return m_Campaigns.Count();
}

CMissionInfo::~CMissionInfo()
{
	for(int i = 0; i < m_Campaigns.Count(); i++) {
		KeyValues *&it = m_Campaigns.Element(i);
		it->deleteThis();
		it = nullptr;
	}
}

static CMissionInfo s_missioninformer;
IMissionInfo *missioninformer = &s_missioninformer;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CMissionInfo, IMissionInfo, MISSION_INFORMER_VERSION, s_missioninformer);