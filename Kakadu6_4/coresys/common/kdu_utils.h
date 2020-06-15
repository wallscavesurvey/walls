/*****************************************************************************/
// File: kdu_utils.h [scope = CORESYS/COMMON]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/*****************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/*****************************************************************************/
// Licensee: Mr David McKenzie
// License number: 00590
// The licensee has been granted a NON-COMMERCIAL license to the contents of
// this source file.  A brief summary of this license appears below.  This
// summary is not to be relied upon in preference to the full text of the
// license agreement, accepted at purchase of the license.
// 1. The Licensee has the right to install and use the Kakadu software and
//    to develop Applications for the Licensee's own use.
// 2. The Licensee has the right to Deploy Applications built using the
//    Kakadu software to Third Parties, so long as such Deployment does not
//    result in any direct or indirect financial return to the Licensee or
//    any other Third Party, which further supplies or otherwise uses such
//    Applications.
// 3. The Licensee has the right to distribute Reusable Code (including
//    source code and dynamically or statically linked libraries) to a Third
//    Party, provided the Third Party possesses a license to use the Kakadu
//    software, and provided such distribution does not result in any direct
//    or indirect financial return to the Licensee.
/******************************************************************************
Description:
   Provides some handy in-line functions.
******************************************************************************/

#ifndef KDU_UTILS_H
#define KDU_UTILS_H

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "kdu_elementary.h"

/* ========================================================================= */
/*                            Convenient Inlines                             */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                           kdu_read                                 */
/*****************************************************************************/

static inline int
kdu_read(kdu_byte * &bp, kdu_byte *end, int nbytes)
{ /* [SYNOPSIS]
	   Reads an integer quantity having an `nbytes' bigendian
	   representation from the array identified by `bp'.  During the process,
	   `bp' is advanced `nbytes' positions.  If this pushes it up to or past
	   the `end' pointer, the function throws an exception of type
	   `kdu_byte *'.
	   [//]
	   The byte order is assumed to be big-endian.  If the local machine
	   architecture is little-endian, the input bytes are reversed.
	 [RETURNS]
	   The value of the integer recovered from the first `nbytes' bytes of
	   the buffer.
	 [ARG: bp]
	   Pointer to the first byte in the buffer from which the integer is
	   to be recovered.
	 [ARG: end]
	   Points immediately beyond the last valid entry in the buffer.
	 [ARG: nbytes]
	   Number of bytes from the buffer which are to be converted into a
	   big-endian integer.  Must be one of 1, 2, 3 or 4.
  */
	int val;

	assert(nbytes <= 4);
	if ((end - bp) < nbytes)
		throw bp;
	val = *(bp++);
	if (nbytes > 1)
		val = (val << 8) + *(bp++);
	if (nbytes > 2)
		val = (val << 8) + *(bp++);
	if (nbytes > 3)
		val = (val << 8) + *(bp++);
	return val;
}

/*****************************************************************************/
/* INLINE                       kdu_read_float                               */
/*****************************************************************************/

static inline float
kdu_read_float(kdu_byte * &bp, kdu_byte *end)
{ /* [SYNOPSIS]
	   Reads a 4-byte single-precision floating point quantity from the
	   array identified by `bp'.  During the process, `bp' is advanced
	   4 bytes.  If this pushes it up to or past the `end' pointer, the
	   function throws an exception of type `kdu_byte *'.
	   [//]
	   The byte order is assumed to be big-endian.  If the local machine
	   architecture is little-endian, the input bytes are reversed.
	 [RETURNS]
	   The value of the floating point quantity recovered from the first
	   4 bytes of the buffer.
	 [ARG: bp]
	   Pointer to the first byte in the buffer from which the integer is
	   to be recovered.
	 [ARG: end]
	   Points immediately beyond the last valid entry in the buffer.
  */
	if ((end - bp) < 4)
		throw bp;
	float val;
	kdu_byte *val_p = (kdu_byte *)&val;
	int n, machine_uses_big_endian = 1;
	((kdu_byte *)&machine_uses_big_endian)[0] = 0;
	if (machine_uses_big_endian)
		for (n = 0; n < 4; n++)
			val_p[n] = *(bp++);
	else
		for (n = 3; n >= 0; n--)
			val_p[n] = *(bp++);
	return val;
}

