/*
 * jpegexiforient.c
 *
 * This is a utility program to get and set the Exif Orientation Tag.
 * It can be used together with jpegtran in scripts for automatic
 * orientation correction of digital camera pictures.
 *
 * The Exif orientation value gives the orientation of the camera
 * relative to the scene when the image was captured.  The relation
 * of the '0th row' and '0th column' to visual position is shown as
 * below.
 *
 * Value | 0th Row     | 0th Column
 * ------+-------------+-----------
 *   1   | top         | left side
 *   2   | top         | right side
 *   3   | bottom      | right side
 *   4   | bottom      | left side
 *   5   | left side   | top
 *   6   | right side  | top
 *   7   | right side  | bottom
 *   8   | left side   | bottom
 *
 * For convenience, here is what the letter F would look like if it were
 * tagged correctly and displayed by a program that ignores the orientation
 * tag:
 *
 *   1        2       3      4         5            6           7          8
 *
 * 888888  888888      88  88      8888888888  88                  88  8888888888
 * 88          88      88  88      88  88      88  88          88  88      88  88
 * 8888      8888    8888  8888    88          8888888888  8888888888          88
 * 88          88      88  88
 * 88          88  888888  888888
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

static jmp_buf exif_jbuf;
static FILE * myfile;		/* My JPEG file */

static unsigned char exif_data[65536L];

/* Return next input byte, or EOF if no more */
#define NEXTBYTE()  getc(myfile)

/* Read one byte, testing for EOF */
static int read_1_byte (void)
{
  int c;

  if ((c = NEXTBYTE())== EOF)
    longjmp(exif_jbuf,1);
  return c;
}

/* Read 2 bytes, convert to unsigned int */
/* All 2-byte quantities in JPEG markers are MSB first */
static unsigned int read_2_bytes (void)
{
  int c1, c2;

  if ((c1 = NEXTBYTE()) == EOF || (c2 = NEXTBYTE())==EOF)
    longjmp(exif_jbuf,1);

  return (((unsigned int) c1) << 8) + ((unsigned int) c2);
}

/*
static const char * progname;
static void usage (FILE *out)
{
  fprintf(out, "jpegexiforient reads or writes the Exif Orientation Tag ");
  fprintf(out, "in a JPEG Exif file.\n");

  fprintf(out, "Usage: %s [switches] jpegfile\n", progname);

  fprintf(out, "Switches:\n");
  fprintf(out, "  --help     display this help and exit\n");
  fprintf(out, "  --version  output version information and exit\n");
  fprintf(out, "  -n         Do not output the trailing newline\n");
  fprintf(out, "  -1 .. -8   Set orientation value 1 .. 8\n");
}
*/

