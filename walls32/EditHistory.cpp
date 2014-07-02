// EditHistory.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "linedoc.h"
#include "EditHistory.h"
#include <string>

/***********************************************************************
class CEditHistory
***********************************************************************/

#ifdef _DEBUG
LPCSTR action_name[7]= {
	"AddChar","DelChar","AddEOL",
	"DelTextRange","AddTextRange","DelAddTextRange","RepShortStr"
};
#endif

CActionDelChar CEditHistory::m_cDelChar;
CActionAddChar CEditHistory::m_cAddChar;
CActionDelTextRange CEditHistory::m_cDelTextRange;
CActionAddTextRange CEditHistory::m_cAddTextRange;
CActionDelAddTextRange CEditHistory::m_cDelAddTextRange;
CActionRepShortStr CEditHistory::m_cRepShortStr;
CActionAddEOL CEditHistory::m_cAddEOL;

void CEditHistory::AddAction(CHistItem &histItem)
{
	// Add edit action to the history vector --
	ASSERT(m_nPos >= 0 && m_nPos <= (int)m_vecHistItem.size());
	m_vecHistItem.push_back(histItem);
	// advance the position
	m_nPos = GetSize();
}

void CEditHistory::Undo()
{
	// Steps back one in the history, calling the Undo
	// virtual function for the previous action.

	if(m_nPos <=0 || m_nPos > (int)m_vecHistItem.size()) {
		ASSERT(FALSE);
		return;
	}
	CHistItem &hist=m_vecHistItem[m_nPos-1];
	hist.m_pAction->Undo(this,hist.m_dataOffset);
	m_nPos--;
	m_LastAction=ACTION_Undo;

	if(m_nPos==m_nPosSaved) m_pDoc->SetUnModified();
}

void CEditHistory::Redo()
{
    // Steps forward one in the history, automatically calling the
    // Redo virtual function for the next action.
	// make sure that it is possible to step forward

	if(m_nPos < 0 || m_nPos >= (int)m_vecHistItem.size()) {
		ASSERT(FALSE);
		return;
	}
	CHistItem &hist=m_vecHistItem[m_nPos++];
	hist.m_pAction->Redo(this,hist.m_dataOffset);
	m_LastAction=ACTION_Redo;
	if(m_nPos==m_nPosSaved) m_pDoc->SetUnModified();
}

void CEditHistory::ClearEnd()
{
	// Clears the edit history beyond the current action
	// at position m_nPos-1.
	// Call this prior to creating each new history class.

	VecHistItemIt itBeg = m_vecHistItem.begin() + m_nPos;
	VecHistItemIt itEnd = m_vecHistItem.end();

	if(itBeg==itEnd) return;

	for(VecHistItemIt it=itEnd;it!=itBeg;) {
		--it;
		e_Action act=it->m_pAction->Action();
		if(act==ACTION_AddTextRange || act==ACTION_DelTextRange || act==ACTION_DelAddTextRange) {
			sTextRangeData *pData=(sTextRangeData *)&m_vecData[it->m_dataOffset];
			if(pData->pText) delete[] pData->pText;
			if(act==ACTION_DelAddTextRange && pData[1].pText) delete[] pData[1].pText;
		}
	}

	size_t cOffset=(*itBeg).m_dataOffset;
	// remove these actions from the array
	m_vecHistItem.erase(itBeg, itEnd);
	// resize m_vecData (does not reduce capacity) --
	m_vecData.resize(cOffset);
	if(m_nPos<m_nPosSaved) m_nPosSaved=-1;
}

CHistItem * CEditHistory::GetCurrentAction()
{
	// make sure that it is possible to step forward
	if(!m_nPos) return NULL;
	
	ASSERT(m_nPos > 0 && m_nPos <= (int)m_vecHistItem.size());

	// return a pointer to the current action
	return &m_vecHistItem[m_nPos-1];
}

#ifdef _DEBUG
void CEditHistory::SaveChecksum(size_t crc)
{
	ASSERT(m_nPos);
	m_vecHistItem[m_nPos-1].crc=crc;
}