/*****************************************************************************/
/* INLINE                       kdu_read_double                              */
/*****************************************************************************/

static inline double
kdu_read_double(kdu_byte * &bp, kdu_byte *end)
{ /* [SYNOPSIS]
	   Same as `kdu_read_float', but reads an 8-byte double-precision
	   quantity from the `bp' array.
  */
	if ((end - bp) < 8)
		throw bp;
	double val;
	kdu_byte *val_p = (kdu_byte *)&val;
	int n, machine_uses_big_endian = 1;
	((kdu_byte *)&machine_uses_big_endian)[0] = 0;
	if (machine_uses_big_endian)
		for (n = 0; n < 8; n++)
			val_p[n] = *(bp++);
	else
		for (n = 7; n >= 0; n--)
			val_p[n] = *(bp++);
	return val;
}

/*****************************************************************************/
/* INLINE                          ceil_ratio                                */
/*****************************************************************************/

static inline int
ceil_ratio(int num, int den)
{ /* [SYNOPSIS]
	   Returns the ceiling function of the ratio `num' / `den', where
	   the denominator is required to be strictly positive.
	 [RETURNS] Non-negative ratio.
	 [ARG: num] Non-negative numerator.
	 [ARG: den] Non-negative denomenator.
  */
	assert(den > 0);
	if (num <= 0)
		return -((-num) / den);
	else
		return 1 + ((num - 1) / den);
}

/*****************************************************************************/
/* INLINE                          floor_ratio                               */
/*****************************************************************************/

static inline int
floor_ratio(int num, int den)
{ /* [SYNOPSIS]
	   Returns the floor function of the ratio `num' / `den', where
	   the denominator is required to be strictly positive.
	 [RETURNS] Non-negative ratio.
	 [ARG: num] Non-negative numerator.
	 [ARG: den] Non-negative denomenator.
  */
	assert(den > 0);
	if (num < 0)
		return -(1 + ((-num - 1) / den));
	else
		return num / den;
}

/*****************************************************************************/
/* INLINE                       long_ceil_ratio                              */
/*****************************************************************************/

static inline int
long_ceil_ratio(kdu_long num, kdu_long den)
{ /* [SYNOPSIS]
	   Returns the ceiling function of the ratio `num' / `den', where
	   the denominator is required to be strictly positive.  The result
	   must fit within a signed 32-bit integer, even if the numerator or
	   denominator do not.
	 [RETURNS] Non-negative ratio.
	 [ARG: num] Non-negative numerator.
	 [ARG: den] Non-negative denomenator.
  */
	assert(den > 0);
	if (num <= 0)
	{
		num = -((-num) / den);
		assert((num >= (kdu_long)KDU_INT32_MIN));
	}
	else
	{
		num = 1 + ((num - 1) / den);
		assert((num <= (kdu_long)KDU_INT32_MAX));
	}
	return (int)num;
}

/*****************************************************************************/
/* INLINE                       long_floor_ratio                             */
/*****************************************************************************/

static inline int
long_floor_ratio(kdu_long num, kdu_long den)
{ /* [SYNOPSIS]
	   Returns the floor function of the ratio `num' / `den', where
	   the denominator is required to be strictly positive.  The result
	   must fit within a signed 32-bit integer, even if the numerator or
	   denominator do not.
	 [RETURNS] Non-negative ratio.
	 [ARG: num] Non-negative numerator.
	 [ARG: den] Non-negative denomenator.
  */
	assert(den > 0);
	if (num < 0)
	{
		num = -(1 + ((-num - 1) / den));
		assert((num >= (kdu_long)KDU_INT32_MIN));
	}
	else
	{
		num = num / den;
		assert((num <= (kdu_long)KDU_INT32_MAX));
	}
	return (int)num;
}

/*****************************************************************************/
/* INLINE                     kdu_hex_hex_decode                             */
/*****************************************************************************/

