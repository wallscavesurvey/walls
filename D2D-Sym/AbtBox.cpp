// AbtBox.cpp : implementation file
//

#include "stdafx.h"
#include "D2D-Sym.h"
#include "AbtBox.h"
#include <Psapi.h>

// CAbtBox dialog

IMPLEMENT_DYNAMIC(CAbtBox, CDialog)

static char *CommaSep(char *outbuf,__int64 num)
{
	static NUMBERFMT fmt={0,0,3,".",",",1};
	//Assumes outbuf has size >= 20!
	TCHAR inbuf[20];
	sprintf(inbuf,"%I64d",num);
	int iRet=::GetNumberFormat(LOCALE_SYSTEM_DEFAULT,0,inbuf,&fmt,outbuf,20);
	if(!iRet) {
		strcpy(outbuf,inbuf);
	}
	return outbuf;
}

CAbtBox::CAbtBox(CWnd* pParent /*=NULL*/)
	: CDialog(CAbtBox::IDD, pParent)
{
}

CAbtBox::~CAbtBox()
{
}

void CAbtBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	if(!pDX->m_bSaveAndValidate) {
		char buf[80],val[20];

		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		::GlobalMemoryStatusEx (&statex);

		CString s;
		sprintf(buf," Physical memory in use by all processes: %s KB (%u%% of total)\n\n",
			CommaSep(val,(__int64)(statex.ullTotalPhys*(statex.dwMemoryLoad/102400.0))),statex.dwMemoryLoad);
		s+=buf;

		{
			PROCESS_MEMORY_COUNTERS pmc;
			DWORD processID = ::GetCurrentProcessId();
			HANDLE hProcess = ::OpenProcess(  PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID );
			if (::GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) ) {

				sprintf(buf," sditemp usage --\n Working set:\t\t%s KB (peak)",CommaSep(val,pmc.PeakWorkingSetSize/1024));
				s+=buf;
				sprintf(buf,", %s KB (current)\n",CommaSep(val,pmc.WorkingSetSize/1024));
				s+=buf;
				sprintf(buf," Virtual memory: \t%s KB",CommaSep(val,statex.ullTotalVirtual/1024));
				s+=buf;
				sprintf(buf," (%.1f%% free)\n\n",100.0*statex.ullAvailVirtual/statex.ullTotalVirtual);
				s+=buf;
			}
		}

		sprintf(buf," Machine Totals --\n Physical memory:\t%s KB",CommaSep(val,statex.ullTotalPhys/1024));
		s+=buf;
		sprintf(buf," (%.1f%% free)\n",100.0*statex.ullAvailPhys/statex.ullTotalPhys);
		s+=buf;


		sprintf(buf," Paging file length: \t%s KB",CommaSep(val,statex.ullTotalPageFile/1024));
		s+=buf;
		sprintf(buf," (%.1f%% free)\n",100.0*statex.ullAvailPageFile/statex.ullTotalPageFile);
		s+=buf;
		GetDlgItem(IDC_ST_STATS)->SetWindowText(s);
	}
}


BEGIN_MESSAGE_MAP(CAbtBox, CDialog)
END_MESSAGE_MAP()


// CAbtBox message handlers
