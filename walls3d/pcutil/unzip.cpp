// unzip.cpp : implementation of Unzip features
// written in January 1995
// Gerbert Orasche
// thesis at the 
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology
//

/////////////////////////////////////////////////////////////////////////////
// CUnzip
// implemented formats:
// gnuzip
// not yet implemented but partly present
// pkzip
// compress (lzw)
// lzh

#include "../stdafx.h"
#include "../hgapp.h"
#include <fstream>
using namespace std;

#include "unzip.h"

// Tables for deflate from PKZIP's appnote.txt. 
// Order of the bit length code lengths 
static unsigned border[] = {    
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
// Copy lengths for literal codes 257..285 
static unsigned short cplens[] = {         
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
        // note: see note #13 above about the 258 in this list. 
// Extra bits for literal codes 257..285 
static unsigned short cplext[] = {         
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99}; // 99==invalid 
// Copy offsets for distance codes 0..29 
static unsigned short cpdist[] = {         
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577};
// Extra bits for distance codes 
static unsigned short cpdext[] = {         
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
        12, 12, 13, 13};

// Table of CRC-32's of all single-byte values (made by makecrc.c)
unsigned long crc_32_tab[] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};

unsigned int mask_bits[] = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

CUnzip::CUnzip()
{
  // original crc 
  orig_crc=0L;     
  // original uncompressed length 
  orig_len=0L;     
  // output count
  bytes_out=0L;
  // handles to buffers  
  m_pcInBuf=NULL;
  m_iInPtr=0;
  m_iInSize=0;
  m_pcOutBuf=NULL;
  m_iMethod=NO_METHOD;
}

CUnzip::~CUnzip()
{
  // release resources
  delete[](m_pcInBuf);
  delete[](m_pcOutBuf);
  /*delete[](slide);*/

  if(InSt.is_open())
  {
    InSt.close();
  }
  if(OutSt.is_open())
  {
    OutSt.close();
  }
}

// opesd input file, checks for magic and
// positions file pointer for further work
int CUnzip::CheckMagic(const char* pInFile)
{
  // buffer for magic number
  unsigned char magic[2];  

  // open input file
  InSt.open((const char*)pInFile,ios::in);
  if(!InSt)
  {
    AfxMessageBox("CUnzip: could not open file.",MB_OK|MB_ICONSTOP,0);
    return(UNZIP_ERROR);
  }

  // allocate buffers
  m_pcInBuf=new char[INBUFSIZ+INBUF_EXTRA];
  if(m_pcInBuf==0)
  {
    AfxMessageBox("CUnzip: Error allocating input buffer",
                   MB_OK|MB_ICONSTOP,0);
    return(UNZIP_ERROR);
  }

  // get magic number from file
  magic[0]=GetByte(InSt);
  magic[1]=GetByte(InSt);

  // test, if gnu-zipped archive
  if(memcmp(magic, GZIP_MAGIC, 2) == 0 ||
     memcmp(magic, OLD_GZIP_MAGIC, 2) == 0)
  {
  	m_iMethod=GetByte(InSt);
    if(m_iMethod!=DEFLATED)
    {
      AfxMessageBox("CUnzip: unknown GZIP version used",MB_OK|MB_ICONSTOP,0);
      return(UNZIP_ERROR);
    }

  }
  // pkunzip not implemented yet
/*  else
  if(memcmp(magic, PKZIP_MAGIC, 2)== 0 && inptr == 2 &&
     memcmp((char*)inbuf, PKZIP_MAGIC, 4) == 0)
  {
    // To simplify the code, we support a zip file when alone only.
    // We are thus guaranteed that the entire local header fits in inbuf.
    m_iInptr=0;
    if(check_zipfile(InSt)!=OK)
      return(UNZIP_ERROR);
  }*/
  else
  if(memcmp(magic,PACK_MAGIC,2)==0)
  {
    m_iMethod=PACKED;
  }
  else
  if(memcmp(magic,LZW_MAGIC,2)==0)
  {
    m_iMethod=COMPRESSED;
  }
  else
  if(memcmp(magic,LZH_MAGIC,2)==0)
  {
    m_iMethod=LZHED;
  }
  return(m_iMethod);
}

int CUnzip::NeedBits(unsigned int BitNum,unsigned long* pBitBuffer,
                      unsigned int* pBitCount)
{
  while(*pBitCount<BitNum)
  {
    *pBitBuffer|=((unsigned long)GetByte(InSt))<<*pBitCount;
    *pBitCount+=8;
    if(m_iInSize==0)
    {
      AfxMessageBox("CUnzip: unexpected end of file.",
                    MB_OK|MB_ICONSTOP,0);
      return(FALSE);
    }
  }
  return(TRUE);
}


// returns one byte from buffer, if buffer is read
// gets a new block
unsigned char CUnzip::GetByte(ifstream& InSt)
{
  // check, if there is data...
  if(m_iInPtr>=m_iInSize)
  {
    // yeah, have to get new data
    InSt.read(m_pcInBuf,INBUFSIZ);
    m_iInSize=(UINT)InSt.gcount();
    // reset file counter
    m_iInPtr=0;
  }
  return(m_pcInBuf[m_iInPtr++]);
} 

// Unzip: unzips up to now only gzip!
int CUnzip::Unzip(char* pOutFile,char* pszTempPath, char* pszExt)
{
  // compression flags 
  unsigned char flags;     
  // magic header 
  int count;
 
  // check, if CheckMagic was called before
  if(m_iMethod==NO_METHOD)
  {
    AfxMessageBox("CUnzip: Internal error - No Method defined",
                   MB_OK|MB_ICONSTOP,0);
    return(UNZIP_ERROR);
  }

  m_pcOutBuf=new unsigned char[2L*WSIZE];
  if(m_pcOutBuf==0)
  {
    AfxMessageBox("CUnzip: Error allocating output buffer",MB_OK,0);
    return(UNZIP_ERROR);
  }

 /* slide=new unsigned char[2L*WSIZE];
  if(slide==0)
  {
    AfxMessageBox("CUnzip: Error allocating buffer",MB_OK,0);
    return(UNZIP_ERROR);
  }*/

  // decompress with right algorithm
  switch(m_iMethod)
  {
    case DEFLATED:
      // get flags
      flags=GetByte(InSt);

      // check for encryption
      if((flags&ENCRYPTED)!=0)
      {
        AfxMessageBox("CUnzip: data is encrypted",MB_OK|MB_ICONSTOP,0);
        return(UNZIP_ERROR);
      }

      // check for multiple volumes
      if((flags&CONTINUATION)!=0)
      {
        AfxMessageBox("CUnzip: multi-part gzip file",MB_OK|MB_ICONSTOP,0);
        return(UNZIP_ERROR);
      }

      if((flags&RESERVED)!=0)
      {
        AfxMessageBox("CUnzip: unknown flags",MB_OK|MB_ICONSTOP,0);
        return(UNZIP_ERROR);
      }

      // originally stamp and extra flag and os-type ignored:
      count=6;
      while(count--)
        GetByte(InSt);

      if((flags&EXTRA_FIELD)!=0)
      {
        unsigned len =(unsigned)GetByte(InSt);
        len |= ((unsigned)GetByte(InSt))<<8;
        AfxMessageBox("CUnzip: extra field ignored",MB_OK,0);
        while(len--)
          GetByte(InSt);
      }

      // Get original file name
      if((flags & ORIG_NAME)!=0)
      {
        // Copy the base name. Keep a directory prefix intact. 
        strcpy(m_pcOutName,pszTempPath);
        int iLenBuf=strlen(m_pcOutName);
        char *pcTemp=m_pcOutName+iLenBuf;
        int count=0;
        while(TRUE)
        {
          *pcTemp=(char)GetByte(InSt);
          count++;
          if(*pcTemp++=='\0')
            break;
          if(pcTemp>=m_pcOutName+OUTNAME_SIZE)
          {
            AfxMessageBox("CUnzip: corrupted input -- file name too large",
                           MB_OK|MB_ICONSTOP,0);
            return(UNZIP_ERROR);
          }
        }

        // no long filenames in Win 3.x!
        if(count>8)
        {
          *(m_pcOutName+iLenBuf+8)=0;
          if((pcTemp=strchr(m_pcOutName,'.')))
          {
            *pcTemp=0;
          }
          strcat(m_pcOutName,pszExt);
        }
      }
      else
      {
        AfxMessageBox("CUnzip: No original filename stored",
                       MB_OK|MB_ICONSTOP,0);
        return(UNZIP_ERROR);
      }

      // Discard file comment if any 
 	  if((flags&COMMENT)!=0)
  	  {
  	    while(GetByte(InSt)!=0)
          ;
      }

      // open outputfile
      OutSt.open(m_pcOutName,ios::out|ios::trunc|ios::binary);
      if(OutSt.fail())
      {
        AfxMessageBox("CUnzip: could not open outputfile.",MB_OK|MB_ICONSTOP,0);
        return(UNZIP_ERROR);
      }

      // and now really decompress!
      if(Inflate(InSt))
        return(UNZIP_ERROR);  
    break;

    case STORED:
    case COMPRESSED:
    case PACKED:
    case LZHED:
    case MAX_METHODS:

    case NO_METHOD:
    default:
      AfxMessageBox("CUnzip: Internal error - Method not implemented.",
                     MB_OK|MB_ICONSTOP,0);
      return(UNZIP_ERROR);
    break;
  }
  // copy file name to calling class
  strcpy(pOutFile,m_pcOutName);
  return(UNZIP_OK);
}


int CUnzip::Inflate(ifstream& InSt)
{
  // last block flag 
  int e;                
  // result code 
  int r;                
  // maximum struct huft's malloc'ed 
  unsigned h;           

  // initialize window, bit buffer
  lbits = 9;          
  // bits in base distance lookup table 
  dbits = 6;          
 
  m_OutCount=0;
  bk=0;
  bb=0;

  // decompress until the last block 
  h=0;
  do
  {
    hufts=0;
    r=inflate_block(&e);
    if(r!=0)
      return(r);
    if(hufts>h)
      h=hufts;
  }
  while(!e);

  // Undo too much lookahead. The next read will be byte aligned so we
  // can discard unused bits in the last meaningful byte.
  while(bk>=8)
  {
    bk-=8;
    m_iInPtr--;
  }

  // flush out m_pcOutBuf 
  flush_output(m_OutCount);

  // return success 
  return(0);
}

// decompress an inflated block 
int CUnzip::inflate_block(int* e)
{
  // block type 
  unsigned t;           
  // bit buffer 
  register unsigned long b;       
  // number of bits in bit buffer 
  register unsigned k;  

  // make local bit buffer 
  b=bb;
  k=bk;

  // read in last block bit 
  if(NeedBits(1,&b,&k)==FALSE)
    return(3);
  *e=(int)b&1;
  DUMPBITS(1)

  // read in block type 
  if(NeedBits(2,&b,&k)==FALSE)
    return(3);
  t = (unsigned)b & 3;
  DUMPBITS(2)


  /* restore the global bit buffer */
  bb = b;
  bk = k;


  /* inflate that block type */
  if (t == 2)
    return inflate_dynamic();
  if (t == 0)
    return inflate_stored();
  if (t == 1)
    return inflate_fixed();


  /* bad block type */
  return(2);
}


int CUnzip::inflate_stored()
/* "decompress" an inflated type 0 (stored) block. */
{
  unsigned n;           /* number of bytes in block */
  unsigned int w;           /* current window position */
  register unsigned long b;       /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */


  /* make local copies of globals */
  b = bb;                       /* initialize bit buffer */
  k = bk;
  w = m_OutCount;                       /* initialize window position */


  /* go to byte boundary */
  n = k & 7;
  DUMPBITS(n);


  /* get the length and its complement */
  if(NeedBits(16,&b,&k)==FALSE)
    return(2);
  n = ((unsigned)b & 0xffff);
  DUMPBITS(16)

  if(NeedBits(16,&b,&k)==FALSE)
    return(2);
  if (n != (unsigned)((~b) & 0xffff))
    // error in compressed data 
    return(1);                   
  DUMPBITS(16)


  /* read and output the compressed data */
  while (n--)
  {
    if(NeedBits(8,&b,&k)==FALSE)
      return(2);
    m_pcOutBuf[w++] = (unsigned char)b;
    if (w == WSIZE)
    {
      flush_output(w);
      w = 0;
    }
    DUMPBITS(8)
  }


  /* restore the globals from the locals */
  m_OutCount = w;                       /* restore global window pointer */
  bb = b;                       /* restore global bit buffer */
  bk = k;
  return(0);
}

// decompress an inflated type 1 (fixed Huffman codes) block.  We should
// either replace this with a custom decoder, or at least precompute the
// Huffman tables. 
int CUnzip::inflate_fixed()
{
  int i;                /* temporary variable */
  struct huft *tl;      /* literal/length code table */
  struct huft *td;      /* distance code table */
  int bl;               /* lookup bits for tl */
  int bd;               /* lookup bits for td */
  unsigned l[288];      /* length list for huft_build */


  /* set up literal table */
  for (i = 0; i < 144; i++)
    l[i] = 8;
  for (; i < 256; i++)
    l[i] = 9;
  for (; i < 280; i++)
    l[i] = 7;
  for (; i < 288; i++)          /* make a complete, but wrong code set */
    l[i] = 8;
  bl = 7;
  if ((i = huft_build(l, 288, 257, cplens, cplext, &tl, &bl)) != 0)
    return i;

  // set up distance table 
  for (i = 0; i < 30; i++)      /* make an incomplete code set */
    l[i] = 5;
  bd = 5;
  if ((i = huft_build(l, 30, 0, cpdist, cpdext, &td, &bd)) > 1)
  {
    huft_free(tl);
    return i;
  }

  // decompress until an end-of-block code 
  if (inflate_codes(tl, td, bl, bd))
    return 1;

  // free the decoding tables, return 
  huft_free(tl);
  huft_free(td);
  return 0;
}


// decompress an inflated type 2 (dynamic Huffman codes) block. 
int CUnzip::inflate_dynamic()
{
  int i;                /* temporary variables */
  unsigned j;
  unsigned int iLastLength;
  unsigned m;           /* mask for bit lengths table */
  unsigned n;           /* number of lengths to get */
  struct huft *tl;      /* literal/length code table */
  struct huft *td;      /* distance code table */
  int bl;               /* lookup bits for tl */
  int bd;               /* lookup bits for td */
  unsigned nb;          /* number of bit length codes */
  unsigned nl;          /* number of literal/length codes */
  unsigned nd;          /* number of distance codes */
#ifdef PKZIP_BUG_WORKAROUND
  unsigned ll[288+32];  /* literal/length and distance code lengths */
#else
  unsigned ll[286+30];  /* literal/length and distance code lengths */
#endif
  register unsigned long b;       /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */

  // make local bit buffer 
  b=bb;
  k=bk;

  // read in table lengths 
  if(NeedBits(5,&b,&k)==FALSE)
    return(2);
  // number of literal/length codes 
  nl = 257 + ((unsigned)b & 0x1f);      
  DUMPBITS(5)

  if(NeedBits(5,&b,&k)==FALSE)
    return(2);
  // number of distance codes 
  nd = 1 + ((unsigned)b & 0x1f);        
  DUMPBITS(5)

  if(NeedBits(4,&b,&k)==FALSE)
    return(2);
  // number of bit length codes 
  nb = 4 + ((unsigned)b & 0xf);         
  DUMPBITS(4)

#ifdef PKZIP_BUG_WORKAROUND
  if (nl > 288 || nd > 32)
#else
  if (nl > 286 || nd > 30)
#endif 
    // bad lengths 
    return(1);                   


  // read in bit-length-code lengths 
  for (j = 0; j < nb; j++)
  {
    if(NeedBits(3,&b,&k)==FALSE)
      return(2);
    ll[border[j]] = (unsigned)b & 7;
    DUMPBITS(3)
  }

  for(; j < 19; j++)
    ll[border[j]]=0;


  // build decoding table for trees--single level, 7 bit lookup 
  bl = 7;
  if ((i = huft_build(ll, 19, 19, NULL, NULL, &tl, &bl)) != 0)
  {
    if (i==1)
      huft_free(tl);
    // incomplete code set 
    return(i);                   
  }

  // read in literal and distance code lengths 
  n = nl + nd;
  m = mask_bits[bl];
  i=0;
  iLastLength=0;
  while ((unsigned)i < n)
  {
    if(NeedBits((unsigned)bl,&b,&k)==FALSE)
      return(2);
    j = (td = tl + ((unsigned)b & m))->b;
    DUMPBITS(j)

    j = td->v.n;
    if(j<16)
    {                 /* length of code in bits (0..15) */
      iLastLength=j;
      ll[i++]=iLastLength;          
    }
    else if (j == 16)           /* repeat last length 3 to 6 times */
    {
      if(NeedBits(2,&b,&k)==FALSE)
        return(2);
      j = 3 + ((unsigned)b & 3);
      DUMPBITS(2)

      if((unsigned)i + j > n)
        return(1);

      while(j--)
        ll[i++]=iLastLength;
    }
    else if (j == 17)           /* 3 to 10 zero length codes */
    {
      if(NeedBits(3,&b,&k)==FALSE)
        return(2);
      j = 3 + ((unsigned)b & 7);
      DUMPBITS(3)

      if((unsigned)i + j > n)
        return 1;

      while(j--)
        ll[i++]=0;
      iLastLength=0;
    }
    else                        /* j == 18: 11 to 138 zero length codes */
    {
      if(NeedBits(7,&b,&k)==FALSE)
        return(2);
      j = 11 + ((unsigned)b & 0x7f);
      DUMPBITS(7)

      if ((unsigned)i + j > n)
        return 1;

      while(j--)
        ll[i++]=0;
      iLastLength=0;
    }
  }

  // free decoding table for trees 
  huft_free(tl);

  // restore the global bit buffer 
  bb = b;
  bk = k;

  // build the decoding tables for literal/length and distance codes 
  bl = lbits;
  if ((i = huft_build(ll, nl, 257, cplens, cplext, &tl, &bl)) != 0)
  {
    if (i == 1) {
      fprintf(stderr, " incomplete literal tree\n");
      huft_free(tl);
    }
    return i;                   /* incomplete code set */
  }
  bd = dbits;
  if ((i = huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd)) != 0)
  {
    if (i == 1) {
      fprintf(stderr, " incomplete distance tree\n");
#ifdef PKZIP_BUG_WORKAROUND
      i = 0;
    }
#else
      huft_free(td);
    }
    huft_free(tl);
    return i;                   /* incomplete code set */
#endif
  }

  // decompress until an end-of-block code 
  if (inflate_codes(tl, td, bl, bd))
    return 1;


  /* free the decoding tables, return */
  huft_free(tl);
  huft_free(td);
  return(0);
}
   

