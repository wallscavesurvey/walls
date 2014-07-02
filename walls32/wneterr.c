#include <windows.h>
#include <string.h>
#include "wnetlib.h"

LPSTR PASCAL net_ErrMsg(LPSTR buffer,int code)
{
    char *p;
    
    /*CAUTION: Maximum length of these strings is NET_SIZ_ERRBUF=80*/
    
    if(code<0) {
      switch(code) {
	      case NET_ErrNearHeap  : p="Insufficient local memory"; break;
	      case NET_ErrFarHeap   : p="Insufficient global memory"; break;
	      case NET_ErrMaxSystems: p="More than 400 loop systems"; break;
	      case NET_ErrDisconnect: p="Too many vectors floated"; break;
	      case NET_ErrConstrain : p="A constrained loop (possibly a duplicate fixed point) was detected"; break;
	      case NET_ErrBwLimit   : p="Bandwidth limit exceeded"; break;
	      case NET_ErrCallBack  : p="Aborted from callback"; break;
	      case NET_ErrSysMem    : p="Loop system too large to solve"; break;
	      case NET_ErrMaxVecs   : p="Vector limit 2**29 exceeded"; break;
	      case NET_ErrMaxNodes  : p="Station limit 2**29 exceeded"; break;
	      case NET_ErrMaxStrVecs: p="Analysis workspace insufficient"; break;
	      case NET_ErrEmpty		: p="No vectors to process"; break;
	      case NET_ErrVersion   : p="Wrong version of WALLNET4.DLL"; break;
	
	     /*The following errors should be impossible --*/
	      case NET_ErrSystem    : p="Bad system"; break;
	      case NET_ErrAlist     : p="Bad alist"; break;
	      case NET_ErrMaxStrings: p="Too many link traverses"; break;
	      case NET_ErrSysConnect: p="System disconnected"; break;
	      case NET_ErrNcCompare : p="Closure count mismatch"; break;
	      case NET_ErrWork      : p="Corrupted workfile"; break;
	      default               : p="Unknown error";
	  }
	  strcpy(buffer,p);
    }
    else {
      switch(net_errtyp) {
        case 1: p=NET_VEC_EXT; break;
        case 2: p=NET_STR_EXT; break;
        case 3: p=NET_PLT_EXT; break;
        case 4: p=NET_NET_EXT; break;
        default: p=0;
      }
      if(p) wsprintf(buffer,"%s file - %s",(LPSTR)p,dbf_Errstr(code));
      else strcpy(buffer,dbf_Errstr(code));
    }
    return buffer;
}
