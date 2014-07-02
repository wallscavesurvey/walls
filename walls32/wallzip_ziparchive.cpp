// wallzip.cpp : Called from ZipDlg.cpp in Walls v.2B7

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <vector>
#include <afx.h>
#include <afxwin.h>
#include <ZipArchive.h>
#include <stdio.h>
#include <trx_str.h>
//#include <filebuf.h>
#include "filecfg.h"
#include "prjtypes.h"

#ifndef DLLExportC
#define DLLExportC extern "C" __declspec(dllexport)
#endif

#define SIZ_MSGBUF 512
static char msgbuf[SIZ_MSGBUF];
static char errmsg[SIZ_MSGBUF];
static bool bZipOpen;
static LPCSTR pAddFile,pZipPath;
static int iSelfCnt;

//=======================================================

enum {
  ZIP_ERR_ADDFILE=1,
  ZIP_ERR_CREATE,
  ZIP_ERR_CLOSE,
  ZIP_ERR_UNKNOWN
};

static NPSTR errstr[]={
  "Error adding",
  "Cannot create ZIP file",
  "Error closing ZIP file",
  "Unknown error"
};

DLLExportC char * PASCAL ErrMsg(int code)
{
	if(code) {
		if(code<ZIP_ERR_CREATE)
			_snprintf(msgbuf,SIZ_MSGBUF,"%s %s.\n%s",errstr[code-1],pAddFile,errmsg);
		else
			_snprintf(msgbuf,SIZ_MSGBUF,"%s.\n%s",errstr[code-1],errmsg);
	}
	return msgbuf;
}

static BOOL IsNTA(LPCSTR fname)
{
	LPCSTR p=trx_Stpext(fname);
	return strlen(p)>=4 && !_memicmp(p,".NTA",4);
}

static void fix_path(char *path)
{
	for(;*path;++path) if(*path=='/') *path='\\';
}

static void build_path(char *path,const std::vector<char *> &pathvec)
{
	char pathbuf[MAX_PATH];
	*pathbuf=0;
	int len=0;
	std::vector<char *>::const_iterator p,pend=pathvec.end();
	
	for(p=pathvec.begin();p!=pend;++p) {
		if(*p) {
			if(**p=='\\' || (*p)[1]==':') len=0;
			if(len && pathbuf[len-1]!='\\') pathbuf[len++]='\\';
			strcpy(pathbuf+len,*p);
			len+=strlen(*p);
		}
	}
	if(*path) {
		if(len && pathbuf[len-1]!='\\') pathbuf[len++]='\\';
		strcpy(pathbuf+len,path);
	}
	strcpy(path,pathbuf);
}

static bool fix_absolute_path(char *pathBuf,const char *pPath)
{
	if(*pathBuf=='\\') {
		memmove(pathBuf+2,pathBuf,strlen(pathBuf)+1);
		*pathBuf=*pPath;
		pathBuf[1]=':';
	}
	if(pathBuf[1]==':') {
		//See if the project path is a prefix --
		int lenPath=trx_Stpnam(pPath)-pPath-1;  //length of PRJ file path prefix, excluding trailing '\'
		int lenItem=strlen(pathBuf);
		if(lenItem>=lenPath && !memicmp(pathBuf,pPath,lenPath)) {
			if(lenItem==lenPath) return false; //no path needed
			if(pathBuf[lenPath]=='\\') memmove(pathBuf,pathBuf+lenPath+1,lenItem-lenPath);
			else pathBuf[1]='$';
		}
		else pathBuf[1]='$';
	}
	return true;
}

