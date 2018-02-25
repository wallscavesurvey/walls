#include <string.h>
/*  Module CFG.C -- Tools to read and parse a configuration file
    ==========================================================
    VOID PASCAL cfg_LineInit(
		int (PASCAL *linefcn)(PSTR pBuffer,int sizBuffer),
		PVOID linebuf,
		int sizLinebuf,
		UINT uPrefix);
    ---------------------------------------------------------
    Initialization made before first call to cfg_GetLine(). The callback
    linefcn() retrieves a ASCIIZ string of max length sizBuffer-1.
    linefcn() must return the string length if successful, or CFG_ERR_EOF
    in case of error or EOF. Workspace is provided by linebuf and
    sizLinebuf. The function cfg_GetArgv(linebuf,uPrefix) will be
    called for each complete line read by cfg_GetLine().

    VOID PASCAL cfg_KeyInit(LPSTR *cfgTypes,int numTypes);
    ---------------------------------------------------------
    Initialization made before first calls to cfg_GetArgv() and
    cfg_GetLine() when keyword checking is required. cfgTypes[] is an
    array of uppercase ASCIIZ strings of length numTypes, sorted
    alphabetically. NOTE: When keyword matching is enabled
    (uPrefix>=CFG_PARSE_KEY), cfg_GetLine() or cfg_GetArgv() will return
    i>0 when cfgTypes[i-1] matches the leading portion of the keyword
    being examined.

    int PASCAL cfg_GetArgv(PSTR pzBuffer,UINT uPrefix)
    ---------------------------------------------------------
    Parses string in pzBuffer to initialize globals cfg_argc and
    cfg_argv[], optionally performing a keyword search on the first
    argument depending on the value of uPrefix. If uPrefix is >1 (see
    below) cfg_KeyInit() must have been called previously.
    
    The argument delimeters are commas (,) and/or whitespace (' ' or
    '\t'). Quoted (' or ") multiword arguments are accepted. A quoted
    string can only have embedded quote characters of the opposing type.
    If '=' is encountered, it will serve both as a token separator and
    as a separate 1-character string in the cfg_argv[] array.

    During parsing, the global character, cfg_commchr (semicolon by
    default), will be treated as a line terminator outside of quoted
    strings. It is expected to prefix comments in data files. When the
    cfg_commchr terminator is encountered, cfg_commptr will be set to
    point to the next character in pzBuffer, otherwise cfg_commptr will
    be set to NULL.
    
    Return value: If buffer pzBuffer is empty (or contains only
    whitespace or a comment) the function returns cfg_argc=0. Otherwise,
    behavior depends on uPrefix:
			
        0   CFG_PARSE_NONE: No parsing or keyword checking is done.
            cfg_argv[0] will point to the first non-whitespace character
            in pzBuffer. cfg_argc=1 is returned.
	
        1   CFG_PARSE_ALL: The entire buffer is parsed and cfg_argc is
            returned. pzBuffer will be altered to contain the data
            pointed to by the cfg_argv[] array of string pointers. No
            keyword search or prefix processing is performed. A maximum
            of CFG_MAXARGS arguments will be processed. The global
            cfg_commptr, if not zero, will point to the first character
            following a comment character, cfg_commchr.
											
        2   CFG_PARSE_KEY: The key value of the first token is returned.
            If no match is found, CFG_ERR_KEYWORD (a negative value) is
            returned. cfg_argv[0] will contain the uppercased keyword.
            If there are more data in the buffer, cfg_argv[1] will
            contain the remaining (unaltered) portion of the buffer,
            begining with the first non- whitespace character, and
            cfg_argc will be set to 2. Otherwise, cfg_argc will be set
            to 1.
						
    <char>  If the first non-whitespace character is uPrefix, the key
            value of the first token is returned. cfg_argv[0] will
            contain the uppercased keyword (without the prefix) and any
            remaining data are placed in cfg_argv[1] (as with
            CFG_PARSE_KEY). Whitespace is allowed between the prefix
            character and keyword. A line consisting of a single prefix
            character will be treated as a blank line (0 returned, with
            cfg_argc=0).

            If the buffer's first non-whitespace character is NOT
            uPrefix, 0 is returned. cfg_argv[0] then points to the
            first non-whitespace character with cfg_argc set to 1.
	        
    int PASCAL cfg_GetLine(void);
    ---------------------------------------------------------
    Reads the next file line and returns a value cfg_GetArgc(), or a
    negative error code (CFG_ERR_...). File lines ending in cfg_contchr
    ('\') will be merged with succeeding lines and placed into
    cfg_linebuf. Blank lines are skipped. Depending on the values
    initialized by cfg_LineInit() and cfg_KeyInit(), cfg_linebuf will
    (after processing by cfg_GetArgv()) contain parsed data pointed to
    by cfg_argv[]. A line count is maintained in the public int,
    cfg_linecount.

    IMPORTANT NOTE: The callback function used for reading a line (see
    cfg_LineInit()) will return either line length (>=0) or CFG_ERR_EOF
    (-1). Therefore, to indicate truncation of overlong lines,
    cfg_GetLine() fails with CFG_ERR_TRUNCATE when the returned line
    length is the largest possible given the buffer size passed to the
    callback. (Note that this buffer size will be smaller than the value
    set by cfg_LineInit() when processing continuation lines.) This is
    simpler than requiring the callback to supply an error code to
    indicate truncation. Other kinds of read errors can be handled by
    exception processing in the main application.
   
=============================================================*/
/*Make DLL compatible --*/
#define _STACKNEAR 0
#include <trx_str.h>

