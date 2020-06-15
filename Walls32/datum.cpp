/*datum.c -- Definitions of Datum and Ellipsoid*/
#include <gpslib.h>

/****************************************************************************/
/*                                                                          */
/* ellipsoid: index into the gEllipsoid[] array, in which                   */
/*                                                                          */
/*     a:          ellipsoid semimajor axis                                 */
/*     invf:       inverse of the ellipsoid flattening f                    */
/*     dx, dy, dz: ellipsoid center with respect to WGS84 ellipsoid center  */
/*                                                                          */
/*     x axis is the prime meridian                                         */
/*     y axis is 90 degrees east longitude                                  */
/*     z axis is the axis of rotation of the ellipsoid                      */
/*                                                                          */
/* The following values for dx, dy and dz were extracted from the output of */
/* the GARMIN PCX5 program. The output also includes values for da and df,  */
/* the difference between the reference ellipsoid and the WGS84 ellipsoid   */
/* semi-major axis and flattening, respectively. These are replaced by the  */
/* data contained in the structure array gEllipsoid[], which was obtained   */
/* from the Defence Mapping Agency document number TR8350.2, "Department of */
/* Defense World Geodetic System 1984."                                     */
/*                                                                          */
/****************************************************************************/

/*CAUTION! This line appears in gpslib.h --*/
/*#define gps_NAD27_datum 15*/
/*Also, all names must have length <=19 chars*/

