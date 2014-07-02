#if 0
//<copyright>
// 
// Copyright (c) 1994,95
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>
#endif

/*
 * File:     color.h
 *
 * Author:   Michael Pichler
 *
 * Created:  24 Jan 94
 *
 * Changed:  24 Jan 94
 *
 */



#ifndef ge3d_color_h
#define ge3d_color_h

typedef struct
{ float R, G, B;
} colorRGB;


/* initialisation/assignment.  c := (r, g, b)  */
#define initRGB(c, r, g, b) \
  (c).R = (r),  (c).G = (g),  (c).B = (b)


#endif
