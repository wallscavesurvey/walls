/*
 * file:        hlsutil.h - conversion RGB <--> HLS
 *
 * author:      Michael Pichler
 *
 * created:     26 Nov 92
 *
 * changed:     23 Apr 93
 *
 */


// h in [0, 360], all other in [0, 1]

void RGBtoHLS (float r, float g, float b, float& h, float& l, float& s);

void HLStoRGB (float h, float l, float s, float& r, float& g, float& b);
