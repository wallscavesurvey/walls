#include "stdafx.h"
#include "WallsMap.h"
#include "ShpLayer.h"

#ifdef _XLS
//#include <Psapi.h>
#include "ShpDef.h"
#include "DlgExportXLS.h"

#include "ExcelFormat.h"
using namespace ExcelFormat;

BOOL CShpLayer::ExportXLS(CDlgExportXLS *pDlg, LPCSTR xlsName, CShpDef &shpdef, VEC_DWORD &vRec)
{

	LPCSTR pMsg = "Probably insufficient memory";
	bool bSwitched = false;

	UINT nFlds = shpdef.numFlds;
	UINT memoCnt = 0;

	UINT nOutRecs = vRec.size();
	if (!nOutRecs) {
		ASSERT(0);
		return 0;
	}
	bool bRet = false;

	UINT nTruncRec = 0, nTruncCnt = 0;
	LPCSTR pTruncFld = 0;
	UINT nR = 0, row = 0;

	{
		BasicExcel xls;
		xls.New(1);
		BasicExcelWorksheet* sheet = xls.GetWorksheet("Sheet1");
		ASSERT(sheet);
		if (!sheet) return 0;

		XLSFormatManager fmt_mgr(xls);
		CellFormat fmt_bold(fmt_mgr);
		ExcelFont font_bold;
		font_bold._weight = FW_BOLD; // 700
		fmt_bold.set_font(font_bold);

		DBF_FLDDEF *pFld = (DBF_FLDDEF *)&shpdef.v_fldDef[0];
		int *pIdx = (int *)&shpdef.v_fldIdx[0];
		for (UINT nF = 0; nF < nFlds; nF++, pIdx++, pFld++) {
			ASSERT(*pIdx > 0);
			BasicExcelCell* cell = sheet->Cell(0, nF);
			LPCSTR p = m_pdb->FldNamPtr(*pIdx);
			cell->Set(p);
			cell->SetFormat(fmt_bold);
			sheet->SetColWidth(nF, nF ? 4000 : 8000);
		}

		AfxGetMainWnd()->BeginWaitCursor();

		for (; nR < nOutRecs; nR++) {
			/*
			if(!(nR%10)) {
				UINT uMem=PercentRamFree();
				if(uMem<10) break;
			}
			*/
			UINT rec = vRec[nR];
			if (!rec) continue;

			ASSERT(!(rec & 0x80000000) || IsRecDeleted(rec & 0x7FFFFFFF));
			rec &= 0x7FFFFFFF;
			if (IsRecDeleted(rec)) continue;

			if (m_pdb->Go(rec)) {
				ASSERT(0);
				pMsg = "Record access error";
				AfxGetMainWnd()->EndWaitCursor();
				goto _fail;
			}
			row++;

			UINT plen;
			LPCSTR pSrc;
			LPSTR p;

			DBF_FLDDEF *pFld = (DBF_FLDDEF *)&shpdef.v_fldDef[0];
			int *pIdx = (int *)&shpdef.v_fldIdx[0];
			for (UINT nF = 0; nF < nFlds; nF++, pIdx++, pFld++) {
				int fSrc = *pIdx;
				ASSERT(fSrc > 0);
				//Copying from source database --
				VERIFY(pSrc = (LPCSTR)m_pdb->FldPtr(fSrc));
				int typ = m_pdb->FldTyp(fSrc);
				if (typ == 'M') {
					UINT dbtRec = CDBTFile::RecNo(pSrc);
					if (dbtRec) {
						plen = pDlg->m_bStripRTF ? 0 : (pDlg->m_uMaxChars + 2); //get all text if stripping
						p = m_pdbfile->dbt.GetText(&plen, dbtRec); //*plen will be actual string length
						if (pDlg->m_bStripRTF && CDBTData::IsTextRTF(p))
							plen = CDBTFile::StripRTF(p, FALSE); //Discard prefix
						if (plen > pDlg->m_uMaxChars) {
							nTruncCnt++;
							if (nTruncCnt == 1) {
								nTruncRec = row + 1;
								pTruncFld = m_pdb->FldNamPtr(fSrc);
							}
							LPSTR p0 = p + pDlg->m_uMaxChars;
							ASSERT(p0 - p >= 4);
							if (p0 - p >= 4)
								strcpy(p0 - 3, "..|");
							else *p0 = 0;
							ASSERT((plen = strlen(p)) == pDlg->m_uMaxChars);
						}
						if (*p) sheet->Cell(row, nF)->SetString(p);
						memoCnt++;
					}
				}
				else {
					int lenSrc = m_pdb->FldLen(fSrc);
					if (typ == 'F' || typ == 'N' && m_pdb->FldDec(fSrc) > 0) {
						sheet->Cell(row, nF)->SetDouble(GetDouble(pSrc, lenSrc));
					}
					else {
						CString s;
						s.SetString(pSrc, lenSrc);
						s.Trim();
						switch (typ) {
						case 'N':
							sheet->Cell(row, nF)->Set(atoi(s));
							break;

						case 'C':
						case 'D':
							sheet->Cell(row, nF)->SetString(s);
							break;

						case 'L':
							sheet->Cell(row, nF)->SetString((*pSrc == 'T' || *pSrc == 'Y') ? "TRUE" : "FALSE");
							break;
						}
					}
				}
			}
			pDlg->m_Progress.SetPos((100 * nR) / nOutRecs);
		}

		//for(UINT nF=0;nF<nFlds;nF++) {
			//sheet->SetColWidth(nF,50);
		//}

		pDlg->SetStatusMsg("Saving data to XLS...");
		if (!(bRet = xls.SaveAs(xlsName))) {
			pMsg = "Error saving data to file";
		}
		AfxGetMainWnd()->EndWaitCursor();
	}

	if (bRet) {
		CString s;
		pDlg->m_csMsg.Format("Created %s with data for %u (of %u) selected records and %u (of %u) fields.", trx_Stpnam(xlsName), row, nOutRecs, nFlds, m_pdb->NumFlds());
		/*
		if(nR<nOutRecs) {
			s.Format("\n\nNOTE: Memory was insufficient to export the last %u records.",nOutRecs-nR);
			pDlg->m_csMsg+=s;
		}
		*/
		if (nTruncCnt) {
			s.Format("\n\nCAUTION: Your cell text limit being %u characters, %u cells have truncated text ending with \"..|\", the first being field %s of row %u.",
				pDlg->m_uMaxChars, nTruncCnt, pTruncFld, nTruncRec);
			pDlg->m_csMsg += s;
		}
		return TRUE;
	}

_fail:
	pDlg->m_csMsg.Format("Unable to create %s - %s.", trx_Stpnam(xlsName), pMsg);
	return FALSE;
}

bool CShpLayer::ExportRecordsXLS(VEC_DWORD &vRec)
{
	CDlgExportXLS dlg(this, vRec);
	if (IDOK == dlg.DoModal()) {
		if (dlg.m_uMaxChars) {
			UINT id = CMsgBox(MB_YESNO, "%s\n\nOpen file with associated application?", (LPCSTR)dlg.m_csMsg);
			if (id == IDYES && (int)ShellExecute(NULL, NULL /*"open"*/, dlg.m_pathName, NULL, NULL, SW_SHOW) <= 32) {
				AfxMessageBox("Open failure - No associated program?");
			}
		}
		else
			AfxMessageBox((LPCSTR)dlg.m_csMsg);
		return true;
	}
	return false;
}

#endif