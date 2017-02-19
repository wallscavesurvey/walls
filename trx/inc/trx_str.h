/*TRX_STR.H -- String functions in TRX_STRM.LIB and TRX_STRS.LIB*/
#ifndef __TRX_STR_H
#define __TRX_STR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __TRX_TYPE_H
#include "trx_type.h"
#endif

#define TRXFCN_S LPSTR PASCAL
#define TRXFCN_I int PASCAL
#define TRXFCN_DWP DWORD * PASCAL
#define TRXFCN_V void PASCAL

#ifndef _MSC_VER
char *_strupr(char *);
char *_strlwr(char *);
#endif

TRXFCN_S trx_Appenstr(LPSTR pstr,LPCSTR arr,UINT len);
TRXFCN_S trx_Appenchr(LPSTR pstr,char chr);
TRXFCN_S trx_CapitalizeKey(LPSTR pdest);
TRXFCN_I trx_Chrcount(LPCSTR csrc,char c);
TRXFCN_I trx_Cmdtail(char ***args,char *pstr,int maxlen);
TRXFCN_I trx_Compstr(LPCSTR s1,LPCSTR s2,int cnt);
TRXFCN_S trx_Getstr(LPSTR pdst,void *psrc,int len);
TRXFCN_S trx_Lowerc(LPSTR pstr);
TRXFCN_S trx_Maskhi(LPSTR pstr);
TRXFCN_I trx_Mergekey(LPBYTE key,LPCSTR p);
TRXFCN_I trx_Poschr(LPCSTR pstr,char c);
TRXFCN_S trx_Stcext(LPSTR buf,LPCSTR pathname,LPSTR ext,int sizbuf);
TRXFCN_S trx_Stccp(LPSTR pdest,LPCSTR csrc);
TRXFCN_S trx_Stcpc(LPSTR cdest,LPCSTR psrc);
TRXFCN_S trx_Stcpp(LPSTR pdest,LPCSTR psrc);
TRXFCN_S trx_Stncc(LPSTR pdest,LPCSTR psrc,UINT sizdest);
TRXFCN_S trx_Stncp(LPSTR pdest,LPCSTR csrc,UINT sizdest);
TRXFCN_S trx_Stnpc(LPSTR cdest,LPCSTR psrc,UINT sizdest);
TRXFCN_S trx_Stnpp(LPSTR pdest,LPCSTR psrc,UINT sizdest);
TRXFCN_S trx_Stpext(LPCSTR pathname);
TRXFCN_S trx_Stpnam(LPCSTR pathname);
TRXFCN_I trx_Strcmpc(LPCSTR s1,LPCSTR s2);
TRXFCN_I trx_Strcmpp(LPCSTR s1,LPCSTR s2);
TRXFCN_S trx_Stxcp(LPSTR s);
TRXFCN_S trx_Stxcpz(LPSTR s);
TRXFCN_S trx_Stxpc(LPSTR s);
TRXFCN_S trx_Trimchr(LPSTR pstr,char chr);
TRXFCN_S trx_Trimlb(LPSTR pdest);
TRXFCN_S trx_Trimtb(LPSTR pdest);
TRXFCN_S trx_Upperc(LPSTR pstr);

//Fcns from julian.c --
//BYTE date[3]=={0,0,0} will convert to Julian day 2086303 (1000-01-01) --
#define TRX_EMPTY_DATE 2086303
TRXFCN_I trx_DateStrToJul(LPCSTR p);
TRXFCN_I trx_DateToJul(void *pDate);
TRXFCN_I trx_GregToJul(int y,int m,int d);
TRXFCN_V trx_JulToDate(int jd,void *pDate);
TRXFCN_S trx_JulToDateStr(int jd,char *buf);
TRXFCN_V trx_JulToGreg(int jd,int *y,int *m,int *d);
TRXFCN_S trx_Strxchg(char *p,char cOld,char cNew);

#define CFG_MAXARGS         30
#define CFG_ERR_EOF         -1
#define CFG_ERR_TRUNCATE    -2
#define CFG_ERR_KEYWORD     -3
enum eCFG {CFG_PARSE_NONE=0,CFG_PARSE_ALL=1,CFG_PARSE_KEY=2};

typedef int (CALLBACK *CFG_LINEFCN)(PSTR,int);
extern PBYTE  cfg_linebuf;
extern int    cfg_linecnt;
extern int    cfg_argc;
extern PSTR   cfg_argv[CFG_MAXARGS];
extern BYTE   cfg_quotes;
extern BYTE   cfg_noexact;
extern BYTE   cfg_contchr;
extern BYTE   cfg_commchr;
extern BYTE	  cfg_equals;
extern BYTE   cfg_ignorecommas;
extern PSTR   cfg_commptr;
extern PSTR   cfg_equalsptr;

TRXFCN_V cfg_KeyInit(PSTR *lineTypes,int numTypes);
TRXFCN_V cfg_LineInit(CFG_LINEFCN lineFcn,PVOID linebuf,
 int buflen,UINT uPrefix);
TRXFCN_I cfg_GetLine(void);
TRXFCN_I cfg_GetArgv(PSTR npzBuffer,UINT uPrefix);
TRXFCN_I cfg_FindKey(LPSTR key);

enum trx_bin_f {TRX_DUP_NONE,TRX_DUP_IGNORE,TRX_DUP_FIRST,TRX_DUP_LAST};
extern int trx_binMatch;
TRXFCN_V  trx_Bdelete(DWORD *seqitem);
TRXFCN_V  trx_Bininit(DWORD *seqbuf,DWORD *seqlen,TRXFCN_IF seqfcn);
TRXFCN_DWP trx_Binsert(DWORD seqitem,int dupflag);
TRXFCN_DWP trx_Blookup(int dupflag);

TRXFCN_DW trx_RevDW(DWORD d);

//mapxmod.asm functions (compiled by nasm) --
void    mapclr(void *pMap,DWORD rec);
UINT    mapset(void *pMap,DWORD rec); /*rtn nonzero if bit set previously*/
UINT    maptst(void *pMap,DWORD rec);
UINT	mapcnt(void *pMap,DWORD mapsize); /*mapsize=DWORDS contained in map*/ 

//Fcns not yet implemented for WIN32 --
#if 0
TRXFCN_S trx_Stccc(LPSTR dest,LPSTR src);
TRXFCN_S trx_Stccpz(LPSTR dest,LPSTR src);
TRXFCN_S trx_Stcppz(LPSTR dest,LPSTR src);
TRXFCN_S trx_Stncpz(LPSTR dest,LPSTR src,int len);
TRXFCN_S trx_Stnppz(LPSTR dest,LPSTR src,int len);
TRXFCN_I trx_Strlen(LPSTR s);
TRXFCN_I trx_ScanWrd(LPVOID s1,WORD target,int len);
TRXFCN_S trx_Strupr(LPSTR s);
TRXFCN_S trx_Strlwr(LPSTR s);
TRXFCN_S trx_Stxpz(LPSTR s);

TRXFCN_V trx_Timestr(LPSTR dest,UINT time);
TRXFCN_V trx_Datestr(LPSTR Dest,UINT Date); /*PC Date to asciiz mm/dd/yy*/
TRXFCN_V trx_DaysDate(LPSTR pstr3,UINT days); /*days since 1 Jan 1900 to d3*/
TRXFCN_S trx_FileDate(LPSTR pstr3,UINT days); /*days since 12/31/77 to d3*/
#endif

#ifdef __cplusplus
}
#endif
#endif


