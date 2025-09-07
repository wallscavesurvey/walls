// expsef.cpp : Compilation/database opperations for CPrjDoc
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "magdlg.h"
#include "expsef.h"
#include "expsefd.h"
#include "wall_srv.h"
#include "wall_emsg.h"

#include <trxfile.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define EXP_DLL_FCN "_Export@4"

#define SIZ_PFX_CACHE 32

static PRJREF *pREF;

typedef int (FAR PASCAL *LPFN_EXPORT)(EXPTYP *);

static LPFN_EXPORT pExport;
static UINT uFLG, numerrors;
static CPrjListNode *pNodeExp;

#define EXP_FCN(id) (pEXP->fcn=id,pExport(pEXP))

extern DWORD numfiles;
extern double fTotalLength;

static EXPTYP *pEXP;
static CTRXFile *pixPFX;

static int PASCAL LogPrefix(char *name)
{
	int e;
	short pfx = 0;

	if (!(e = pixPFX->Find((LPBYTE)name))) {
		pixPFX->GetRec(&pfx, sizeof(pfx));
	}
	else if (e == CTRXFile::ErrMatch || e == CTRXFile::ErrEOF) {
		pfx = (short)pixPFX->NumRecs() + 1;
		if (e = pixPFX->InsertKey(&pfx, (LPBYTE)name)) pfx = 0;
	}

	return pfx;
}

static int PASCAL SetOptions(CPrjListNode *pNode)
{
	if (pNode->Parent()) {
		if (SetOptions(pNode->Parent())) return EXP_ERR_SETOPTIONS;
	}
	else {
		pEXP->title = NULL;
		EXP_FCN(EXP_SETOPTIONS); //Set to default
	}

	if (!pNode->OptionsEmpty()) {
		pEXP->title = pNode->Options();
		if (EXP_FCN(EXP_SETOPTIONS)) return EXP_ERR_SETOPTIONS;
	}
	return 0;
}

int PASCAL exp_CB(int fcn_id, LPSTR lpStr)
{
	int e = 0;
	BYTE buf[TRX_MAX_KEYLEN + 1];

	switch (fcn_id) {

	case FCN_CVTLATLON:
	{
		NTVRec *pNTV = (NTVRec *)lpStr;
		ASSERT(pREF);
		MD.format = 0;
		MD.lat.isgn = MD.lon.isgn = 1;
		MD.lon.fdeg = pNTV->xyz[0];
		MD.lat.fdeg = pNTV->xyz[1];
		MD.lon.fmin = MD.lon.fsec = MD.lat.fmin = MD.lat.fsec = 0.0;
		MD.datum = pREF->datum;
		MD.zone = pREF->zone; //force to root of reference
		if ((e = MF(MAG_CVTLL2UTM3)) && e != MAG_ERR_OUTOFZONE) {
			return EXP_ERR_DATEDECL;
		}
		pNTV->xyz[0] = MD.east;
		pNTV->xyz[1] = MD.north;
		CMainFrame::PutRef(pREF); //restore MD
		return 0;
	}

	case FCN_DATEDECL:
	{
		ASSERT(pREF);
		CMainFrame::PutRef(pREF);
		DWORD dw = pEXP->date;
		// dw=(y<<9)+(m<<5)+d; //15:4:5
		MD.y = ((dw >> 9) & 0x7FFF);
		MD.m = ((dw >> 5) & 0xF);
		MD.d = (dw & 0x1F);
		if (MF(MAG_CVTDATE)) return EXP_ERR_DATEDECL;
		*(double *)lpStr = (double)MD.decl;
		return 0;
	}

	case FCN_PREFIX:
	{
		e = strlen(lpStr);
		if (e > 128) return 0;
		memcpy(buf + 1, lpStr, *buf = e); //Pascal string required
		e = LogPrefix((char *)buf);
		return e;
	}

	case FCN_ERROR:
	{
		numerrors++;
		return CPrjDoc::m_pLogDoc->log_write(lpStr);
	}

	} /*switch*/

	//ASSERT(FALSE);

	return e;
}

int CPrjDoc::ExportBranch(CPrjListNode *pNode)
{
	int e;

	pEXP->flags = uFLG;

	if (!pNode->m_dwFileChk) return 0;

	if (pNode->m_bLeaf) {

		if (pREF = pNode->GetAssignedRef()) {
			pEXP->flags |= EXP_USEDATES * pNode->InheritedState(FLG_DATEMASK);
		}
		pEXP->pREF = pREF; //for writing conv, zone and datum name to SEF

		//Set the compile options, if any --
		//SetOptions must be called AFTER exp_Open()!
		pEXP->path = SurveyPath(pNode);

		if (!(e = EXP_FCN(EXP_SRVOPEN)) && !(e = SetOptions(pNode))) {
			pEXP->name = pNode->Name();
			pEXP->title = pNode->Title();
			e = EXP_FCN(EXP_SURVEY);
			numfiles++;
		}

		if (e && e != EXP_ERR_SETOPTIONS) {
			m_pErrorNode = pNode;
			m_nLineStart = (LINENO)pEXP->lineno;
			m_nCharStart = pEXP->charno;
		}

	}
	else {
		//Branch node processing --
		pEXP->title = pNode->Title();
		if (!(e = EXP_FCN(EXP_OPENDIR))) {
			pNode = (CPrjListNode *)pNode->m_pFirstChild;

			while (pNode) {
				if (!pNode->m_bFloating) {
					if (e = ExportBranch(pNode)) break;
				}
				pNode = (CPrjListNode *)pNode->m_pNextSibling;
			}
			if (!e) EXP_FCN(EXP_CLOSEDIR);
		}

	}
	return e;
}

