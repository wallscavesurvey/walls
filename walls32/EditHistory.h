// EditHistory.h
//

#ifndef EDITHISTORY_H
#define EDITHISTORY_H

// files to include
#include <vector>			// std vector template
#include <string>

#define HIST_MAX_LOCALALLOC 256

#ifdef _DEBUG
	#define _GET_RANGE_CRC

	#ifdef _GET_RANGE_CRC
		#define GET_RANGE_CRC GetRangeCrc
	#else
		#define GET_RANGE_CRC GetRangeSize
	#endif

	#define RETURN_ goto return_
#else
	#undef _GET_RANGE_CRC
	#define RETURN_ return bRet
#endif

/***********************************************************************/
class CEditHistory;
struct CLineRange;

//Action types --
enum e_Action {
	ACTION_AddChar,ACTION_DelChar,ACTION_AddEOL,
	ACTION_DelTextRange,ACTION_AddTextRange,ACTION_DelAddTextRange,ACTION_RepShortStr,
	ACTION_Undo,ACTION_Redo
};

class IActionBase
{
public:
	// virtual function declarations
	virtual void Redo(CEditHistory *pHist,size_t dataOffset) = 0;
	virtual void Undo(CEditHistory *pHist,size_t dataOffset) = 0;
	virtual e_Action Action() = 0;
};

class CHistItem
{
public:
	CHistItem()
	{
#ifdef _DEBUG
		crc=0;
#endif
	}
	CHistItem(size_t dataOffset,IActionBase *pAction) :
			m_dataOffset(dataOffset),m_pAction(pAction) {}
	size_t m_dataOffset;
	IActionBase *m_pAction;
#ifdef _DEBUG
	size_t crc;
#endif
};


/***********************************************************************
	Global Definitions:
***********************************************************************/
typedef std::vector<CHistItem>::iterator VecHistItemIt;		
typedef	std::vector<char>::iterator VecDataIt;
typedef std::vector<char>::reverse_iterator VecDataRevIt;

/***********************************************************************
classes derived from IActionBase --
***********************************************************************/
class CActionAddChar : public IActionBase
{
	friend class CEditHistory;
public:
	// virtual overrides
	virtual void Redo(CEditHistory *pHist,size_t dataOffset);
	virtual void Undo(CEditHistory *pHist,size_t dataOffset);
	virtual e_Action Action();
};

class CActionDelChar : public IActionBase
{
	friend class CEditHistory;
public:
	// virtual overrides
	virtual void Redo(CEditHistory *pHist,size_t dataOffset);
	virtual void Undo(CEditHistory *pHist,size_t dataOffset);
	virtual e_Action Action();
};

class CActionAddTextRange : public IActionBase
{
	friend class CEditHistory;
public:
	// virtual overrides
	virtual void Redo(CEditHistory *pHist,size_t dataOffset);
	virtual void Undo(CEditHistory *pHist,size_t dataOffset);
	virtual e_Action Action();
};

class CActionDelTextRange : public IActionBase
{
	friend class CEditHistory;
public:
	// virtual overrides
	virtual void Redo(CEditHistory *pHist,size_t dataOffset);
	virtual void Undo(CEditHistory *pHist,size_t dataOffset);
	virtual e_Action Action();
	static void UndoRedo(CEditHistory *pHist,UINT uUpd,size_t dataOffset);
	static void RedoUndo(CEditHistory *pHist,UINT uUpd,size_t dataOffset);
};

class CActionDelAddTextRange : public IActionBase
{
	friend class CEditHistory;
public:
	// virtual overrides
	virtual void Redo(CEditHistory *pHist,size_t dataOffset);
	virtual void Undo(CEditHistory *pHist,size_t dataOffset);
	virtual e_Action Action();
};

class CActionRepShortStr : public IActionBase
{
	friend class CEditHistory;
public:
	// virtual overrides
	virtual void Redo(CEditHistory *pHist,size_t dataOffset);
	virtual void Undo(CEditHistory *pHist,size_t dataOffset);
	virtual e_Action Action();
};

