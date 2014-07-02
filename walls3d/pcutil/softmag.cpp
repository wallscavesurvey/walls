#include <stdio.h>
#include <string.h>
#include "file.h"
#include <pcutil\pcstuff.h>


extern FILE *efopen();
extern MagicStruct magic[];
static int magindex;

BOOL  match(const char *s, int bLen, char *outBuf, int outBufLen);
long mcheck(const char *s, MagicStruct *m);
void mprint(MagicStruct *m, const char *s, char *bufPos, char *bufPosEnd);
                 
int nmagic = 0;

//
// softmagic - lookup one file in database 
// (already read from magic-numbers file by apprentice).
// Passed the name and FILE * of one file to be typed.
//
BOOL softmagic(const char *buf, int bLen, char *outBuf, int outBufLen)
{
    magindex = 0;
    if (!match(buf, bLen, outBuf, outBufLen))
        return FALSE;       // error !

    return TRUE;
}

//
// go through the whole list, stopping if you find a match.
// Be sure to process every continuation of this match.
//
BOOL match(const char *s, int bLen, char *outBuf, int outBufLen)
{                                                                         
    char *outBufPos = outBuf;               // next position for output
    char *outBufEnd = outBuf + outBufLen;   // calculate output buffer end

    while (magindex < nmagic) 
    {
        // if main entry matches, print it...
        if (mcheck(s, &magic[magindex])) 
        {
            mprint(&magic[magindex], s, outBufPos, outBufEnd);
            
            // and any continuations that match 
            while (magic[magindex+1].contflag && magindex < nmagic) 
            {
                ++magindex;
                if (mcheck(s, &magic[magindex]))
                {
                    if (outBufPos<outBufEnd)
                    {
                        *outBufPos++ = ' ';
                        mprint(&magic[magindex], s, outBufPos, outBufEnd);
                    }
                }
            }
            return TRUE;       // all through
        } 
        else 
        {
            // main entry didn't match, flush its continuations 
            while (magic[magindex+1].contflag && magindex < nmagic) 
            {
                ++magindex;
            }
        }
        ++magindex;             // on to the next 
    }
    return FALSE;               // no match at all 
}


void mprint(MagicStruct *m, const char *s, char *bufPos, char *bufPosEnd)
{
    VALUETYPE *p = (VALUETYPE *)(s+m->offset);
    char *pp;
    char tmpBuf[512];

    switch (m->type) 
    {
        case BYTE_SIZE:
            sprintf(tmpBuf, m->desc, p->b);
            break;
        case SHORT_SIZE:
            sprintf(tmpBuf, m->desc, p->h);
            break;
        case LONG_SIZE:
            sprintf(tmpBuf, m->desc, p->l);
            break;
        case STRING:
            if ((pp=strchr(p->s, '\n')) != NULL)
                *pp = '\0';
            sprintf(tmpBuf, m->desc, p->s);
            break;
        default:
//            warning("invalid m->type (%d) in mprint()", m->type);  
            tmpBuf[0] = '0';
    }
    
    int tmpBufLen = strlen(tmpBuf);
    if ( (bufPos+tmpBufLen) < bufPosEnd )
    {
        strcpy(bufPos, tmpBuf);
        bufPos += tmpBufLen;
    }
} // mprint
    
    
long mcheck(const char *s, MagicStruct *m)
{
    union VALUETYPE *p = (union VALUETYPE *)(s+m->offset);
    long l = m->value.l;
    long v;

    switch (m->type) 
    {
    case BYTE_SIZE:
        v = p->b; break;
    case SHORT_SIZE:
        v = p->h; break;
    case LONG_SIZE:
        v = p->l; break;
    case STRING:
        l = 0;
        // What we want here is:
        // v = strncmp(m->value.s, p->s, m->vallen);
        // but ignoring any nulls.  bcmp doesn't give -/+/0
        // and isn't universally available anyway.
        //
        {
            register unsigned char *a = (unsigned char*)m->value.s;
            register unsigned char *b = (unsigned char*)p->s;
            register int len = m->vallen;

            while (--len >= 0)
                if ((v = *b++ - *a++) != 0)
                    break;
        }
        break;
    default:
//        warning("invalid type %d in mcheck()", m->type);
        return 0;
    }

    switch (m->reln) 
    {
    case '=':
        return v == l;
    case '>':
        return v > l;
    case '<':
        return v < l;
    case '&':
        return v & l;
    default:
//        warning("mcheck: can't happen: invalid relation %d", m->reln);
        return 0;
    }
} // mcheck
