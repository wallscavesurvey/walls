//Routines in utility.cpp --

#ifndef _UTILITY_H
#define _UTILITY_H

typedef std::vector<BYTE> VEC_BYTE;
typedef VEC_BYTE::iterator it_byte;
typedef std::vector<LPCSTR> VEC_PCSTR;
typedef VEC_PCSTR::iterator it_pcstr;

BOOL   DirCheck(LPSTR pathname, BOOL bPrompt = 0);
BOOL   IsNumeric(LPCSTR p);
BOOL   MakeFileDirectoryCurrent(LPCSTR pathName);
int    MakeNestedDir(char *path);

#endif