DATUM const gDatum[] = {
  { "Adindan",                5,   -162,    -12,    206 },
  //  { "Afgooye",               15,    -43,   -163,     45 },	
  //  { "Ain el Abd 1970",       14,   -150,   -251,     -2 },	
  //  { "Anna 1 Astro 1965",      2,   -491,    -22,    435 },	
	{ "Arc 1950",               5,   -143,    -90,   -294 },
	{ "Arc 1960",               5,   -160,     -8,   -300 },
	//  { "Ascension Island `58",  14,   -207,    107,     52 },	
	//  { "Astro B4 Sorol Atoll",  14,    114,   -116,   -333 },	
	//  { "Astro Beacon \"E\"",    14,    145,     75,   -272 },	
	//  { "Astro DOS 71/4",        14,   -320,    550,   -494 },
	//  { "Astronomic Stn `52",    14,    124,   -234,    -25 },	
	  { "Australian Geod `66",    2,   -133,    -48,    148 },
	  { "Australian Geod `84",    2,   -134,    -48,    149 },
	  //  { "Bellevue (IGN)",        14,   -127,   -769,    472 },	
	  //  { "Bermuda 1957",           4,    -73,    213,    296 },	
	  //  { "Bogota Observatory",    14,    307,    304,   -318 },
		{ "Camp Area Astro",       14,   -104,   -129,    239 },
		//  { "Campo Inchauspe",       14,   -148,    136,     90 },	
		//  { "Canton Astro 1966",     14,    298,   -304,   -375 },	
		  { "Cape",                   5,   -136,   -108,   -292 },
		  //  { "Cape Canaveral",         4,     -2,    150,    181 },	
		  //  { "Carthage",               5,   -263,      6,    431 },	
		  //  { "CH-1903",                3,    674,     15,    405 },	
		  //  { "Chatham 1971",          14,    175,    -38,    113 },	
		  //  { "Chua Astro",            14,   -134,    229,    -29 },	
		  //  { "Corrego Alegre",        14,   -206,    172,     -6 },	
		  //  { "Djakarta (Batavia)",     3,   -377,    681,    -50 },	
		  //  { "DOS 1968",              14,    230,   -199,   -752 },	
		  //  { "Easter Island 1967",    14,    211,    147,    111 },	
			{ "European 1950",         14,    -87,    -98,   -121 },
			{ "European 1979",         14,    -86,    -98,   -119 },
			//  { "Finland Hayford",       14,    -78,   -231,    -97 },	
			//  { "Gandajika Base",        14,   -133,   -321,     50 },	
			  { "Geodetic Datum `49",    14,     84,    -22,    209 },
			  //  { "Guam 1963",              4,   -100,   -248,    259 },	
			  //  { "GUX 1 Astro",           14,    252,   -209,   -751 },	
			  //  { "Hjorsey 1955",          14,    -73,     46,    -86 },	
				{ "Hong Kong 1963",        14,   -156,   -271,   -189 },
				{ "Hu-Tzu-Shan",			 14,   -634,   -549,   -201 },
				{ "Indian Bangladesh",      6,    289,    734,    257 },
				//  { "Indian Thailand",        6,    214,    836,    303 },	
				//  { "Ireland 1965",           1,    506,   -122,    611 },	
				//  { "ISTS 073 Astro `69",    14,    208,   -435,   -229 },	
				//  { "Johnston Island",       14,    191,    -77,   -204 },	
				//  { "Kandawala",              6,    -97,    787,     86 },	
				//  { "Kerguelen Island",      14,    145,   -187,    103 },	
				//  { "Kertau 1948",            7,    -11,    851,      5 },	
				//  { "L.C. 5 Astro",           4,     42,    124,    147 },	
				//  { "Liberia 1964",           5,    -90,     40,     88 },	
				//  { "Luzon Mindanao",         4,   -133,    -79,    -72 },	
				//  { "Luzon Philippines",      4,   -133,    -77,    -51 },	
				//  { "Mahe 1971",              5,     41,   -220,   -134 },	
				//  { "Marco Astro",           14,   -289,   -124,     60 },	
				//  { "Massawa",                3,    639,    405,     60 },	
				//  { "Merchich",               5,     31,    146,     47 },	
				//  { "Midway Astro 1961",     14,    912,    -58,   1227 },	
				//  { "Minna",                  5,    -92,    -93,    122 },	
				  { "NAD27 Alaska",           4,     -5,    135,    172 },
				  //  { "NAD27 Bahamas",          4,     -4,    154,    178 },	
					{ "NAD27 Canada",           4,    -10,    158,    187 },
					//  { "NAD27 Canal Zone",       4,      0,    125,    201 },	
					//  { "NAD27 Caribbean",        4,     -7,    152,    178 },	
					//  { "NAD27 Central",          4,      0,    125,    194 },	
					  { "NAD27 CONUS",            4,     -8,    160,    176 },
					  { "NAD27 Cuba",             4,     -9,    152,    178 },
					  //  { "NAD27 Greenland",        4,     11,    114,    195 },	
						{ "NAD27 Mexico",           4,    -12,    130,    190 },
						{ "NAD27 San Salvador",     4,      1,    140,    165 },
						{ "NAD83",                 11,      0,      0,      0 },
						//  { "Nahrwn Masirah Ilnd",    5,   -247,   -148,    369 },	
						//  { "Nahrwn Saudi Arbia",     5,   -231,   -196,    482 },	
						//  { "Nahrwn United Arab",     5,   -249,   -156,    381 },	
						//  { "Naparima BWI",          14,     -2,    374,    172 },	
						//  { "Observatorio 1966",     14,   -425,   -169,     81 },	
						//  { "Old Egyptian",          12,   -130,    110,    -13 },	
						//  { "Old Hawaiian",           4,     61,   -285,   -181 },	
						  { "Oman",                   5,   -346,     -1,    224 },
						  { "Ord Srvy Grt Britn",     0,    375,   -111,    431 },
						  //  { "Pico De Las Nieves",    14,   -307,    -92,    127 },	
						  //  { "Pitcairn Astro 1967",   14,    185,    165,     42 },	
							{ "Prov So Amrican `56",   14,   -288,    175,   -376 },
							//  { "Prov So Chilean `63",   14,     16,    196,     93 },	
							  { "Puerto Rico",            4,     11,     72,   -101 },
							  //  { "Qatar National",        14,   -128,   -283,     22 },	
							  //  { "Qornoq",                14,    164,    138,   -189 },	
							  //  { "Reunion",               14,     94,   -948,  -1262 },	
							  //  { "Rome 1940",             14,   -225,    -65,      9 },	
							  //  { "RT 90",                  3,    498,    -36,    568 },	
							  //  { "Santo (DOS)",           14,    170,     42,     84 },	
							  //  { "Sao Braz",              14,   -203,    141,     53 },	
							  //  { "Sapper Hill 1943",      14,   -355,     16,     74 },	
							  //  { "Schwarzeck",            21,    616,     97,   -251 },	
								{ "South American `69",    16,    -57,      1,    -41 },
								//  { "South Asia",             8,      7,    -10,    -26 },	
								//  { "Southeast Base",        14,   -499,   -249,    314 },	
								//  { "Southwest Base",        14,   -104,    167,    -38 },	
								//  { "Timbalai 1948",          6,   -689,    691,    -46 },	
								  { "Tokyo",                  3,   -128,    481,    664 },
								  //  { "Tristan Astro 1968",    14,   -632,    438,   -609 },	
								  //  { "Viti Levu 1916",         5,     51,    391,    -36 },	
								  //  { "Wake-Eniwetok `60",     13,    101,     52,    -39 },	
									{ "WGS 1972",                19,      0,      0,      5 },
									{ "WGS 1984",                20,      0,      0,      0 }
									//  { "Zanderij",              14,   -265,    120,   -358 }		
};

