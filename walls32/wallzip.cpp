// wallzip.cpp : Called from ZipDlg.cpp in Walls v.2B7

#include "stdafx.h"
#include <vector>
#include "filecfg.h"
#include "prjtypes.h"

#include "zip.h"

#define SIZ_MSGBUF 512
static char msgbuf[SIZ_MSGBUF];
static char errmsg[SIZ_MSGBUF];
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

static LPCSTR pMissingFile=NULL;

char * zip_ErrMsg(int code)
{
	if(code) {
		if(code<0 || code>ZIP_ERR_UNKNOWN) code=ZIP_ERR_UNKNOWN;
		if(code==ZIP_ERR_ADDFILE)
			_snprintf(msgbuf,SIZ_MSGBUF,"%s %s.\n%s",errstr[code-1],pMissingFile,errmsg);
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
		trx_Stncc(prjNewPath,pPath,MAX_PATH-1);
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

static void replaceChr(LPSTR name,char cOld,char cNew)
{
	for(;*name;name++) {
		if(*name==cOld) *name=cNew;
	}
}

//=====================================================================================================
//Main entry point --
int zip_Export(LPCSTR pathname,char **pFile,int iFileCnt,UINT flags)
{
	HZIP hz;

	char rootPath[MAX_PATH],prjBuffer[MAX_PATH];
	size_t rootPathLen;

	*errmsg=0;
    lasterrorZ=0;

	ASSERT(pFile[0] && strlen(pFile[0])<MAX_PATH);

	trx_Stncc(rootPath,pFile[0],MAX_PATH);
	*trx_Stpnam(rootPath)=0;
	rootPathLen=strlen(rootPath);

	hz=CreateZip(pathname,0);
	if(!hz) {
		FormatZipMessageZ(lasterrorZ,errmsg,SIZ_MSGBUF);
		return ZIP_ERR_CREATE;
	}

	int e=0,numData=0,numNTA=0,iSelfCnt=0,iExternalFiles=0;
	int i=iFileCnt;
	LPCSTR pFilePath=NULL;

	while(i--) {
		pFilePath=pFile[i];

		if(i && !_stricmp(pathname,pFilePath)) iSelfCnt++;
		else {
			LPCSTR pAddFile;
			if(strlen(pFilePath)>=rootPathLen && !_memicmp(pFilePath,rootPath,rootPathLen)) {
				pAddFile=pFilePath+rootPathLen;
				if(!i && iExternalFiles) {
					pFilePath=make_prj_temp(prjBuffer,pFilePath);
				}
			}
			else {
				ASSERT(i);
				ASSERT(pFilePath[1]==':');
				pAddFile=trx_Stncc(prjBuffer,pFilePath,MAX_PATH);
				replaceChr(prjBuffer,':','$');
				iExternalFiles++;
			}

			if(ZR_OK==ZipAdd(hz,pAddFile,pFilePath)) {
				if(IsNTA(pFilePath)) numNTA++;
				else numData++;
			}
			else { 
				pMissingFile=pFilePath;
				if(lasterrorZ==ZR_NOFILE) lasterrorZ=0;
				else {
					FormatZipMessageZ(lasterrorZ,errmsg,SIZ_MSGBUF);
					e=ZIP_ERR_ADDFILE;
					break;
				}
			}
		}
	}

	if(CloseZipZ(hz) && !e) {
		FormatZipMessageZ(lasterrorZ,errmsg,SIZ_MSGBUF);
		e=ZIP_ERR_CLOSE;
	}

	if(pFilePath==prjBuffer) unlink(pFilePath);

	if(e) {
		unlink(pathname);
		return e;
	}

	if(!pMissingFile) 
		sprintf(msgbuf,"\n%d NTA file(s) and all %d data files referenced by the project are included.",
		numNTA,numData);
	else sprintf(msgbuf," %d of %d project files are included.\n"
		"One referenced file that could not be added is %s.",
		numData+numNTA,iFileCnt-iSelfCnt,trx_Stpnam(pMissingFile));

	if(iExternalFiles) sprintf(msgbuf+strlen(msgbuf),"\nNOTE: %d file(s) were not in or beneath the project folder. The archived\n"
		"project has those files in subfolders named \"<drive letter>$\".",iExternalFiles);

	return 0;
}