int CPrjDoc::ExportSEF(CPrjListNode *pNode, const char *pathname, UINT flags)
{

	CTRXFile ixPFX;     //Branch node's original NTA file (segment index)
	EXPTYP exp;
	int e;

	m_pErrorNode = NULL;

	//Pathname was already checked

	//Load DLL --
	//UINT prevMode=SetErrorMode(SEM_NOOPENFILEERRORBOX);
	HINSTANCE hinst = LoadLibrary(EXP_DLL_NAME); //EXP_DLL_NAME
	pExport = NULL;

	if (hinst)
		pExport = (LPFN_EXPORT)GetProcAddress(hinst, EXP_DLL_FCN);

	if (!pExport) {
		if (hinst) FreeLibrary(hinst);
		CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_IMPLOADDLL1, EXP_DLL_NAME);
		return -1;
	}

	//Create a temporary index file for converting name prefixes to short forms --
	//Non-flushed mode is the default --
	if (!(e = ixPFX.Create(WorkPath(pNode, TYP_TMP), sizeof(short)))) {
		ixPFX.SetUnique(); //We will only insert unique names
		ixPFX.SetExact(); //We will use "insert if not found"
		//Assign cache and make it the default for indexes --
		if (e = ixPFX.AllocCache(SIZ_PFX_CACHE, FALSE)) ixPFX.CloseDel();
	}
	if (e) {
		FreeLibrary(hinst);
		AfxMessageBox(IDS_ERR_MEMORY);
		return -1;
	}
	pixPFX = &ixPFX;

	numfiles = numerrors = 0;
	pEXP = &exp;
	uFLG = flags;
	exp.fTotalLength = EXP_VERSION;
	exp.path = pathname;
	exp.title = (char *)exp_CB;

	InitLogPath(pNode);

	BeginWaitCursor();

	//Initialize SEF file --

	if (!(e = EXP_FCN(EXP_OPEN))) {
		pEXP->charno = e = ExportBranch(pNode);
		//Error status determines if SEF is to be deleted --
		if (EXP_FCN(EXP_CLOSE) && !e) e = EXP_ERR_CLOSE;
	}

	if (pixPFX) ixPFX.CloseDel(TRUE); 	//Delete cache

	if (m_fplog) fclose(m_fplog);

	EndWaitCursor();

	switch (e) {

	case 0:
		if (!exp.numvectors && !exp.numfixedpts) AfxMessageBox(IDS_ERR_NTVEMPTY);
		else {
			CString s;
			s.Format("Created %s --\n\nFiles processed: %u  Fixed Pts: %u  Vectors: %u\nTotal corrected vector length: %.2f m (%.2f ft)",
				TrimPath(CString(pathname), 50), numfiles, exp.numfixedpts, exp.numvectors, exp.fTotalLength, exp.fTotalLength / 0.3048);
			if (numerrors)
				s.AppendFormat("\nLogged cautions or non-fatal errors: %u", numerrors);

			if (exp.maxnamlen > 12) {
				s.AppendFormat("\n\nNOTE: The longest prefixed name has %u characters (>12), so it's likely only Walls can import the file.",
					exp.maxnamlen);
				if (exp.flags&EXP_NOCVTPFX) s += " Consider exporting again with the \"Suppress name prefix length reduction\" box unchecked.";
			}
			AfxMessageBox(s);
		}
		break;

	case EXP_ERR_SETOPTIONS:
		CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_SRVOPTION, pNode->Title(), pEXP->title);
		break;

	case EXP_ERR_DATEDECL:
		CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_DATEDECL3, MD.y, MD.m, MD.d);
		break;

	case EXP_ERR_LOADDECL:
		ASSERT(0);
		//CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_IMPLOADDLL1,MAG_DLL_NAME);
		break;

	case EXP_ERR_CLOSE:
		CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_EXPCLOSE1, pathname);
		break;

	case EXP_ERR_VERSION:
		CMsgBox(MB_ICONEXCLAMATION, "Version " EXP_VERSION_STR " (not %s) of " EXP_DLL_NAME " required.", pEXP->title);
		break;

	default:
		//An error was found in the text file at line number pEXP->lineno --
		if (e != EXP_ERR_DATEDECL && e != EXP_ERR_LOADDECL) {
			if (m_pErrorNode) {
				CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_EXPTEXT3, m_pErrorNode->Name(), m_nLineStart, pEXP->title);
			}
			else
				CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_EXPTEXT1, pEXP->title);
		}
		break;
	}

	FreeLibrary(hinst);

	if (m_fplog) {
		if (IDOK == CMsgBox(MB_OKCANCEL, IDS_PRJ_EXPORTLOG)) {
			LaunchFile(m_pNodeLog, CPrjDoc::TYP_LOG);
		}
	}

	return e;
}