static LPCSTR make_prj_temp(char * prjNewPath,LPCSTR pPath)
{
	CFileCfg prjOld(prjtypes,PRJ_CFG_NUMTYPES); //don't ignore comments
	CFileCfg::m_bIgnoreComments=FALSE;

	if(!prjOld.OpenFail(pPath,CFile::modeRead|CFile::shareDenyWrite)) {
		CFileBuffer prjNew(512);
		char pathBuf[MAX_PATH];
		strcpy(prjNewPath,pPath);
		strcat(prjNewPath,"$");
		if(!prjNew.OpenFail(prjNewPath,CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive)) {
			int typ;
			bool isSurvey=false,pathFound=true;
			char *str;
			std::vector<char *> pathstack;
			while((typ=prjOld.GetLine())>=0) {

				if(!pathFound && typ>0 && typ!=PRJ_CFG_NAME && typ!=PRJ_CFG_PATH) {
					pathFound=true;
					*pathBuf=0;
					build_path(pathBuf,pathstack);
					if(*pathBuf && fix_absolute_path(pathBuf,pPath))
						prjNew.WriteArgv(".%s\t%s\n",prjtypes[PRJ_CFG_PATH-1],pathBuf);
				}

				switch(typ) {
					case PRJ_CFG_BOOK :
						pathstack.push_back(NULL);
						isSurvey=false;
						break;

					case PRJ_CFG_ENDBOOK :
						ASSERT(pathstack.size()>0);
						free(pathstack.back());
						pathstack.pop_back();
						break;

					case PRJ_CFG_SURVEY :
						pathFound=false;
						isSurvey=true;
						break;

					case PRJ_CFG_PATH :
						//Write only complete paths -- none for book items:
						ASSERT(prjOld.Argc()==2);
						pathFound=true;
						if(prjOld.Argc()==2) {
							strcpy(pathBuf,prjOld.Argv(1));
							fix_path(pathBuf);
							if(!isSurvey) {
								ASSERT(pathstack.size()>0 && pathstack.back()==NULL);
								if(str=(char *)malloc(strlen(pathBuf)+1)) {
									pathstack.back()=strcpy(str,pathBuf);
								}
							}
							else {
								//This is a path for a file item --
								if(*pathBuf!='\\' && pathBuf[1]!=':') {
									build_path(pathBuf,pathstack);
								}
								if(fix_absolute_path(pathBuf,pPath)) {
									prjNew.WriteArgv(".%s\t%s\n",prjOld.Argv(0),pathBuf);
								}
							}
						}
						continue;

				}
				if(typ>0) prjNew.WriteArgv(".%s\t%s\n",prjOld.Argv(0),prjOld.Argv(1));
				else prjNew.WriteArgv("%s\n",prjOld.LineBuffer());
			}
			ASSERT(pathstack.empty());
			prjNew.Close();
			pPath=prjNewPath;
		}
		prjOld.Close();
	}
	return pPath;
}

//=====================================================================================================
//Main entry point --
DLLExportC int PASCAL Export(LPCSTR pathname,char **pFile,int iFileCnt,UINT flags)
{
	int e=0;
	CZipArchive zip;
	pAddFile=NULL;
	pZipPath=pathname;

	TRY {
		zip.SetAdvanced(750000);
		zip.Open(pathname,CZipArchive::zipCreate);
		zip.SetSystemCompatibility(0);
		CZipPathComponent zpc(pFile[0]);
		zip.SetRootPath(zpc.GetFilePath());
	}
	CATCH(CException, pEx) {
		pEx->GetErrorMessage(errmsg,SIZ_MSGBUF);
		zip.Close(CZipArchive::afAfterException);
		e=ZIP_ERR_CREATE;
	}
	END_CATCH

	if(e) return e;

	int numData=0,numNTA=0;
	iSelfCnt=0;
	zip.m_iExternalFiles=0;
	char prjBuffer[MAX_PATH];
	int i=iFileCnt;
	LPCSTR pFilePath=NULL;

	while(i--) {
		pFilePath=pFile[i];

		if(!i && zip.m_iExternalFiles) {
			pFilePath=make_prj_temp(prjBuffer,pFilePath);
		}

		TRY {
			CZipAddNewFileInfo zanfi(pFilePath,false);
			zanfi.m_iComprLevel = Z_DEFAULT_COMPRESSION;
			zanfi.m_iSmartLevel = 7;
			if(zip.AddNewFile(zanfi)) {
				if(IsNTA(pFile[i])) numNTA++;
				else numData++;
			}
			else {
				if(_stricmp(pZipPath,pFile[i])) pAddFile=pFile[i];
				else iSelfCnt++;
			}
		}
		CATCH(CException, pEx) {
			pEx->GetErrorMessage(errmsg,SIZ_MSGBUF);
			zip.Close(CZipArchive::afAfterException);
			pAddFile=pFile[i];
			e=ZIP_ERR_ADDFILE;
			break;
		}
		END_CATCH
	}

	if(pFilePath==prjBuffer) unlink(pFilePath);

	if(e) return e;

	TRY {
		zip.Close(CZipArchive::afNoException,true);
	}
	CATCH(CException, pEx) {
		pEx->GetErrorMessage(errmsg,SIZ_MSGBUF);
		zip.Close(CZipArchive::afAfterException);
		e=ZIP_ERR_CLOSE;
	}
	END_CATCH
	if(e) return e;

	if(!pAddFile) 
		sprintf(msgbuf,"\n%d NTA file(s) and all %d data files referenced by the project are included.",
		numNTA,numData);
	else sprintf(msgbuf," %d of %d project files are included.\n"
		"One referenced file that is missing or cannot be added is %s.",
		numData+numNTA,iFileCnt-iSelfCnt,trx_Stpnam(pAddFile));

	if(zip.m_iExternalFiles) sprintf(msgbuf+strlen(msgbuf),"\nNOTE: %d file(s) were not in or beneath the project folder. The archived\n"
		"project has those files in subfolders named \"<drive letter>$\".",zip.m_iExternalFiles);

	return 0;
}