struct ELLIPSOID const gEllipsoid[] = {
  { /*Airy 1830",               */ 6377563.396, 299.3249646   },
  { /*Modified Airy",           */ 6377340.189, 299.3249646   },
  { /*Australian National",     */ 6378160.0,   298.25        },
  { /*Bessel 1841",             */ 6377397.155, 299.1528128   },
  { /*Clarke 1866",             */ 6378206.4,   294.9786982   },
  { /*Clarke 1880",             */ 6378249.145, 293.465       },
  { /*Everest (India 1830)",    */ 6377276.345, 300.8017      },
  { /*Everest (1948)",          */ 6377304.063, 300.8017      },
  { /*Modified Fischer 1960",   */ 6378155.0,   298.3         },
  { /*Everest (Pakistan)",      */ 6377309.613, 300.8017      },
  { /*Indonesian 1974",         */ 6378160.0,   298.247       },
  { /*GRS 80",                  */ 6378137.0,   298.257222101 },
  { /*Helmert 1906",            */ 6378200.0,   298.3         },
  { /*Hough 1960",              */ 6378270.0,   297.0         },
  { /*International 1924",      */ 6378388.0,   297.0         },
  { /*Krassovsky 1940",         */ 6378245.0,   298.3         },
  { /*South American 1969",     */ 6378160.0,   298.25        },
  { /*Everest (Malaysia 1969)", */ 6377295.664, 300.8017      },
  { /*Everest (Sabah Sarawak)", */ 6377298.556, 300.8017      },
  { /*WGS 72",                  */ 6378135.0,   298.26        },
  { /*WGS 84",                  */ 6378137.0,   298.257223563 },
  { /*Bessel 1841 (Namibia)",   */ 6377483.865, 299.1528128   },
  { /*Everest (India 1956)",    */ 6377301.243, 300.8017      }
};

/* define global variables */
int gps_nDatums = sizeof(gDatum) / sizeof(DATUM);
int gps_datum;

/****************************************************************************/
/* Calculate datum parameters.                                              */
/****************************************************************************/
void datumParams(double *a, double *es)
{
	/* flattening */
	double f = 1.0 / gEllipsoid[gDatum[gps_datum].ellipsoid].invf;

	*es = 2 * f - f * f;                                 /* eccentricity^2 */
	*a = gEllipsoid[gDatum[gps_datum].ellipsoid].a;   /* semimajor axis */
}