// Write the output window window[0..outcnt-1] and update crc and bytes_out.
// (Used for the decompressed data only.)
void CUnzip::flush_window()
{
  if(m_OutCount==0)
    return;
  updcrc(m_pcOutBuf,m_OutCount);

  OutSt.write((const char *)m_pcOutBuf,m_OutCount);
  bytes_out+=(unsigned long)m_OutCount;
  m_OutCount=0;
}

// Run a set of bytes through the crc shift register.  If s is a NULL
// pointer, then initialize the crc shift register contents instead.
// Return the current crc in either case.
unsigned long CUnzip::updcrc(unsigned char* s,unsigned int n)
{
  register unsigned long c;         /* temporary variable */

  static unsigned long crc = (unsigned long)0xffffffffL; /* shift register contents */

  if(s==NULL)
  {
    c = 0xffffffffL;
  }
  else
  {
    c=crc;
    if(n)
      do
      {
        c=crc_32_tab[((int)c ^ (*s++)) & 0xff] ^ (c >> 8);
      }
      while(--n);
  }
  crc=c;
  return(c ^ 0xffffffffL);       /* (instead of ~c for 64-bit machines) */
}



/*unsigned *b;             code lengths in bits (all assumed <= BMAX) 
unsigned n;              number of codes (assumed <= N_MAX) 
unsigned s;              number of simple-valued codes (0..s-1) 
unsigned short *d;                  list of base values for non-simple codes 
unsigned short *e;                  list of extra bits for non-simple codes 
struct huft **t;         result: starting table 
int *m;                  maximum lookup bits, returns actual */

