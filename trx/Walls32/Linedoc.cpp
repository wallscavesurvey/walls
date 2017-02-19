// linedoc.cpp : implementation of the CLineDoc and
//               CLineHint classes
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "txtfrm.h"
#include "linedoc.h"
#include "lineview.h"
#include "filebuf.h"
#include "itemprop.h"

//Used in override of SaveModified() --
#ifdef _DEBUG
#include <zlib.h>
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CLineHint,CObject)
IMPLEMENT_DYNCREATE(CLineDoc,CDocument)

BEGIN_MESSAGE_MAP(CLineDoc,CDocument)
	//{{AFX_MSG_MAP(CLineDoc)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////
//Static file scope variables and functions --

static BOOL bLinesClipped; //Used by ClipPaste() and PutClipLine()
BOOL CLineDoc::m_bNewEmptyFile; //Used by OnNewDocument() if not NULL
BOOL CLineDoc::m_bSaveAndCloseError; //Used with CLineDoc::SaveAndClose()
BOOL CLineDoc::m_bReadOnlyOpen;

#ifndef SIZE_T_MAX
#define SIZE_T_MAX UINT_MAX
#endif

static BOOL IsOK(BOOL bOK,UINT ids_err)
{
   if(!bOK) CMsgBox(MB_ICONEXCLAMATION,ids_err);
   return bOK;
}

static inline int TabCount(BYTE *pLine, int len)
{
  int c=0;
  for(;len;len--) if(*pLine++=='\t') c++;
  return c;
}

static BOOL TrimTabs(BYTE *pLine,int& len)
{
  int n=len;

  for(int c=0;n;n--)
    if(*pLine++=='\t' && ++c>CLineDoc::MAXLINTABS) break;
  len-=n;
  return n>0;
}

static BOOL LineTruncateFail(BYTE *pLine,LINENO nLine,int& linlen)
{
  BYTE c;
  int len=linlen;

  if(len>=CLineDoc::MAXLINLEN+1) {
    //linlen==MAXLINLEN+2 if pLine was truncated when read from disk --
    if((len>CLineDoc::MAXLINLEN+1 || pLine[len-1]!='\r') &&
      IDCANCEL==CMsgBox(MB_OKCANCEL,IDS_ERR_TRUNCLINE1,nLine))
      return TRUE;
    len=CLineDoc::MAXLINLEN;
  }
  while(len && ((c=pLine[len-1])=='\r' || c==' ' || c=='\t')) len--;

  if(TrimTabs(pLine,len) &&
    IDCANCEL==CMsgBox(MB_OKCANCEL,IDS_ERR_TRUNCTABS1,nLine))
    return TRUE;

  linlen=len;
  return FALSE;
}

///////////////////////////////////////////////////////////////////////
//CLineDoc Variables and Methods --

CLineDoc::CLineDoc()
{
    //Cannot use this due to an embedded CObject, CLineHint --
    //AFX_ZERO_INIT_OBJECT(CDocument);

    //This initialization is required so DeleteContents() will
    //not try freeing memory not yet allocated --
    m_pFirstBuffer=NULL;
    m_pLinePtr=NULL;

    //Support updating of font for all documents being viewed.
    //The MFC method for traversing open documents is apparently undocumented --
    CMainFrame* mf=(CMainFrame*)AfxGetApp()->m_pMainWnd;
    m_pNextLineDoc=mf->m_pFirstLineDoc;
    mf->m_pFirstLineDoc=m_Hist.m_pDoc=this;

	//Allocate local line edit buffer. In the unlikely event that
	//m_pActiveBuffer==NULL, OnOpenDocument() and OnNewDocument()
	//will fail with an IDS_ERR_OPENFILERES message box --
    m_pActiveBuffer=(BYTE *)malloc(MAXLINLEN+3);

    //The following is done also by DeleteContents() and should
    //be unnecessary here except to avoid an ASSERT() --
    m_nActiveLine=0;
    m_bActiveChange=m_bSaveAs=m_bTitleMarked=FALSE;
    
    //The application may have set m_pszInitFrameTitle and
    //m_pszInitIconTitle to strings that are meant to replace the
    //window captions supplied by MFC --

	m_bReadOnly=m_bReadOnlyOpen;
}

CLineDoc::~CLineDoc()
{
    CMainFrame* mf=(CMainFrame*)AfxGetApp()->m_pMainWnd;
    CLineDoc *pd=mf->m_pFirstLineDoc;
    if(pd==this) mf->m_pFirstLineDoc=m_pNextLineDoc;
    else {
      while(pd && pd->m_pNextLineDoc!=this) pd=pd->m_pNextLineDoc;
      if(pd) pd->m_pNextLineDoc=m_pNextLineDoc;
    }
    free(m_pActiveBuffer);
}

///////////////////////////////////////////////////////////////////////
// CDocument overrides


BOOL CLineDoc::BlockAlloc(UINT size)
{
	CBuffer *p=(CBuffer *)malloc(sizeof(CBuffer));
	if(!p || (p->pBuf=(LPBYTE)malloc(size))==NULL) {
	  free(p); //NULL p is ignored
	  return FALSE;
	}
	p->pNext=m_pFirstBuffer;
	m_cBufferSpace=size;
	m_pFirstBuffer=p;
	return TRUE;
}

void CLineDoc::BlockReduce(UINT size)
{
	CBuffer *p=m_pFirstBuffer;
	if(p) {
		if(!size) {
			m_pFirstBuffer=p->pNext;
			free(p->pBuf);
			free(p);
		}
		else _expand(p,size);
	}
    m_cBufferSpace=0;
}

UINT CLineDoc::AppendLine(BYTE *pLine,int linlen)
{
  //Our MAXLINES includes a final empty line that the user can position
  //to but cannot extend --
  /*
  if(m_nNumLines==MAXLINES-1) {
    if(IDOK==CMsgBox(MB_OKCANCEL,IDS_ERR_TRUNCFILE)) return IDS_ERR_TRUNCFILE;
    return IDS_ERR_MAXLINES;
  }
  */

  //Rather than truncate we'll force a cancel if PutLine() fails due
  //to lack of memory. With line truncation the user can either cancel
  //or continue --
  if(LineTruncateFail(pLine,m_nNumLines+1,linlen) ||
    !IsOK(PutLine(pLine,linlen,m_nNumLines),IDS_ERR_FILEOPENMEM))
    return IDS_ERR_FILEOPENMEM;

  return 0;
}

#ifdef _SURVEY
static char *GetFileExt(const char *path)
{
    //Assumes white space has been trimmed --
    static char ext[4];

    *ext=0;
    int len=strlen(path);
    for(int i=len;i-- && len-i<=4;)
      if(path[i]=='.') {
        strcpy(ext,path+i+1);
        break;
      }
    return ext;
}
#endif

void CLineDoc::SetFileFlags(const char *p)
{
	p=GetFileExt(p);
	if(!_stricmp(p,"SRV")) {
		m_bSrvFile=FILE_SRV;
		m_EditFlags=CMainFrame::m_EditFlags[1];
	}
	else {
		m_bSrvFile=(_stricmp(p,"LST")&&_stricmp(p,"LOG"))?0:FILE_LST;
		m_EditFlags=CMainFrame::m_EditFlags[0];
	}
}

