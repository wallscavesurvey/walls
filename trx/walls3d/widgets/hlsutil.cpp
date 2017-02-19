/*
 * File :       hlsutil.C - parts of eden/kernel/gutil.C
 *
 * Purpose :    Implementation of graphic utility functions
 *
 * Created :    26 Jun 90    Keith Andrews, IICM
 *
 * Modified :   30 Jan 92    Keith Andrews, IICM
 *
 *              23 Apr 93    Michael Pichler - adapted to GL conventions (not CGI)
 *
 */



#include "hlsutil.h"

#pragma warning(disable: 4244)

inline float min (float a, float b)
{ return (a < b) ? a : b; }

inline float max (float a, float b)
{ return (a > b) ? a : b; }



void RGBtoHLS (float r, float g, float b, float& h, float& l, float& s)
// Input  : RGB values (r, g, b): floats in range [0, 1]
// Output : HLS values in h, l and s.
//          h in [0,360]; l, s in [0,1]; s == 0.0 -> h not defined.
{
  double mx ;
  double mi ;
  double temp ;

  mx = max( r, max( g, b ) ) ;
  mi = min( r, min( g, b ) ) ;

  l = ( mx + mi ) / 2 ;
  temp = mx - mi ;

  if ( temp == 0.0 )
    s = 0.0 ;                                      // grey scale
  else {
    s = ( l > 0.5 ) ? temp / ( 2 - mx - mi ) : temp / ( mx + mi ) ;

    double rd = ( mx - r ) / temp ;
    double gd = ( mx - g ) / temp ;
    double bd = ( mx - b ) / temp ;

    if ( mx == r )                          // magenta..yellow
      h = ( mi == g ) ? 1 + bd : 3 - gd ;
    else if ( mx == g )                     // yellow..cyan
      h = ( mi == b ) ? 3 + rd : 5 - bd ;
    else                                    // cyan..magenta
      h = ( mi == r ) ? 5 + gd : 1 - rd ;

    if ( h < 6 )
      h = h * 60.0 ;
    else
      h = 0.0 ;
  }
}



static double Hue_Result (double t1, double t2, double hue)
{
  if ( hue > 360.0 )
    hue -= 360.0 ;

  if ( hue < 60.0 )
    return t1 + ( t2 - t1 ) * hue / 60.0 ;
  else if ( hue < 180.0 )
    return t2 ;
  else if ( hue < 240.0 )
    return t1 + ( t2 - t1 ) * ( 240.0 - hue ) / 60.0 ;
  else
    return t1 ;
}


void HLStoRGB (float h, float l, float s, float& r, float& g, float& b)
// Input  : HLS values (h, l and s)
//          h in [0,360] ; l,s in [0,1] ; s == 0.0 -> h not defined.
// Output : RGB values (r, g, b).
{
  double t1 ;
  double t2 ;

  t2 = ( l > 0.5 ) ? l + s - l * s : l + l * s ;
  t1 = 2 * l - t2 ;

  if ( s > 0.0 ) {
    r = Hue_Result( t1, t2, h );
    g = Hue_Result( t1, t2, h + 240.0);
    b = Hue_Result( t1, t2, h + 120.0);
  }
  else  // h not defined
    r = g = b = l;
}