// Given a list of code lengths and a maximum table size, make a set of
// tables to decode that set of codes.  Return zero on success, one if
// the given code set is incomplete (the tables are still built in this
// case), two if the input is invalid (all zero length codes or an
// oversubscribed set of lengths), and three if not enough memory. 
int CUnzip::huft_build(unsigned int* b,unsigned int n,unsigned int s,
               unsigned short* d,unsigned short* e,struct huft** t,
               int* m)
{
  unsigned a;                   /* counter for codes of length k */
  unsigned c[BMAX+1];           /* bit length count table */
  unsigned f;                   /* i repeats in table every f entries */
  int g;                        /* maximum code length */
  int h;                        /* table level */
  register unsigned i;          /* counter, current code */
  register unsigned j;          /* counter */
  register int k;               /* number of bits in current code */
  int l;                        /* bits per table (returned in m) */
  register unsigned *p;         /* pointer into c[], b[], or v[] */
  register struct huft *q;      /* points to current table */
  struct huft r;                /* table entry for structure assignment */
  struct huft *u[BMAX];         /* table stack */
  unsigned v[N_MAX];            /* values in order of bit length */
  register int w;               /* bits before this table == (l * h) */
  unsigned x[BMAX+1];           /* bit offsets, then code stack */
  unsigned *xp;                 /* pointer into x */
  int y;                        /* number of dummy codes added */
  unsigned z;                   /* number of entries in current table */


  /* Generate counts for each bit length */
  memset(c,0,sizeof(c));
  p=b;
  i=n;
  do
  {
    c[*p]++;                    /* assume all entries <= BMAX */
    p++;                      /* Can't combine with above line (Solaris bug) */
  }
  while(--i);

  if (c[0]==n)                /* null input--all zero length codes */
  {
    *t=(struct huft *)NULL;
    *m=0;
    return(0);
  }


  // Find minimum and maximum length, bound *m by those 
  l=*m;
  for(j=1;j<=BMAX;j++)
    if(c[j])
      break;

  // minimum code length 
  k=j;                        
  if((unsigned)l<j)
    l=j;

  for (i=BMAX;i;i--)
    if (c[i])
      break;

  // maximum code length 
  g=i;                        
  if((unsigned)l>i)
    l=i;
  *m=l;

  // Adjust last length count to fill out codes, if needed 
  for (y=(1<<j);j<i;j++,y<<=1)
    if((y-=c[j])<0)
      // bad input: more codes than bits 
      return(2);                 

  if((y-=c[i])<0)
    return(2);

  c[i]+=y;

  // Generate starting offsets into the value table for each length 
  j=0;
  x[1]=0;
  p=c+1;
  xp=x+2;

  // note that i == g from above 
  while(--i)
  {                 
    *xp++=(j+=*p++);
  }

  // Make a table of values in order of bit lengths 
  p=b;
  i=0;
  do
  {
    if((j=*p++)!=0)
      v[x[j]++]=i;
  }
  while(++i<n);

  // Generate the Huffman codes and for each, make the table entries 
  i=0;
  // first Huffman code is zero 
  x[0]=0;                 
  // grab values in bit order 
  p=v;                        
  // no tables yet--level -1 
  h=-1;                       
  // bits decoded == (l * h) 
  w=-l;                       
  // just to keep compilers happy 
  u[0]=(struct huft *)NULL;   
  q=(struct huft *)NULL;      
  z=0;                        

  // go through the bit lengths (k already is bits in shortest code) 
  for (;k<=g;k++)
  {
    a=c[k];
    while(a--)
    {
      // here i is the Huffman code of length k bits for value *p 
      // make tables up to required level 
      while(k>w+l)
      {
        h++;
        // previous table always l bits 
        w+=l;                 

        // compute minimum size table less than or equal to l bits 
        // upper limit on table size 
        z=(z=g-w)>(unsigned)l ? l : z;  

        // try a k-w bit table 
        if ((f = 1 << (j = k - w)) > a + 1)     
        {                       
          // too few codes for k-w bit table 
          // deduct codes from patterns left 
          f-=a+1;           
          xp=c+k;

          // try smaller tables up to z bits 
          while(++j<z)       
          {
            if((f<<=1)<=*++xp)
               // enough codes to use up j bits 
              break;           
            // else deduct codes from patterns 
            f-=*xp;           
          }
        }
        // table entries for j-bit table 
        z=1<<j;             

        // allocate and link in new table 
        q=(struct huft *)malloc((z + 1)*sizeof(struct huft));
        if(q==(struct huft *)NULL)
        {
          if(h)
            huft_free(u[0]);
          // not enough memory 
          return(3);             
        }

        // track memory usage 
        hufts+=z+1;         
        // link to list for huft_free() 
        *t=q+1;             
        *(t=&(q->v.t))=(struct huft *)NULL;
        // table starts after link 
        u[h]=++q;             

        // connect to last table, if there is one 
        if(h)
        {
          // save pattern for backing up 
          x[h]=i;             
          // bits to dump before this table 
          r.b=(unsigned char)l;         
          // bits in this table
          r.e=(unsigned char)(16 + j);  
          // pointer to this table 
          r.v.t=q;           
          // (get around Turbo C bug)  
          j= i >> (w-l);     
          // connect to last table 
          u[h-1][j]= r;        
        }
      }

      // set up table entry in r 
      r.b=(unsigned char)(k-w);
      if (p >= v + n)
        // out of values--invalid code 
        r.e = 99;               
      else
      if(*p<s)
      {
        // 256 is end-of-block code 
        r.e=(unsigned char)(*p < 256 ? 16 : 15);
        // simple code is just the value     
        r.v.n=(unsigned short)(*p);  
        // one compiler does not like *p++            
        p++;                           
      }
      else
      {
        // non-simple--look up in lists 
        r.e=(unsigned char)e[*p-s];   
        r.v.n = d[*p++ - s];
      }

      // fill code-like entries with r 
      f=1<<(k - w);
      for(j=i>>w;j<z;j+=f)
        q[j]=r;

      // backwards increment the k-bit code i 
      for (j=1<<(k - 1);i&j;j>>=1)
        i^=j;
      i^=j;

      // backup over finished tables 
      while((i&((1<<w)-1))!=x[h])
      {
        // don't need to update q 
        h--;                    
        w-=l;
      }
    }
  }


  // Return true (1) if we were given an incomplete table 
  return((y!=0)&&(g!=1));
}

