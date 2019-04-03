
#include "sfmphonemeextractor.h"
#include "movieobjects/dmeanimationset.h"
// Unmangle all this
class CSFMPhonemeExtractor
{
	CSFMPhonemeExtractor();
	~CSFMPhonemeExtractor();

public:
	bool Init();
	void Shutdown();

	int FindExtractor();
	// int FindExtractor(CPhonemeExtractor *pExtractor, int id)
	void GetExtractor(int a2, CUtlString *a3, PE_APITYPE *a4);
	void GetAPIInfo(int a2, CUtlString *a3, PE_APITYPE *a4);
	int GetAPICount();
	int FindFacialChannelsClip(int *a1);

	bool GetSentence(CDmeGameSound *a2, CSentence *a3);
	bool GetWaveFormat(const char *a2, CUtlBuffer *a3, int *a4, CSentence *a5, bool *a6);
	// char GetWaveFormat(const char *pchFileName, CUtlBuffer *pBuf, int, CSentence *, bool

	void BuildPhonemeLogList(void *, void *);
	int BuildPhonemeToPresetMapping(int a1, CDmeAnimationSet *a2, CDmePresetGroup *a3, int a4);
	void LogPhonemes(int a2, ExtractDesc_t *a3);
	void ReApply(ExtractDesc_t *a2);
	void Extract(CSentence *a2, const PE_APITYPE *a3, ExtractDesc_t *a4, bool a5);
	
	int WriteDefaultValuesIntoLogLayers(int a1, int a2);
	int WriteCurrentValuesIntoLogLayers(int a1, int a2);
	void StampControlValueLogs(int a1, int a2, float a3, int a4);
	void ClearInterstitialSpaces(int a1, int a2 , int a3, int a4, int a5, ExtractDesc_t& a6);
	
};

CSFMPhonemeExtractor::CSFMPhonemeExtractor()
{
}

CSFMPhonemeExtractor::~CSFMPhonemeExtractor()
{
}

//==================================================================================

bool CSFMPhonemeExtractor::Init ()
{
	return true;
}

void __thiscall CSFMPhonemeExtractor::Shutdown()
{
	
}

//==================================================================================

int CSFMPhonemeExtractor::FindExtractor()
{
	return 0;
}

bool CSFMPhonemeExtractor::GetWaveFormat(const char *a2, CUtlBuffer *a3, int *a4, CSentence *a5, bool *a6)
{
	return false;
}

bool CSFMPhonemeExtractor::GetSentence(CDmeGameSound *a2, CSentence *a3)
{
	return false;
}

void CSFMPhonemeExtractor::GetAPIInfo(int a2, CUtlString *a3, PE_APITYPE *a4)
{
}

int CSFMPhonemeExtractor::GetAPICount()
{
	return 0;
}

int CSFMPhonemeExtractor::FindFacialChannelsClip(int *a1)
{
	return 0;
}

void CSFMPhonemeExtractor::BuildPhonemeLogList(void *, void *)
{
	
}

int CSFMPhonemeExtractor::BuildPhonemeToPresetMapping(int a1, CDmeAnimationSet *a2, CDmePresetGroup *a3, int a4)
{
	return 0;
}

void CSFMPhonemeExtractor::LogPhonemes(int a2, ExtractDesc_t *a3)
{
	
}

void CSFMPhonemeExtractor::ReApply(ExtractDesc_t *a2)
{
	
}

void CSFMPhonemeExtractor::Extract(CSentence *a2, const PE_APITYPE *a3, ExtractDesc_t *a4, bool a5)
{
	
}

int CSFMPhonemeExtractor::WriteDefaultValuesIntoLogLayers(int a1, int a2)
{
	return 0;
}

int CSFMPhonemeExtractor::WriteCurrentValuesIntoLogLayers(int a1, int a2)
{
	return 0;
}

void CSFMPhonemeExtractor::StampControlValueLogs(int a1, int a2, float a3, int a4)
{
	
}

void CSFMPhonemeExtractor::ClearInterstitialSpaces(int a1, int a2, int a3, int a4, int a5, ExtractDesc_t& a6)
{
	
}