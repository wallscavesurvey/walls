// MemUsageDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MemUsageDlg.h"
#include <Psapi.h>


// CMemUsageDlg dialog

IMPLEMENT_DYNAMIC(CMemUsageDlg, CDialog)

CMemUsageDlg::CMemUsageDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMemUsageDlg::IDD, pParent)
{

}

CMemUsageDlg::~CMemUsageDlg()
{
}

void CMemUsageDlg::DoDataExchange(CDataExchange* pDX)
{
	//CDialog::DoDataExchange(pDX);

	if (!pDX->m_bSaveAndValidate) {
		char buf[80], val[20];

		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		::GlobalMemoryStatusEx(&statex);


		CString s;
		sprintf(buf, " Physical memory in use by all processes: %s KB (%u%% of total)\n\n",
			CommaSep(val, (__int64)(statex.ullTotalPhys*(statex.dwMemoryLoad / 102400.0))), statex.dwMemoryLoad);
		s += buf;

		{
			PROCESS_MEMORY_COUNTERS pmc;
			DWORD processID = ::GetCurrentProcessId();
			HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

			//NOTE: This function requires PSAPI_VERSION=1 be defined for project if program is to work under XP as well as WIN 7!!!
			if (::GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {

				sprintf(buf, " WallsMap usage --\n Working set:\t\t%s KB (peak)", CommaSep(val, pmc.PeakWorkingSetSize / 1024));
				s += buf;
				sprintf(buf, ", %s KB (current)\n", CommaSep(val, pmc.WorkingSetSize / 1024));
				s += buf;
				sprintf(buf, " Virtual memory: \t%s KB", CommaSep(val, statex.ullTotalVirtual / 1024));
				s += buf;
				sprintf(buf, " (%.1f%% free)\n\n", 100.0*statex.ullAvailVirtual / statex.ullTotalVirtual);
				s += buf;
			}
			::CloseHandle(hProcess);
		}

		sprintf(buf, " Machine Totals --\n Physical memory:\t%s KB", CommaSep(val, statex.ullTotalPhys / 1024));
		s += buf;
		sprintf(buf, " (%.1f%% free)\n", 100.0*statex.ullAvailPhys / statex.ullTotalPhys);
		s += buf;


		sprintf(buf, " Paging file length: \t%s KB", CommaSep(val, statex.ullTotalPageFile / 1024));
		s += buf;
		sprintf(buf, " (%.1f%% free)\n", 100.0*statex.ullAvailPageFile / statex.ullTotalPageFile);
		s += buf;
		GetDlgItem(IDC_ST_STATS)->SetWindowText(s);
	}
}

BEGIN_MESSAGE_MAP(CMemUsageDlg, CDialog)
END_MESSAGE_MAP()