BOOL CLineDoc::OnOpenDocument(const char* pszPathName)
{
	//The standard CDocument::OnOpenDocument() opens the document,
	//deserializes it, and then closes the document. We will avoid the
	//framework's serialization mechanism.

    //Arcane Issue No.1287 --
	//CLineDoc::MAXLINES (16128) includes a final empty line that the user
	//can position to but cannot extend, hence the user is presented a limit
	//of MAXLINES-1. If successful, OnOpenDocument() will call PutLine() to add
	//a last empty line to the memory version UNLESS the last file character is
	//not an LF. Whichever the case, when the file is saved the final (implied)
	//CRLF is not written to disk. This ability to save a non-terminated last
	//line (or rather the ability to NOT automatically append a missing CRLF)
	//can be made into a user preference.

 	if(!IsOK(m_pActiveBuffer!=NULL,IDS_ERR_FILEOPENMEM)) return FALSE;
 	
 	if(m_bNewEmptyFile) {
	  ASSERT(!m_bReadOnly);
 	  SetTitle(pszPathName);
 	  return OnNewDocument();
 	}

	CFileBuffer file(READBUFSIZ);
	if(file.OpenFail(pszPathName,CFile::modeRead|CFile::shareDenyWrite)) return FALSE;

 	DeleteContents();
	BeginWaitCursor();

	TRY
	{
		int linlen,ids_err;
		while((linlen=file.ReadLine((LPSTR)m_pActiveBuffer,MAXLINLEN+2))!=-1) {
          //We'll copy up to MAXLINLEN+2 bytes to the line buffer.
          //If this maximum is reached OR if the last of MAXLINLEN+1 bytes
          //is not CR, AppendLine() will prompt for truncation or cancel.
          if(ids_err=AppendLine(m_pActiveBuffer,linlen)) {
            //if(ids_err==(UINT)IDS_ERR_TRUNCFILE) break;
			file.Abort(); // will not throw an exception
			DeleteContents();   // remove failed contents
			EndWaitCursor();
			return FALSE;
          }
		}
		file.Close();
	}
	CATCH(CFileException,e)
	{
		file.Abort(); // will not throw an exception
		DeleteContents();   // remove failed contents
		EndWaitCursor();
		TRY
			file.ReportException(pszPathName,e,FALSE,AFX_IDP_FAILED_TO_OPEN_DOC);
		END_TRY
		return FALSE;
	}
	END_CATCH

	EndWaitCursor();
	
	//Append an empty line to the file unless the last file line
	//successfully appended was without a terminating CRLF (linlen>0),
	//in which case we have already implicitly appended the CRLF.
	//Upon saving we will always strip the trailing CRLF.
	
    if(!IsOK(PutLine(NULL,0,m_nNumLines),IDS_ERR_FILEOPENMEM)) {
		DeleteContents();
		return FALSE;
    }
    
    SetFileFlags(pszPathName);
    SetPathName(pszPathName);
    SetModifiedFlag(FALSE); // Always start off "unmodified"
	#ifdef _DEBUG
		m_Hist.crc0=GET_RANGE_CRC(CLineRange(1,0,m_nNumLines,INT_MAX));
	#endif
    return TRUE;
}

BOOL CLineDoc::OnNewDocument()
{
	//CWinapp::OnFileNew() checks if there are more than one
	//CMultiDocTemplate registered. Prompts user in this case,
	//then calls pTemplate->OpenDocumentFile(NULL). This
	//function uses pDocument->SetTitle() to assign a temporary
	//name and then calls pDocument->OnNewDocument().
	//Since we want to use a single template, the version here
	//can prompt the user for a file type, then change both the title
	//and document pathname to have the desired default extension --
    
    if(m_bNewEmptyFile) {
        SetFileFlags(GetTitle());
		m_bNewEmptyFile=FALSE;
    }
    else {
	    CString title=GetTitle();
	    //We could prompt user here for type to avoid using multiple
	    //document templates --
	    title+=PRJ_SURVEY_EXT;
	    SetTitle(title);
	    m_strPathName=title;      // default path = current directory?
	    m_EditFlags=CMainFrame::m_EditFlags[m_bSrvFile=FILE_SRV];
	}

    //Note: CDocument version does this, but empties m_strPathname.
    //We want OnFileSaveAs() to offer a complete name as the default.
    //The MFC version takes the first 8 chars of GetTitle() and appends
    //the template's default extension if m_strPathName is empty.

	DeleteContents();

	//The new document consists of one empty line. When saved, the
	//last CRLF will be taken off --
	if(!IsOK(m_pActiveBuffer && PutLine(NULL,0,0),IDS_ERR_FILERES)) return FALSE;
	//Putline() sets the modified flag --
	SetModifiedFlag(FALSE);
	return TRUE;
}

void CLineDoc::TrimWS(BYTE *pLine)
{
  //Trim space and tab characters from line --
  UINT len=*pLine;
  while(len && IsWS(pLine[len])) len--;
  *pLine=(BYTE)len;
}

BOOL CLineDoc::LoadActiveBuffer(LINENO nLineNo)
{
  //Return TRUE if load accomplished without truncating an existing line --
  BOOL bRet=TRUE;
  
  if(nLineNo!=m_nActiveLine) {
    if(m_bActiveChange) bRet=SaveActiveBuffer();
    m_nActiveLine=nLineNo--;
    memcpy(m_pActiveBuffer,m_pLinePtr[nLineNo],*m_pLinePtr[nLineNo]+1);
  }
  return bRet;
}

BOOL CLineDoc::SaveActiveBuffer()
{
  //Return TRUE if save accomplished without truncation --
  
  if(m_bActiveChange) {
    ASSERT(m_nActiveLine);
    BYTE *p=m_pActiveBuffer;

    m_bActiveChange=FALSE;
    if(!PutLine(p+1,*p,m_nActiveLine-1)) {
      //We will instead update the existing line without extending
      //its length and also notify the user of truncation.
      //This should occur only in case of memory allocation failure.
      
      CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_LINESAVE1,m_nActiveLine);
      //Get pointer to original version --
      LPBYTE pLine=m_pLinePtr[m_nActiveLine-1];
      ASSERT(*pLine<*p); //Failure implies original line was shorter!
      *p=*pLine;
      TrimWS(p);
      memcpy(pLine,p,*p+1);
      CLineRange range(m_nActiveLine,*p,m_nActiveLine,*p+1);
      m_hint.pRange=&range;
      UpdateViews(UPD_DELETE);
      SetModifiedFlag(TRUE);
      return FALSE;
    }
  }
  return TRUE;
}

BOOL CLineDoc::NormalizeRange(LINENO& nLine0,int& nChr0,LINENO& nLine1,int& nChr1)
{
    if(nLine1<nLine0 || nChr1<nChr0 && nLine0==nLine1) {
      LINENO line=nLine0;
      int chr=nChr0;
      nLine0=nLine1;
      nLine1=line;
      nChr0=nChr1;
      nChr1=chr;
    }
    if(nChr1==0 && nLine1>nLine0) {
      nLine1--;
      nChr1=INT_MAX;
    }
    return (nChr0!=nChr1) || (nLine0!=nLine1);
}

