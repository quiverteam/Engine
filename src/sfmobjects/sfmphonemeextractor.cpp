#include "cbase.h"
#include "sfmphonemeextractor.h"

// Unmangle all this
class CSFMPhonemeExtractor
{
	CSFMPhonemeExtractor();
	~CSFMPhonemeExtractor();

public:
	char Init(CSFMPhonemeExtractor *, int a2);
	void Shutdown(CSFMPhonemeExtractor *this);

	int FindExtractor(DWORD *this, int a2);
	// int FindExtractor(CPhonemeExtractor *pExtractor, int id)
	void GetExtractor(CSFMPhonemeExtractor *this, int a2, struct CUtlString *a3, enum PE_APITYPE *a4);
	void GetAPIInfo(CSFMPhonemeExtractor *this, int a2, struct CUtlString *a3, enum PE_APITYPE *a4);
	int __thiscall GetAPICount(CSFMPhonemeExtractor *this)
	int FindFacialChannelsClip(_DWORD *a1);

	bool __thiscall GetSentence(CSFMPhonemeExtractor *this, struct CDmeGameSound *a2, struct CSentence *a3);
	char __stdcall GetWaveFormat(const char *a2, struct CUtlBuffer *a3, int *a4, struct CSentence *a5, bool *a6);
	// char GetWaveFormat(const char *pchFileName, CUtlBuffer *pBuf, int, CSentence *, bool

	_DWORD *__stdcall BuildPhonemeLogList(_DWORD *a1, int *a2)
	int __stdcall BuildPhonemeToPresetMapping(int a1, CDmeAnimationSet *a2, CDmePresetGroup *a3, int a4);
	void __thiscall LogPhonemes(CSFMPhonemeExtractor *this, int a2, struct ExtractDesc_t *a3);
	void __thiscall ReApply(CSFMPhonemeExtractor *this, struct ExtractDesc_t *a2);
	void __userpurge Extract(CSFMPhonemeExtractor *this@<ecx > , struct CSentence *a2@<edi > , const enum PE_APITYPE *a3, struct ExtractDesc_t *a4, bool a5);
	
	int __stdcall WriteDefaultValuesIntoLogLayers(int a1, int a2);
	int __stdcall WriteCurrentValuesIntoLogLayers(int a1, int a2)
	_DWORD *__stdcall StampControlValueLogs(int a1, int a2, float a3, int a4);
	void __userpurge ClearInterstitialSpaces(int a1@<ecx > , int a2@<ebx > , int a3@<esi > , int a4, int a5, _DWORD *a6);
	
};

CSFMPhonemeExtractor *__thiscall CSFMPhonemeExtractor::CSFMPhonemeExtractor(CSFMPhonemeExtractor *this)
{
	CSFMPhonemeExtractor *result; // eax

	result = this;
	*(_DWORD *)this = &CSFMPhonemeExtractor::`vftable';
		*((_DWORD *)this + 1) = 0;
	*((_DWORD *)this + 2) = 0;
	*((_DWORD *)this + 3) = 0;
	*((_DWORD *)this + 4) = 0;
	*((_DWORD *)this + 5) = 0;
	*((_DWORD *)this + 6) = -1;
	return result;
}

int __thiscall CSFMPhonemeExtractor::~CSFMPhonemeExtractor(CSFMPhonemeExtractor *this)
{
	CSFMPhonemeExtractor *v1; // esi
	int result; // eax

	v1 = this;
	result = CUtlVector<Extractor, CUtlMemory<Extractor, int>>::~CUtlVector<Extractor, CUtlMemory<Extractor, int>>((char *)this + 4);
	*(_DWORD *)v1 = &ISFMPhonemeExtractor::`vftable';
		return result;
}

//==================================================================================

char __usercall CSFMPhonemeExtractor::Init@<al > (CSFMPhonemeExtractor *this@<ecx > , int a2@<edi > )
{
	CSFMPhonemeExtractor *v2; // esi
	int v3; // eax
	void(*v4)(const char *, ...); // edi
	struct CSysModule *v5; // eax
	void *(__cdecl *v6)(const char *, int *); // eax
	int(__thiscall ***v7)(_DWORD); // eax
	int v9; // [esp+14h] [ebp-218h]
	int v10; // [esp+1Ch] [ebp-210h]
	int v11; // [esp+20h] [ebp-20Ch]
	int v12; // [esp+24h] [ebp-208h]
	struct CSysModule *v13; // [esp+28h] [ebp-204h]
	char v14[4]; // [esp+2Ch] [ebp-200h]
	char v15; // [esp+30h] [ebp-1FCh]

	v2 = this;
	v3 = (*(int(__stdcall **)(const char *, const char *, int *))(*(_DWORD *)g_pFullFileSystem + 124))(
		"phonemeextractors/*.dll",
		"EXECUTABLE_PATH",
		&v10);
	if (v3)
	{
		v9 = a2;
		v4 = (void(*)(const char *, ...))_Warning;
		do
		{
			V_snprintf(v14, 512, "phonemeextractors/%s", v3);
			v5 = (struct CSysModule *)(*(int(__stdcall **)(char *, _DWORD, signed int, int))(*(_DWORD *)g_pFullFileSystem
				+ 100))(
					v14,
					0,
					1,
					v9);
			v13 = v5;
			if (v5)
			{
				v6 = Sys_GetFactory(v5);
				if (v6)
				{
					v7 = (int(__thiscall ***)(_DWORD))v6("PHONEME_EXTRACTOR_001", 0);
					*(_DWORD *)v14 = v7;
					if (v7)
					{
						v12 = (**v7)(v7);
						CUtlVector<Extractor, CUtlMemory<Extractor, int>>::InsertBefore(*((_DWORD *)v2 + 4), &v12);
					}
					else
					{
						v4("Unable to get IPhonemeExtractor interface version %s from %s\n", "PHONEME_EXTRACTOR_001", &v15);
					}
				}
				v9 = v11;
				v3 = (*(int(**)(void))(*(_DWORD *)g_pFullFileSystem + 112))();
			}
			else
			{
				v9 = v11;
				v3 = (*(int(**)(void))(*(_DWORD *)g_pFullFileSystem + 112))();
			}
		} while (v3);
	}
	(*(void(__stdcall **)(int))(*(_DWORD *)g_pFullFileSystem + 120))(v10);
	return 1;
}

void __thiscall CSFMPhonemeExtractor::Shutdown(CSFMPhonemeExtractor *this)
{
	CSFMPhonemeExtractor *v1; // ebx
	int v2; // esi
	int v3; // edi

	v1 = this;
	v2 = *((_DWORD *)this + 4) - 1;
	if (v2 >= 0)
	{
		v3 = 12 * v2;
		do
		{
			(*(void(__stdcall **)(_DWORD))(*(_DWORD *)g_pFullFileSystem + 104))(*(_DWORD *)(*((_DWORD *)v1 + 1) + v3 + 4));
			--v2;
			v3 -= 12;
		} while (v2 >= 0);
	}
	*((_DWORD *)v1 + 4) = 0;
}

//==================================================================================

int __thiscall CSFMPhonemeExtractor::FindExtractor(DWORD *this, int a2)
{
	int v2;
	int result;
	DWORD *v4;

	v2 = this[4];
	result = 0;
	if (v2 <= 0)
		return -1;
	v4 = (_DWORD *)this[1];
	while (*v4 != a2)
	{
		++result;
		v4 += 3;
		if (result >= v2)
			return -1;
	}
	return result;
}

char __stdcall CSFMPhonemeExtractor::GetWaveFormat(const char *a2, struct CUtlBuffer *a3, int *a4, struct CSentence *a5, bool *a6)
{
	char v6; // bl
	unsigned int v7; // eax
	unsigned int v9; // eax
	struct IterateRIFF *v10; // [esp+0h] [ebp-38h]
	char v11; // [esp+10h] [ebp-28h]
	char v12; // [esp+18h] [ebp-20h]
	int v13; // [esp+1Ch] [ebp-1Ch]
	char v14; // [esp+20h] [ebp-18h]
	char v15; // [esp+3Ch] [ebp+4h]

	InFileRIFF::InFileRIFF((InFileRIFF *)&v11, a2, g_pFSIOReadBinary);
	IterateRIFF::IterateRIFF((IterateRIFF *)&v14, (struct InFileRIFF *)&v11, v13);
	v6 = 0;
	v15 = 0;
	*a6 = 0;
	if (IterateRIFF::ChunkAvailable((IterateRIFF *)&v14))
	{
		while (true)
		{
			v7 = IterateRIFF::ChunkName((IterateRIFF *)&v14);
			if (v7 == 544501094)
				break;
			if (v7 == 1413563478)
			{
				*a6 = 1;
				ParseSentence(a5, v10);
			}
			else if (v7 == 1635017060)
			{
				*a4 = IterateRIFF::ChunkSize((IterateRIFF *)&v14);
				v15 = 1;
			}
			if (v6)
				goto LABEL_7;
		LABEL_9:
			IterateRIFF::ChunkNext((IterateRIFF *)&v14);
			if (!IterateRIFF::ChunkAvailable((IterateRIFF *)&v14))
			{
				if (v6 && v15)
				{
					InFileRIFF::~InFileRIFF((InFileRIFF *)&v11);
					return 1;
				}
				goto LABEL_16;
			}
		}
		v9 = IterateRIFF::ChunkSize((IterateRIFF *)&v14);
		CUtlBuffer::SeekPut(a3, 0, v9);
		IterateRIFF::ChunkRead((IterateRIFF *)&v12, *(void **)a3);
		v6 = 1;
	LABEL_7:
		if (v15 && *a6)
		{
			InFileRIFF::~InFileRIFF((InFileRIFF *)&v11);
			return 1;
		}
		goto LABEL_9;
	}
LABEL_16:
	InFileRIFF::~InFileRIFF((InFileRIFF *)&v11);
	return 0;
}