class CActionAddEOL : public IActionBase
{
	friend class CEditHistory;
public:
	// virtual overrides
	virtual void Redo(CEditHistory *pHist,size_t dataOffset);
	virtual void Undo(CEditHistory *pHist,size_t dataOffset);
	virtual e_Action Action();
};

#pragma pack(1)
struct sRange {
	int line0;
	int line1;
	byte chr0;
	byte chr1;
};
struct sTextRangeData {
	int nLen;
	byte *pText;
	sRange range;
};
struct sAddCharData {
	int lineno;   //CLineDoc line number
	short count;  //number of characters saved for this action
	byte charno;  //line offset of first saved char
	char chr;
};
struct sDelCharData {
	int lineno;   //CLineDoc line number
	short count;  //number of characters saved for this action (can be negative)
	byte charno;  //line offset of first deleted char
	char chr;
};
struct sRepShortStrData {
	int lineno;   //CLineDoc line number
	byte count;   //number of characters replaced (twice this is saved)
	byte charno;  //line offset of first char to replace
};
struct sAddEOLData {
	int line0;
	byte nLen;
	byte chr0;
};
#pragma pack()

/***********************************************************************
class CEditHistory
	This class is designed to manage user action history. After an
	action has been handled the pointer to the original action should be
	stored in this class for undo and redo functionality.
***********************************************************************/


class CEditHistory  
{
	friend class CLineDoc;
	friend class CActionAddTextRange; //Needs to access m_cDelTextRange
public:
	// classes constructor/deconstructor
	CEditHistory() : m_pDoc(NULL),m_nPos(0),m_nPosSaved(0),m_LastAction(ACTION_Undo) {
		m_vecData.reserve(128);
	}
	~CEditHistory() {ClearHistory();}

	// classes member functions
	void AddAction(CHistItem &histItem);
	void ClearEnd();
	void ClearHistory() {m_nPos=0; ClearEnd();}
	bool IsUndoable() {return m_nPos>0;}
	bool IsRedoable() {return m_nPos<(int)m_vecHistItem.size();}
	void Undo();
	void Redo();
	void ClearLastAction() {if(m_nPos>0) {--m_nPos; ClearEnd();}}
	CHistItem * GetCurrentAction();
	int GetPos() const { return m_nPos; }
	int GetSize() const { return (int)m_vecHistItem.size(); }
	bool AddChar(int lineno,int charno,char c);
	bool DelChar(int lineno,int charno,char c);

#ifdef _DEBUG
	void SaveChecksum(size_t crc);
	void CompareChecksum(UINT uUpd,size_t crc);
	size_t crc0;
#endif

	//Public variables --
	CLineDoc *m_pDoc;
	std::vector<char> m_vecData; //buffer for all saved data

private:
	bool AddTextRange(LPCSTR pText,int len,CLineRange &range,BOOL bDeleteAlso);
	bool RepShortStr(LPCSTR pOldText,LPCSTR pNewText,int len,CLineRange &range);
	bool DelTextRange(CLineRange &range,BOOL bDeleteOnly);
	bool AddPasteRange(CLineRange &range,BOOL bDeleteAlso);
	bool AddEOL(int line0,int chr0,int nLen);

	//Private variables --
	std::vector<CHistItem> m_vecHistItem; // actions array
	int m_nPos;	// position in history
	int m_nPosSaved;
	e_Action m_LastAction;

	static CActionDelChar m_cDelChar;
	static CActionAddChar m_cAddChar;
	static CActionDelTextRange m_cDelTextRange;
	static CActionAddTextRange m_cAddTextRange;
	static CActionDelAddTextRange m_cDelAddTextRange;
	static CActionRepShortStr m_cRepShortStr;
	static CActionAddEOL m_cAddEOL;
};

#endif	// EDITHISTORY_H