void CLineDoc::DeleteLineRange(LINENO nLine0,LINENO nLine1)
{
  // When deleting lines we only need to rearrange the pointer array, m_pLinePtr.
  // For the sake of efficiency we'll consider three situations:
  //
  //   1. Deleting a single line -- 1 memcpy required.
  //   2. Deleting >1 and <=(m_nMaxLines-m_nAllocLines) -- 2 memcpy's.
  //   3. Deleting >(m_nMaxLines-m_nAllocLines) -- malloc and 3 memcpy's.
  //
  // In all cases the deleted line ptrs are moved to the "available slot"
  // segment. Keeping the slots sorted by size is a possible (but unlikely)
  // optimization.
  //
  // CAUTION: To be completely safe use memmove rather than memcpy. Though regions
  // technically overlap, most versions of memcpy work fine (check the assembly!).

  LPBYTE FAR *pPtrDeleted=m_pLinePtr+(nLine0-1);
  LINENO numdel=nLine1-nLine0+1;

  if(numdel<=0) return;

  ASSERT(m_nNumLines>numdel);

  if(numdel==1) {
     LPBYTE pDeleted=*pPtrDeleted;
     memcpy(pPtrDeleted,pPtrDeleted+1,(m_nNumLines-nLine0)*sizeof(LPBYTE));
     m_pLinePtr[m_nNumLines--]=pDeleted;
  }
  else if(numdel<=m_nMaxLines-m_nAllocLines) {
     memcpy(m_pLinePtr+m_nAllocLines,pPtrDeleted,numdel*sizeof(LPBYTE));
     memcpy(pPtrDeleted,pPtrDeleted+numdel,(m_nAllocLines-nLine1+numdel)*sizeof(LPBYTE));
     m_nNumLines-=numdel;
  }
  else {
    //Failure to preserve the slots for reuse is not critical --
    LPBYTE FAR *pPtrSave=(LPBYTE FAR *)malloc(numdel*sizeof(LPBYTE));
    if(pPtrSave) memcpy(pPtrSave,pPtrDeleted,numdel*sizeof(LPBYTE));
    memcpy(pPtrDeleted,pPtrDeleted+numdel,(m_nAllocLines-nLine1)*sizeof(LPBYTE));
    if(pPtrSave) {
      memcpy(pPtrDeleted+m_nAllocLines-nLine1,pPtrSave,numdel*sizeof(LPBYTE));
      free(pPtrSave);
    }
    else m_nAllocLines-=numdel;
    m_nNumLines-=numdel;
  }

  SetModifiedFlag(TRUE);
}

BOOL CLineDoc::DeleteTextRange(LINENO line0,int chr0,LINENO line1,int chr1)
{
   //Delete normalized range of text, possibly merging portions
   //of range.line0 and range.line1. Return FALSE if the merge would produce a
   //line too long. The active buffer contains range.line0. If range.line0 is
   //deleted by this operation, the buffer will be reloaded with the
   //line that takes its place.
   //NOTE: Assume range.chr1==INT_MAX inplies range.line1<m_nNumLines!

  if(line0<line1 || chr1==INT_MAX) {
     //Lines must be deleted.

     if(chr0) {
       // We are truncating EOL. Let's merge bounding lines --
       if(chr1==INT_MAX ) {
          line1++;
          chr1=0;
       }
       LPBYTE pLine1=LinePtr(line1); //line to be merged then deleted
       int len=*pLine1; //Length of last line of range
       if(chr1) {
	     //chr1 is length first part of this line that will be
		 //deleted rather than merged with line0
		   if(chr1>len) {
			   len=0;
			   ASSERT(FALSE);
		   }
           else len-=chr1;
       }
       if(len) {
         if(chr0>*m_pActiveBuffer) chr0=*m_pActiveBuffer;
         if(chr0+len>CLineDoc::MAXLINLEN) {
	       CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MAXLINLEN);
	       return FALSE;
	     }
	     memcpy(m_pActiveBuffer+chr0+1,pLine1+chr1+1,len);
	   }
	   *m_pActiveBuffer=chr0+len; //**Old bug -- this and the next line was in the above block!
	   MarkActiveBuffer();
	   DeleteLineRange(line0+1,line1);
	   return TRUE;
	 }

	 //We are deleting the first line entirely --
	 DiscardActiveBuffer();
	 if(chr1<INT_MAX)  line1--;
	 DeleteLineRange(line0,line1);
	 LoadActiveBuffer(line0);
	 if(chr1==INT_MAX) return TRUE;
	 chr0=0;
   }

   BYTE *pLine=m_pActiveBuffer;
   int len=*pLine;
   if(chr1>=len) *pLine=chr0;
   else {
     memcpy(pLine+chr0+1,pLine+chr1+1,len-chr1);
     *pLine=(BYTE)(len-(chr1-chr0));
   }
   MarkActiveBuffer();
   return TRUE;
}

BOOL CLineDoc::PutClipLine(HPBYTE hpLine,HPBYTE pEOL,int nChrOffset)
{
  int len;
  BYTE linbuf[MAXLINLEN+1];
  LONG lLen=(LONG)(pEOL-hpLine);

  ASSERT(*pEOL=='\n');

  //We silently accept LF-only (Unix) or CRLF terminated lines.
  //If the user then modifies the file and accepts the changes,
  //CRLF terminated (DOS/Windows) lines will be written out.
  //If this automatic conversion is not wanted we can use a flag
  //to inform the user of this, or else change the behavior.

  //Should always be the case for CF_TEXT line data --
  if(lLen && pEOL[-1]=='\r') lLen--;
  //else m_bUnixConvert=TRUE;

  if(lLen+nChrOffset>MAXLINLEN) {
    bLinesClipped=TRUE;
    len=MAXLINLEN-nChrOffset;
  }
  else len=(int)LOWORD(lLen);

  if(nChrOffset) memcpy(linbuf,m_pActiveBuffer+1,nChrOffset);
  memcpy(linbuf+nChrOffset,hpLine,len); //should handle huge pointers

  len+=nChrOffset;
  if(TrimTabs(linbuf,len)) bLinesClipped=TRUE;

  return PutLine(linbuf,len,m_nNumLines);
}

BOOL CLineDoc::InsertEOL(int nChrOffset)
{
	//Insert a single EOL in the active line at char offset nChrOffset.
	//This is equivalent to truncating the active line at nChrOffset chars length
	//and inserting a new line beneath it consisting of the clipped portion.

	int linlen=*m_pActiveBuffer;
	ASSERT(m_nActiveLine && nChrOffset<=linlen);
	//First, *append* a new line consisting of the trailing string segment --
	if(!PutLine(m_pActiveBuffer+nChrOffset+1,linlen-nChrOffset,m_nNumLines)) return FALSE;

	//Save the newly allocated pointer --
	LPBYTE pSav=m_pLinePtr[m_nNumLines-1];

	//Shift all lines *below* the active line downward by one position --
	if(m_nActiveLine<m_nNumLines-1)
	  memmove(m_pLinePtr+m_nActiveLine+1,m_pLinePtr+m_nActiveLine,
	    (m_nNumLines-m_nActiveLine-1)*sizeof(LPBYTE));
	m_pLinePtr[m_nActiveLine]=pSav;

    //If there was a clipped string segment adjust the active line's length --
    if(nChrOffset<linlen) {
		*m_pActiveBuffer=nChrOffset;
		MarkActiveBuffer();
	}
	return TRUE;
}

int CLineDoc::InsertLines(LINENO nLineIndex,HPBYTE FAR *pLinePtr,int nLineCount,int nChrOffset)
{
	LINENO n; //count of lines inserted --

	//Insert multiple lines in support of ClipPaste(). Since the clipboard
	//is open during this operation we will silently truncate overlong
	//lines, setting a global flag bLinesClipped when this happens.
	//PutClipLine() fails only in the event of memory allocation failure
	//in which case we will stop (but not reverse) the inserting of lines.
	//We can inform the user of either type of truncation after closing the
	//clipboard.

	//Save the initial line count --
	LINENO nLineNumSav=m_nNumLines;

    //First, we *append* the lines --
	for(n=0;n<nLineCount;n++) {
		if(!PutClipLine(pLinePtr[n],pLinePtr[n+1]-1,nChrOffset)) break;
		nChrOffset=0;
	}
	if(!n) return 0;

	//Next, lets use the supplied pointer array, pLinePtr, to save the newly
	//allocated line pointers so they will not be destroyed when we open the gap
	//in our active pointer array, m_pLinePtr[] --
	memcpy(pLinePtr,m_pLinePtr+nLineNumSav,n*sizeof(LPBYTE));

	//Make space for n new lines in m_pLinePtr[] and insert them at
	//line offset nLineIndex, taking them from the array pLinePtr --
	memmove(m_pLinePtr+nLineIndex+n,m_pLinePtr+nLineIndex,
		(nLineNumSav-nLineIndex)*sizeof(LPBYTE));
    memcpy(m_pLinePtr+nLineIndex,pLinePtr,n*sizeof(LPBYTE));

	return n;
}