bool __thiscall CSFMPhonemeExtractor::GetSentence(CSFMPhonemeExtractor *this, struct CDmeGameSound *a2, struct CSentence *a3)
{
	CSFMPhonemeExtractor *v3; // esi
	const char *v4; // eax
	char *v5; // eax
	bool result; // al
	bool v7; // [esp+29h] [ebp-435h]
	int v8; // [esp+2Ah] [ebp-434h]
	int v9; // [esp+2Eh] [ebp-430h]
	int v10; // [esp+36h] [ebp-428h]
	char v11; // [esp+5Eh] [ebp-400h]
	char v12; // [esp+25Eh] [ebp-200h]

	v3 = this;
	v4 = CUtlString::Get((struct CDmeGameSound *)((char *)a2 + 68));
	v5 = PSkipSoundChars(v4);
	V_snprintf(&v11, 512, "sound/%s", v5);
	V_FixSlashes(&v11, 92);
	(*(void(__stdcall **)(char *, const char *, char *, signed int, _DWORD, _DWORD))(*(_DWORD *)g_pFullFileSystem + 48))(
		&v11,
		"GAME",
		&v12,
		512,
		0,
		0);
	CUtlBuffer::CUtlBuffer((CUtlBuffer *)&v9, 0, 0, 0);
	v7 = 0;
	if (GetWaveFormat(v3, &v11, (struct CUtlBuffer *)&v9, &v8, a3, &v7))
	{
		if (v10 >= 0 && v9)
			(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v9);
		result = v7;
	}
	else
	{
		if (v10 >= 0)
		{
			if (v9)
				(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v9);
		}
		result = 0;
	}
	return result;
}

void __thiscall CSFMPhonemeExtractor::GetAPIInfo(CSFMPhonemeExtractor *this, int a2, struct CUtlString *a3, enum PE_APITYPE *a4)
{
	CSFMPhonemeExtractor *v4; // edi
	const char *v5; // eax

	v4 = this;
	v5 = (const char *)(*(int(**)(void))(**(_DWORD **)(*((_DWORD *)this + 1) + 12 * a2 + 8) + 4))();
	CUtlString::Set(a3, v5);
	*(_DWORD *)a4 = *(_DWORD *)(12 * a2 + *((_DWORD *)v4 + 1));
}

int __thiscall CSFMPhonemeExtractor::GetAPICount(CSFMPhonemeExtractor *this)
{
	return *((_DWORD *)this + 4);
}

int __stdcall CSFMPhonemeExtractor::FindFacialChannelsClip(_DWORD *a1)
{
	int v1; // ebx
	int v2; // esi
	int v3; // edi
	int v4; // eax
	int v5; // eax
	int v6; // eax
	int v7; // eax

	v1 = 0;
	v2 = a1[3] - 1;
	if (v2 < 0)
		goto LABEL_18;
	v3 = 28 * v2;
	do
	{
		v4 = (*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*(_DWORD *)(v3 + *a1 + 4));
		if (v4 && (v5 = v4 - 4) != 0)
			v6 = v5 + 4;
		else
			v6 = 0;
		v7 = FindAncestorReferencingElement<CDmeChannelsClip>(v6);
		if (v1)
		{
			if (v1 != v7)
				_Warning("Selected controls overlap multiple channels clips!!!\n");
		}
		else if (v7)
		{
			v1 = v7;
		}
		--v2;
		v3 -= 28;
	} while (v2 >= 0);
	if (!v1)
		LABEL_18:
	_Warning("Unable to determine destination channels clip!!!\n");
	return v1;
}

_DWORD *__stdcall CSFMPhonemeExtractor::BuildPhonemeLogList(_DWORD *a1, int *a2)
{
	_DWORD *result; // eax
	int *v3; // esi
	_DWORD *v4; // ebx
	int v5; // eax
	int v6; // eax
	CDmeChannel *v7; // ecx
	struct CDmeLog *v8; // ebp
	int v9; // edi
	int v10; // eax
	int v11; // ecx
	int v12; // eax
	bool v13; // zf
	struct CDmeLog **v14; // eax
	int v15; // [esp+8h] [ebp-8h]
	int v16; // [esp+Ch] [ebp-4h]
	signed int v17; // [esp+18h] [ebp+8h]

	result = a1;
	v16 = 0;
	if (a1[3] > 0)
	{
		v3 = a2;
		v15 = 0;
		do
		{
			v17 = 3;
			v4 = (_DWORD *)(v15 + *result + 4);
			do
			{
				v5 = (*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*v4);
				if (v5 && v5 != 4)
				{
					v6 = (*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*v4);
					v7 = (CDmeChannel *)(v6 ? v6 - 4 : 0);
					v8 = CDmeChannel::GetLog(v7);
					if (v8)
					{
						v9 = v3[3];
						v10 = v3[1];
						if (v9 + 1 > v10)
							CUtlMemory<CDmeLog *, int>::Grow(v9 - v10 + 1);
						++v3[3];
						v11 = *v3;
						v12 = v3[3] - v9 - 1;
						v13 = v3[3] - v9 == 1;
						v3[4] = *v3;
						if (v12 >= 0 && !v13)
							memmove((void *)(v11 + 4 * v9 + 4), (const void *)(v11 + 4 * v9), 4 * v12);
						v14 = (struct CDmeLog **)(*v3 + 4 * v9);
						if (v14)
							*v14 = v8;
					}
				}
				++v4;
				--v17;
			} while (v17);
			result = a1;
			v15 += 28;
			++v16;
		} while (v16 < a1[3]);
	}
	return result;
}

