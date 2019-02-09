//
// CHistory:
// implements undo and redo.
//

class CHistoryTrack
{
public:
	CHistoryTrack(
};

typedef CTypedPtrList<CPtrList, CHistoryTrack*> CHistoryTrackList;

class CHistory
{
public:
	CHistory();
	~CHistory();

	// mark undo position:
	void MarkUndoPosition();

	// keep this object so we can undo changes to it:
	void Keep(CMapClass *pObject);
	// keep the relationship between objects:
	void Keep(CMapClass *pParent, CMapClass *pChild);
	// store this pointer for destruction if unused during undo lifetime:
	void KeepForDestruction(CMapClass *pObject);

	CHistoryTrackList *CurTrack;
	CTypedPtrList<CPtrList, CHistoryTrackList*> Tracks;
};