BOOL CLineDoc::ClipPaste(CLineRange& range,HPBYTE hpMem,size_t sizMem)
{
    UINT ids_err=0; //No error msg by default
	BOOL bIsModified=FALSE;
	BOOL bAllowTrunc=FALSE;
    HPBYTE FAR *hpArray=NULL;
    HGLOBAL hMem=0;
    LINENO numLines;
    LINENO numPtrs;
    HPBYTE hpEOL;
    BYTE *pLine;
	int sizTrail,len;

    ASSERT(m_nActiveLine==range.line0);
    UnLoadActiveBuffer();  //Use the buffer as scrap

_retry0:

	if(!hpMem) {
		//Do nothing if clipboard is either empty or "in use" --
		if(!::IsClipboardFormatAvailable(CF_TEXT) ||
		!AfxGetMainWnd()->OpenClipboard()) return FALSE;

		if(!(hMem=::GetClipboardData(CF_TEXT))) goto _retClose;

		hpMem=(HPBYTE)::GlobalLock(hMem);

		//Obtain the actual size of the clipboard item --
		sizMem=(size_t)((HPBYTE)memchr(hpMem,0,::GlobalSize(hMem))-hpMem);
	}

    //Our strategy is to first allocate an array of huge pointers
    //to point to the contiguous lines in the clipboard data --

    numLines=numPtrs=0;
    while(hpEOL=(HPBYTE)memchr(hpMem,'\n',sizMem)) {
		if(numLines+m_nNumLines==MAXLINES) {
			//We can give the user the option to truncate or cancel if
			//the editor's line capacity is reached, but we don't want
			//to display a message box while the clipboard is open --
			if(bAllowTrunc && numLines) {
			   bLinesClipped=FALSE;
			   sizTrail=0;
			   goto _retry1;
			}
			//Casts required due to compiler bug(?) --
			ids_err=numLines?(UINT)IDS_ERR_MAXCLIPLINES1:(UINT)IDS_ERR_MAXCLIPLINES;
			goto _retAlloc;
		}
		numLines++;
		if(numLines>=numPtrs) {
			 LPVOID p=(LPVOID)realloc(hpArray,sizeof(HPBYTE)*(numPtrs+=MAXLININC));
			 if(p==NULL) {
				ids_err=IDS_ERR_MEMORY;
				goto _retAlloc;
			 }
			 hpArray=(HPBYTE FAR *)p;
			 if(numLines==1) hpArray[0]=hpMem;
		}
		hpEOL++;
		sizMem-=(DWORD)(hpEOL-hpMem);
		hpArray[numLines]=hpMem=hpEOL;
    }

    bLinesClipped=sizMem>MAXLINLEN;

    sizTrail=bLinesClipped?MAXLINLEN:(int)LOWORD(sizMem);

    //The item consists of numLines CRLF-terminated lines and a trailing
    //string, hpMem, of length sizTrail.

_retry1:
	pLine=m_pActiveBuffer;
	len=*pLine;
    ASSERT(range.chr0<=len);

    if(!numLines) {
      //Try inserting a section of length sizTrail, truncating
      //as necessary --
		if(len+sizTrail>MAXLINLEN) {
		  bLinesClipped=TRUE;
		  if(range.chr0+sizTrail>MAXLINLEN) {
		    len=range.chr0;
		    sizTrail=MAXLINLEN-len;
		  }
		  else len=MAXLINLEN-sizTrail;
		}
		pLine+=range.chr0+1;
		if(range.chr0<len) memmove(pLine+sizTrail,pLine,len-range.chr0);
		memcpy(pLine,hpMem,sizTrail);
		len+=sizTrail;
		if(TrimTabs(m_pActiveBuffer+1,len)) bLinesClipped=TRUE;
		if(!PutLine(m_pActiveBuffer+1,len,range.line0-1)) {
		  ids_err=IDS_ERR_MEMORY;
		  goto _retAlloc;
		}
		range.line1=range.line0;
		range.chr1=range.chr0+sizTrail;
		bIsModified=TRUE;
    }
	else {
	  if(sizTrail||range.chr0) {
			//We need to revise the current line before shifting it down with
			//the insertion. The first range.chr0 chars of the current line,
			//which are *replaced* by the trailing string, are retained in
			//the active buffer. PutClipLine() prefixes the first inserted
			//line with this original segment--
			BYTE linbuf[MAXLINLEN];
			int cpylen=len-range.chr0;
			if(cpylen+sizTrail>MAXLINLEN) {
			  bLinesClipped=TRUE;
				cpylen=MAXLINLEN-sizTrail;
			}
			if(sizTrail) memcpy(linbuf,hpMem,sizTrail);
			if(cpylen) memcpy(linbuf+sizTrail,pLine+range.chr0+1,cpylen);
			if((cpylen+=sizTrail) && TrimTabs(linbuf,cpylen)) bLinesClipped=TRUE;
			if(!PutLine(linbuf,cpylen,range.line0-1)) {
			  ids_err=IDS_ERR_MEMORY;
			  goto _retAlloc;
			}
			bIsModified=TRUE;
		}
		numPtrs=InsertLines(range.line0-1,hpArray,numLines,range.chr0);
		if(numPtrs) bIsModified=TRUE;
		if(bIsModified) {
		  range.line1=range.line0+numPtrs;
		  range.chr1=sizTrail;
	    }
		if(numPtrs<numLines) ids_err=IDS_ERR_CLIPPASTE2;
	}

_retAlloc:
    free(hpArray);
    if(hMem) ::GlobalUnlock(hMem);

_retClose:
	if(!hMem) return ids_err==0;

    ::CloseClipboard();

    if(ids_err==IDS_ERR_MAXCLIPLINES1) {
      ids_err=0;
      if(CMsgBox(MB_OKCANCEL,IDS_ERR_MAXCLIPLINES1,numLines)==IDOK) {
        bAllowTrunc=TRUE;
        goto _retry0;
      }
    }
    if(!ids_err && bLinesClipped) ids_err=IDS_ERR_LINESCLIPPED;
    //Only ids_err==IDS_ERR_CLIPPASTE2 uses the extra params --
    if(ids_err) CMsgBox(MB_ICONEXCLAMATION,ids_err,numPtrs,numLines);

	return bIsModified;
}

#ifdef _GET_RANGE_CRC
LONG CLineDoc::GetRangeCrc(CLineRange& range)
{
  LINENO n;
  LINENO nMax=range.line1;

  if(range.chr1!=INT_MAX) nMax--;

  uLong crc = adler32(0L, Z_NULL, 0);

  //crc = crc32(crc, buffer, length);

  byte *buf;
  byte c=0x0A;

  for(n=range.line0;n<=nMax;n++) {
	  buf=(n==m_nActiveLine)?m_pActiveBuffer:LinePtr(n);
	  crc=adler32(crc,buf+1,*buf);
	  crc=adler32(crc,&c,1);
  }
  if(range.chr1!=INT_MAX) {
	  buf=(n==m_nActiveLine)?m_pActiveBuffer:LinePtr(n);
	  crc=adler32(crc,buf+1,*buf-range.chr1);
	  crc=adler32(crc,&c,1);
  }
  return crc;
}

