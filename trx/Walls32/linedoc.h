// linedoc.h : interface of the CLineDoc and CLineHint classes
//
/////////////////////////////////////////////////////////////////////////////

#ifndef LINEDOC_H
#define LINEDOC_H

#ifndef EDITHISTORY_H
#include "EditHistory.h"
#endif

#define _SURVEY 
#undef _INSERT_TABS

typedef int LINENO;
typedef BYTE *HPBYTE;

#define LINENO_MAX INT_MAX
#define PH(n) (((CLineHint *)pHint)->n)

struct CLineRange
{
    LINENO line0;
    int chr0;
    LINENO line1;
    int chr1;
    CLineRange() {}
    CLineRange(LINENO nLine0,int nChr0,LINENO nLine1,int nChr1)
    {
      line0=nLine0;
      chr0=nChr0;
      line1=nLine1;
      chr1=nChr1;
    }
};


class CLineHint : public CObject
{
	DECLARE_DYNAMIC(CLineHint)
public:
    UINT upd;
    CLineRange *pRange;
};

struct CBuffer
{
   LPBYTE pBuf;
   CBuffer *pNext;
};

class CLineDoc : public CDocument
{
    DECLARE_DYNCREATE(CLineDoc)
	friend class CEditHistory;
public:
	CLineDoc();
    BYTE *m_pActiveBuffer;  //Line edit buffer used by views 
    BOOL m_bActiveChange;   //Indicates line buffer has been edited
	BOOL m_bSaveAs;			//Indicates inside SaveAs function

    LINENO m_nActiveLine;   //Accessed by CWedView::OnUpdateLine()
    
    enum {UPD_DELETE=1,UPD_INSERT=2,UPD_REPLACE=4,UPD_SCROLL=8,
          UPD_CLEARSEL=16,UPD_BARS=32,UPD_CLIPCOPY=64,
          UPD_CLIPCUT=65,UPD_CLIPPASTE=128,UPD_TRUNCABORT=256,
		  UPD_UNDO=512,UPD_REDO=1024,UPD_UNDOREDO=1024+512};
	
    enum {MAXLINLEN=0xFF,MAXLININC=0x400,MAXLINES=0x3F000000,READBUFSIZ=0x800,
          SAVEBUFSIZ=0x800,LINBUFSIZ=0x2000,MAXLINTABS=16};
          
    BOOL m_bTitleMarked;       //Maintained in OnUpdateFileSave(), Accessed by CTxtFrame
	BOOL m_bReadOnly;
	static BOOL m_bReadOnlyOpen;
    
    UINT m_EditFlags;
            
    CLineDoc *m_pNextLineDoc;
        
    //Points to a frame title character array of length m_lenFrameTitle.
    //There is space for SIZ_FRAMETITLE_SFX extra characters --
    CString m_csFrameTitle;
	int m_lenFrameTitle;
    static BOOL m_bNewEmptyFile;
    
    enum {FILE_SRV=1,FILE_LST=2};
    BOOL m_bSrvFile;
	CEditHistory m_Hist;
    
protected:
    HFILE m_hFile;
    LPBYTE FAR *m_pLinePtr;    //Array: |line ptrs|avail slot ptrs|free ptrs|
    LINENO m_nMaxLines;        //Current length of m_pLinePtr[]
    LINENO m_nNumLines;        //Current number of line ptrs (memory version of file)
    LINENO m_nAllocLines;      //m_nNumLines+(number of allocated slots)
                               //There are m_nMaxLines-m_nAllocLines unallocated ptrs.
    UINT m_cBufferSpace;
    CBuffer *m_pFirstBuffer;
    CLineHint m_hint;

private:
    //Set in the constructor (disabled when m_pszFrameTitle processing is expected by caller) --
    //BOOL m_bEnableFileSaveAs; 
    void MarkDocTitle(BOOL bChg);
	BOOL BlockAlloc(UINT size);
	void BlockReduce(UINT size);
    BOOL AllocLine(BYTE linlen,LINENO nLineIndex);
    BOOL ExtendMaxLines(LINENO nMinAdd);
    BOOL DeleteTextRange(LINENO line0,int chr0,LINENO line1,int chr1);
    BOOL DeleteTextRange(CLineRange &range)
    {
      return DeleteTextRange(range.line0,range.chr0,range.line1,range.chr1);
    }
    void DeleteLineRange(LINENO nLine0,LINENO nLine1);
    BOOL PutClipLine(HPBYTE hpLine,HPBYTE pEOL,int nChrOffset);
    BOOL InsertEOL(int nChrOffset);
    int  InsertLines(LINENO nLineIndex,HPBYTE FAR *pLinePtr,int nLineCount,int nChrOffset);
    LONG GetRangeSize(CLineRange& range);
#ifdef _DEBUG
	LONG GetRangeCrc(CLineRange& range);
	void Check_CRC(UINT uUpd);
#endif
    BOOL ClipCopy(CLineRange& range);
    BOOL ClipPaste(CLineRange& range,HPBYTE hpMem,size_t sizMem);
	void MarkActiveBuffer(BOOL bChanged=TRUE) {m_bActiveChange=bChanged;}
	BOOL LoadActiveBuffer(LINENO nLineNo);
	BOOL PutLine(BYTE *pLine,int linlen,LINENO nLineIndex);
    UINT AppendLine(BYTE *pLine,int linlen);
	