void CEditHistory::CompareChecksum(UINT uUpd,size_t crc)
{
	size_t c;

	if(uUpd&CLineDoc::UPD_UNDO) {
		//During first undo, m_nPos==1 --
		c=(m_nPos>1)?m_vecHistItem[m_nPos-2].crc:crc0;
	}
	else {
		//During first redo, m_nPos==1 --
		c=m_vecHistItem[m_nPos-1].crc;
	}
	if(crc!=c) {
		const CHistItem *pHist=GetCurrentAction();
		#ifdef _GET_RANGE_CRC
		TRACE("%s.%sdo() created file with bad CRC\n",action_name[pHist->m_pAction->Action()],
			(uUpd&CLineDoc::UPD_UNDO)?"Un":"Re");
		#else
		TRACE("%s.%sdo() created bad file size: %d vs. %d\n",action_name[pHist->m_pAction->Action()],
			(uUpd&CLineDoc::UPD_UNDO)?"Un":"Re",crc,c);
		#endif
		ASSERT(FALSE);
	}
}
#endif

//====================================================================
//Static functions used by action routines --
//====================================================================

static void get_sRange(sRange &srange,CLineRange &range)
{
	//Used by DelTextRange() and AddPasteRange() --
	srange.line0=range.line0;
	srange.chr0=(byte)range.chr0;
	if(range.chr1==INT_MAX) {
		srange.line1=range.line1+1;
		srange.chr1=0;
	}
	else {
		srange.line1=range.line1;
		srange.chr1=(byte)range.chr1;
	}
}

static void get_pText(CLineDoc *pDoc,sTextRangeData *pData,CLineRange &range,BOOL bDeleteOnly)
{
	//Used by CActionDelTextRange::Redo() and DelTextRange() --

	if(pData->nLen>HIST_MAX_LOCALALLOC) pData->pText = new byte[pData->nLen];
	else {
		pData->pText=(byte *)&pData[bDeleteOnly?1:2];
	}
	pDoc->CopyRange(pData->pText,range,pData->nLen);
	if(pData->nLen<=HIST_MAX_LOCALALLOC) pData->pText=NULL;
}

//====================================================================
//Action class operations along with their corresponding
//CEditHistory member functions that create them --
//====================================================================

e_Action CActionAddChar::Action()
{
	return ACTION_AddChar;
}

void CActionAddChar::Undo(CEditHistory *pHist,size_t dataOffset)
{
	sAddCharData *pData=(sAddCharData *)&pHist->m_vecData[dataOffset];
	CLineRange range(pData->lineno,pData->charno,pData->lineno,pData->charno+pData->count);
	VERIFY(pHist->m_pDoc->Update(CLineDoc::UPD_UNDO|CLineDoc::UPD_DELETE,NULL,0,range));
}

void CActionAddChar::Redo(CEditHistory *pHist,size_t dataOffset)
{
	sAddCharData *pData=(sAddCharData *)&pHist->m_vecData[dataOffset];
	CLineRange range(pData->lineno,pData->charno,pData->lineno,pData->charno);
	VERIFY(pHist->m_pDoc->Update(CLineDoc::UPD_REDO|CLineDoc::UPD_INSERT,
		&pData->chr,pData->count,range));
}
bool CEditHistory::AddChar(int lineno,int charno,char c)
{
	CHistItem const *pHistItem;
	sAddCharData *pData;

	ASSERT(charno<=255);

	// clear the end of the history
	ClearEnd();

	if( c==0x09 || m_LastAction!=ACTION_AddChar ||
		!(pHistItem=GetCurrentAction()) || pHistItem->m_pAction->Action()!=ACTION_AddChar ||
		(pData=(sAddCharData *)&m_vecData[pHistItem->m_dataOffset])->lineno!=lineno ||
		charno!=pData->charno+pData->count) {
			size_t sz=m_vecData.size();
			//Place following in try/catch!
			//============================
			m_vecData.resize(sz+sizeof(*pData));
			pData=(sAddCharData *)&m_vecData[sz];
			pData->lineno=lineno;
			pData->charno=charno;
			pData->count=1;
			pData->chr=c;
			AddAction(CHistItem(sz,&m_cAddChar));
			m_LastAction=ACTION_AddChar;
	}
	else {
		++pData->count;
		//Place following in try/catch!
		//============================
		m_vecData.push_back(c);
		//============================
	}

	return true;
}

