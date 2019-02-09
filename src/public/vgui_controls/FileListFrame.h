
#ifndef FILELISTFRAME_H
#define FILELISTFRAME_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/frame.h"
#include "tier1/utlstring.h"

//-----------------------------------------------------------------------------
// Purpose: Modal dialog for a list of files + an operation to perform
//-----------------------------------------------------------------------------
class COperationFileListFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(COperationFileListFrame, vgui::Frame);

public:
	// NOTE: The dialog ID is used to allow dialogs to have different configurations saved 
	COperationFileListFrame(vgui::Panel *pParent, const char *pTitle, const char *pColumnHeader, bool bShowDescription, bool bShowOkOnly = false, int nDialogID = 1);
	virtual ~COperationFileListFrame();

	// Command handler
	virtual void OnCommand(const char *pCommand);
	virtual void PerformLayout();

	// Adds files to the frame
	void ClearAllOperations();
	void AddOperation(const char *pOperation, const char *pFileName);
	void AddOperation(const char *pOperation, const char *pFileName, const Color& clr);

	// Resizes the operation column to fit the operation text
	void ResizeOperationColumnToContents();

	// Sets the column header for the 'operation' column
	void SetOperationColumnHeaderText(const char *pText);

	// Shows the panel
	void DoModal(KeyValues *pContextKeyValues = NULL, const char *pMessage = NULL);

	// Retrieves the number of files, the file names, and operations
	int GetOperationCount();
	const char *GetFileName(int i);
	const char *GetOperation(int i);

	// Retreives the description (only if it was shown)
	const char *GetDescription();

private:
	virtual bool PerformOperation() { return true; }
	const char *CompletionMessage();
	void CleanUpMessage();

	vgui::ListPanel *m_pFileBrowser;
	vgui::Splitter *m_pSplitter;
	vgui::TextEntry *m_pDescription;
	vgui::Button *m_pYesButton;
	vgui::Button *m_pNoButton;
	KeyValues *m_pContextKeyValues;
	CUtlString m_MessageName;
	char *m_pText;
};

#endif // FILELISTFRAME_H