	void UnLoadActiveBuffer()
	{
	  SaveActiveBuffer();
	  m_nActiveLine=0;
	}
	void DiscardActiveBuffer()
	{
	  m_nActiveLine=0;
	  m_bActiveChange=FALSE;
	}
    //void UpdateViews(UINT uUpd,CLineRange *pRange);
    void UpdateViews(UINT uUpd)
    {
      m_hint.upd=uUpd;
      UpdateAllViews(NULL,0L,&m_hint);
    }
    
public:
	// Operations used by views --
    BOOL Update(UINT uUpd,LPCSTR pChar,int nLen,CLineRange range);
    void CheckTitleMark(BOOL bChg);

	//Used by CEditHistory --
	void CopyRange(HPBYTE pDest,CLineRange& range,int nLenMax);
	
	//Used by CWedView::OnUpdateLine() --
	BOOL SaveActiveBuffer();
	void DisplayLineViews();
	
	//Used by CMainFrame::CloseLineDoc() --
    BOOL Close(BOOL bNoSave);

	//Used by CPrjView::OnPrjCompileItem() --
	static BOOL m_bSaveAndCloseError;
	void SaveAndClose(BOOL bClose=TRUE);
    void SetFileFlags(const char *p);
	BOOL IsSrvFile() {return m_bSrvFile==FILE_SRV;}
	BOOL IsLstFile() {return m_bSrvFile==FILE_LST;}
    void ClearSelections();
	void SetUnModified(); //used only by m_Hist
    LINENO GetLineCount() {return m_nNumLines;}
    int MaxLineLen() {return MAXLINLEN;}
	LPBYTE LinePtr(LINENO nLineNo)
    {
	  return (nLineNo==m_nActiveLine)?m_pActiveBuffer:m_pLinePtr[nLineNo-1];
	}
	static BOOL IsWS(BYTE c) {return c=='\t'||c==' ';}
	static void TrimWS(BYTE *pLine);
    static BOOL NormalizeRange(LINENO& nLine0,int& nChr0,LINENO& nLine1,int& nChr1);
    static BOOL NormalizeRange(CLineRange &range)
    {
      return NormalizeRange(range.line0,range.chr0,range.line1,range.chr1);
    }
	void UpdateOpenFileTitle(LPCSTR newName,LPCSTR newTitle);
	static BOOL SaveAllOpenFiles();
	static CLineDoc * GetLineDoc(LPCSTR pathname);
	static void UpdateOpenFilePath(LPCSTR oldPath,LPCSTR newPath);

	void ToggleWriteProtect(); //called only from CWedView context menu selection
	int SaveReadOnly(BOOL bNoPrompt);
	BOOL SetReadOnly(CPrjListNode *pNode,BOOL bReadOnly,BOOL bNoPrompt);

// Implementation
protected:
    virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(const char* pszPathName);
	virtual BOOL OnSaveDocument(const char* pszPathName);
	virtual void OnCloseDocument();
	virtual void DeleteContents();
    //Upon attempting to close, CDocument::SaveModified() checks IsModified()
    //and returns TRUE if ok to continue, whether user saves or cancels.
    //We overide it to flush any pending changes in the line buffer.
    virtual BOOL SaveModified();
    
public:
	virtual ~CLineDoc();
	
// Generated message map functions
protected:
	//{{AFX_MSG(CLineDoc)
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