//====================================================================
e_Action CActionDelChar::Action()
{
	return ACTION_DelChar;
}

void CActionDelChar::Undo(CEditHistory *pHist,size_t dataOffset)
{
	sDelCharData *pData=(sDelCharData *)&pHist->m_vecData[dataOffset];

	LPCSTR p;
	int n;
	int chr0;
	std::string s;

	if(pData->count>0) {

		ASSERT(pData->charno>=pData->count-1);
		ASSERT((dataOffset+pData->count+sizeof(sDelCharData)-1) <= pHist->m_vecData.size());
		//VecDataRevIt itr(pHist->m_vecData.begin()+dataOffset+pData->count+sizeof(sDelCharData)-1); //fails!!
		VecDataRevIt itr(pHist->m_vecData.begin()+(dataOffset+pData->count+sizeof(sDelCharData)-1)); //works!!
		s.append(itr,itr+pData->count);
		p=s.data();
		n=pData->count;
		chr0=pData->charno-n+1;
	}
	else {
		p=&pData->chr;
		n=-pData->count;
		chr0=pData->charno;
	}
	CLineRange range(pData->lineno,chr0,pData->lineno,chr0);
	VERIFY(pHist->m_pDoc->Update(CLineDoc::UPD_UNDO|CLineDoc::UPD_INSERT,p,n,range));
}

void CActionDelChar::Redo(CEditHistory *pHist,size_t dataOffset)
{
	sDelCharData *pData=(sDelCharData *)&pHist->m_vecData[dataOffset];
	int n;
	int chr0;

	if(pData->count>0) {
		ASSERT(pData->charno>=pData->count-1);
		n=pData->count;
		chr0=pData->charno-n+1;
	}
	else {
		n=-pData->count;
		chr0=pData->charno;
	}
	CLineRange range(pData->lineno,chr0,pData->lineno,chr0+n);
	VERIFY(pHist->m_pDoc->Update(CLineDoc::UPD_REDO|CLineDoc::UPD_DELETE,NULL,0,range));
}

bool CEditHistory::DelChar(int lineno,int charno,char c)
{
	const CHistItem *pHistItem;
	sDelCharData *pData;

	ASSERT(charno<=255);

	// clear the end of the history
	ClearEnd();

	if( m_LastAction!=ACTION_DelChar ||
		!(pHistItem=GetCurrentAction()) || pHistItem->m_pAction->Action()!=ACTION_DelChar ||
		(pData=(sDelCharData *)&m_vecData[pHistItem->m_dataOffset])->lineno!=lineno ||

		!(/*BS key*/  (pData->count>0 && charno==pData->charno-pData->count) ||
		  /*DEL key*/ (pData->count<0 && charno==pData->charno) ||
		  /*Second press of DEL key*/ (pData->count==1 && charno==pData->charno && (pData->count=-1)))) {

			size_t sz=m_vecData.size();
			//Place following in try/catch!
			//============================
			m_vecData.resize(sz+sizeof(*pData));
			pData=(sDelCharData *)&m_vecData[sz];
			pData->lineno=lineno;
			pData->charno=charno;
			pData->count=1;
			pData->chr=c;
			AddAction(CHistItem(sz,&m_cDelChar));
			//============================
			m_LastAction=ACTION_DelChar;
	}
	else {
		if(pData->count<0) --pData->count;
		else ++pData->count;
		//Place following in try/catch!
		//============================
		m_vecData.push_back(c);
		//============================
	}

	return true;
}

//====================================================================
e_Action CActionDelTextRange::Action()
{
	return ACTION_DelTextRange;
}

void CActionDelTextRange::UndoRedo(CEditHistory *pHist,UINT uUpd,size_t dataOffset)
{
	//Reinsert deleted text in document, delete of document text not required.

	sTextRangeData *pData=(sTextRangeData *)&pHist->m_vecData[dataOffset];
	CLineRange range(pData->range.line0,pData->range.chr0,pData->range.line1,pData->range.chr0);
	byte *pText=pData->pText;
	if(!pText) pText=(byte *)&pData[1];
	uUpd|=((range.line0!=range.line1)?CLineDoc::UPD_CLIPPASTE:CLineDoc::UPD_INSERT);
	range.line1=range.line0;
	//NOTE: We've nullified range to avoid a deletion also --
	VERIFY(pHist->m_pDoc->Update(uUpd,(LPCSTR)pText,pData->nLen,range));
	//We no longer need to store text representation since it's now in the document --
	if(pData->pText) {
		delete[] pData->pText;
		pData->pText=NULL;
	}
}

