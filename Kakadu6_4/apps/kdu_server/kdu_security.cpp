/*****************************************************************************/
// File: kdu_security.cpp [scope = APPS/CLIENT-SERVER]
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
   Elementary encryption/decryption services for use in client-server
applications.
******************************************************************************/

#include "kdu_security.h"

/* ========================================================================= */
/*                               Fixed Tables                                */
/* ========================================================================= */

kdu_byte kd_powers_of_x_in_gf8[30] =
  { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f,
    0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4,
    0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91
  }; /* These are the first 30 powers of x (0x02) in GF8.  We don't
        actually need them all.  The entry with index 0 is x^0. */

kdu_byte kd_rijndael_sbox[256] =
  {
 99, 124, 119, 123, 242, 107, 111, 197,  48,   1, 103,  43, 254, 215, 171, 118,
202, 130, 201, 125, 250,  89,  71, 240, 173, 212, 162, 175, 156, 164, 114, 192,
183, 253, 147,  38,  54,  63, 247, 204,  52, 165, 229, 241, 113, 216,  49,  21,
  4, 199,  35, 195,  24, 150,   5, 154,   7,  18, 128, 226, 235,  39, 178, 117,
  9, 131,  44,  26,  27, 110,  90, 160,  82,  59, 214, 179,  41, 227,  47, 132,
 83, 209,   0, 237,  32, 252, 177,  91, 106, 203, 190,  57,  74,  76,  88, 207,
208, 239, 170, 251,  67,  77,  51, 133,  69, 249,   2, 127,  80,  60, 159, 168,
 81, 163,  64, 143, 146, 157,  56, 245, 188, 182, 218,  33,  16, 255, 243, 210,
205,  12,  19, 236,  95, 151,  68,  23, 196, 167, 126,  61, 100,  93,  25, 115,
 96, 129,  79, 220,  34,  42, 144, 136,  70, 238, 184,  20, 222,  94,  11, 219,
224,  50,  58,  10,  73,   6,  36,  92, 194, 211, 172,  98, 145, 149, 228, 121,
231, 200,  55, 109, 141, 213,  78, 169, 108,  86, 244, 234, 101, 122, 174,   8,
186, 120,  37,  46,  28, 166, 180, 198, 232, 221, 116,  31,  75, 189, 139, 138,
112,  62, 181, 102,  72,   3, 246,  14,  97,  53,  87, 185, 134, 193,  29, 158,
225, 248, 152,  17, 105, 217, 142, 148, 155,  30, 135, 233, 206,  85,  40, 223,
140, 161, 137,  13, 191, 230,  66, 104,  65, 153,  45,  15, 176,  84, 187,  22 
  };

kdu_byte kd_rijndael_inv_sbox[256] =
  {
 82,   9, 106, 213,  48,  54, 165,  56, 191,  64, 163, 158, 129, 243, 215, 251,
124, 227,  57, 130, 155,  47, 255, 135,  52, 142,  67,  68, 196, 222, 233, 203,
 84, 123, 148,  50, 166, 194,  35,  61, 238,  76, 149,  11,  66, 250, 195,  78,
  8,  46, 161, 102,  40, 217,  36, 178, 118,  91, 162,  73, 109, 139, 209,  37,
114, 248, 246, 100, 134, 104, 152,  22, 212, 164,  92, 204,  93, 101, 182, 146,
108, 112,  72,  80, 253, 237, 185, 218,  94,  21,  70,  87, 167, 141, 157, 132,
144, 216, 171,   0, 140, 188, 211,  10, 247, 228,  88,   5, 184, 179,  69,   6,
208,  44,  30, 143, 202,  63,  15,   2, 193, 175, 189,   3,   1,  19, 138, 107,
 58, 145,  17,  65,  79, 103, 220, 234, 151, 242, 207, 206, 240, 180, 230, 115,
150, 172, 116,  34, 231, 173,  53, 133, 226, 249,  55, 232,  28, 117, 223, 110,
 71, 241,  26, 113,  29,  41, 197, 137, 111, 183,  98,  14, 170,  24, 190,  27,
252,  86,  62,  75, 198, 210, 121,  32, 154, 219, 192, 254, 120, 205,  90, 244,
 31, 221, 168,  51, 136,   7, 199,  49, 177,  18,  16,  89,  39, 128, 236,  95,
 96,  81, 127, 169,  25, 181,  74,  13,  45, 229, 122, 159, 147, 201, 156, 239,
160, 224,  59,  77, 174,  42, 245, 176, 200, 235, 187,  60, 131,  83, 153,  97,
 23,  43,   4, 126, 186, 119, 214,  38, 225, 105,  20,  99,  85,  33,  12, 125
  };

/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                         mul2_gf8                                   */
/*****************************************************************************/

static inline kdu_byte
  mul2_gf8(kdu_byte val)
{
  if (val & 0x80)
    return (val<<1) ^ 0x1B;
  else
    return val<<1;
}


/* ========================================================================= */
/*                               kdu_rijndael                                */
/* ========================================================================= */

/*****************************************************************************/
/*                          kdu_rijndael::set_key                            */
/*****************************************************************************/

