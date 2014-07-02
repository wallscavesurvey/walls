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

#ifndef BITIO_H
#define BITIO_H

#include "global_e.h"


//extern void lco_flushbuff();

#define PUT_ZEROES(n)									\
{                                                       \
        lco_bits -= (n);                                    \
        while (lco_bits <= 24) {                            \
				myputc((byte)(lco_reg >> 24));		\
                lco_reg <<= 8;                              \
                lco_bits += 8;                              \
        }                                               \
}

/*
 * Put an n-bit number x in the output stream (inline code).
 * Check for output bytes of the form 0xff and stuff
 * a 0 bit following each one.
 */

#define PUTBITS(x, n)									\
{														\
	assert(n <= 24 && (n) >= 0 && ((1<<(n))>(x)));		\
        lco_bits -= (n);                                    \
        lco_reg |= (x) << lco_bits;                             \
        while (lco_bits <= 24) {                            \
            byte outbyte = (byte)(lco_reg >> 24);			\
			myputc(outbyte);							\
			if ( outbyte == 0xff ) {					\
				lco_bits += 7;								\
				lco_reg <<= 7;								\
                /* stuff a 0 at MSB */					\
				lco_reg &= ~(1<<(8*sizeof(lco_reg)-1));			\
			}											\
			else {										\
				lco_bits += 8;								\
				lco_reg <<= 8;								\
			}											\
        }                                               \
}

#define PUT_ONES(n)                                     \
{                                                       \
	if ( n < 24 ) {										\
	    PUTBITS((1<<n)-1,n);							\
	}													\
	else {												\
	    register unsigned nn = n;						\
	    while ( nn >= 24 ) {							\
		PUTBITS((1<<24)-1,24);							\
		nn -= 24;										\
	    }												\
	    if ( nn ) PUTBITS((1<<nn)-1,nn);				\
	}													\
}

#endif /* BITIO_H */