void CActionDelTextRange::RedoUndo(CEditHistory *pHist,UINT uUpd,size_t dataOffset)
{
	//Redelete text in document, subsequent insertion of old document text not required.

	sTextRangeData *pData=(sTextRangeData *)&pHist->m_vecData[dataOffset];
	CLineRange range(pData->range.line0,pData->range.chr0,pData->range.line1,pData->range.chr1);
	//Place following in try/catch!
	//============================
	ASSERT(!pData->pText);
	get_pText(pHist->m_pDoc,pData,range,TRUE); //TRUE for delete only
	//============================
	VERIFY(pHist->m_pDoc->Update(CLineDoc::UPD_DELETE|uUpd,NULL,0,range));
}

void CActionDelTextRange::Undo(CEditHistory *pHist,size_t dataOffset)
{
	UndoRedo(pHist,CLineDoc::UPD_UNDO,dataOffset);
}

void CActionDelTextRange::Redo(CEditHistory *pHist,size_t dataOffset)
{
	RedoUndo(pHist,CLineDoc::UPD_REDO,dataOffset);
}

bool CEditHistory::DelTextRange(CLineRange &range,BOOL bDeleteOnly)
{
	if(bDeleteOnly && range.chr1-range.chr0==1 && range.line0==range.line1) {
		//Delete one character only --
		return DelChar(range.line0,range.chr0,m_pDoc->m_pActiveBuffer[range.chr0+1]);
	}

	ClearEnd();
	size_t sz=m_vecData.size();
	size_t nLen=m_pDoc->GetRangeSize(range)-1; //don't count trailing null

	//Place following in try/catch!
	//============================
	m_vecData.resize(sz+sizeof(sTextRangeData)*(bDeleteOnly?1:2)+((nLen>HIST_MAX_LOCALALLOC)?0:nLen));
	sTextRangeData *pData=(sTextRangeData *)&m_vecData[sz];
    get_sRange(pData->range,range);
	pData->pText=NULL;
	pData->nLen=nLen;
	get_pText(m_pDoc,pData,range,bDeleteOnly);

	IActionBase *pAction;
	if(bDeleteOnly) pAction=&m_cDelTextRange;
	else pAction=&m_cDelAddTextRange;
	AddAction(CHistItem(sz,pAction));
	//============================
	if(bDeleteOnly) m_LastAction=ACTION_DelTextRange;
	else {
		m_LastAction=ACTION_DelAddTextRange;
		pData[1].pText=NULL;
		pData[1].nLen=-1; //insert range unititialized!
		pData[1].range=pData->range;
		pData[1].range.chr1=pData->range.chr0;
	}

	return true;
}

//====================================================================
e_Action CActionAddTextRange::Action()
{
	return ACTION_AddTextRange;
}

void CActionAddTextRange::Undo(CEditHistory *pHist,size_t dataOffset)
{
	//Redelete deleted text in document, reinsertion of old document text not required.
	pHist->m_cDelTextRange.RedoUndo(pHist,CLineDoc::UPD_UNDO,dataOffset);
}

void CActionAddTextRange::Redo(CEditHistory *pHist,size_t dataOffset)
{
	//Reinsert deleted text in document, redeletion of document text not required.
	pHist->m_cDelTextRange.UndoRedo(pHist,CLineDoc::UPD_REDO,dataOffset);
}

