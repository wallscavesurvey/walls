#ifndef LIBLOCO_H
#define LIBLOCO_H

enum e_loco_err {
	LCO_ErrNone, LCO_ErrNoCompression, LCO_ErrNoSOI, LCO_ErrInitParams,
	LCO_ErrNoEOI, LCO_ErrDecodeLen, LCO_ErrEncodeLen, LCO_ErrEOF, LCO_ErrInit
};

void lco_UnInit();

int lco_Encode(BYTE *dst, int srcRows, int srcCols);
int lco_Decode(BYTE *dst, int sizDst, int sizSrc);
#endif
