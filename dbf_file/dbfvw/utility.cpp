// UTILITY.CPP -- Custom helper functions for MFC programs

#include "stdafx.h"
#include <stdarg.h>
#include <stdio.h>
#include "utility.h"

int CDECL CMsgBox(UINT nType,char *format,...)
{
  char buf[256];
  
  va_list marker;

  va_start(marker,format);
  _vsnprintf(buf,256,format,marker);
  va_end(marker);
  return AfxMessageBox(buf,nType);
}