static inline const char *
kdu_hex_hex_decode(char src[], const char *src_lim = NULL)
{ /* [SYNOPSIS]
	   Performs in-place hex-hex decoding of URI's, overwriting the supplied
	   `src' string with its hex-hex decoded equivalent.  This function is
	   used by `jp2_data_references' as well as various client-server
	   components.  The decoding proceeds until a null-terminator is
	   encountered, or `src_lim' is reached, at which point the decoded
	   string is null-terminated and the function returns a pointer to `src'.
	   If `src_lim' does indeed point to a location within the `src' buffer,
	   it is possible that the null-terminator will be inserted at that
	   location so that *`src_lim'='\0' upon return -- this happens if no
	   hex-hex coded characters are encountered.
	 [RETURNS]
	   A pointer to the original `src' buffer.
  */
	char *dp, *result = src;
	for (dp = src; (*src != '\0') && (src != src_lim); dp++, src++)
	{
		int hex1, hex2;
		if ((*src == '%') &&
			(isdigit(hex1 = toupper(src[1])) ||
			((hex1 >= (int) 'A') && (hex1 <= (int) 'F'))) &&
				(isdigit(hex2 = toupper(src[2])) ||
			((hex2 >= (int) 'A') && (hex2 <= (int) 'F'))))
		{
			int decoded = 0;
			if ((hex1 >= (int) 'A') && (hex1 <= (int) 'F'))
				decoded += (hex1 - (int) 'A') + 10;
			else
				decoded += (hex1 - (int) '0');
			decoded <<= 4;
			if ((hex2 >= (int) 'A') && (hex2 <= (int) 'F'))
				decoded += (hex2 - (int) 'A') + 10;
			else
				decoded += (hex2 - (int) '0');
			*dp = (char)decoded;
			src += 2;
		}
		else
			*dp = *src;
	}
	*dp = '\0';
	return result;
}

/*****************************************************************************/
/* INLINE                     kdu_hex_hex_encode                             */
/*****************************************************************************/

static inline int
kdu_hex_hex_encode(const char *src, char dst[],
	const char *src_lim = NULL, const char *special_chars = NULL)
{ /* [SYNOPSIS]
	   This function is, in some sense, the dual of `kdu_hex_hex_decode'.
	   It is used by `jp2_data_references' as well as various client-server
	   components.  Since hex-hex encoding generally increases the length of
	   a string, it cannot be performed in place.  The function can be invoked
	   with a NULL `dst' argument to determine the number of characters in the
	   hex-hex encoded result, allowing you to allocate a buffer large enough
	   to supply in a second call to the function.
	 [RETURNS]
	   Number of characters which are (or would be) written to the
	   `dst' buffer, not including the terminating null character.
	 [ARG: dst]
	   If NULL, the function returns only the number of bytes that
	   it would write to a non-NULL `dst' buffer, not counting the null
	   terminator.
	 [ARG: src_lim]
	   The `src' string will be read up to but not including any address
	   passed by this argument.  This allows you to hex-hex encode an
	   initial prefix of the string, if desired.
	 [ARG: special_chars]
	   By default, the function hex-hex encodes all characters outside the
	   range 0x21 to 0x7E (note that 0x20 is the ASCII space character),
	   in addition to the non-URI-legal characters defined in RFC2396.
	   In some cases, ambiguity may remain unless additional reserved
	   characters are hex-hex encoded.  In particular, for JPIP
	   communications, the "?" and "&" characters have special meaning
	   in URL's and within POST'ed queries, so ambiguity could occur if
	   these characters are found within a target filename or a JP2 box
	   type code.  Special characters like this may be supplied by a
	   non-NULL `special_chars' argument.
  */
	const char *excluded_chars = "<>\"#%{}|\\^[]`";
	int num_octets = 0;
	for (; (src != src_lim) && (*src != '\0'); src++, num_octets++)
	{
		char ch = *src;
		if ((((kdu_uint32)ch) < 0x21) || (((kdu_uint32)ch) > 0x7E) ||
			(strchr(excluded_chars, ch) != NULL) ||
			((special_chars != NULL) && (strchr(special_chars, ch) != NULL)))
		{ // Need to hex-hex encode
			if (dst != NULL)
			{
				*(dst++) = '%';
				for (int d = 0; d < 2; d++, ch <<= 4)
				{
					int digit = (((int)ch) >> 4) & 0x0F;
					if (digit < 10)
						*(dst++) = (char)(digit + '0');
					else
						*(dst++) = (char)(digit - 10 + 'A');
				}
			}
			num_octets += 2; // 2 characters extra
		}
		else if (dst != NULL)
			*(dst++) = ch;
	}
	if (dst != NULL)
		*dst = '\0';
	return num_octets;
}

#endif // KDU_UTILS_H