bool CEditHistory::AddPasteRange(CLineRange &range,BOOL bDeleteAlso)
{
	//AddPasteRange() --
	//  Might be called from AddTextRange() --
	//  Fcn saves paste range to previous DelAddTextRange action if bDeleteAlso==true.
	//  Undo() will convert paste range to range+ptr before deleting paste area
	//  and reinserting previously deleted text in doc.
	//  Redo() will delete range from doc and "repaste" saved text.
	//  When Redo() returns, memory for pasted text is freed if pText!=NULL.

	sTextRangeData *pData;

	size_t nLen=m_pDoc->GetRangeSize(range)-1; //don't count trailing null
	size_t sz=m_vecData.size(); //base from which to resize vecData
	byte *pText=NULL;

	if(bDeleteAlso) {
		const CHistItem *pHist=GetCurrentAction();
		ASSERT(pHist && m_nPos==m_vecHistItem.size() && pHist->m_pAction->Action()==ACTION_DelAddTextRange);
		if(nLen<=HIST_MAX_LOCALALLOC) {
			m_vecData.resize(sz+nLen); //can throw
			pText=(byte *)&m_vecData[sz];
		}
		pData=(sTextRangeData *)&m_vecData[pHist->m_dataOffset]+1;
		ASSERT(pData->nLen==-1 && pData->pText==NULL);
	}
	else {
		ClearEnd();
		m_vecData.resize(sz+sizeof(sTextRangeData)+((nLen<=HIST_MAX_LOCALALLOC)?nLen:0));
		pData=(sTextRangeData *)&m_vecData[sz];
		if(nLen<=HIST_MAX_LOCALALLOC) pText=(byte *)&pData[1];
		AddAction(CHistItem(sz,&m_cAddTextRange));
	}

	if(pText) m_pDoc->CopyRange(pText,range,nLen);
	get_sRange(pData->range,range);
	pData->nLen=nLen;
	pData->pText=NULL;
	m_LastAction=ACTION_AddTextRange;
	return true;
}

bool CEditHistory::AddTextRange(LPCSTR pText,int nLen,CLineRange &range,BOOL bDeleteAlso)
{
	if(!bDeleteAlso && nLen==1) {
		//Add one character only --
		return AddChar(range.line0,range.chr0,*pText);
	}
	return AddPasteRange(range,bDeleteAlso);
}

//====================================================================
e_Action CActionDelAddTextRange::Action()
{
	return ACTION_DelAddTextRange;
}

static void replace_TextRange(UINT uUpd,CLineDoc *pDoc,sTextRangeData *pData,int idxDel)
{
	int idxIns=1-idxDel;

	//Get range of text to delete --
	CLineRange range(pData[idxDel].range.line0,pData[idxDel].range.chr0,pData[idxDel].range.line1,pData[idxDel].range.chr1);
	
	//We need to save this text if it's not already saved --
	ASSERT(!pData[idxDel].pText);
	if(pData[idxDel].nLen>HIST_MAX_LOCALALLOC) {
		pData[idxDel].pText = new byte[pData[idxDel].nLen];
		pDoc->CopyRange(pData[idxDel].pText,range,pData[idxDel].nLen);
	}

	//Get pointer to inserted text --
	byte *pText=pData[idxIns].pText;
	if(!pText && pData[idxIns].nLen>0) {
		pText=(byte *)&pData[2];
		if(idxIns && pData->nLen>0 && pData->nLen<=HIST_MAX_LOCALALLOC) pText+=pData->nLen;
	}

	//Check if a paste is required instead of simply an insert --
	if(pData[idxIns].nLen<=0) uUpd|=CLineDoc::UPD_DELETE;
	else uUpd|=((pData[idxIns].range.line0!=pData[idxIns].range.line1)?CLineDoc::UPD_CLIPPASTE:CLineDoc::UPD_INSERT);

	VERIFY(pDoc->Update(uUpd,(LPCSTR)pText,pData[idxIns].nLen,range));

	//We no longer need to store the inserted text (if externally allocated)
	//since it's now in the document --
	if(pData[idxIns].pText) {
		delete[] pData[idxIns].pText;
		pData[idxIns].pText=NULL;
	}
}

void CActionDelAddTextRange::Undo(CEditHistory *pHist,size_t dataOffset)
{
	//Reinsert deleted text in document, deletion of inserted text is also required.

	sTextRangeData *pData=(sTextRangeData *)&pHist->m_vecData[dataOffset];
    //pData[0] specifies text to insert, pData[1] specifies text to delete --
	replace_TextRange(CLineDoc::UPD_UNDO,pHist->m_pDoc,pData,1);
}

void CActionDelAddTextRange::Redo(CEditHistory *pHist,size_t dataOffset)
{
	//Reinsert added text in document, redeletion of deleted text is also required.

	sTextRangeData *pData=(sTextRangeData *)&pHist->m_vecData[dataOffset];
    //pData[1] specifies text to insert, pData[0] specifies text to delete --
	replace_TextRange(CLineDoc::UPD_REDO,pHist->m_pDoc,pData,0);
}

