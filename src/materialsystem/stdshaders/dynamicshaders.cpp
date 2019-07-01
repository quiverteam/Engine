void AddDataToMaterial(KeyValues* pMat, KeyValues* pData, int recursionDepth = 0)
{
	UnpackMaterial(pData, recursionDepth);

	for (KeyValues* pValue = pData->GetFirstValue(); pValue != NULL; pValue = pValue->GetNextValue())
	{
		const char* pszParam = pValue->GetName();
		const char* pszValue = pValue->GetString();

		pMat->SetString(pszParam, pszValue);
	}
}

void UnpackMaterial(KeyValues* pKV, int recursionDepth)
{
	Assert(pKV);

	if (recursionDepth > 99)
	{
		Warning("ABORTING MATERIAL UNPACKING OF %s - INFINITELY RECURSIVE!\n", pKV->GetName());
		return;
	}

	const bool bIsPatch = !Q_stricmp(pKV->GetName(), "patch");

	for (KeyValues* pSub = pKV->GetFirstSubKey(); pSub != NULL; pSub = pSub->GetNextKey())
	{
		const char* pszName = pSub->GetName();

		if (!Q_stricmp(pszName, "include")) // a string
		{
			const char* pFile = pSub->GetString();
			if (!pFile || !*pFile)
				continue;

			KeyValues* pInlcude = new KeyValues("include");
			if (pInlcude->LoadFromFile(g_pFullFileSystem, pFile))
			{
				AddDataToMaterial(pKV, pInlcude, recursionDepth);
				if (bIsPatch) // recursive patches!
					pKV->SetName(pInlcude->GetName());
			}

			pInlcude->deleteThis();
		}
		else if (!Q_stricmp(pszName, "replace") || !Q_stricmp(pszName, "insert")) // a subkey
		{
			for (KeyValues* pValue = pSub->GetFirstValue(); pValue != NULL; pValue = pValue->GetNextValue())
			{
				pKV->SetString(pValue->GetName(), pValue->GetString());
			}
		}
	}
}

IMaterialVar* FindMaterialVar(IMaterial* pMat, const char* varName)
{
	bool bSuccess = false;
	IMaterialVar* pVar = pMat->FindVar(varName, &bSuccess);
	return bSuccess ? pVar : NULL;
}
bool SetMaterialVar_String(IMaterial* pMat, const char* varName, const char* value)
{
	IMaterialVar* pVar = FindMaterialVar(pMat, varName);
	if (!pVar)
		return false;
	pVar->SetStringValue(value);
	return true;
}
bool SetMaterialVar_Vec(IMaterial* pMat, const char* varName, const float a, const float b, const float c, const float d)
{
	IMaterialVar* pVar = FindMaterialVar(pMat, varName);
	if (!pVar)
		return false;
	pVar->SetVecValue(a, b, c, d);
	return true;
}
bool SetMaterialVar_Vec(IMaterial* pMat, const char* varName, const float a, const float b, const float c)
{
	IMaterialVar* pVar = FindMaterialVar(pMat, varName);
	if (!pVar)
		return false;
	pVar->SetVecValue(a, b, c);
	return true;
}
bool SetMaterialVar_Vec(IMaterial* pMat, const char* varName, const float a, const float b)
{
	IMaterialVar* pVar = FindMaterialVar(pMat, varName);
	if (!pVar)
		return false;
	pVar->SetVecValue(a, b);
	return true;
}
bool SetMaterialVar_Vec(IMaterial* pMat, const char* varName, const float a)
{
	IMaterialVar* pVar = FindMaterialVar(pMat, varName);
	if (!pVar)
		return false;
	pVar->SetFloatValue(a);
	return true;
}
bool SetMaterialVar_Int(IMaterial* pMat, const char* varName, const int a)
{
	IMaterialVar* pVar = FindMaterialVar(pMat, varName);
	if (!pVar)
		return false;
	pVar->SetIntValue(a);
	return true;
}

void ReloadGameShaders(GenericShaderData* data, char** pszMaterialList, int iNumMaterials)
{
	// reload materials
	if (pszMaterialList && iNumMaterials)
	{
		for (int i = 0; i < iNumMaterials; i++)
		{
			IMaterial* pMat = materials->FindMaterial(pszMaterialList[i], TEXTURE_GROUP_MODEL);
			if (IsErrorMaterial(pMat))
				continue;

			KeyValues* pKV = new KeyValues("");
			char tmppath[MAX_PATH];
			Q_snprintf(tmppath, MAX_PATH, "materials/%s.vmt", pszMaterialList[i]);
			if (!pKV->LoadFromFile(g_pFullFileSystem, tmppath))
			{
				pKV->deleteThis();
				continue;
			}

			UnpackMaterial(pKV);

			pMat->SetShaderAndParams(pKV);

			pMat->Refresh();
			pMat->RecomputeStateSnapshots();

			pKV->deleteThis();
		}
		return;
	}
}

void DoMaterialReload()
{
	g_pMaterialSystem->UncacheAllMaterials();
	g_pMaterialSystem->CacheUsedMaterials();
	g_pMaterialSystem->ReloadMaterials();
}

void LoadShaders()
{
	KeyValues* pKV = new KeyValues("shaders/shaders.vdf");
}