/*Globals --*/
int     cfg_linecnt;
int     cfg_argc;
PSTR   cfg_argv[CFG_MAXARGS];
PSTR   cfg_equalsptr="=";
PSTR   cfg_commptr;
PBYTE  cfg_linebuf;
BYTE    cfg_contchr='\\';
BYTE    cfg_commchr=';';
BYTE	cfg_escchr='\\';
BYTE	cfg_quotes=TRUE;
BYTE    cfg_equals=TRUE;
BYTE	cfg_skipempty=TRUE;
BYTE    cfg_ignorecommas=FALSE;

//Default: Require exact matches. Otherwise extra chars allowed --
BYTE    cfg_noexact=FALSE;

static CFG_LINEFCN _cfg_linefcn;
static PSTR *_cfg_type;
static int _cfg_numtypes,_cfg_sizlinebuf;
static UINT _cfg_prefix;
static BOOL _cfg_eof;

TRXFCN_V cfg_KeyInit(PSTR *lineTypes,int numTypes)
{
  _cfg_type=lineTypes;
  _cfg_numtypes=numTypes;
}

TRXFCN_V cfg_LineInit(CFG_LINEFCN lineFcn,PVOID linebuf,
  int buflen,UINT uPrefix)
{
  cfg_linecnt=0;
  cfg_linebuf=(PBYTE)linebuf;
  _cfg_sizlinebuf=buflen;
  _cfg_linefcn=lineFcn;
  _cfg_eof=FALSE;
  _cfg_prefix=uPrefix;
}

TRXFCN_I cfg_FindKey(LPSTR key)
{
  int cut,i;
  int max=_cfg_numtypes;
  int min=0;

  _strupr(key);

  while(max>min) {
    cut=(max+min)>>1;
    if((i=strcmp(_cfg_type[cut],key))==0) return cut+1;
    if(i>0) max=cut;
    else {
	  if(cfg_noexact && !memcmp(_cfg_type[cut],key,strlen(_cfg_type[cut]))) return cut+1;
	  min=cut+1;
    }
  }
  return CFG_ERR_KEYWORD;
}

static PBYTE NEAR PASCAL skipspc(PBYTE p)
{
  while(*p==' ' || *p=='\t') p++;
  return(p);
}

static BOOL NEAR PASCAL get_escchr(PBYTE p)
{
        if(p[1]=='n') p[1]='\n';
        strcpy(p,p+1);
        return *p!=0;
}