//====================================================================
e_Action CActionRepShortStr::Action()
{
	return ACTION_RepShortStr;
}

void CActionRepShortStr::Undo(CEditHistory *pHist,size_t dataOffset)
{
	sRepShortStrData *pData=(sRepShortStrData *)&pHist->m_vecData[dataOffset];
	VERIFY(pHist->m_pDoc->Update(CLineDoc::UPD_UNDO|CLineDoc::UPD_REPLACE,
		(LPCSTR)(pData+1),pData->count,
		CLineRange(pData->lineno,pData->charno,pData->lineno,pData->charno)));
}

void CActionRepShortStr::Redo(CEditHistory *pHist,size_t dataOffset)
{
	sRepShortStrData *pData=(sRepShortStrData *)&pHist->m_vecData[dataOffset];
	VERIFY(pHist->m_pDoc->Update(CLineDoc::UPD_REDO|CLineDoc::UPD_REPLACE,
		(LPCSTR)(pData+1)+pData->count,pData->count,
		CLineRange(pData->lineno,pData->charno,pData->lineno,pData->charno)));
}

bool CEditHistory::RepShortStr(LPCSTR pOldStr,LPCSTR pNewStr,int len,CLineRange &range)
{
	// clear the end of the history
	ClearEnd();

	size_t sz=m_vecData.size();
	//Place following in try/catch!
	//============================
	m_vecData.resize(sz+sizeof(sRepShortStrData)+2*len);
	sRepShortStrData *pData=(sRepShortStrData *)&m_vecData[sz];
	pData->lineno=range.line0;
	pData->charno=range.chr0;
	pData->count=len;
	memcpy(++pData,pOldStr,len);
	memcpy((LPSTR)pData+len,pNewStr,len);
	AddAction(CHistItem(sz,&m_cRepShortStr));
	//============================
	m_LastAction=ACTION_RepShortStr;
	return true;
}

//====================================================================
e_Action CActionAddEOL::Action()
{
	return ACTION_AddEOL;
}

void CActionAddEOL::Undo(CEditHistory *pHist,size_t dataOffset)
{
	//Delete EOL at chr0 on line0 (should be at end), then reinsert whitespace at chr0.
	sAddEOLData *pData=(sAddEOLData *)&pHist->m_vecData[dataOffset];
	CLineRange range(pData->line0,pData->chr0,pData->line0,INT_MAX);
	LPCSTR p=NULL;
	int upd=CLineDoc::UPD_UNDO;
	if(pData->nLen) {
		upd+=CLineDoc::UPD_INSERT;
		p=(LPCSTR)&pData[1];
	}
	else upd+=CLineDoc::UPD_DELETE;
	VERIFY(pHist->m_pDoc->Update(upd,p,pData->nLen,range));
}

void CActionAddEOL::Redo(CEditHistory *pHist,size_t dataOffset)
{
	//Delete nLen WS chars at chr0 on line0, then insert EOL at chr0.
	sAddEOLData *pData=(sAddEOLData *)&pHist->m_vecData[dataOffset];
	CLineRange range(pData->line0,pData->chr0,pData->line0,pData->chr0+pData->nLen);
	VERIFY(pHist->m_pDoc->Update(CLineDoc::UPD_REDO|CLineDoc::UPD_INSERT,NULL,0,range));
}

bool CEditHistory::AddEOL(int line0,int chr0,int nLen)
{
	// clear the end of the history
	ClearEnd();

	size_t sz=m_vecData.size();
	//Place following in try/catch!
	//============================
	m_vecData.resize(sz+sizeof(sAddEOLData)+nLen);
	sAddEOLData *pData=(sAddEOLData *)&m_vecData[sz];
	pData->line0=line0;
	pData->chr0=chr0;
	pData->nLen=nLen;
	if(nLen) memcpy(++pData,m_pDoc->m_pActiveBuffer+chr0+1,nLen);
	AddAction(CHistItem(sz,&m_cAddEOL));
	//============================
	m_LastAction=ACTION_AddEOL;
	return true;
}

//====================================================================
//EOF