void CLineDoc::Check_CRC(UINT uUpd)
{
	CLineRange range;
	range.line0=1;
	range.line1=m_nNumLines;
	range.chr0=0;
	range.chr1=INT_MAX;
	size_t crc=GET_RANGE_CRC(range); //at least 1
	if(uUpd&UPD_UNDOREDO) {
		m_Hist.CompareChecksum(uUpd,crc);
	}
	else {
		m_Hist.SaveChecksum(crc);
	}
}
#endif

LONG CLineDoc::GetRangeSize(CLineRange& range)
{
  LINENO n;
  LONG size=1;	 //include room for trailing NULL
  LINENO nMax=range.line1;

  if(range.chr1!=INT_MAX) nMax--;

  for(n=range.line0;n<=nMax;n++) {
	  size+=(2+((n==m_nActiveLine)?*m_pActiveBuffer:*LinePtr(n)));
  }
  if(range.chr1!=INT_MAX) size+=range.chr1;
  return size-range.chr0;
}

void CLineDoc::CopyRange(HPBYTE pDest,CLineRange& range,int nLenMax)
{
	LPBYTE pSrc;
	int len;
	LINENO n=range.line0;
	LINENO nMax=range.line1;

#ifdef _DEBUG
	len=GetRangeSize(range)-1;
	if(nLenMax!=len) {
		ASSERT(FALSE);
	}
#endif

	if(range.chr1!=INT_MAX) nMax--;
	pSrc=(n==m_nActiveLine)?m_pActiveBuffer:LinePtr(n);
	len=*pSrc-range.chr0;
	pSrc+=range.chr0+1;

	while(n<=nMax) {
		if(len+2>nLenMax) {
			ASSERT(FALSE);
			return;
		}
		nLenMax-=len+2;
		memcpy(pDest,pSrc,len);
		pDest+=len;
		*pDest++='\r';
		*pDest++='\n';
		n++;
		pSrc=(n==m_nActiveLine)?m_pActiveBuffer:LinePtr(n);
		len=*pSrc++;
	}
	if(n==range.line1 && (len=range.chr1) && (n!=range.line0 || (len-=range.chr0))) {
		if(len>nLenMax) {
			ASSERT(FALSE);
			return;
		}
		nLenMax-=len;
		memcpy(pDest,pSrc,len);
	}
	ASSERT(!nLenMax);
}

BOOL CLineDoc::ClipCopy(CLineRange& range)
{
	int nLenMax=GetRangeSize(range);
	HGLOBAL	hMem=::GlobalAlloc(GHND,nLenMax);

	if(!IsOK(hMem!=NULL,IDS_ERR_CLIPCOPY)) return FALSE;

	CopyRange((HPBYTE)::GlobalLock(hMem),range,nLenMax-1);
	::GlobalUnlock(hMem);

    if(AfxGetMainWnd()->OpenClipboard()) {
	  BOOL bRet = ::EmptyClipboard() && ::SetClipboardData(CF_TEXT,hMem);
		::CloseClipboard();
		if(bRet) return TRUE;
	}

	::GlobalFree(hMem);
	return IsOK(FALSE,IDS_ERR_CLIPCOPY);
}

BOOL CLineDoc::Update(UINT uUpd,LPCSTR pChar,int nLen,CLineRange range)
{
    BOOL bSelection=NormalizeRange(range);
    BOOL bDeleteOnly=uUpd&UPD_DELETE;
    BOOL bRet=TRUE;
	UINT upd;
	int linlen;

    //Allow ReplaceAll function an escape route --
	if(!LoadActiveBuffer(range.line0) && (uUpd&UPD_TRUNCABORT)) return FALSE;
	linlen=*m_pActiveBuffer;

    if(uUpd & UPD_CLIPCOPY) {
	  //UPD_CLIPCUT==UPD_DELETE|UPD_CLIPCOPY
	  if(!bSelection || !ClipCopy(range)) return FALSE;
	  if(!bDeleteOnly) return TRUE;
	}

	//First, clear the selection in all views. For the active view this also
	//moves the insertion point to the start of the normalized text range,
	//although it does not yet scroll it into view --
    m_hint.pRange=&range;
	UpdateViews(UPD_CLEARSEL);

	if(bDeleteOnly && !bSelection) {
	  range.chr1+=nLen;
	  if(range.chr1>linlen) range.chr1=INT_MAX;
	  bSelection=TRUE;
	}

    if(bSelection && range.chr1==INT_MAX && range.line1==m_nNumLines) {
        //We cannot replace or delete the final EOL --
        range.chr1=*LinePtr(range.line1);
        if(range.line0==range.line1 && range.chr0==range.chr1) {
          if(bDeleteOnly) return TRUE;
          bSelection=FALSE;
        }
    }

    if(bSelection) {

	  if(!(uUpd&UPD_UNDOREDO) &&
		  !m_Hist.DelTextRange(range,bDeleteOnly || (!nLen && !(uUpd&UPD_CLIPPASTE)))) return FALSE;

      //For now, ignore update if user is notified of merge failure --
	  if(!DeleteTextRange(range)) {
		  if(!(uUpd&UPD_UNDOREDO)) m_Hist.ClearLastAction();
		  return FALSE;
	  }

	  //Switch to insert mode if necessary (old bug) --
	  if(uUpd&UPD_REPLACE) {
		  uUpd=((uUpd&~UPD_REPLACE)|UPD_INSERT);
	  }

      BOOL bLinesDeleted=range.line1>range.line0 || range.chr1==INT_MAX;

      //The scrolling/bar updating, if required, is deferred if
      //a text insertion follows --
      if(bDeleteOnly) {
        upd=UPD_SCROLL|UPD_DELETE;
	    if(bLinesDeleted) upd|=UPD_BARS;
      }
      else upd=UPD_DELETE;

      //Let views invalidate for the deletion --
      UpdateViews(upd);
      if(bDeleteOnly) RETURN_;

	  upd=bLinesDeleted?UPD_BARS:0;
      linlen=*m_pActiveBuffer; //may have changed due to merge
    }
    else upd=0;

#ifdef _DEBUG
	if(range.chr0>linlen) {
		ASSERT(FALSE);
	}
#endif

    //The last view update operation will normally conclude with scrolling
    //to the active position, resetting the caret, and (if lines were deleted or
	//inserted) updating the scroll bars.
	
	upd |= UPD_SCROLL;

    if(uUpd & UPD_CLIPPASTE) {
      //ClipPaste() if successful will set the range of the inserted block --
      if(ClipPaste(range,(HPBYTE)pChar,nLen)) {
		if(!(uUpd&UPD_UNDOREDO)) m_Hist.AddPasteRange(range,bSelection);
        if(range.line1>range.line0) upd |=UPD_BARS;
		upd |= UPD_INSERT;
	  }
	  else bRet=FALSE;
	  UpdateViews(upd);
      RETURN_;
    }

    //For now, assume one string (no EOL) or one EOL ('\r') is being inserted
    //or replaced --

    upd|=(uUpd&(UPD_REPLACE|UPD_INSERT));
/*
    if(range.line0==MAXLINES && (upd&(UPD_REPLACE|UPD_INSERT))!=0) {
      IsOK(FALSE,IDS_ERR_MAXLINES);
      return FALSE;
    }
*/
    BYTE *pDest=m_pActiveBuffer+1+range.chr0;
    
	if(upd&UPD_INSERT) {

	  if(!nLen) {
	     //Truncate line0 and insert a new line *beneath* it containing the clipped
	     //portion. Invoked by CWedView::InsertEOL() --
	     upd |= UPD_BARS;

#ifdef _DEBUG
		 if(bSelection && !(uUpd&UPD_REDO)) Check_CRC(uUpd);
#endif
		 //How much whitespace precedes chr0?
		 nLen=range.chr0;
		 while(nLen>0 && IsWS(m_pActiveBuffer[nLen])) nLen--;
		 if(!(uUpd&UPD_UNDOREDO) && !m_Hist.AddEOL(range.line0,nLen,range.chr0-nLen)) return FALSE;

	     if(IsOK(InsertEOL(range.chr0),IDS_ERR_MEMORY)) {
		    ASSERT(*m_pActiveBuffer==range.chr0);
			if(*m_pActiveBuffer>(byte)nLen) {
				*m_pActiveBuffer=(byte)nLen;
				MarkActiveBuffer();
			}
			range.line1=range.line0;
			range.chr1=INT_MAX;
	        UpdateViews(upd);
			RETURN_;
	     }

		 if(!(uUpd&UPD_UNDOREDO)) m_Hist.ClearLastAction();
		 if(bSelection) {
		    //At least handle UPD_SCROLL/BARS after a delete.
			upd&=~UPD_INSERT; 
			UpdateViews(upd);
		 }
	     return FALSE;
	  }
	  
	  if(IsOK(linlen+nLen<=MAXLINLEN,IDS_ERR_MAXLINLEN) &&  (*pChar!=VK_TAB ||
        IsOK(TabCount(m_pActiveBuffer,*m_pActiveBuffer)<MAXLINTABS,IDS_ERR_MAXLINTABS)))
      {
		if(range.chr0<linlen) memmove(pDest+nLen,pDest,linlen-range.chr0);
	    memcpy(pDest,pChar,nLen);
	    *m_pActiveBuffer=(BYTE)(linlen+nLen);
	    MarkActiveBuffer();
		if(!(uUpd&UPD_UNDOREDO)) {
			range.line1=range.line0;
			range.chr1=range.chr0+nLen;
			m_Hist.AddTextRange(pChar,nLen,range,bSelection);
		}
      }
      else {
        if(!bSelection) return FALSE;
        bRet=FALSE;
        nLen=0;
      }
    }
	else { //UPD_REPLACE
	  //We can assume we are NOT handling VK_TAB or VK_RETURN
	  //for which it is assumed the view uses UPD_INSERT (see above) --
	  if(range.chr0+nLen>linlen) linlen=range.chr0+nLen;
	  if(IsOK(linlen<=MAXLINLEN,IDS_ERR_MAXLINLEN)) {
		  ASSERT(!bSelection && range.line0==range.line1); //No deletion occurred
		  if(!(uUpd&UPD_UNDOREDO) && !m_Hist.RepShortStr((LPCSTR)pDest,pChar,nLen,range)) return FALSE;
		  memcpy(pDest,pChar,nLen);
		  *m_pActiveBuffer=(BYTE)linlen;
		  MarkActiveBuffer();
	  }
      else {
        bRet=FALSE;
        nLen=0;
      }
	}

	//nLen==0 if nothing was inserted, bSelection==FALSE if nothing was deleted  --
	if(!nLen && !bSelection)  return bRet;
	//If selected text was deleted we still need to tell views
	//to set m_nNumLines and to adjust their scroll bars --

	range.line1=range.line0;
	range.chr1=range.chr0+nLen;
	//m_hint.pRange=&range;    //already set!
	UpdateViews(upd);

#ifdef _DEBUG
return_:
	Check_CRC(uUpd);
#endif
    return bRet;
}

