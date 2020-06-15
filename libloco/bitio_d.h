/* bitio.h --- for I/O routines
 *
 * Initial code by Alex Jakulin,  Aug. 1995
 *
 * Modified and optimized: Gadiel Seroussi, October 1995
 *
 * Color Enhancement: Guillermo Sapiro, August 1996
 *
 * Modified and added Restart marker and input tables by:
 * David Cheng-Hsiu Chu, and Ismail R. Ismail march 1999
 */

#ifndef BITIO_D_H
#define BITIO_D_H

#include "global_d.h"

#define FILL_MACRO

#define mygetc() ((_nti_buffer_pos < lco_inEOF) ? (*_nti_buffer_pos++):(lco_inErr++,0))
#define myungetc(x) (--_nti_buffer_pos,*_nti_buffer_pos=(x))

#ifdef FILL_MACRO
 /* loads more data in the input buffer (inline code )*/
#define FILLBUFFER(no) \
 {                        \
		assert(no+lco_bits <= 24);				\
        lco_reg <<= no;                         \
        lco_bits += no;                         \
        while (lco_bits >= 0) {                 \
            byte x = mygetc();					\
			if ( x == 0xff ) {                  \
				if ( lco_bits < 8 ) {			\
					myungetc(0xff);				\
					break;						\
				}								\
				x = mygetc();					\
				if ( !(x & 0x80) )  { /* non-marker: drop 0 */	\
					lco_reg |= (0xff << lco_bits) | ((x & 0x7f)<<(lco_bits-7));\
					lco_bits -= 15;				\
				}								\
				else {							\
					/* marker: hope we know what we're doing */\
					/* the "1" bit following ff is NOT dropped */\
					lco_reg |= (0xff << lco_bits) | (x <<(lco_bits-8));\
					lco_bits -= 16;				\
				}								\
				continue;						\
			}									\
            lco_reg |= x << lco_bits;			\
            lco_bits -= 8;						\
        }										\
}
#else
void fillbuffer(int no);
#define FILLBUFFER(no) fillbuffer(no)
#endif

#define GETBITS(x, n)\
 { x = lco_reg >> (32 - (n));\
   FILLBUFFER(n);\
 }

#endif /* BITIO_H */