void
  kdu_rijndael::set_key(kdu_byte key[])
{
  int i, j;

  for (i=0; i < 16; i++)
    this->key[i] = key[i];
  key_set = true;

  // Generate the round-key now.
  int Nk = 4; // 4 columns for all our keys (4 bytes per column)
  int Nb = 4; // 4 columns for all our data arrays (4 bytes per column)
  for (i=0; i < Nk; i++)
    for (j=0; j < 4; j++)
      round_key[4*i+j] = key[4*i+j];
  for (i=Nk; i < (Nb*(KD_NUM_ROUNDS+1)); i++)
    {
      kdu_byte temp[4];
      for (j=0; j < 4; j++)
        temp[j] = round_key[4*(i-1)+j];
      if ((i % Nk) == 0)
        {
          kdu_byte tmp = temp[0];
          temp[0] = kd_rijndael_sbox[temp[1]];
          temp[1] = kd_rijndael_sbox[temp[2]];
          temp[2] = kd_rijndael_sbox[temp[3]];
          temp[3] = kd_rijndael_sbox[tmp];
          temp[0] ^= kd_powers_of_x_in_gf8[(i/Nk)-1];
        }
      for (j=0; j < 4; j++)
        round_key[4*i+j] = round_key[4*(i-Nk)+j] ^ temp[j];
    }
}

/*****************************************************************************/
/*                          kdu_rijndael::set_key                            */
/*****************************************************************************/

void
  kdu_rijndael::set_key(const char *string)
{
  int i, j, shift;
  const char *cp;

  for (i=0; i < 16; i++)
    key[i] = 0;
  for (shift=0, i=0; i < 79; i++) // Random number
    {
      for (j=i, cp=string; *cp != '\0'; cp++, j++, shift+=3)
        {
          if (shift >= 32)
            shift -= 32;
          j &= 3;
          kdu_uint32 val32 = key[4*j];
          val32 = (val32<<8) + key[4*j+1];
          val32 = (val32<<8) + key[4*j+2];
          val32 = (val32<<8) + key[4*j+3];
          val32 ^= ((kdu_uint32) *cp) << shift;
          key[4*j]   = (kdu_byte)(val32 >> 0 ); // Here, we effectively permute
          key[4*j+1] = (kdu_byte)(val32 >> 8 ); // the bytes, because we read
          key[4*j+2] = (kdu_byte)(val32 >> 16); // `val32' as a bigendian word
          key[4*j+3] = (kdu_byte)(val32 >> 24); // but write it as littlendian
        }
    }
  set_key(key);
}

/*****************************************************************************/
/*                          kdu_rijndael::encrypt                            */
/*****************************************************************************/

void
  kdu_rijndael::encrypt(kdu_byte data[])
{
  assert(key_set);
  kdu_byte *round_key_scan = round_key;
  int j;

  for (j=0; j < 16; j++)
    data[j] ^= *(round_key_scan++);
  for (int r=1; r < KD_NUM_ROUNDS; r++)
    { // Non-final rounds
      for (j=0; j < 16; j++)
        data[j] = kd_rijndael_sbox[data[j]];
      shift_rows_left(data);
      mix_columns(data);
      for (j=0; j < 16; j++)
        data[j] ^= *(round_key_scan++);
    }

  // Final round
  for (j=0; j < 16; j++)
    data[j] = kd_rijndael_sbox[data[j]];
  shift_rows_left(data);
  for (j=0; j < 16; j++)
    data[j] ^= *(round_key_scan++);
}

/*****************************************************************************/
/*                          kdu_rijndael::decrypt                            */
/*****************************************************************************/

void
  kdu_rijndael::decrypt(kdu_byte data[])
{
  assert(key_set);

  assert(0); // Still need to implement the decryption.
}

/*****************************************************************************/
/*                      kdu_rijndael::shift_rows_left                        */
/*****************************************************************************/

void
  kdu_rijndael::shift_rows_left(kdu_byte dat[])
{
  kdu_byte tmp;

  // Shift row 1
  tmp=dat[1]; dat[1]=dat[5]; dat[5]=dat[9]; dat[9]=dat[13]; dat[13]=tmp;

  // Shift row 2
  tmp=dat[2]; dat[2]=dat[10]; dat[10]=tmp;
  tmp=dat[6]; dat[6]=dat[14]; dat[14]=tmp;

  // Shift row 3
  tmp=dat[15]; dat[15]=dat[11]; dat[11]=dat[7]; dat[7]=dat[3]; dat[3]=tmp;
}

/*****************************************************************************/
/*                        kdu_rijndael::mix_columns                          */
/*****************************************************************************/

void
  kdu_rijndael::mix_columns(kdu_byte dat[])
{
  kdu_byte sum[4], in1, in2;
  for (int j=4; j > 0; j--, dat += 4)
    {
      // First column of matrix
      in1 = dat[0]; in2 = mul2_gf8(in1);
      sum[0]=in2; sum[1]=in1; sum[2]=in1; sum[3]=in1^in2;

      // Second column of matrix
      in1 = dat[1]; in2 = mul2_gf8(in1);
      sum[0]^=in1^in2; sum[1]^=in2; sum[2]^=in1; sum[3]^=in1;

      // Third column of matrix
      in1 = dat[2]; in2 = mul2_gf8(in1);
      sum[0]^=in1; sum[1]^=in1^in2; sum[2]^=in2; sum[3]^=in1;

      // Fourth column of matrix
      in1 = dat[3]; in2 = mul2_gf8(in1);
      sum[0]^=in1; sum[1]^=in1; sum[2]^=in1^in2; sum[3]^=in2;

      // Write back result
      dat[0] = sum[0];
      dat[1] = sum[1];
      dat[2] = sum[2];
      dat[3] = sum[3];
    }
}