// Free the malloc'ed tables built by huft_build(), which makes a linked
// list of the tables it made, with the links in a dummy first entry of
// each table. 
int CUnzip::huft_free(struct huft* HufTables)
{
  register struct huft* HufCount;
  register struct huft* HufHelp;


  // Go through linked list, freeing from the malloced (t[-1]) address. 
  HufCount=HufTables;
  while(HufCount!=(struct huft *)NULL)
  {
    HufHelp=(--HufCount)->v.t;
    free(HufCount);
    HufCount=HufHelp;
  } 
  return(0);
}

/*struct huft *tl, *td;    literal/length and distance decoder tables 
int bl, bd;              number of bits decoded by tl[] and td[] 
 inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
int CUnzip::inflate_codes(struct huft* tl,struct huft* td,int bl,int bd)
{
  register unsigned e;  /* table entry flag/number of extra bits */
  unsigned n, d;        /* length and index for copy */
  unsigned int w;           /* current window position */
  struct huft *t;       /* pointer to table entry */
  unsigned ml, md;      /* masks for bl and bd bits */
  register unsigned long b;       /* bit buffer */
  register unsigned k;  /* number of bits in bit buffer */


  // make local copies of globals 
  // initialize bit buffer 
  b = bb;                       
  k = bk;
  // initialize window position 
  w = m_OutCount;                       

  // inflate the coded data 
  // precompute masks for speed 
  ml = mask_bits[bl];           
  md = mask_bits[bd];

  // do until end of block 
  while(TRUE)                      
  {
    if(NeedBits((unsigned)bl,&b,&k)==FALSE)
      return(2);
    if ((e = (t = tl + ((unsigned)b & ml))->e) > 16)
      do {
        if (e == 99)
          return(1);
        DUMPBITS(t->b)
        e -= 16;
        if(NeedBits(e,&b,&k)==FALSE)
          return(2);
      } while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
    DUMPBITS(t->b)
    if (e == 16)                /* then it's a literal */
    {
      m_pcOutBuf[w++] = (unsigned char)t->v.n;
      if (w == WSIZE)
      {
        flush_output(w);
        w = 0;
      }
    }
    else                        /* it's an EOB or a length */
    {
      /* exit if end of block */
      if (e == 15)
        break;

      /* get length of block to copy */
      if(NeedBits(e,&b,&k)==FALSE)
        return(2);
      n = t->v.n + ((unsigned)b & mask_bits[e]);
      DUMPBITS(e);

      /* decode distance of block to copy */
      if(NeedBits((unsigned)bd,&b,&k)==FALSE)
        return(2);
      if ((e = (t = td + ((unsigned)b & md))->e) > 16)
        do {
          if (e == 99)
            return 1;
          DUMPBITS(t->b)
          e -= 16;
          if(NeedBits(e,&b,&k)==FALSE)
            return(2);
        } while ((e = (t = t->v.t + ((unsigned)b & mask_bits[e]))->e) > 16);
      DUMPBITS(t->b)
      if(NeedBits(e,&b,&k)==FALSE)
        return(2);
      d = w - t->v.n - ((unsigned)b & mask_bits[e]);
      DUMPBITS(e)
/*      Tracevv((stderr,"\\[%d,%d]", w-d, n));*/

      /* do the copy */
      do {
        n -= (e = (e = WSIZE - ((d &= WSIZE-1) > w ? d : w)) > n ? n : e);
#if !defined(NOMEMCPY) && !defined(DEBUG)
        // (this test assumes unsigned comparison) 
        if (w - d >= e)         
        {
          memcpy(m_pcOutBuf+w,m_pcOutBuf+d,(size_t)e);
          w += e;
          d += e;
        }
        else                      /* do it slow to avoid memcpy() overlap */
#endif /* !NOMEMCPY */
          do
          {
            m_pcOutBuf[w++]=m_pcOutBuf[d++];
          }
          while (--e);
        if (w == WSIZE)
        {
          flush_output(w);
          w = 0;
        }
      }
      while(n);
    }
  }


  // restore the globals from the locals 
  m_OutCount=w;                 
  // restore global bit buffer       
  bb = b;                       
  bk = k;

  // done 
  return(0);
}
