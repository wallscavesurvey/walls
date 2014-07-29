#include "stdafx.h"
#include "dbtfileRO.h"
//#include <ShpDBF.h>
//#include "shplayer.h"
#include "utility.h"

CDBTData CDBTFile::dbt_data;

BOOL CDBTFile::Open(LPSTR pathName,UINT flags,CFileException *pEx /*=NULL*/)
{
	strcpy(trx_Stpext(pathName),".dbt");
	CFileException ex;
	if(!CFile::Open(pathName,flags,&ex)) {
		LPCSTR pCause=(ex.m_cause==CFileException::fileNotFound)?"missing": "not accessible";
		CMsgBox("File: %s\nOpen failure: The shapefile has memo fields but the .DBT component is %s.\n",
			trx_Stpnam(pathName),pCause);
		return FALSE;
	}
	dbt_data.nUsers++;
	m_uFlags=0;
	return TRUE;
}

void CDBTFile::Close()
{
	if(IsOpen()) {
		ASSERT(dbt_data.nUsers);
		//Free static data structure used with all DBT files for both
		//reading and writing if it's no longer needed --
		if(!--dbt_data.nUsers && dbt_data.lenData) {
			free(dbt_data.pData);
			dbt_data.pData=NULL;
			dbt_data.lenData=0;
		}
		CFile::Close(); //should already be flushed
	}
}

int CDBTFile::RecNo(LPCSTR p10)
{
	if(!p10) return 0;
	char buf[12];
	int n=10;
	for(;n && (*p10==' ' || *p10=='0');n--,p10++);
	if(n) {
		memcpy(buf,p10,n);
		buf[n]=0;
		n=atoi(buf);
	}
	return n;
}

LPSTR CDBTFile::GetTextSample(LPSTR buf,UINT bufSiz,UINT recNo)
{
	ASSERT(bufSiz>0 && bufSiz<=512);
	bufSiz--;
	*buf=0;
	if(!recNo || !IsOpen()) {
		return buf;
	}
	try {
		Seek(recNo*512,CFile::begin);
		if(Read(buf,bufSiz)!=bufSiz) throw 0;
		buf[bufSiz]=0;
		LPSTR p=strchr(buf,0x1A);
		if(p) *p=0;
	}
	catch(...) {
		ASSERT(0);
	}
	return buf;
}

LPSTR CDBTFile::GetText(UINT *pLen,UINT recNo)
{
	LPSTR pRet="";

	UINT maxLen=*pLen;

	*pLen=0;

	if(!recNo || !IsOpen())
		return pRet;

	try {
		Seek(recNo*512,CFile::begin);
		LPSTR pData,pEnd;

		if(!dbt_data.AllocMin(512)) return pRet;

		if(maxLen) {
			pData=dbt_data.pData;
			if(Read(pData,maxLen)!=maxLen) return pRet;
			if(pEnd=(LPSTR)memchr(pData,0x1A,maxLen)) {
				*pEnd=0;
				*pLen=(pEnd-pData)+1;
			}
			else {
				pData[maxLen-1]=0;
			}
			return pData;
		}
		
		UINT len=512;
		while(true) {
			pData=dbt_data.pData+(len-512);
			if(Read(pData,512)!=512) break;
			LPSTR pEnd=(LPSTR)memchr(pData,0x1A,512);
			if(pEnd) {
				*pEnd=0;
				*pLen=len-(512-(pEnd-pData));
				return dbt_data.pData;
			}
			len+=512;
			if(!dbt_data.AllocMin(len)) break;
		}
	}
	catch (...) {
	}
	return pRet;
}

//Strip out rtf codes
int CDBTFile::StripRTF(LPSTR pText,BOOL bKeepPrefix/*=FALSE*/)
{
	bool bBrace = false;
	bool bSlash = false;
	bool bFirstLetter = false;
	LPSTR p,pNext=pText;

	for (p=pText;*p; p++)
	{
		char ch = *p;
		if (ch == '\\')
		{
			//if(!bKeepPrefix) {
				if(p[1]=='t' && p[2]=='a' && p[3]=='b') {
					//*pNext++=' ';
					//*pNext++=' ';
					*pNext++=' ';
					p+=3;
				}
			//}
			bSlash = true;
			continue;
		}
		if (ch == '{')
		{
			bBrace = true;
			bSlash = false;
			continue;
		}
		if (ch == '}')
		{
			bSlash = bBrace = false;
			continue;
		}
		if(xisspace(ch))
		{
			bSlash = false;
			//Let it fall through so the space is added
			//if we have found first letter
			if (!bFirstLetter) continue;
		}

		if (!bSlash && !bBrace)
		{
			if (!bFirstLetter)	{
				if(bKeepPrefix) *pNext++='{';
				bFirstLetter = true;
			}
			*pNext++ = ch;
		}
	}
	*pNext=0;

	//Always remove extra spaces --
	pNext=pText;
	bSlash=false;
	for(p=pText;*p;p++) {
		if(*p==' ') {
			if(bSlash) continue;
			bSlash=true;
		}
		else bSlash=false;
		*pNext++=*p;
	}
	*pNext=0;

	return pNext-pText;
}


				

			