static inline int GetTrimmedLength(LPBYTE p)
{
  int len=*p;
  while(len && CLineDoc::IsWS(p[len])) len--;
  return len;
}

BOOL CLineDoc::OnSaveDocument(const char* pszPathName)
{
    if(m_bActiveChange) {
	  ASSERT(!m_bReadOnly);
	  SaveActiveBuffer();
	  if(!IsModified()) {
	    CheckTitleMark(FALSE);
	    return TRUE;
	  }
	}
	else if(!IsModified()) return TRUE;

	if(m_bReadOnly) {
		ASSERT(FALSE);
		return TRUE;
	}
	
	//BAK file processing should be inserted here!

	CFileBuffer file(SAVEBUFSIZ);
	if(file.OpenFail(pszPathName,CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive)) {
	    m_bSaveAndCloseError=TRUE;
		return FALSE;
    }
    
	BeginWaitCursor();

	TRY
	{
		for(LINENO n=0;n<m_nNumLines;n++) {
		  file.Write(m_pLinePtr[n]+1,GetTrimmedLength(m_pLinePtr[n]));
		  if(n==m_nNumLines-1) break;
		  file.Write("\r\n",2);
		}
	    file.Close();
	}
	CATCH(CFileException,e)
	{
		file.Abort(); // will not throw an exception
		EndWaitCursor();

		TRY
			file.ReportException(pszPathName,e,TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);
		END_TRY
	    m_bSaveAndCloseError=TRUE;
		return FALSE;
	}
	END_CATCH
	
  	EndWaitCursor();

    SetModifiedFlag(FALSE);
    CheckTitleMark(FALSE);
	m_Hist.m_nPosSaved=m_Hist.m_nPos;
    CPrjDoc::FixAllFileChecksums(pszPathName);

    return TRUE;
}

void CLineDoc::DeleteContents()
{
	// Clear out the CDocument object for potential reuse.

	if(m_pLinePtr) {
	  free(m_pLinePtr);
	  m_pLinePtr=NULL;
	}
	while(m_pFirstBuffer) {
	  CBuffer *p=m_pFirstBuffer;
	  free(p->pBuf);
	  m_pFirstBuffer=p->pNext;
	  free(p);
	}
	m_nNumLines=m_nAllocLines=m_nMaxLines=m_nActiveLine=0;
	m_cBufferSpace=0;
	m_bActiveChange=m_bTitleMarked=m_bNewEmptyFile=FALSE;
}

///////////////////////////////////////////////////////////////////////
// Operations

BOOL CLineDoc::ExtendMaxLines(LINENO nMinMax)
{
    LINENO n;

    if(m_nMaxLines==MAXLINES) return FALSE;
    //Reallocate the line pointer array, making it MAXLININC (0x400)
    //elements (4K bytes) larger --
    if((n=m_nMaxLines+MAXLININC)>MAXLINES) n=MAXLINES;

    if(n<nMinMax) return FALSE;
    VOID FAR *pv=realloc(m_pLinePtr,n*sizeof(LPBYTE));
    if(pv==NULL) {
      #ifdef _DEBUG
      n*=sizeof(LPBYTE); //examine this
      #endif
      return FALSE;
    }
    m_nMaxLines=n;
    m_pLinePtr=(LPBYTE FAR *)pv;
    return TRUE;
}