TRXFCN_I cfg_GetArgv(PSTR pzBuffer,UINT uPrefix)
{
	PBYTE p=(PBYTE)pzBuffer;
	char c;
	
	cfg_argc=0;
    cfg_commptr=0;

    if(*(p=skipspc(p))==0 || *p==cfg_commchr) {
      if(*p) cfg_commptr=++p;
      return 0;
    }
	
	if(uPrefix==CFG_PARSE_NONE || uPrefix>CFG_PARSE_KEY && *p!=uPrefix) {
	    cfg_argv[cfg_argc++]=p;
	    return (uPrefix==CFG_PARSE_NONE)?1:0;
        }

        /*For now we allow whitespace between the prefix character and
          keyword. A line consisting of a single prefix character will
          be treated as a blank line.*/

        if(uPrefix>CFG_PARSE_KEY &&
          (*(p=skipspc(++p))==0 || *p==cfg_commchr)) {
          if(*p) cfg_commptr=++p;
          return 0;
        }
	
    /*p is now positioned at a non-whitespace character
      following the prefix character (if any) -- */
    
    while(*p) {
      /*At this point, p points to the beginning of a non-null token, to a
      comma representing a null token, to a first quote character,
      or to the comment character --*/
	  
      c=*p;
      if(c==cfg_commchr) {
        *p++=0;
        cfg_commptr=p;
        break;
      }

      if(cfg_argc==CFG_MAXARGS) break;

      if(cfg_quotes && c=='"') {
        *p++=0;
        cfg_argv[cfg_argc++]=p;
        while(*p && *p!='"' && (*p!=cfg_escchr || get_escchr(p))) p++;
        if(*p) *p=' ';
        /*The inner loop will terminate the quoted argument*/
      }
      else if(c!='=' || !cfg_equals) {
        cfg_argv[cfg_argc++]=p;
        if(c==',') {
          *p=0;
          p=skipspc(++p);
          continue;
        }
        p++;
      }

      do {
        /*At this point we are checking *p for the termination
          of the token which has already been assigned to cfg_argv[].
          *p==0 is possible only upon first entry. Herafter, we break
          this loop only when *p==0 or when p is positioned
          at the start of a new token --*/
          
        switch(c=*p) {
          case '='  : if(!cfg_equals) {
                         p++;
                         continue;
                      }
                      if(cfg_argc==CFG_MAXARGS) break;
                      cfg_argv[cfg_argc++]=cfg_equalsptr;
                      *p=0;
                      p=skipspc(++p);
                      break;
                      
          case ' '  :
          case '\t' : *p=0;
                      p=skipspc(++p);
                      if(*p==',') p=skipspc(++p);

          case 0    : /*If this is the first argument, check for a keyword match --*/
                      if(cfg_argc==1 && uPrefix>=CFG_PARSE_KEY) {
                         if(*p) cfg_argv[cfg_argc++]=p;
                         return cfg_FindKey(cfg_argv[0]);
                      }
                      break;
                      
		  case ',':   if(cfg_ignorecommas) {
                         p++;
                         continue;
 		              }
			          *p=0;
                      p=skipspc(++p);
                      /*two successive commas will insert a null token --*/
                      break;

          case '"' : if(cfg_quotes) break;

          default   : if(c==cfg_commchr) {
                        *p=0;
                        cfg_commptr=p+1;
                      }
                      else p++;
                      continue;
        } /*switch*/
        
        break;
        
      } while(*p);
      
    } /* while(*p) */

    if(cfg_argc==1 && uPrefix>=CFG_PARSE_KEY) return cfg_FindKey(cfg_argv[0]);
    return cfg_argc;
}

TRXFCN_I cfg_GetLine(void)
{
  int i,sz;
  PBYTE p=cfg_linebuf;
  BOOL bContinued=FALSE;

  cfg_argc=0;
  if(_cfg_eof) return CFG_ERR_EOF;

  while(TRUE) {
    sz=_cfg_sizlinebuf-(p-cfg_linebuf);
    /*Note: sz >= 1*/
    cfg_linecnt++;
    if((i=_cfg_linefcn(p,sz))<0) {
      cfg_linecnt--;
      _cfg_eof=TRUE;
      if(bContinued) break;
      return CFG_ERR_EOF;
    }
    if(i==sz-1) {
      /*Assume truncation!*/
      bContinued=FALSE;
      break;
    }
    if(i) {
      bContinued=TRUE;
      p+=i-1;
      if(*p!=cfg_contchr) break;
      *p++=' ';
    }
    else {
		if(!cfg_skipempty) bContinued=TRUE;
		if(bContinued) break;
	}
  }
  
  i=cfg_GetArgv((PSTR)cfg_linebuf,_cfg_prefix);
  
  return bContinued?i:CFG_ERR_TRUNCATE;
}    