int __stdcall CSFMPhonemeExtractor::BuildPhonemeToPresetMapping(int a1, CDmeAnimationSet *a2, CDmePresetGroup *a3, int a4)
{
	int v4; // edi
	bool v5; // zf
	bool v6; // sf
	int v7; // esi
	unsigned __int16 v8; // ax
	const char *v9; // eax
	CDmElement *v10; // esi
	struct CDmAttribute *v11; // esi
	int *v12; // ecx
	const char *v13; // eax
	struct CDmePreset *v14; // ebp
	const char *v15; // eax
	char *v16; // eax
	unsigned __int16 v17; // ax
	unsigned __int16 v18; // dx
	int v19; // ecx
	int v20; // esi
	_WORD *v21; // eax
	int v22; // eax
	int v23; // edx
	int v24; // eax
	int v25; // eax
	char **v26; // eax
	int result; // eax
	char v28; // [esp+15h] [ebp-75h]
	int v29; // [esp+16h] [ebp-74h]
	unsigned int v30; // [esp+1Ah] [ebp-70h]
	char *v31; // [esp+1Eh] [ebp-6Ch]
	struct CDmePreset *v32; // [esp+22h] [ebp-68h]
	char *v33; // [esp+26h] [ebp-64h]
	int(__cdecl *v34)(int, int); // [esp+2Eh] [ebp-5Ch]
	int v35; // [esp+32h] [ebp-58h]
	int v36; // [esp+36h] [ebp-54h]
	int v37; // [esp+3Ah] [ebp-50h]
	__int16 v38; // [esp+3Eh] [ebp-4Ch]
	__int16 v39; // [esp+40h] [ebp-4Ah]
	__int16 v40; // [esp+42h] [ebp-48h]
	__int16 v41; // [esp+44h] [ebp-46h]
	int v42; // [esp+46h] [ebp-44h]
	char v43; // [esp+4Ah] [ebp-40h]
	char v44; // [esp+6Ah] [ebp-20h]

	v4 = 0;
	v5 = *(_DWORD *)(a1 + 12) == 0;
	v6 = *(_DWORD *)(a1 + 12) < 0;
	v34 = UniquePhonemeLessFunc;
	v35 = 0;
	v36 = 0;
	v37 = 0;
	v38 = -1;
	v39 = 0;
	v40 = -1;
	v41 = -1;
	v42 = 0;
	if (!v6 && !v5)
	{
		do
		{
			v29 = *(_DWORD *)(*(_DWORD *)a1 + 4 * v4);
			if ((unsigned __int16)CUtlRBTree<CBasePhonemeTag *, unsigned short, bool(__cdecl *)(CBasePhonemeTag * const &, CBasePhonemeTag * const &), CUtlMemory<UtlRBTreeNode_t<CBasePhonemeTag *, unsigned short>, unsigned short>>::Find(&v29) == -1)
			{
				CUtlRBTree<CBasePhonemeTag *, unsigned short, bool(__cdecl *)(CBasePhonemeTag * const &, CBasePhonemeTag * const &), CUtlMemory<UtlRBTreeNode_t<CBasePhonemeTag *, unsigned short>, unsigned short>>::FindInsertionPosition(
					&v29,
					&v31,
					&v30);
				v7 = (unsigned __int16)CUtlRBTree<CBasePhonemeTag *, unsigned short, bool(__cdecl *)(CBasePhonemeTag * const &, CBasePhonemeTag * const &), CUtlMemory<UtlRBTreeNode_t<CBasePhonemeTag *, unsigned short>, unsigned short>>::NewNode(&v34);
				CUtlRBTree<CBasePhonemeTag *, unsigned short, bool(__cdecl *)(CBasePhonemeTag * const &, CBasePhonemeTag * const &), CUtlMemory<UtlRBTreeNode_t<CBasePhonemeTag *, unsigned short>, unsigned short>>::LinkToParent(
					v7,
					(__int16)v31,
					v30);
				++v39;
				if (v35 + 12 * (unsigned __int16)v7 != -8)
					*(_DWORD *)(v35 + 12 * (unsigned __int16)v7 + 8) = v29;
			}
			++v4;
		} while (v4 < *(_DWORD *)(a1 + 12));
	}
	v8 = CUtlRBTree<CBasePhonemeTag *, unsigned short, bool(__cdecl *)(CBasePhonemeTag * const &, CBasePhonemeTag * const &), CUtlMemory<UtlRBTreeNode_t<CBasePhonemeTag *, unsigned short>, unsigned short>>::FirstInorder(&v34);
	v30 = v8;
	if (v8 != 0xFFFF)
	{
		while (1)
		{
			v9 = ConvertPhoneme(*(unsigned __int16 *)(*(_DWORD *)(v35 + 12 * v8 + 8) + 8));
			V_strncpy(&v43, v9, 32);
			V_snprintf(&v44, 32, "p_%s", &v43);
			v10 = CDmeAnimationSet::FindMapping(a2, &v43);
			if (v10)
			{
				if (!(`CDmElement::GetValue<CUtlString>'::`2'::`local static guard' & 1) )
				{
					`CDmElement::GetValue<CUtlString>'::`2'::`local static guard' |= 1u;
						CUtlString::CUtlString((CUtlString *)&`CDmElement::GetValue<CUtlString>'::`2'::defaultVal);
					dword_70A4 = 0;
					CUtlString::Set((CUtlString *)&`CDmElement::GetValue<CUtlString>'::`2'::defaultVal, 0);
					atexit(`CDmElement::GetValue<CUtlString>'::`2'::`dynamic atexit destructor for 'defaultVal'');
				}
				v11 = CDmElement::FindAttribute(v10, "preset");
				if (v11)
				{
					if (!(`CDmAttribute::GetValue<CUtlString>'::`2'::`local static guard' & 1) )
					{
						`CDmAttribute::GetValue<CUtlString>'::`2'::`local static guard' |= 1u;
							CUtlString::CUtlString((CUtlString *)&`CDmAttribute::GetValue<CUtlString>'::`2'::defaultVal);
						dword_6E78 = 0;
						CUtlString::Set((CUtlString *)&`CDmAttribute::GetValue<CUtlString>'::`2'::defaultVal, 0);
						atexit(`CDmAttribute::GetValue<CUtlString>'::`2'::`dynamic atexit destructor for 'defaultVal'');
					}
					if ((*((_BYTE *)v11 + 12) & 0x1F) == 5)
					{
						v12 = (int *)*((_DWORD *)v11 + 1);
					}
					else if ((unsigned __int8)CDmAttribute::IsTypeConvertable<CUtlString>(v11))
					{
						if (!(`CDmAttribute::GetValue<CUtlString>'::`7'::`local static guard' & 1) )
						{
							`CDmAttribute::GetValue<CUtlString>'::`7'::`local static guard' |= 1u;
								CUtlString::CUtlString((CUtlString *)&`CDmAttribute::GetValue<CUtlString>'::`7'::tempVal);
							atexit(`CDmAttribute::GetValue<CUtlString>'::`7'::`dynamic atexit destructor for 'tempVal'');
						}
						CDmAttribute::CopyDataOut<CUtlString>(v11, &`CDmAttribute::GetValue<CUtlString>'::`7'::tempVal);
						v12 = &`CDmAttribute::GetValue<CUtlString>'::`7'::tempVal;
					}
					else
					{
						v12 = &`CDmAttribute::GetValue<CUtlString>'::`2'::defaultVal;
					}
				}
				else
				{
					v12 = &`CDmElement::GetValue<CUtlString>'::`2'::defaultVal;
				}
				v13 = CUtlString::Get((CUtlString *)v12);
				V_strncpy(&v44, v13, 32);
			}
			v14 = CDmePresetGroup::FindPreset(a3, &v44);
			if (v14)
			{
				v33 = &v43;
				if ((unsigned __int16)CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>::Find(&v33) == -1)
				{
					v16 = (char *)(*(int(__stdcall **)(unsigned int))(*(_DWORD *)*_g_pMemAlloc + 4))(strlen(&v43) + 1);
					if (v16)
						strcpy(v16, &v43);
					else
						v16 = 0;
					v31 = v16;
					v32 = v14;
					CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>::FindInsertionPosition(
						&v31,
						&v29,
						&v28);
					v17 = CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>::NewNode(a4);
					v18 = v29;
					v19 = v17;
					v20 = 16 * v17;
					v21 = (_WORD *)(v20 + *(_DWORD *)(a4 + 4));
					v5 = (_WORD)v29 == -1;
					v21[2] = v29;
					v21[1] = -1;
					*v21 = -1;
					v21[3] = 0;
					if (v5)
					{
						*(_WORD *)(a4 + 16) = v19;
					}
					else
					{
						v22 = v18;
						v23 = *(_DWORD *)(a4 + 4);
						v24 = 16 * v22;
						if (v28)
							*(_WORD *)(v24 + v23) = v19;
						else
							*(_WORD *)(v24 + v23 + 2) = v19;
					}
					CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>::InsertRebalance(v19);
					v25 = *(_DWORD *)(a4 + 4);
					++*(_WORD *)(a4 + 18);
					v26 = (char **)(v20 + v25 + 8);
					if (v26)
					{
						*v26 = v31;
						v26[1] = (char *)v32;
					}
				}
			}
			else
			{
				v15 = CUtlString::Get((CDmeAnimationSet *)((char *)a2 + 4));
				_Warning("Animation set '%s' missing phoneme preset for '%s' -> '%s'\n", v15, &v43, &v44);
			}
			v30 = (unsigned __int16)CUtlRBTree<CBasePhonemeTag *, unsigned short, bool(__cdecl *)(CBasePhonemeTag * const &, CBasePhonemeTag * const &), CUtlMemory<UtlRBTreeNode_t<CBasePhonemeTag *, unsigned short>, unsigned short>>::NextInorder(v30);
			if (v30 == 0xFFFF)
				break;
			v8 = v30;
		}
	}
	result = CUtlRBTree<CBasePhonemeTag *, unsigned short, bool(__cdecl *)(CBasePhonemeTag * const &, CBasePhonemeTag * const &), CUtlMemory<UtlRBTreeNode_t<CBasePhonemeTag *, unsigned short>, unsigned short>>::RemoveAll(&v34);
	if (v37 >= 0)
	{
		result = v35;
		if (v35)
		{
			(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v35);
			result = 0;
		}
	}
	return result;
}

void __thiscall CSFMPhonemeExtractor::LogPhonemes(CSFMPhonemeExtractor *this, int a2, struct ExtractDesc_t *a3)
{
	float *v3; // edi
	CDmeAnimationSet *v4; // ecx
	const char *v5; // eax
	CDmeAnimationSet *v6; // ecx
	bool v7; // zf
	bool v8; // sf
	_DWORD *v9; // ebx
	int v10; // eax
	const char *v11; // eax
	unsigned __int8 v12; // of
	int v13; // ecx
	int v14; // ST3C_4
	int v15; // eax
	int v16; // edx
	__int64 v17; // kr00_8
	double v18; // st7
	int v19; // ebx
	double v20; // st7
	double v21; // st6
	int v22; // ebx
	int v23; // edi
	char *v24; // eax
	double v25; // st7
	float v26; // ST34_4
	int v27; // eax
	int v28; // eax
	double v29; // st7
	double v30; // st6
	double v31; // rt2
	double v32; // rtt
	double v33; // st6
	double v34; // st7
	double v35; // st6
	double v36; // st7
	int v37; // esi
	int v38; // edi
	int v39; // ebx
	int v40; // eax
	int i; // esi
	char *v42; // eax
	int j; // esi
	int v44; // esi
	int v45; // edi
	int v46; // eax
	int v47; // esi
	int v48; // eax
	int v49; // ebx
	float v50; // ST24_4
	int v51; // eax
	int v52; // edi
	int v53; // eax
	int *v54; // ecx
	int v55; // ecx
	int v56; // eax
	int *v57; // eax
	double v58; // st7
	double v59; // st6
	double v60; // st5
	double v61; // st4
	double v62; // st6
	double v63; // rt1
	double v64; // st5
	double v65; // st7
	int v66; // ST24_4
	const char *v67; // eax
	unsigned __int16 v68; // ax
	int v69; // eax
	int v70; // ebx
	int v71; // eax
	double v72; // st7
	float v73; // ST34_4
	const char *v74; // eax
	unsigned __int16 v75; // ax
	float v76; // ecx
	float v77; // ST34_4
	int v78; // eax
	int *v79; // eax
	char *v80; // [esp+38h] [ebp-108h]
	int *v81; // [esp+3Ch] [ebp-104h]
	float *v82; // [esp+40h] [ebp-100h]
	float v83; // [esp+44h] [ebp-FCh]
	int v84; // [esp+4Ch] [ebp-F4h]
	int v85; // [esp+50h] [ebp-F0h]
	char *v86; // [esp+54h] [ebp-ECh]
	int v87; // [esp+58h] [ebp-E8h]
	CDmePresetGroup *v88; // [esp+5Ch] [ebp-E4h]
	char *v89; // [esp+60h] [ebp-E0h]
	int v90; // [esp+64h] [ebp-DCh]
	char *v91; // [esp+68h] [ebp-D8h]
	float v92; // [esp+6Ch] [ebp-D4h]
	int v93; // [esp+70h] [ebp-D0h]
	int v94; // [esp+74h] [ebp-CCh]
	int v95; // [esp+78h] [ebp-C8h]
	int v96; // [esp+7Ch] [ebp-C4h]
	int v97[2]; // [esp+80h] [ebp-C0h]
	int v98[2]; // [esp+88h] [ebp-B8h]
	float v99; // [esp+90h] [ebp-B0h]
	int v100; // [esp+94h] [ebp-ACh]
	int v101; // [esp+98h] [ebp-A8h]
	int v102; // [esp+9Ch] [ebp-A4h]
	double v103; // [esp+A0h] [ebp-A0h]
	double v104; // [esp+A8h] [ebp-98h]
	int v105[2]; // [esp+B0h] [ebp-90h]
	int v106; // [esp+B8h] [ebp-88h]
	int v107; // [esp+BCh] [ebp-84h]
	int v108; // [esp+C0h] [ebp-80h]
	int v109; // [esp+C4h] [ebp-7Ch]
	int v110; // [esp+C8h] [ebp-78h]
	int v111; // [esp+CCh] [ebp-74h]
	int v112; // [esp+D0h] [ebp-70h]
	signed __int64 v113; // [esp+D4h] [ebp-6Ch]
	int v114; // [esp+DCh] [ebp-64h]
	int v115; // [esp+E0h] [ebp-60h]
	int v116; // [esp+E4h] [ebp-5Ch]
	signed __int64 v117; // [esp+E8h] [ebp-58h]
	int v118; // [esp+F0h] [ebp-50h]
	int v119; // [esp+F4h] [ebp-4Ch]
	double v120; // [esp+F8h] [ebp-48h]
	int v121; // [esp+100h] [ebp-40h]
	int v122; // [esp+104h] [ebp-3Ch]
	double v123; // [esp+108h] [ebp-38h]
	double v124; // [esp+110h] [ebp-30h]
	double v125; // [esp+118h] [ebp-28h]
	double v126; // [esp+120h] [ebp-20h]
	int v127; // [esp+128h] [ebp-18h]
	int v128; // [esp+12Ch] [ebp-14h]
	int v129; // [esp+134h] [ebp-Ch]

	v3 = (float *)(*((_DWORD *)a3 + 3) + 132 * a2);
	v89 = (char *)this;
	v4 = (CDmeAnimationSet *)*((_DWORD *)a3 + 15);
	v87 = (int)v3;
	if (v4 && *(_DWORD *)v3 && *((_DWORD *)v3 + 1))
	{
		v88 = CDmeAnimationSet::FindPresetGroup(v4, "phoneme");
		if (!v88)
		{
			v5 = CUtlString::Get((CUtlString *)(*((_DWORD *)a3 + 15) + 4));
			_Warning("Animation set '%s' missing preset group 'phoneme'\n", v5);
			return;
		}
		if (!*(_DWORD *)(CDmeAnimationSet::GetPhonemeMap(*((_DWORD *)a3 + 15)) + 12))
			CDmeAnimationSet::RestoreDefaultPhonemeMap(*((CDmeAnimationSet **)a3 + 15));
		v6 = (CDmeAnimationSet *)*((_DWORD *)a3 + 15);
		v118 = -1;
		v115 = 0;
		v116 = 0;
		v117 = 281470681743360i64;
		v119 = 0;
		v114 = (int)CaselessStringLessThan;
		BuildPhonemeToPresetMapping((int)(v3 + 28), v6, v88, (int)&v114);
		v91 = (char *)FindFacialChannelsClip((_DWORD *)a3 + 8);
		if (!v91)
			goto LABEL_18;
		v7 = *((_DWORD *)a3 + 11) == 0;
		v8 = *((_DWORD *)a3 + 11) < 0;
		v107 = 0;
		v108 = 0;
		v109 = 0;
		v110 = -1;
		v111 = 0;
		v112 = -1;
		v113 = 0xFFFFFFFFi64;
		v106 = (int)CaselessStringLessThan;
		v90 = 0;
		if (!v8 && !v7)
		{
			v92 = 0.0;
			do
			{
				v9 = (_DWORD *)(LODWORD(v92) + *((_DWORD *)a3 + 8));
				v10 = (*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*v9);
				v11 = CUtlString::Get((CUtlString *)(v10 + 4));
				v104 = COERCE_DOUBLE(__PAIR__((unsigned int)v9, (unsigned int)MemAlloc_StrDup(v11)));
				CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::Insert(&v104);
				LODWORD(v92) += 28;
				v12 = __OFSUB__(v90 + 1, *((_DWORD *)a3 + 11));
				v8 = v90++ + 1 - *((_DWORD *)a3 + 11) < 0;
			} while (v8 ^ v12);
		}
		if (!a2 && *((_DWORD *)a3 + 6) > 1)
			ClearInterstitialSpaces((int)v89, 0, (int)a3, (int)v91, (int)&v106, a3);
		v13 = *((_DWORD *)a3 + 13);
		v14 = *((_DWORD *)a3 + 14);
		*(float *)&v95 = 0.0;
		v96 = 0;
		v97[0] = 0;
		v97[1] = 0;
		v98[0] = 0;
		CDmeClip::BuildClipStack(*(_DWORD *)v3, &v95, v13, v14);
		if (!v97[1])
		{
			CDmeClip::BuildClipStack(*(_DWORD *)v3, &v95, *((_DWORD *)a3 + 13), 0);
			if (!v97[1])
			{
				_Msg("Couldn't build stack sound clip to current shot\n");
				CUtlVector<CDmeHandle<CDmeClip, 0>, CUtlMemory<CDmeHandle<CDmeClip, 0>, int>>::~CUtlVector<CDmeHandle<CDmeClip, 0>, CUtlMemory<CDmeHandle<CDmeClip, 0>, int>>(&v95);
				CUtlDict<LogPreview_t *, int>::RemoveAll(&v106);
				CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::~CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>(&v106);
			LABEL_18:
				CUtlDict<CDmePreset *, unsigned short>::RemoveAll(&v114);
				CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>::~CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>(&v114);
				return;
			}
		}
		CDmeClip::FromChildMediaTime(&v129, &v95, 0, 0);
		v15 = DmeTime_t::RoundSecondsToTMS(v3[26]);
		CDmeClip::FromChildMediaTime(&v127, &v95, v15, 0);
		v81 = &v128;
		CDmeClip::GetStartInChildMediaTime(*(_DWORD *)v3);
		v80 = (char *)&v124 + 4;
		CDmeClip::GetEndInChildMediaTime(*(_DWORD *)v3);
		v16 = *((_DWORD *)a3 + 14);
		v17 = *(_QWORD *)((char *)a3 + 52);
		v98[1] = 0;
		v99 = 0.0;
		v100 = 0;
		v101 = 0;
		v102 = 0;
		CDmeClip::BuildClipStack(v89, &v98[1], v17, HIDWORD(v17));
		CDmeClip::ToChildMediaTime(&v127, &v98[1], v128, 0);
		CDmeClip::ToChildMediaTime((char *)&v124 + 4, &v98[1], LODWORD(v126), 0);
		v18 = 0.0;
		if (0.0 != v3[26])
		{
			v86 = (char *)(HIDWORD(v124) - v127);
			v18 = (double)(HIDWORD(v124) - v127) * 0.000099999997 / v3[26];
		}
		v123 = v18;
		v90 = v127;
		v19 = 0;
		v119 = 0;
		v120 = 0.0;
		v121 = 0;
		v122 = 0;
		BuildPhonemeLogList((_DWORD *)a3 + 8, &v119);
		if (v121 > 0)
		{
			do
				(*(void(__cdecl **)(char *, int *))(**(_DWORD **)(v119 + 4 * v19++) + 180))(v80, v81);
			while (v19 < v121);
		}
		v20 = (double)SHIDWORD(v126) * 0.000099999997;
		if (v20 < 0.0)
			v20 = 0.0;
		v104 = v20;
		v21 = 0.000099999997 * (double)SLODWORD(v124);
		if (v21 > v3[26])
			v21 = v3[26];
		v103 = v21;
		v97[1] = DmeTime_t::RoundSecondsToTMS(v20 * v123) + v90 - 1;
		WriteCurrentValuesIntoLogLayers(v97[1], v105);
		v91 = (char *)(DmeTime_t::RoundSecondsToTMS(v103 * v123) + v90 + 1);
		WriteCurrentValuesIntoLogLayers(v91, v105);
		if (*((_BYTE *)a3 + 8))
		{
			AddAnimSetBookmarkAtSoundMediaTime("start", v97[1], v97[1], (int)&v93, (int)a3);
			v22 = 0;
			if (*((_DWORD *)v3 + 31) > 0)
			{
				do
				{
					v23 = *(_DWORD *)(*(_DWORD *)(v85 + 112) + 4 * v22);
					*(float *)&v89 = COERCE_FLOAT(ConvertPhoneme(*(unsigned __int16 *)(v23 + 8)));
					v24 = (char *)DmeTime_t::RoundSecondsToTMS(*(float *)v23);
					v25 = *(float *)(v23 + 4);
					v86 = v24;
					v26 = v25;
					v27 = DmeTime_t::RoundSecondsToTMS(v26);
					AddAnimSetBookmarkAtSoundMediaTime(v89, (int)v86, v27, (int)&v93, (int)a3);
					++v22;
				} while (v22 < *(_DWORD *)(v85 + 124));
			}
			AddAnimSetBookmarkAtSoundMediaTime("end", (int)v91, (int)v91, (int)&v93, (int)a3);
			v3 = (float *)v85;
		}
		v28 = *((_DWORD *)a3 + 1);
		if (v28 && v28 != 1)
		{
			v29 = 1000.0;
			v30 = 1.0;
			if (*((float *)a3 + 16) <= 1000.0)
			{
				if (*((float *)a3 + 16) >= 1.0)
					v30 = *((float *)a3 + 16);
				v31 = v30;
				v30 = 1.0;
				v29 = v31;
			}
			v32 = v30;
			v33 = v30 / v29;
			v34 = v32;
			v125 = v33;
			v35 = 0.001;
			if (*((float *)a3 + 17) > 0.001)
				v35 = *((float *)a3 + 17);
			*(float *)&v89 = v35;
			*(float *)v98 = v34 / v35;
			v36 = v104;
			if (v103 > v104)
			{
				do
				{
					v87 = DmeTime_t::RoundSecondsToTMS(v36);
					v46 = DmeTime_t::RoundSecondsToTMS(*(double *)&v101 * v120);
					v88 = (CDmePresetGroup *)&v86[v46];
					WriteDefaultValuesIntoLogLayers((int)&v86[v46], (int)&v103);
					v7 = *((_DWORD *)v3 + 31) == 0;
					v8 = *((_DWORD *)v3 + 31) < 0;
					v84 = 0;
					if (!v8 && !v7)
					{
						do
						{
							v47 = *(_DWORD *)(*((_DWORD *)v82 + 28) + 4 * v84);
							*(float *)&v48 = COERCE_FLOAT(DmeTime_t::RoundSecondsToTMS(*(float *)v47));
							v49 = v48;
							v50 = *(float *)(v47 + 4);
							v83 = *(float *)&v48;
							v51 = DmeTime_t::RoundSecondsToTMS(v50);
							v52 = v51;
							v94 = v51;
							v53 = DmeTime_t::RoundSecondsToTMS(*(float *)&v85);
							v54 = (int *)&v83;
							if (v49 <= v87)
								v54 = &v87;
							v55 = *v54;
							v56 = v87 + v53;
							if (v52 >= v56)
							{
								v115 = v56;
								v57 = &v115;
							}
							else
							{
								v57 = &v94;
							}
							LODWORD(v83) = *v57 - v87;
							v58 = (double)(v55 - v87) * 0.000099999997 * *(float *)&v95;
							v59 = 0.000099999997 * (double)SLODWORD(v83) * *(float *)&v95;
							v60 = 1.0;
							if (v58 < 1.0 && v59 > 0.0)
							{
								v61 = v59;
								v62 = 0.0;
								if (v61 <= 1.0)
									v60 = v61;
								v63 = v60;
								v64 = v58;
								v65 = v63;
								if (v64 >= 0.0)
									v62 = v64;
								v66 = *(unsigned __int16 *)(v47 + 8);
								v83 = v65 - v62;
								v67 = ConvertPhoneme(v66);
								if (v67)
								{
									LODWORD(v126) = v67;
									v68 = CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>::Find(&v126);
								}
								else
								{
									v68 = -1;
								}
								if (v68 != 0xFFFF)
								{
									v69 = *(_DWORD *)(16 * v68 + v110 + 12);
									if (v69)
										StampControlValueLogs(v69, (int)v88, v83, (int)&v103);
								}
							}
							v12 = __OFSUB__(v84 + 1, *((_DWORD *)v82 + 31));
							v8 = v84++ + 1 - *((_DWORD *)v82 + 31) < 0;
						} while (v8 ^ v12);
						v3 = v82;
					}
					v36 = *(double *)&v101 + v123;
					*(double *)&v101 = v36;
				} while (*(double *)&v99 > v36);
			}
		}
		else
		{
			v7 = *((_DWORD *)v3 + 31) == 0;
			v8 = *((_DWORD *)v3 + 31) < 0;
			v86 = 0;
			v88 = 0;
			if (!v8 && !v7)
			{
				do
				{
					v70 = *(_DWORD *)(*((_DWORD *)v3 + 28) + 4 * (_DWORD)v88);
					*(float *)&v71 = COERCE_FLOAT(DmeTime_t::RoundSecondsToTMS(*(float *)v70));
					v72 = *(float *)(v70 + 4);
					v92 = *(float *)&v71;
					v73 = v72;
					v98[0] = DmeTime_t::RoundSecondsToTMS(v73);
					v74 = ConvertPhoneme(*(unsigned __int16 *)(v70 + 8));
					if (v74)
					{
						LODWORD(v125) = v74;
						v75 = CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>::Find(&v125);
					}
					else
					{
						v75 = -1;
					}
					if (v75 != 0xFFFF)
					{
						v89 = *(char **)(16 * v75 + HIDWORD(v113) + 12);
						if (*(float *)&v89 != 0.0)
						{
							v76 = v92;
							v92 = v123;
							v77 = v123;
							v85 = LODWORD(v76);
							DmeTime_t::operator*=(&v85, &v118, LODWORD(v77));
							v78 = v90 + v85;
							v7 = *((_DWORD *)a3 + 1) == 0;
							v85 += v90;
							if (v7)
							{
								if (v78 - 1 > v97[1])
								{
									WriteDefaultValuesIntoLogLayers(v85 - 1, (int)v105);
									if (v86)
										StampControlValueLogs((int)v86, v85 - 1, 1.0, (int)v105);
								}
								v86 = v89;
							}
							WriteDefaultValuesIntoLogLayers(v85, (int)v105);
							StampControlValueLogs((int)v89, v85, 1.0, (int)v105);
							if (!*((_DWORD *)a3 + 1) && v88 == (CDmePresetGroup *)(*((_DWORD *)v3 + 31) - 1))
							{
								v85 = v98[0];
								DmeTime_t::operator*=(&v85, &v104, LODWORD(v92));
								v12 = __OFSUB__(v85 + v90, v91);
								v8 = v85 + v90 - (signed int)v91 < 0;
								v85 += v90;
								v79 = &v85;
								if (!(v8 ^ v12))
									v79 = (int *)&v91;
								v85 = *v79;
								WriteDefaultValuesIntoLogLayers(v85 - 1, (int)v105);
								StampControlValueLogs((int)v89, v85 - 1, 1.0, (int)v105);
								WriteDefaultValuesIntoLogLayers(v85, (int)v105);
							}
						}
					}
					v12 = __OFSUB__((char *)v88 + 1, *((_DWORD *)v3 + 31));
					v8 = (signed int)((char *)v88 - *((_DWORD *)v3 + 31) + 1) < 0;
					v88 = (CDmePresetGroup *)((char *)v88 + 1);
				} while (v8 ^ v12);
			}
		}
		v37 = 0;
		if (v121 > 0)
		{
			v38 = v121;
			v39 = v119;
			do
				(*(void(__stdcall **)(_DWORD, signed int))(**(_DWORD **)(v39 + 4 * v37++) + 184))(0.000099999997, 1);
			while (v37 < v38);
		}
		if (v120 >= 0.0 && v119)
			(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v119);
		v40 = v98[1];
		for (i = v101 - 1; i >= 0; --i)
		{
			if (g_pDataModel)
			{
				CDmeElementRefHelper::Unref(*(_DWORD *)(v40 + 4 * i), 0);
				v40 = v97[1];
			}
		}
		v99 = 0.0;
		if (v98[1] >= 0)
		{
			if (v40)
			{
				(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v40);
				v40 = 0;
				v97[1] = 0;
			}
			v98[0] = 0;
		}
		v100 = v40;
		if (v98[1] >= 0)
		{
			if (v40)
			{
				(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v40);
				v97[1] = 0;
			}
			v98[0] = 0;
		}
		v42 = v91;
		for (j = v94 - 1; j >= 0; --j)
		{
			if (g_pDataModel)
			{
				CDmeElementRefHelper::Unref(*(_DWORD *)&v42[4 * j], 0);
				v42 = v89;
			}
		}
		v92 = 0.0;
		if ((signed int)v91 >= 0)
		{
			if (v42)
			{
				(*(void(__stdcall **)(char *))(*(_DWORD *)*_g_pMemAlloc + 20))(v42);
				v42 = 0;
				*(float *)&v89 = 0.0;
			}
			v90 = 0;
		}
		v93 = (int)v42;
		if ((signed int)v91 >= 0)
		{
			if (v42)
			{
				(*(void(__stdcall **)(char *))(*(_DWORD *)*_g_pMemAlloc + 20))(v42);
				*(float *)&v89 = 0.0;
			}
			v90 = 0;
		}
		v44 = CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::FirstInorder(&v103);
		if (v44 != -1)
		{
			v45 = HIDWORD(v103);
			do
			{
				(*(void(__stdcall **)(_DWORD))(*(_DWORD *)*_g_pMemAlloc + 20))(*(_DWORD *)(v45 + 24 * v44 + 16));
				v44 = CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::NextInorder(v44);
			} while (v44 != -1);
		}
		CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::RemoveAll(&v103);
		CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::RemoveAll(&v103);
		if (v104 >= 0.0 && HIDWORD(v103))
			(*(void(__stdcall **)(_DWORD))(*(_DWORD *)*_g_pMemAlloc + 20))(HIDWORD(v103));
		CUtlDict<CDmePreset *, unsigned short>::RemoveAll(&v109);
		CUtlRBTree<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short, CUtlMap<char const *, CDmePreset *, unsigned short>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, CDmePreset *, unsigned short>::Node_t, unsigned short>, unsigned short>>::RemoveAll(&v109);
		if (v112 >= 0)
		{
			if (v110)
				(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v110);
		}
	}
}

void __thiscall CSFMPhonemeExtractor::ReApply(CSFMPhonemeExtractor *this, struct ExtractDesc_t *a2)
{
	CSFMPhonemeExtractor *v2; // ebx
	int v3; // eax
	int i; // esi

	v2 = this;
	if (*((_BYTE *)a2 + 8))
	{
		v3 = CDmeAnimationSet::GetBookmarks(*((_DWORD *)a2 + 15));
		CDmaArrayBase<enum  DmElementHandle_t, CDmaDataInternal<CUtlVector<enum  DmElementHandle_t, CUtlMemory<enum  DmElementHandle_t, int>>>>::RemoveAll(v3);
	}
	for (i = 0; i < *((_DWORD *)a2 + 6); ++i)
		LogPhonemes(v2, i, a2);
}

void __userpurge CSFMPhonemeExtractor::Extract(CSFMPhonemeExtractor *this@<ecx > , struct CSentence *a2@<edi > , const enum PE_APITYPE *a3, struct ExtractDesc_t *a4, bool a5)
{
	struct ExtractDesc_t *v5; // ebx
	CSFMPhonemeExtractor *v6; // ebp
	int v7; // edx
	int v8; // eax
	int v9; // esi
	_DWORD *v10; // ecx
	int v11; // edi
	const char *v12; // eax
	const char *v13; // eax
	const char *v14; // eax
	char *v15; // eax
	unsigned __int16 v16; // ax
	double v17; // st7
	signed int v18; // ebx
	int v19; // esi
	int v20; // ecx
	int v21; // esi
	const char *v22; // eax
	double v23; // st7
	double v24; // st7
	int i; // esi
	int v26; // eax
	double v27; // st5
	int v28; // edx
	bool v29; // zf
	bool v30; // sf
	int v31; // ecx
	int j; // esi
	int v33; // eax
	int k; // esi
	struct CSentence *v35; // [esp+38h] [ebp-500h]
	float v36; // [esp+48h] [ebp-4F0h]
	float v37; // [esp+4Ch] [ebp-4ECh]
	int v38; // [esp+54h] [ebp-4E4h]
	int v39; // [esp+58h] [ebp-4E0h]
	CSFMPhonemeExtractor *v40; // [esp+5Ch] [ebp-4DCh]
	int v41; // [esp+60h] [ebp-4D8h]
	int v42; // [esp+64h] [ebp-4D4h]
	int v43; // [esp+68h] [ebp-4D0h]
	int v44; // [esp+6Ch] [ebp-4CCh]
	int v45; // [esp+70h] [ebp-4C8h]
	int v46; // [esp+74h] [ebp-4C4h]
	int v47; // [esp+78h] [ebp-4C0h]
	char v48; // [esp+A0h] [ebp-498h]
	int v49; // [esp+A4h] [ebp-494h]
	int v50; // [esp+B0h] [ebp-488h]
	char v51; // [esp+ECh] [ebp-44Ch]
	char v52; // [esp+138h] [ebp-400h]
	char v53; // [esp+338h] [ebp-200h]
	char v54; // [esp+344h] [ebp-1F4h]

	v5 = a4;
	v6 = this;
	v40 = this;
	if (*((_DWORD *)a4 + 15))
	{
		v7 = *((_DWORD *)this + 4);
		v8 = 0;
		if (v7 > 0)
		{
			v9 = *((_DWORD *)this + 1);
			v35 = a2;
			v10 = (_DWORD *)*((_DWORD *)this + 1);
			while (*v10 != *(_DWORD *)a3)
			{
				++v8;
				v10 += 3;
				if (v8 >= v7)
					return;
			}
			if (v8 != -1)
			{
				v44 = v9 + 12 * v8;
				v42 = 0;
				if (*((_DWORD *)a4 + 6) > 0)
				{
					v38 = 0;
					do
					{
						v11 = v38 + *((_DWORD *)v5 + 3);
						*(float *)(v11 + 104) = 0.0;
						CSentence::CSentence((CSentence *)&v51);
						CSentence::CSentence((CSentence *)&v48);
						v12 = CUtlString::Get((CUtlString *)(v11 + 8));
						CSentence::SetText((CSentence *)&v51, v12);
						v13 = CUtlString::Get((CUtlString *)(v11 + 8));
						CSentence::SetText((CSentence *)&v48, v13);
						v14 = CUtlString::Get((CUtlString *)(*(_DWORD *)(v11 + 4) + 68));
						v15 = PSkipSoundChars(v14);
						V_snprintf(&v52, 512, "sound/%s", v15);
						V_FixSlashes(&v52, 92);
						(*(void(__stdcall **)(char *, const char *, char *, signed int, _DWORD, _DWORD))(*(_DWORD *)g_pFullFileSystem
							+ 48))(
								&v52,
								"GAME",
								&v53,
								512,
								0,
								0);
						CUtlBuffer::CUtlBuffer((CUtlBuffer *)&v45, 0, 0, 0);
						if (GetWaveFormat(
							v6,
							&v52,
							(struct CUtlBuffer *)&v45,
							&v43,
							(struct CSentence *)(v11 + 28),
							(bool *)(v11 + 108)))
						{
							v16 = *(_WORD *)(v45 + 14);
							if (v16 > 8u)
							{
								v17 = (double)*(unsigned int *)(v45 + 4);
								v18 = v16;
								v36 = v17;
								v41 = *(unsigned __int16 *)(v45 + 2);
								v37 = (double)(v16 * v41 >> 3);
								v39 = v43 / ((signed int)v16 >> 3);
								if (*(_WORD *)v45 == 2)
								{
									v19 = *(unsigned __int16 *)(v45 + 18);
									v37 = 0.5;
									v20 = 7 * *(unsigned __int16 *)(v45 + 2) + *(unsigned __int16 *)(v45 + 2) * (v19 - 2) / 2;
									v18 = 16;
									v21 = v43 / v20 * v19;
									v39 = v21;
									if (v43 % v20)
										v39 = 2 * (v43 % v20 - v20) / v41 + v21 + *(unsigned __int16 *)(v45 + 18);
								}
								if (v17 > 0.0)
									*(float *)(v11 + 104) = (double)v39 / v17;
								v22 = CUtlString::Get((CUtlString *)(v11 + 8));
								CSentence::CreateEventWordDistribution((CSentence *)&v51, v22, *(float *)(v11 + 104));
								if (*(_BYTE *)(v11 + 24) && *(_BYTE *)(v11 + 108))
								{
									_Msg("Using .wav file phonemes for (%s)\n", &v52);
									CSentence::operator=(&v48, v11 + 28);
								}
								else
								{
									(*(void(__thiscall **)(_DWORD, char *, signed int))(**(_DWORD **)(v44 + 8) + 8))(
										*(_DWORD *)(v44 + 8),
										&v54,
										(signed int)(v37 * v36 * *(float *)(v11 + 104)));
									if (*(_DWORD *)a3 != 1 || v41 != 2 || v18 != 16)
										v23 = v37;
									else
										v23 = v37 + v37;
									v24 = v23 * v36;
									for (i = 0; i < v50; ++i)
									{
										v26 = *(_DWORD *)(v49 + 4 * i);
										if (v26)
										{
											v27 = 1.0 / v24;
											*(float *)v26 = (double)*(unsigned int *)(v26 + 32) * (1.0 / v24);
											v28 = 0;
											v29 = *(_DWORD *)(v26 + 20) == 0;
											v30 = *(_DWORD *)(v26 + 20) < 0;
											*(float *)(v26 + 4) = (double)*(unsigned int *)(v26 + 36) * (1.0 / v24);
											if (!v30 && !v29)
											{
												do
												{
													v31 = *(_DWORD *)(*(_DWORD *)(v26 + 8) + 4 * v28);
													if (v31)
													{
														*(float *)v31 = (double)*(unsigned int *)(v31 + 16) * v27;
														*(float *)(v31 + 4) = (double)*(unsigned int *)(v31 + 20) * v27;
													}
													++v28;
												} while (v28 < *(_DWORD *)(v26 + 20));
											}
										}
									}
									if (a5)
										SaveSentenceToWavFile(&v48, v35);
								}
								for (j = 0; j < *(_DWORD *)(v11 + 124); ++j)
									operator delete(*(void **)(*(_DWORD *)(v11 + 112) + 4 * j));
								*(_DWORD *)(v11 + 124) = 0;
								BuildPhonemeStream(&v48);
								if (v47 >= 0)
								{
									if (v45)
									{
										(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v45);
										v45 = 0;
									}
									v46 = 0;
								}
								CSentence::~CSentence((CSentence *)&v48);
								CSentence::~CSentence((CSentence *)&v51);
								v5 = a4;
							}
							else
							{
								_Warning("Cannot extract phonemes from '%s', %u bits per sample.\n", &v52, v16);
								if (v47 >= 0)
								{
									if (v45)
									{
										(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v45);
										v45 = 0;
									}
									v46 = 0;
								}
								CSentence::~CSentence((CSentence *)&v48);
								CSentence::~CSentence((CSentence *)&v51);
							}
							v6 = v40;
						}
						else
						{
							if (v47 >= 0)
							{
								if (v45)
								{
									(*(void(__stdcall **)(int))(*(_DWORD *)*_g_pMemAlloc + 20))(v45);
									v45 = 0;
								}
								v46 = 0;
							}
							CSentence::~CSentence((CSentence *)&v48);
							CSentence::~CSentence((CSentence *)&v51);
						}
						v38 += 132;
						++v42;
					} while (v42 < *((_DWORD *)v5 + 6));
				}
				if (*((_BYTE *)v5 + 8))
				{
					v33 = CDmeAnimationSet::GetBookmarks(*((_DWORD *)v5 + 15));
					CDmaArrayBase<enum  DmElementHandle_t, CDmaDataInternal<CUtlVector<enum  DmElementHandle_t, CUtlMemory<enum  DmElementHandle_t, int>>>>::RemoveAll(v33);
				}
				for (k = 0; k < *((_DWORD *)v5 + 6); ++k)
					LogPhonemes(v6, k, v5);
			}
		}
	}
}

int __stdcall CSFMPhonemeExtractor::WriteDefaultValuesIntoLogLayers(int a1, int a2)
{
	int result; // eax
	int i; // ebp
	_DWORD *v4; // esi
	const char **v5; // edi
	_DWORD *v6; // ebx
	int v7; // eax
	CDmeLog *v8; // esi
	int v9; // eax
	int v10; // esi
	const char *v11; // eax
	struct CDmAttribute *v12; // eax
	float *v13; // eax
	CDmElement *v14; // [esp+10h] [ebp-8h]
	float v15; // [esp+14h] [ebp-4h]

	result = CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::FirstInorder(a2);
	for (i = result; result != -1; i = result)
	{
		v4 = *(_DWORD **)(*(_DWORD *)(a2 + 4) + 24 * i + 20);
		v14 = (CDmElement *)(*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*v4);
		v5 = (const char **)s_pDefaultAttributeValueNames;
		v6 = v4 + 1;
		do
		{
			v7 = (*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*v6);
			if (v7)
			{
				if (v7 != 4)
				{
					v8 = CDmeChannel::GetLog((CDmeChannel *)(v7 - 4));
					if (v8)
					{
						if ((*(unsigned __int8(__thiscall **)(CDmeLog *, _DWORD))(*(_DWORD *)v8 + 20))(
							v8,
							CDmeTypedLog<float>::m_classType))
						{
							v9 = CDmeLog::GetTopmostLayer(v8);
							v10 = CDmeTypedLog<float>::GetLayer(v8, v9);
							if (v10)
							{
								v11 = *v5;
								if (!(`CDmElement::GetValue<float>'::`2'::`local static guard' & 1) )
								{
									`CDmElement::GetValue<float>'::`2'::`local static guard' |= 1u;
										`CDmElement::GetValue<float>'::`2'::defaultVal = 0.0;
									dword_3C24 = 0;
								}
								v12 = CDmElement::FindAttribute(v14, v11);
								if (v12)
									v13 = (float *)CDmAttribute::GetValue<float>(v12);
								else
									v13 = &`CDmElement::GetValue<float>'::`2'::defaultVal;
								v15 = *v13;
								CDmeTypedLogLayer<float>::InsertKey(v10, a1, &v15, 0);
							}
						}
					}
				}
			}
			++v5;
			++v6;
		} while ((signed int)v5 < (signed int)&sfm_phonemeextractor);
		result = CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::NextInorder(i);
	}
	return result;
}

int __stdcall CSFMPhonemeExtractor::WriteCurrentValuesIntoLogLayers(int a1, int a2)
{
	int v2; // esi
	int result; // eax
	int i; // ebx
	_DWORD *v5; // edi
	signed int v6; // ebp
	int v7; // eax
	CDmeLog *v8; // esi
	int v9; // eax
	int v10; // esi
	float v11; // [esp+10h] [ebp-4h]

	v2 = a2;
	result = CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::FirstInorder(a2);
	for (i = result; result != -1; i = result)
	{
		v5 = (_DWORD *)(*(_DWORD *)(*(_DWORD *)(v2 + 4) + 24 * i + 20) + 4);
		v6 = 3;
		do
		{
			v7 = (*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*v5);
			if (v7)
			{
				if (v7 != 4)
				{
					v8 = CDmeChannel::GetLog((CDmeChannel *)(v7 - 4));
					if (v8)
					{
						if ((*(unsigned __int8(__thiscall **)(CDmeLog *, _DWORD))(*(_DWORD *)v8 + 20))(
							v8,
							CDmeTypedLog<float>::m_classType))
						{
							v9 = CDmeLog::GetTopmostLayer(v8);
							v10 = CDmeTypedLog<float>::GetLayer(v8, v9);
							if (v10)
							{
								v11 = *(float *)CDmeTypedLogLayer<float>::GetValue(v10, a1);
								CDmeTypedLogLayer<float>::InsertKey(v10, a1, &v11, 0);
							}
						}
					}
				}
			}
			++v5;
			--v6;
		} while (v6);
		v2 = a2;
		result = CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::NextInorder(i);
	}
	return result;
}

_DWORD *__stdcall CSFMPhonemeExtractor::StampControlValueLogs(int a1, int a2, float a3, int a4)
{
	int v4; // esi
	int v5; // eax
	int v6; // edi
	_DWORD *result; // eax
	int v8; // ebp
	const char *v9; // eax
	int v10; // eax
	_DWORD *v11; // ebx
	signed int v12; // edi
	int v13; // eax
	CDmeChannel *v14; // esi
	CDmeLog *v15; // esi
	int v16; // eax
	int v17; // esi
	float v18; // ST24_4
	float *v19; // eax
	int v20; // [esp+28h] [ebp-1Ch]
	_DWORD *v21; // [esp+2Ch] [ebp-18h]
	float v22; // [esp+30h] [ebp-14h]
	_DWORD *v23; // [esp+34h] [ebp-10h]
	int v24; // [esp+38h] [ebp-Ch]
	const char *v25; // [esp+3Ch] [ebp-8h]
	_DWORD *v26; // [esp+48h] [ebp+4h]

	v4 = CDmePreset::GetControlValues(a1);
	CDmaArrayConstBase<enum  DmElementHandle_t, CDmaDataExternal<CUtlVector<enum  DmElementHandle_t, CUtlMemory<enum  DmElementHandle_t, int>>>>::CDmaArrayConstBase<enum  DmElementHandle_t, CDmaDataExternal<CUtlVector<enum  DmElementHandle_t, CUtlMemory<enum  DmElementHandle_t, int>>>>(&v23);
	v5 = *(_DWORD *)(v4 + 24);
	v6 = 0;
	if (v5 && (*(_BYTE *)(v5 + 12) & 0x1F) == 15)
	{
		v24 = *(_DWORD *)(v4 + 24);
		result = *(_DWORD **)(v5 + 4);
	}
	else
	{
		v24 = 0;
		result = 0;
	}
	v23 = result;
	v20 = 0;
	if (result[3] > 0)
	{
		do
		{
			v8 = (*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*(_DWORD *)(*result + 4 * v6));
			if (v8)
			{
				if ((*(unsigned __int8(__thiscall **)(int, _DWORD))(*(_DWORD *)v8 + 20))(v8, CDmElement::m_classType))
				{
					v9 = CUtlString::Get((CUtlString *)(v8 + 4));
					if (v9)
					{
						v25 = v9;
						v10 = CUtlRBTree<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int, CUtlMap<char const *, LogPreview_t *, int>::CKeyLess, CUtlMemory<UtlRBTreeNode_t<CUtlMap<char const *, LogPreview_t *, int>::Node_t, int>, int>>::Find(&v25);
						if (v10 != -1)
						{
							v11 = *(_DWORD **)(*(_DWORD *)(a4 + 4) + 24 * v10 + 20);
							v21 = v11;
							v26 = v11 + 1;
							v12 = 0;
							while (1)
							{
								v13 = (*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*v26);
								if (v13)
								{
									v14 = (CDmeChannel *)(v13 - 4);
									if (v13 != 4)
									{
										if ((*(int(__stdcall **)(_DWORD))(*(_DWORD *)g_pDataModel + 56))(*v11))
										{
											v15 = CDmeChannel::GetLog(v14);
											if (v15)
											{
												if ((*(unsigned __int8(__thiscall **)(CDmeLog *, _DWORD))(*(_DWORD *)v15 + 20))(
													v15,
													CDmeTypedLog<float>::m_classType))
												{
													v16 = CDmeLog::GetTopmostLayer(v15);
													v17 = CDmeTypedLog<float>::GetLayer(v15, v16);
													if (v17)
													{
														v18 = *(float *)CDmElement::GetValue<float>(s_pDefaultAttributeValueNames[v12]);
														v22 = (*(float *)CDmElement::GetValue<float>(s_pAttributeValueNames[v12]) - v18) * a3;
														v19 = (float *)CDmeTypedLogLayer<float>::GetValue(v17, a2);
														v22 = v18 + *v19 - v18 + v22;
														CDmeTypedLogLayer<float>::InsertKey(v17, a2, &v22, 0);
													}
												}
											}
										}
									}
								}
								++v26;
								++v12;
								if (v12 >= 3)
									break;
								v11 = v21;
							}
							v6 = v20;
						}
					}
				}
			}
			result = v23;
			v20 = ++v6;
		} while (v6 < v23[3]);
	}
	return result;
}

void __userpurge CSFMPhonemeExtractor::ClearInterstitialSpaces(int a1@<ecx > , int a2@<ebx > , int a3@<esi > , int a4, int a5, _DWORD *a6)
{
	bool v6; // zf
	bool v7; // sf
	float *v8; // esi
	int v9; // ST28_4
	int v10; // ST24_4
	int v11; // eax
	double v12; // st7
	int v13; // esi
	float v14; // ST28_4
	int v15; // ebx
	CDmeLog *v16; // esi
	int v17; // eax
	struct CDmeLogLayer *v18; // eax
	CDmeLogLayer *v19; // ebx
	int j; // esi
	int v21; // [esp+2Ch] [ebp-88h]
	int v22; // [esp+30h] [ebp-84h]
	int v23; // [esp+34h] [ebp-80h]
	int v24; // [esp+34h] [ebp-80h]
	int v25; // [esp+38h] [ebp-7Ch]
	int v26; // [esp+38h] [ebp-7Ch]
	signed int v27; // [esp+3Ch] [ebp-78h]
	int i; // [esp+3Ch] [ebp-78h]
	signed int v29; // [esp+40h] [ebp-74h]
	int v30; // [esp+44h] [ebp-70h]
	int v31; // [esp+48h] [ebp-6Ch]
	int v32; // [esp+4Ch] [ebp-68h]
	int v33; // [esp+50h] [ebp-64h]
	int v34; // [esp+54h] [ebp-60h]
	int v35; // [esp+58h] [ebp-5Ch]
	int v36; // [esp+5Ch] [ebp-58h]
	int v37; // [esp+60h] [ebp-54h]
	float v38; // [esp+64h] [ebp-50h]
	float v39; // [esp+6Ch] [ebp-48h]
	char v40; // [esp+70h] [ebp-44h]
	int v41; // [esp+78h] [ebp-3Ch]
	int v42; // [esp+7Ch] [ebp-38h]
	int v43; // [esp+80h] [ebp-34h]
	int v44; // [esp+84h] [ebp-30h]
	int v45; // [esp+88h] [ebp-2Ch]
	int v46; // [esp+8Ch] [ebp-28h]
	int v47; // [esp+90h] [ebp-24h]
	int v48; // [esp+94h] [ebp-20h]
	int v49; // [esp+98h] [ebp-1Ch]
	int v50; // [esp+9Ch] [ebp-18h]
	int v51; // [esp+A0h] [ebp-14h]
	int v52; // [esp+A4h] [ebp-10h]
	int v53; // [esp+A8h] [ebp-Ch]
	int v54; // [esp+ACh] [ebp-8h]
	int v55; // [esp+B0h] [ebp-4h]
	void *retaddr; // [esp+B4h] [ebp+0h]

	v6 = a6[6] == 0;
	v33 = a1;
	if (!v6 && *a6 != 2)
	{
		v22 = a2;
		v21 = a3;
		v41 = 0;
		v42 = 0;
		v43 = 0;
		v44 = 0;
		v45 = 0;
		BuildPhonemeLogList(a6 + 8, &v41);
		v6 = a6[6] == 0;
		v7 = a6[6] < 0;
		v27 = 0x7FFFFFFF;
		v29 = -2147483647;
		v30 = 0;
		if (!v7 && !v6)
		{
			v31 = 0;
			do
			{
				v8 = (float *)(v31 + a6[3]);
				v9 = a6[14];
				v10 = a6[13];
				v51 = 0;
				v52 = 0;
				v53 = 0;
				v54 = 0;
				v55 = 0;
				v46 = 0;
				v47 = 0;
				v48 = 0;
				v49 = 0;
				v50 = 0;
				CDmeClip::BuildClipStack(*(_DWORD *)v8, &v51, v10, v9, v21, v22);
				CDmeClip::FromChildMediaTime(&v36, &v53, 0, 0);
				v11 = DmeTime_t::RoundSecondsToTMS(v8[26]);
				CDmeClip::FromChildMediaTime(&v37, &v53, v11, 0);
				CDmeClip::GetStartInChildMediaTime(*(_DWORD *)v8);
				CDmeClip::GetEndInChildMediaTime(*(_DWORD *)v8);
				CDmeClip::BuildClipStack(a4, &v46, a6[13], a6[14], &v39, &v39);
				CDmeClip::ToChildMediaTime(&v36, &v46, v34, 0);
				CDmeClip::ToChildMediaTime(&v37, &v46, v35, 0);
				v12 = 0.0;
				if (0.0 != v8[26])
				{
					v32 = v37 - v36;
					v12 = (double)(v37 - v36) * 0.000099999997 / v8[26];
				}
				v39 = v12;
				v13 = v36;
				v14 = v12;
				*(float *)&v32 = v38;
				DmeTime_t::operator*=(&v32, &v40, LODWORD(v14));
				v15 = v30 + v13;
				v30 = v37;
				DmeTime_t::operator*=(&v30, &v39, LODWORD(v38));
				if (v15 < v23)
					v23 = v15;
				if (v30 + v13 > v25)
					v25 = v30 + v13;
				CUtlVector<CDmeHandle<CDmeClip, 0>, CUtlMemory<CDmeHandle<CDmeClip, 0>, int>>::~CUtlVector<CDmeHandle<CDmeClip, 0>, CUtlMemory<CDmeHandle<CDmeClip, 0>, int>>(&v44);
				CUtlVector<CDmeHandle<CDmeClip, 0>, CUtlMemory<CDmeHandle<CDmeClip, 0>, int>>::~CUtlVector<CDmeHandle<CDmeClip, 0>, CUtlMemory<CDmeHandle<CDmeClip, 0>, int>>(&v49);
				v29 += 132;
				++v27;
			} while (v27 < a6[6]);
		}
		v24 = v23 - 1;
		v26 = v25 + 1;
		for (i = 0; i < v42; ++i)
		{
			v16 = *(CDmeLog **)(*(_DWORD *)&v40 + 4 * i);
			v17 = CDmeLog::GetTopmostLayer(*(CDmeLog **)(*(_DWORD *)&v40 + 4 * i));
			v18 = CDmeLog::GetLayer(v16, v17);
			v19 = v18;
			if (*a6)
			{
				(*(void(__thiscall **)(struct CDmeLogLayer *))(*(_DWORD *)v18 + 92))(v18);
			}
			else
			{
				WriteDefaultValuesIntoLogLayers(v24, retaddr);
				WriteDefaultValuesIntoLogLayers(v26, retaddr);
				for (j = CDmeLogLayer::GetKeyCount(v19) - 1; j >= 0; --j)
				{
					CDmeLogLayer::GetKeyTime(v19, &v38, j);
					if (v36 > v21 && v36 < v22)
						(*(void(__thiscall **)(CDmeLogLayer *, int, signed int))(*(_DWORD *)v19 + 88))(v19, j, 1);
				}
			}
		}
		if (v41 >= 0)
		{
			if (*(_DWORD *)&v40)
				(*(void(__stdcall **)(_DWORD))(*(_DWORD *)*_g_pMemAlloc + 20))(*(_DWORD *)&v40);
		}
	}
}