BOOL CLineDoc::AllocLine(BYTE linlen,LINENO nLineIndex)
{
  //Configures m_pLinePtr[nLineIndex] to point to a new uninitialized
  //buffer of length linlen+1 --

  LINENO n;
  BOOL bOrphan;

  for(n=m_nNumLines;n<m_nAllocLines;n++) if(*m_pLinePtr[n]>=linlen) break;
  if(n<m_nAllocLines) {
    //We found a slot large enough in the unused portion of the
    //allocated pointer array --
    if(n==nLineIndex) {
      //We are appending a new line, using the first available slot --
      ASSERT(n==m_nNumLines);
      m_nNumLines++;
    }
    else {
      //Swap pointers so that the new m_pLinePtr[nLineIndex] points to
      //a slot of sufficient size. Currently we allow the wastage of
      //*m_pLinePtr[nLineIndex]-linlen bytes, which is not recoverable
      //until the file is closed and reopened.
      LPBYTE p=m_pLinePtr[nLineIndex];
      m_pLinePtr[nLineIndex]=m_pLinePtr[n];
      ASSERT(p);
      m_pLinePtr[n]=p;
      if(nLineIndex==m_nNumLines) m_nNumLines++;
    }
    return TRUE;
  }

  //At this point n==m_nAllocLines. We need to initialize a new line pointer
  //if we are to preserve all existing pointers to allocated space.
  //The complete pointer array (uncluding unused elements) is currently
  //m_nMaxLines long. We may need to extend it --
  if(n==m_nMaxLines && !ExtendMaxLines(m_nMaxLines+1)) {
    //If there are no unused preallocated pointers then we've probably
    //exceeded MAXLINES (assuming a realloc of the pointer array didn't fail).
    //Abort if we are trying to append --
    if(n==nLineIndex) return FALSE;
    //Let's simply orphan the space pointed to by m_pLinePtr[nLineIndex].
    //Recall we are suballocating -- this is not a leak!
    bOrphan=TRUE;
  }
  else bOrphan=FALSE;

  //We may need to create space for a new line. We do this in increments
  //of size LINBUFSIZ (0x2000 or 8K) bytes. Note that m_cBufferSpace and
  //m_pFirstBuffer are cleared by DeleteContents()  --
  if(m_cBufferSpace<=linlen) {
    //This reduction (extend()) of an 8K block probably has little value --
    if(m_cBufferSpace) BlockReduce(LINBUFSIZ-m_cBufferSpace);
    if(!BlockAlloc(LINBUFSIZ)) return FALSE;
  }

  //At this point m_pFirstBuffer->pBuf is a buffer of length LINBUFSIZ
  //with m_cBufferSpace bytes unused space remaining.

  //If bOrphan==TRUE, we are going to simply discard the current value of
  //m_pLinePtr[nLineIndex] without extending m_nAllocLines --

  if(!bOrphan) {
    //m_pLinePtr[nLineIndex<=m_nNumLines] either points to a slot that is
    //too small, or is an uninitialized pointer with nLineIndex==m_nAllocLines
    //in which case there are no "unused" pointers. In the first case we save
    //the old pointer as the last unused pointer --
    if(m_nAllocLines!=nLineIndex) m_pLinePtr[m_nAllocLines++]=m_pLinePtr[nLineIndex];
    else m_nAllocLines++;
  }

  //We take just enough space to hold the line --
  m_pLinePtr[nLineIndex]=m_pFirstBuffer->pBuf+LINBUFSIZ-m_cBufferSpace;
  m_cBufferSpace-=linlen+1;

  //If we are appending a new line..
  if(nLineIndex==m_nNumLines) m_nNumLines++;
  return TRUE;
}

BOOL CLineDoc::PutLine(BYTE *pLine,int linlen,LINENO nLineIndex)
{
    if(nLineIndex>=m_nNumLines || linlen>*m_pLinePtr[nLineIndex]) {
      //Store line in a new location --
      if(!AllocLine(linlen,nLineIndex)) return FALSE;
    }
    else {
      //Let's avoid unnecessary setting of the modified flag --
      if(linlen==*m_pLinePtr[nLineIndex] && (!linlen ||
        !memcmp(pLine,m_pLinePtr[nLineIndex]+1,linlen))) return TRUE;
    }

    if(*m_pLinePtr[nLineIndex]=linlen)
      memcpy(m_pLinePtr[nLineIndex]+1,pLine,linlen);

    SetModifiedFlag(TRUE);
    return TRUE;
}

#if 0
void CLineDoc::SetPathName(const char* pszPathName, BOOL bAddToMRU)
{
    //We could override the CDocument version in order to customize the
    //document related captions that appear on the various view and mainframe
    //windows that employ the FWS_ADDTOTITLE style attribute. For example,
    //CMDIChildWnd::OnUpdateFrameTitle() will call CMDIFrameWnd::OnUpdateFrameTitle()
    //(which handles the application frame's caption), and then call GetWindowText(),
    //CDocument::GetTitle(), and SetWindowText() to append the document title
    //along with a view number when required.

    //The CDocument version of this function does not include the path prefix
    //in the title. We want to make this optional.

    //NOTE: Apart from this we want to use the above framework functionality.
    //We have also overridden OnUpdateFileSave() to maintain a changed file
    //asterisk suffix on the title. Alternatively, we could leave the path
    //off the title and let our derived CMDIChildWnd handle the WM_SETTEXT
    //and WM_GETTEXT messages (triggered by Set/GetWindowText), using what
    //the framework believes is the caption to maintain a different actual
    //caption. We could also shorten it for an iconized window.

	m_strPathName = pszPathName;
	ASSERT(!m_strPathName.IsEmpty());       // must be set to something

	// Set the document title based on the complete pathname --
	if(m_bCaptionCustom) SetTitle(pszPathName);
	else {
		char szTitle[_MAX_FNAME];
		if (::GetFileTitle(m_strPathName, szTitle, _MAX_FNAME) == 0)
		{
			::CharUpper(szTitle);       // always upper case
			SetTitle(szTitle);
		}
	}
	if (bAddToMRU) AfxGetApp()->AddToRecentFileList(pszPathName);
}
#endif

void CLineDoc::MarkDocTitle(BOOL bChg)
{
	 m_csFrameTitle.Truncate(m_lenFrameTitle);
     if(bChg) m_csFrameTitle+='*';
	 POSITION vpos=GetFirstViewPosition();
	 if(vpos) ((CTxtFrame *)(GetNextView(vpos)->GetParent()))->SetWindowText(m_csFrameTitle);
}

void CLineDoc::CheckTitleMark(BOOL bChg)
{
    if(m_bTitleMarked!=bChg) {
      //Adds or removes '*' to title
      MarkDocTitle(m_bTitleMarked=bChg);
      
      //If there are open projects, this would be a good time to set
      //the m_nEditsPending counts in the appropriate nodes --
      CPrjDoc::FixAllEditsPending(GetPathName(),bChg);
    }
}

void CLineDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
    //For a changed file, flag the window/icon title with an asterisk.
    //Change this code to use a different kind of flag.

    //NOTE: CDocument::OnFileSave() calls DoSave(m_pszPathName,TRUE)
    //which acts on TRUE to update the document title with the pathname
    //by calling SetPathName() (which calls SetTitle()) when successful.
    //Thus we don't need to ever clear the flag for a saved file.
    //We have also overridden SetPathName() to optionally include the
    //file's path in the title.

    BOOL bChg=m_bActiveChange || IsModified();
    CheckTitleMark(bChg);
	pCmdUI->Enable(bChg);
}

void CLineDoc::OnFileSaveAs()
{
	CString strFilter;
	CString newName=(LPCSTR)m_strPathName;

	if(!AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;
	if(!DoPromptPathName(newName,
	  OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST,1,strFilter,FALSE,
	  IDS_SRV_FILESAVEAS,
	  NULL)) {
		return;
	}

	//Don't allow SaveAs if new name matches a currently open file --
	if(GetLineDoc(newName)) {
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_SRVSAVEAS,(LPCSTR)newName);
		return;
	}

	CPrjListNode *pNode=CPrjDoc::FindFileNode(NULL,(LPCSTR)newName);
	if(pNode) {
		if(IDOK!=CMsgBox(MB_OKCANCEL,IDS_PRJ_SAVEASLEAF,pNode->Title())) return;
	}

	BOOL bReadOnly=m_bReadOnly;
    BOOL bModified=IsModified();
	SetModifiedFlag(TRUE);
	m_bReadOnly=FALSE;

	if(!OnSaveDocument(newName))
	{
		//CFile::Remove(newName);
		if(!bModified) SetModifiedFlag(FALSE);
		m_bReadOnly=bReadOnly;
		return;
	}

	// reset the title and change the document name
	SetPathName(newName);

	if(pNode) UpdateOpenFileTitle(pNode->Title(),pNode->Name());
	else UpdateOpenFileTitle(trx_Stpnam(GetPathName()),"");
}