int getset_exiforient(int set_flag, char *fname)
{
  unsigned int length, i;
  int is_motorola; /* Flag for byte order */
  unsigned int offset, number_of_tags, tagnum;

  if (set_flag) {
    if ((myfile = fopen(fname, "rb+")) == NULL) {
      return -1;
    }
  } else {
    if ((myfile = fopen(fname, "rb")) == NULL) {
		return -1;
    }
  }
  if(setjmp(exif_jbuf))
	  return 0;

  /* Read File head, check for JPEG SOI + Exif APP1 */
  for (i = 0; i < 4; i++)
    exif_data[i] = (unsigned char) read_1_byte();
  if (exif_data[0] != 0xFF ||
      exif_data[1] != 0xD8 ||
      exif_data[2] != 0xFF ||
      exif_data[3] != 0xE1)
    return 0;

  /* Get the marker parameter length count */
  length = read_2_bytes();
  /* Length includes itself, so must be at least 2 */
  /* Following Exif data length must be at least 6 */
  if (length < 8)
    return 0;
  length -= 8;
  /* Read Exif head, check for "Exif" */
  for (i = 0; i < 6; i++)
    exif_data[i] = (unsigned char) read_1_byte();
  if (exif_data[0] != 0x45 ||
      exif_data[1] != 0x78 ||
      exif_data[2] != 0x69 ||
      exif_data[3] != 0x66 ||
      exif_data[4] != 0 ||
      exif_data[5] != 0)
    return 0;
  /* Read Exif body */
  for (i = 0; i < length; i++)
    exif_data[i] = (unsigned char) read_1_byte();

  if (length < 12) return 0; /* Length of an IFD entry */

  /* Discover byte order */
  if (exif_data[0] == 0x49 && exif_data[1] == 0x49)
    is_motorola = 0;
  else if (exif_data[0] == 0x4D && exif_data[1] == 0x4D)
    is_motorola = 1;
  else
    return 0;

  /* Check Tag Mark */
  if (is_motorola) {
    if (exif_data[2] != 0) return 0;
    if (exif_data[3] != 0x2A) return 0;
  } else {
    if (exif_data[3] != 0) return 0;
    if (exif_data[2] != 0x2A) return 0;
  }

  /* Get first IFD offset (offset to IFD0) */
  if (is_motorola) {
    if (exif_data[4] != 0) return 0;
    if (exif_data[5] != 0) return 0;
    offset = exif_data[6];
    offset <<= 8;
    offset += exif_data[7];
  } else {
    if (exif_data[7] != 0) return 0;
    if (exif_data[6] != 0) return 0;
    offset = exif_data[5];
    offset <<= 8;
    offset += exif_data[4];
  }
  if (offset > length - 2) return 0; /* check end of data segment */

  /* Get the number of directory entries contained in this IFD */
  if (is_motorola) {
    number_of_tags = exif_data[offset];
    number_of_tags <<= 8;
    number_of_tags += exif_data[offset+1];
  } else {
    number_of_tags = exif_data[offset+1];
    number_of_tags <<= 8;
    number_of_tags += exif_data[offset];
  }
  if (number_of_tags == 0) return 0;
  offset += 2;

  /* Search for Orientation Tag in IFD0 */
  for (;;) {
    if (offset > length - 12) return 0; /* check end of data segment */
    /* Get Tag number */
    if (is_motorola) {
      tagnum = exif_data[offset];
      tagnum <<= 8;
      tagnum += exif_data[offset+1];
    } else {
      tagnum = exif_data[offset+1];
      tagnum <<= 8;
      tagnum += exif_data[offset];
    }
    if (tagnum == 0x0112) break; /* found Orientation Tag */
    if (--number_of_tags == 0) return 0;
    offset += 12;
  }

  if (set_flag) {
    /* Set the Orientation value */
    if (is_motorola) {
      exif_data[offset+2] = 0; /* Format = unsigned short (2 octets) */
      exif_data[offset+3] = 3;
      exif_data[offset+4] = 0; /* Number Of Components = 1 */
      exif_data[offset+5] = 0;
      exif_data[offset+6] = 0;
      exif_data[offset+7] = 1;
      exif_data[offset+8] = 0;
      exif_data[offset+9] = (unsigned char)set_flag;
      exif_data[offset+10] = 0;
      exif_data[offset+11] = 0;
    } else {
      exif_data[offset+2] = 3; /* Format = unsigned short (2 octets) */
      exif_data[offset+3] = 0;
      exif_data[offset+4] = 1; /* Number Of Components = 1 */
      exif_data[offset+5] = 0;
      exif_data[offset+6] = 0;
      exif_data[offset+7] = 0;
      exif_data[offset+8] = (unsigned char)set_flag;
      exif_data[offset+9] = 0;
      exif_data[offset+10] = 0;
      exif_data[offset+11] = 0;
    }
    fseek(myfile, (4 + 2 + 6 + 2) + offset, SEEK_SET);
    fwrite(exif_data + 2 + offset, 1, 10, myfile);
  } else {
    /* Get the Orientation value */
    if (is_motorola) {
      if (exif_data[offset+8] != 0) return 0;
      set_flag = exif_data[offset+9];
    } else {
      if (exif_data[offset+9] != 0) return 0;
      set_flag = exif_data[offset+8];
    }
    if (set_flag > 8) return 0;
  }
  return set_flag;
}
