// Shp2Grid.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Shp2Grid.h"
#include <ShpStruct.h>
#include "shpfile.h"

#pragma comment(lib, "rpcrt4.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The one and only application object

CWinApp theApp;

static char path[MAX_PATH];
static char keybuf[MAX_PATH];

using namespace std;

int Process(int argc,TCHAR* argv[]);

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
            nRetCode = 1;
        }
        else
        {
			nRetCode = Process(argc,argv);
        }
    }
    else
    {
        // TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
        nRetCode = 1;
    }

    return nRetCode;
}

static BOOL GetTempFilePathWithExtension(CString &csPath,LPCSTR pExt)
 {
   UUID uuid;
   unsigned char* sTemp;
   HRESULT hr = UuidCreateSequential(&uuid);
   if (hr != RPC_S_OK) return FALSE;
   hr = UuidToString(&uuid, &sTemp);
   if (hr != RPC_S_OK) return FALSE;
   CString sUUID(sTemp);
   sUUID.Remove('-');
   RpcStringFree(&sTemp);

   TCHAR pszPath[MAX_PATH];
   if(!::GetTempPath(MAX_PATH,pszPath)) return FALSE;
   csPath.Format("%s%s.%s",pszPath,(LPCSTR)sUUID,pExt);
   if(csPath.GetLength()>MAX_PATH) {
	   csPath.Empty();
	   return FALSE;
   }
   return TRUE;
}

static __int32 FlipBytes(__int32 nData)
{
	__asm mov eax, [nData];
	__asm bswap eax;
	__asm mov [nData], eax; // you can take this out but leave it just to be sure
	return(nData);
}

static int Process(int argc, TCHAR* argv[])
{
	CShpDBF db;
	CTRXFile trx;
	CFile cfShp; //Same as CFile for now 
	CFileException ex;
	VEC_DWORD2 v_dpt;
	CString sTemp;
	TRXREC rec;
	LPCSTR pErr=0;
	LPSTR p=(LPSTR)dos_FullPath(argv[1],".dbf");
	if(!p) {
		sTemp.Format("Bad pathname: %s", argv[1]);
		pErr=sTemp;
		goto _fail;
	}
	LPSTR pExt=trx_Stpext(strcpy(path,p));

	if(db.Open(path)) {
		pErr="Failure opening .dbf component";
		goto _fail;
	}

	strcpy(pExt,".shp");

	if(!cfShp.Open(path,CFile::modeRead|CFile::shareDenyWrite|CFile::osSequentialScan,&ex)) {
		ex.GetErrorMessage(keybuf,MAX_PATH);
		sTemp.Format("Failure opening .shp component: %s",keybuf);
		pErr=sTemp;
		goto _fail;
	}

	SHP_MAIN_HDR hdr;
	if(cfShp.Read(&hdr,sizeof(hdr))<sizeof(hdr) || FlipBytes(hdr.fileCode)!=SHP_FILECODE || hdr.shpType!=1) {
		pErr="Bad shapefile header or not of type Point";
		goto _fail0;
	}

	UINT dataLen=(2*FlipBytes(hdr.fileLength)-sizeof(hdr));
	UINT sizRec=sizeof(SHP_POINT_REC);
	UINT numRecs=dataLen/sizRec;
	v_dpt.reserve(numRecs);

	pErr="Error reading shp";

	while(dataLen>=sizeof(SHP_POINT_REC)) {
		SHP_POINT_REC rec;
		if(cfShp.Read(&rec,sizeof(SHP_POINT_REC))!=sizeof(SHP_POINT_REC))
			goto _fail0;
		if(FlipBytes(rec.length)*2!=20)
		   goto _fail0;
		v_dpt.push_back(DWORD2(rec.fpt));
		dataLen-=sizRec;
	}
	ASSERT(numRecs==v_dpt.size());
	cfShp.Close();

	if(db.NumRecs()!=numRecs) {
		pErr="Bad .dbf recored count";
		goto _fail;
	}
	WORD fType=db.FldNum("TYPE");
	if(!fType || db.FldLen(fType)<5) {
		pErr="Field TYPE of length >= 5 not present";
		goto _fail;
	}

	pErr="Failure creating index";
	if(!GetTempFilePathWithExtension(sTemp,"trx") || trx.Create(sTemp,sizeof(TRXREC)) || trx.AllocCache(64,TRUE)) {
		goto _fail;
	}
	trx.SetExact();
	trx.SetUnique();
	*keybuf=(char)sizeof(DWORD2);
	for(UINT r=1; r<=numRecs; r++) {
		memcpy(keybuf+1, &v_dpt[r-1], sizeof(DWORD2));
		bool bFound=!trx.Find(keybuf);
		TRXREC rec; //sets count to 0
		if(bFound && trx.GetRec(&rec,sizeof(TRXREC)))
			goto _fail;
		if(db.Go(r))
			goto _fail;
		p=(LPSTR)db.FldPtr(fType);
		if(*p=='?') p++;
		if(*p=='C' || *p=='E') {
			if(p[3]=='i') rec.nCavities++;
			else {
				ASSERT(p[3]=='e' || p[3]=='r');
				rec.nCaves++;
			}
		}
		else {
			ASSERT(*p=='S');
			if(p[1]=='h') rec.nShelters++;
			else if(p[1]=='p') rec.nSprings++;
			else {
				ASSERT(p[1]=='i');
				rec.nCavities++;
			}
		}
		if(!bFound) {
			if(trx.InsertKey(&rec,keybuf))
				goto _fail;
		}
		else if(trx.PutRec(&rec))
			goto _fail;
	}

	db.Close();
	{VEC_DWORD2().swap(v_dpt);}
	numRecs=trx.NumRecs();

	//Write shapefile --
	p=(LPSTR)dos_FullPath(argv[2],".dbf");
	if(!p) {
		sTemp.Format("Bad output pathname: %s", argv[2]);
		pErr=sTemp;
		goto _fail;
	}
	if(CreateFiles(strcpy(path,p))) {
		pErr="Failure creating shapefile";
		goto _fail;
	}

	for(int e=trx.First(); !e; e=trx.Next()) {
		VERIFY(!trx.Get(&rec,keybuf));
		if(WriteShpRec(rec, *(DWORD2 *)(keybuf+1))) {
			pErr="Failure writing output record";
			goto _fail;
		}
	}
	if(CloseFiles()) {
		pErr="Error closing shapefile";
		goto _fail;
	}
	WritePrjFile();
	printf("Shapefile with %u records created successfully.\n",numRecs);
	trx.CloseDel();
	return 0;

_fail0:
	cfShp.Close();
_fail:
	db.Close();
	trx.CloseDel();
	printf("Aborted -- %s.",pErr);
	return -1;

}