BOOL CLineDoc::SaveModified()
{
    //This is the CDocument version with three modifications. The third one (a check
    //for errno==ENOENT) omits the "save as" dialog when the file doesn't exist.
    //If it doesn't exist (and the user has already confirmed the save) then we want it
    //to be created without further prompting!

    SaveActiveBuffer();  //*** First modification (line inserted)
    
	if (!IsModified())	return TRUE;        // ok to continue

	if(m_bReadOnly) {
		ASSERT(FALSE);
		return TRUE;
	}

	// get name/title of document
	CString name = m_strPathName;
	ASSERT(!name.IsEmpty());		//*** CDocument version loads AFX_IDS_UNTITLED

	CString prompt;
	AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, name);
	switch (AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE))
	{
	case IDCANCEL:
		return FALSE;       // don't continue

	case IDYES:
		// If so, either Save or Update, as appropriate
		if (_access(m_strPathName, 6) != 0 && errno!=ENOENT)  //*** errno modification
		{
			if (!DoSave(NULL))
				return FALSE;   // don't continue
		}
		else
		{
			if (!DoSave(m_strPathName))
				return FALSE;   // don't continue
		}
		break;

	case IDNO:
		// If not saving changes, revert the document
		break;

	default:
		ASSERT(FALSE);
		break;
	}
	return TRUE;    // keep going
}

void CLineDoc::SaveAndClose(BOOL bClose)
{
    //Unconditionally save and optionally close the document.
    //Called from CPrjView::OnCompileItem().
    
    POSITION pos=GetFirstViewPosition();
    if(pos) {
      CView *pView=GetNextView(pos);
      pView->PostMessage(WM_COMMAND,ID_FILE_SAVE);
      if(bClose) pView->PostMessage(WM_COMMAND,ID_FILE_CLOSE);
    }
}

void CLineDoc::OnCloseDocument()
{
  CPrjDoc::FixAllEditsPending(GetPathName(),FALSE);
  CDocument::OnCloseDocument();
}

void CLineDoc::OnFileSave()
{
    //NOTE: We bypass CDocument::OnFileSave() --> DoSave() --
	OnSaveDocument(m_strPathName);
}

BOOL CLineDoc::Close(BOOL bNoSave)
{
	POSITION vpos=GetFirstViewPosition();
	if(vpos) {
	  if(bNoSave) {
	    m_bActiveChange=FALSE;
	    SetModifiedFlag(FALSE);
	  }
	  return 0==GetNextView(vpos)->SendMessage(WM_COMMAND,ID_FILE_CLOSE);
	}
	return FALSE;
}

void CLineDoc::UpdateOpenFileTitle(LPCSTR newTitle,LPCSTR newName)
{
	POSITION vpos=GetFirstViewPosition();
	if(!vpos) return;
	CTxtFrame *pFrm=(CTxtFrame *)(GetNextView(vpos)->GetParent());

	m_csFrameTitle=m_bReadOnly?"<LOCK> ":"";
	if(*newName && _stricmp(newName,newTitle))
		m_csFrameTitle.AppendFormat("%s - %s", newName, newTitle);
	else m_csFrameTitle+=newTitle;
	m_lenFrameTitle=m_csFrameTitle.GetLength();
	if(IsModified()||m_bActiveChange) m_csFrameTitle+="*";
	pFrm->SetWindowText(m_csFrameTitle);
}

BOOL CLineDoc::SaveAllOpenFiles()
{
	//static function --
    CLineDoc::m_bSaveAndCloseError=FALSE;
	//Save open files only for compiles, not reviews --        
	CMultiDocTemplate *pTmp=((CWallsApp *)AfxGetApp())->m_pSrvTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	while(pos != NULL) {
	  //Post messages ID_FILE_SAVE and/or ID_FILE_CLOSE --
	  ((CLineDoc *)pTmp->GetNextDoc(pos))->SaveAndClose(FALSE);  //TRUE -> Close also
	}
	Pause(); //Wait for all posted messages to be processed()
	return CLineDoc::m_bSaveAndCloseError==FALSE;
}

CLineDoc *CLineDoc::GetLineDoc(LPCSTR pathname)
{
	CMultiDocTemplate *pTmp=((CWallsApp *)AfxGetApp())->m_pSrvTemplate;
	POSITION pos = pTmp->GetFirstDocPosition();
	CLineDoc *pDoc;

	while(pos != NULL) {
		pDoc=(CLineDoc *)pTmp->GetNextDoc(pos);
		if(!_stricmp(pDoc->GetPathName(),pathname)) return pDoc;
	}
	return NULL;
}

void CLineDoc::UpdateOpenFilePath(LPCSTR oldPath,LPCSTR newPath)
{
	CLineDoc *pDoc=GetLineDoc(oldPath);
	if(pDoc) pDoc->m_strPathName=newPath;
}

void CLineDoc::DisplayLineViews()
{
	POSITION pos;
	CLineView *pView=NULL;

	if(pos=GetFirstViewPosition()) {
		pView=(CLineView *)GetNextView(pos);
		CWnd *pWnd;
		if(pView && (pWnd=pView->GetParentFrame())) {
			if(pWnd->IsIconic()) pWnd->ShowWindow(SW_RESTORE);
			else pWnd->BringWindowToTop();
		}
	}
	
	if(pView && CLineView::m_nLineStart) {
	     //Since m_nLineStart was NOT reset to zero, we know the survey was reactivated
	     //rather than newly opened. We must reposition the caret from here --
	     pView->ScrollToError();
	}
	else CLineView::m_nLineStart=0;
}

void CLineDoc::SetUnModified()
{
	if(SaveActiveBuffer()) {
		SetModifiedFlag(FALSE);
		CheckTitleMark(FALSE);
	}
	else m_Hist.ClearHistory();
}

int CLineDoc::SaveReadOnly(BOOL bNoPrompt)
{
	if(IsModified()||m_bActiveChange) {
		ASSERT(!m_bReadOnly);
		if(bNoPrompt || IDOK==CMsgBox(MB_OKCANCEL,IDS_PRJ_OPENCHMOD1,m_strPathName)) {
			SaveAndClose(FALSE);
			Pause(); //Wait for all posted message to be processed()
			return 2; //Saved file, possibly created a new file
		}
		return 0; //User cancel
	}
	return 1; //No save required but OK to change status
}

BOOL CLineDoc::SetReadOnly(CPrjListNode *pNode,BOOL bReadOnly,BOOL bNoPrompt)
{
	if(bReadOnly && !SaveReadOnly(bNoPrompt)) return FALSE; //0=cancel, 1-unchanged, 2=saved

	 //SetFileReadOnly(): -1=error,0=absent,1-nochange,2=changed --
	if(SetFileReadOnly(m_strPathName,bReadOnly,bNoPrompt)<0) return FALSE;

	if(m_bReadOnly!=bReadOnly) {
		m_bReadOnly=bReadOnly;
		if(pNode) {
			UpdateOpenFileTitle(pNode->Title(),pNode->Name());
			if(hPropHook && pItemProp->m_pNode==pNode) {
				pItemProp->UpdateReadOnly(m_bReadOnly);
			}
		}
		else UpdateOpenFileTitle(trx_Stpnam(m_strPathName),"");
		return TRUE;
	}

	return FALSE;
}

void CLineDoc::ToggleWriteProtect()
{
	SetReadOnly(CPrjDoc::FindFileNode(NULL,m_strPathName),!m_bReadOnly,FALSE); //allow prompts
}