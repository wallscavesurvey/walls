#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timeb.h>

//=================================================
//  typedefs from the Garmin Protocol Specification
//=================================================
typedef unsigned char byte;
typedef unsigned long longword;
typedef int (FAR PASCAL *LPFN_GPS_CB)(LPCSTR msg);

extern LPFN_GPS_CB Walls_CB;

#define NO_ALTITUDE (float)1.0e25

#define GPS_ERR_CUTOFF	-2

enum e_errmsg {
	GPS_ERR_NONE = 0,
	GPS_ERR_ALLOC,
	GPS_ERR_SENDRECORD,
	GPS_ERR_NOWPEOF,
	GPS_ERR_NOTRKEOF,
	GPS_ERR_PROTOCOL,
	GPS_ERR_NORESPOND,
	GPS_ERR_PROTVIOL,
	GPS_ERR_PROTUNK,
	GPS_ERR_MAXNAKS,
	GPS_ERR_NUMRECS,
	GPS_ERR_BADWP,
	GPS_ERR_PORTOUT,
	GPS_ERR_ZONECOUNT,
	GPS_ERR_NODATA,
	GPS_ERR_SRVCREATE,
	GPS_ERR_PRJCREATE,
	GPS_ERR_CSVCREATE,
	GPS_ERR_CANCELLED
};

void push_waypoint(void);
void push_track(void);

//
// Serial port constants
//
#define GARMIN_BAUD_RATE	9600
#define UnixSecsMinusGpsSecs 631065600L

#define MAX_LENGTH	255	/* maximum length of Garmin binary message */
#define INTTYP short
#define WRDTYP WORD
void AsyncOut(unsigned char *msg, int count);
void AsyncSet(int wBaudRate, int wByteSize, int wStopBits, int wParity);
int AsyncInit(int port);
void AsyncStop(void);

extern void delay(long);

typedef INTTYP Symbol_Type;
typedef unsigned INTTYP word;

#define TIMEOUT 3 // seconds to wait for response from serial port
#define RADtoDEG  57.295779513082322
#define DEGtoRAD  0.017453292519943296
#define M_PI        3.14159265358979323846
#define Pi (double)M_PI
static const double Degree = Pi / 180.0;

int AsyncIn(void);
void AsyncSend(byte * message, int length);
int AsyncOutStat(void);
int AsyncInStat(void);

#define COM1 0
#define COM2 1
#define COM3 2
#define COM4 3
#define COM5 4
#define COM6 5
#define COM7 6
#define COM8 7
#define COM9 8
#define COM10 9
#define COM11 10
#define COM12 11
#define COM13 12
#define COM14 13
#define COM15 14

// default icon used in icon type conversion
#define default_Garmin_icon 18
#define default_G12_icon 0

struct RECORD {
	char type;			// Record Type W,T,R,D,P
	char ident[41];		// Record ident
	char ns;			//  N/S for latitude
	double la;			// latitude   in dd.ddddddd
	char ew;			//  E/W for longitude
	double lo;			// longitude  in dd.ddddddd
	INTTYP monthn;			// month number of record
	char datetime[25];	// date/time string as read
	INTTYP dayofmonth;		// day of month 1-31
	INTTYP year;			// 4 digit year of record
	INTTYP ht, mt, st;		// time of record
	INTTYP dla, mla;		// Deg & Min for Lat
	INTTYP dlo, mlo;		// Deg & Min for Long
	INTTYP tnthsla, tnthslo;	// tenths of a minute*10000
	double minla, minlo;	// Lat/Long minutes in decimal
	double secla, seclo;	// Lat/Long seconds in decimal
	float ProxDist;		// Proximity Distance
	short icon_smbl; 	// Garmin smbl coverted to SA
	short icon_dspl;	// Display style.  Consistant with
						//  later garminunits, i.e.
						//
						//  1 = Symbol by itself
						//  2 = Symbol with waypoint name
						//  3 = Symbol with comment

	byte color;         //  Waypoint symbol Color for Color units:
						//
						//   12CX : 0 - Default waypoint color
						//          1 - Red
						//          2 - Green
						//          3 - Blue
//
// garmin additions from beta.  They are used by various Pilot units
//
	byte gclass;
	byte status;		//for Lowrance/Eagle waypoints
	byte subclass[18];  //This was originally 13!! (DMcK)
	INTTYP alt;
	INTTYP dpth;
	char state[3];
	char cc[3];			// garmin
	char name[31];		// garmin
	char comment[51];   // Comment field 20 max for 45
	char facility[31];  // facility name
	char city[25];      // garmin (24)
	char addr[51];      // address
	char cross_road[51];// intersecting road label
	float altitude;
};

extern struct RECORD record;

// Garmin message type characters
#define ACK			0x06	// last record received was ok
#define ALM 		0x1f	// Almanac record
#define EOFREC  	0x0c	// EOF leadin for transters
#define GPSpos		0x11	// GPS Position record
#define PROX		0x13	// GPS Proximity Position record
#define MYID		0xff	// Version/ID record
#define NAK			0x15	// last record received was bad
#define NREC		0x1b	// number of records to receive
#define RTE_NAM 	0x1d	// Route name record
#define RTE_WPT 	0x1e	// Route waypoint record
#define TIMEREC 	0x0e	// UTC Time record
#define TRK 		0x22	// Track record
#define WPT 		0x23	// Waypoint record
#define Volts 		0x28	// GPS Voltages record
#define SENDID 		0xfe	// Send ID

/* Defines for Com Port Paramaters, the second paramater to AsyncSet() */
#define BITS_8      0x03
#define BITS_7      0x02
#define STOP_1      0x00
#define STOP_2      0x04
#define EVEN_PARITY 0x18
#define ODD_PARITY  0x08
#define NO_PARITY   0x00

/* Defines for AsyncHand() */
#define	DTR			0x01
#define	RTS			0x02
#define	USER		0x04
#define	LOOPBACK	0x10

/* Defines for AsyncStat() */
#define	D_CTS		0x0100
#define	D_DSR		0x0200
#define	D_RI		0x0400
#define	D_DCD		0x0800
#define	CTS			0x1000
#define	DSR			0x2000
#define	RI			0x4000
#define	DCD			0x8000
#define	PARITY		0x0004
#define	THREMPTY	0x0020
#define	BREAKDET	0x1000

//=======================================
//
// Structures for waypoint records
//
//=======================================

#pragma pack(1)
typedef struct
{
	INTTYP product_ID;
	INTTYP software_version;
	byte a[1];  // fake the below by....
// char product_description[]; null-terminated string
// ... zero or more additional null-terminated strings
} Product_Data_Type;

typedef struct
{
	long lat; /* latitude in semicircles */
	long lon; /* longitude in semicircles */
} Semicircle_Type;

//
//  7.5.1. D100_Wpt_Type
typedef struct
{
	char ident[6];			// identifier
	Semicircle_Type posn;	// position
	unsigned long unused;	// should be set to zero
	char cmnt[40];			// comment
} D100_Wpt_Type;

//  7.5.2. D101_Wpt_Type
//
// Example products: GPSMAP 210 and GPSMAP 220 (both prior to version 4.00).
typedef struct
{
	char ident[6];			// identifier
	Semicircle_Type posn;	// position
	longword unused;		// should be set to zero
	char cmnt[40];			// comment
	float dst;				// proximity distance (meters)
	byte smbl;				// symbol id
} D101_Wpt_Type;

//The enumerated values for the "smbl" member of the D101_Wpt_Type
//   are the same as those for Symbol_Type (see Section 7.4.9 on page 28).
//   However, since the "smbl" member of the D101_Wpt_Type is only 8-bits
//   (instead of 16-bits), all Symbol_Type values whose upper byte is
//   non-zero are unallowed in the D101_Wpt_Type.
//7.5.3. D102_Wpt_Type
//
// Example products: GPSMAP 175, GPSMAP 210 and GPSMAP 220.
typedef struct
{
	char ident[6];			// identifier
	Semicircle_Type posn;	// position
	longword unused;		// should be set to zero
	char cmnt[40];			// comment
	float dst;				// proximity distance (meters)
	Symbol_Type smbl;		// symbol id
} D102_Wpt_Type;

//7.5.4. D103_Wpt_Type
// Example products: GPS 12, GPS 12 XL, GPS 48 and GPS II Plus.
typedef struct
{
	char ident[6];			// identifier
	Semicircle_Type posn;	// position
	longword unused;		// should be set to zero
	char cmnt[40];			// comment
	byte smbl;				// symbol id
	byte dspl;				// display option
} D103_Wpt_Type;
// The enumerated values for the "dspl" member of the D103_Wpt_Type
//    are shown below:
//enum
//{
//	dspl_name = 0, /* Display symbol with waypoint name */
//	dspl_none = 1, /* Display symbol by itself */
//	dspl_cmnt = 2 /* Display symbol with comment */
//};

//7.5.5. D104_Wpt_Type
// Example products: GPS III.
typedef struct
{
	char ident[6];			// identifier
	Semicircle_Type posn;	// position
	longword unused;		// should be set to zero
	char cmnt[40];			// comment
	float dst;				// proximity distance (meters)
	Symbol_Type smbl;		// symbol id
	byte dspl;				// display option
} D104_Wpt_Type;
// The enumerated values for the "dspl" member of the 
//   D104_Wpt_Type are shown below:
//enum
//{
//	dspl_smbl_only = 1, /* Display symbol by itself */
//	dspl_smbl_name = 3, /* Display symbol with waypoint name */
//	dspl_smbl_cmnt = 5, /* Display symbol with comment */
//};

// 7.5.6. D105_Wpt_Type
typedef struct
{
	Semicircle_Type posn;	// position
	Symbol_Type smbl;		// symbol id
	char wpt_ident[1]; 		// null-terminated string -- length isn't
							//   really specified, I added 1 for a place
							//   holder for the decode of D105 waypoints
} D105_Wpt_Type;

// 7.5.7. D106_Wpt_Type
//  Example products: StreetPilot (route waypoints).
typedef struct
{
	byte gclass;				// class
	byte subclass[13];		// subclass
	Semicircle_Type posn;	// position
	Symbol_Type smbl;		// symbol id
	char wpt_ident[1];  	// null-terminated string -- length isn't
							//   really specified, I added 10 for a place
							//   holder for the decode of D106 waypoints
/* char lnk_ident[]; null-terminated string */
} D106_Wpt_Type;
// The enumerated values for the "class" member of the D106_Wpt_Type
//   are as follows:
//	Zero:     indicates a user waypoint ("subclass" is ignored).
//	Non-zero: indicates a non-user waypoint ("subclass" must be valid).
//
//  For non-user waypoints (such as a city in the GPS map database), the
//	GPS will provide a non-zero value in the "class" member, and the
//	"subclass" member will contain valid data to further identify
//	the non-user waypoint.  If the Host wishes to transfer this
//	waypoint back to the GPS (as part of a route), the Host must
//	leave the "class" and "subclass" members unmodified.
//
//	For user waypoints, the Host must ensure that the "class" member
//	is zero, but the "subclass" member will be ignored and should be
//	set to zero.
//	
//	The "lnk_ident" member provides a string that indicates the name
//	of the path from this waypoint to the next waypoint in the
//	route.
//	
//	For example, "HIGHWAY 101" might be placed in "lnk_ident" to
//	show that the path from this waypoint to the next waypoint is
//	along Highway 101.  The "lnk_ident" string may be empty (i.e.,
//	no characters other than the null terminator), which indicates
//	that no particular path is specified.
//

typedef struct
{
	char 			ident[6];	// identifier
	Semicircle_Type posn;		// position
	longword 		unused;		// should be set to zero
	char 			cmnt[40];	// comment
	byte 			smbl;		// symbol id
	byte 			dspl;		// display option
	float 			dst;		// proximity distance (meters)
	byte 			color;		// waypoint color
} D107_Wpt_Type;
//  The enumerated values for the "smbl" member of the D107_Wpt_Type are the
//  same as the the "smbl" member of the D103_Wpt_Type.
//
//  The enumerated values for the "dspl" member of the D107_Wpt_Type are
//  shown below are the same as the the "dspl" member of the D103_Wpt_Type.
//  
//  The enumerated values for the "color" member of the D107_Wpt_Type are
//  shown below:
//  
//enum
//{
//	clr_default = 0,	// Default waypoint color
//	clr_red = 1,		// Red
//	clr_green = 2,		// Green
//	clr_blue = 3		// Blue
//};

// D108_Wpt_Type
//  Example products: eMap
typedef struct 														// size
{
	byte			wpt_class; 		// class (see below) 				1
	byte			color; 			// color (see below) 				1 
	byte			dspl; 			// display options (see below) 		1
	byte			attr; 			// attributes (see below) 			1
	Symbol_Type		smbl; 			// waypoint symbol 					2
	byte			subclass[18]; 	// subclass 						18
	Semicircle_Type	posn; 			// 32 bit semicircle 				8
	float 			alt; 			// altitude in meters 				4
	float 			dpth; 			// depth in meters 					4
	float 			dist; 			// proximity distance in meters		4
	char 			state[2]; 		// state 							2
	char 			cc[2]; 			// country code 					2
	char 			ident[51]; 		//	variable length string 			1-51
// char 			comment[]; 			waypoint user comment 			1-51
// char 			facility[]; 		facility name 					1-31
// char 			city[]; 			city name 						1-25
// char 			addr[]; 			address number 					1-51
// char 			cross_road[]; 		intersecting road label 		1-51
} D108_Wpt_Type;

//enum
//{
//	USER_WPT =      0x00, // User waypoint
//	AVTN_APT_WPT =  0x40, // Aviation Airport waypoint
//	AVTN_INT_WPT =  0x41, // Aviation Intersection waypoint
//	AVTN_NDB_WPT =  0x42, // Aviation NDB waypoint
//	AVTN_VOR_WPT =  0x43, // Aviation VOR waypoint
//	AVTN_ARWY_WPT = 0x44, // Aviation Airport Runway waypoint
//	AVTN_AINT_WPT = 0x45, // Aviation Airport Intersection
//	AVTN_ANDB_WPT = 0x46, // Aviation Airport NDB waypoint
//	MAP_PNT_WPT =   0x80, // Map Point waypoint
//	MAP_AREA_WPT =  0x81, // Map Area waypoint
//	MAP_INT_WPT =   0x82, // Map Intersection waypoint
//	MAP_ADRS_WPT =  0x83, // Map Address waypoint
//	MAP_LABEL_WPT = 0x84, // Map Label Waypoint
//	MAP_LINE_WPT =  0x85, // Map Line Waypoint
//};

//The "color" member can be one of the following values:
//enum {	Black, 
//		Dark_Red, 
//		Dark_Green, 
//		Dark_Yellow,
//		Dark_Blue, 
//		Dark_Magenta, 
//		Dark_Cyan, 
//		Light_Gray,
//		Dark_Gray, 
//		Red, 
//		Green, 
//		Yellow,
//		Blue, 
//		Magenta, 
//		Cyan, 
//		White,
//		Default_Color = 0xFF
//};

// The enumerated values for the "dspl" member of the D108_Wpt_Type are the
//     same as the the "dspl" member of the D103_Wpt_Type.

// The "attr" member should be set to a value of 0x60.

// The "subclass" member of the D108_Wpt_Type is used for map waypoints
//   only, and should be set to 0x0000 0x00000000 0xFFFFFFFF 0xFFFFFFFF
//   0xFFFFFFFF for other classes of waypoints.

// The "alt" and "dpth" members may or may not be supported on a given
//    unit.  A value of 1.0e25 in either of these fields indicates that this
//    parameter is not supported or is unknown for this waypoint.

//-----------------------------------------
//D109 Waypoint Type (GPS V)

typedef struct							/*                                 size */
{
	byte            dtyp;           /* data packet type (0x01 for D109)1    */
	byte            wpt_class;      /* class                           1    */
	byte            dspl_color;     /* display & color (see below)     1    */
	byte            attr;           /* attributes (0x70 for D109)      1    */
	Symbol_Type     smbl;           /* waypoint symbol                 2    */
	byte            subclass[18];   /* subclass                        18   */
	Semicircle_Type posn;           /* 32 bit semicircle               8    */
	float           alt;            /* altitude in meters              4    */
	float           dpth;           /* depth in meters                 4    */
	float           dist;           /* proximity distance in meters    4    */
	char            state[2];       /* state                           2    */
	char            cc[2];          /* country code                    2    */
	longword        ete;            /* outbound link ete in seconds    4    */
	char            ident[51];      /* variable length string          1-51 */
/*  char            comment[];         waypoint user comment           1-51 */
/*  char            facility[];        facility name                   1-31 */
/*  char            city[];            city name                       1-25 */
/*  char            addr[];            address number                  1-51 */
/*  char            cross_road[];      intersecting road label         1-51 */
} D109_Wpt_Type;

/*All fields are defined the same as D108 except as noted below.

dtyp - Data packet type, must be 0x01 for D109.

dsp_color - The 'dspl_color' member contains three fields; bits 0-4 specify
the color, bits 5-6 specify the waypoint display attribute and bit 7 is unused
and must be 0. Color values are as specified for D108 except that the default
value is 0x1f. Display attribute values are as specified for D108.

attr - Attribute. Must be 0x70 for D109.

ete - Estimated time en route in seconds to next waypoint. Default value is
0xffffffff.
*/

typedef struct /* size */
{
	byte dtyp;				/* data packet type (0x01 for D110)1 */
	byte wpt_class;			/* class 1 */
	byte dspl_color;		/* display & color (see below) 1 */
	byte attr;				/* attributes (0x80 for D110) 1 */
	Symbol_Type smbl;		/* waypoint symbol 2 */
	byte subclass[18];		/* subclass 18 */
	Semicircle_Type posn;	/* 32 bit semicircle 8 */
	float alt;				/* altitude in meters 4 */
	float dpth;				/* depth in meters 4 */
	float dist;				/* proximity distance in meters 4 */
	char state[2];			/* state 2 */
	char cc[2];				/* country code 2 */
	longword ete;			/* outbound link ete in seconds 4 */
	float temp;				/* temperature 4 */
	longword time;			/* timestamp 4 */
	int wpt_cat;			/* category membership 2 */
	char ident[];		/* variable length string 1-51 */
	/* char comment[]; waypoint user comment 1-51 */
	/* char facility[]; facility name 1-31 */
	/* char city[]; city name 1-25 */
	/* char addr[]; address number 1-51 */
	/* char cross_road[]; intersecting road label 1-51 */
} D110_Wpt_Type;

//7.5.8. D150_Wpt_Type
//  Example products: GPS 150, GPS 155, GNC 250 and GNC 300.
typedef struct
{
	char ident[6];			// identifier
	char cc[2];				// country code
	byte gclass;				// class
	Semicircle_Type posn;	// position
	INTTYP alt;				// altitude (meters)
	char city[24];			// city
	char state[2];			// state
	char name[30];			// facility name
	char cmnt[40];			// comment
} D150_Wpt_Type;
//	The enumerated values for the "class" member of the D150_Wpt_Type 
//     are shown below:
//enum
//{
//	apt_wpt_class = 0, /* airport waypoint class */
//	int_wpt_class = 1, /* intersection waypoint class */
//	ndb_wpt_class = 2, /* NDB waypoint class */
//	vor_wpt_class = 3, /* VOR waypoint class */
//	usr_wpt_class = 4, /* user defined waypoint class */
//	rwy_wpt_class = 5, /* airport runway threshold waypoint class */
//	aint_wpt_class = 6 /* airport intersection waypoint class */
//};
//	The "city," "state," "name," and "cc"members are invalid when the
//   "class" member is equal to usr_wpt_class. The "alt" member is valid
//   only when the "class" member is equal to apt_wpt_class.

//7.5.9. D151_Wpt_Type
// Example products: GPS 55 AVD, GPS 89.
typedef struct
{
	char ident[6];			// identifier
	Semicircle_Type posn;	// position */
	longword unused;		// should be set to zero */
	char cmnt[40];			// comment */
	float dst;				// proximity distance (meters) */
	char name[30];			// facility name */
	char city[24];			// city */
	char state[2];			// state */
	INTTYP alt;				// altitude (meters) */
	char cc[2];				// country code */
	char unused2;			// should be set to zero */
	byte gclass;				// class */
} D151_Wpt_Type;
//  The enumerated values for the "class" member of the D151_Wpt_Type
//    are shown below:
//enum
//{
//	apt_wpt_class = 0, /* airport waypoint class */
//	vor_wpt_class = 1, /* VOR waypoint class */
//	usr_wpt_class = 2 /* user defined waypoint class */
//};
// 	The "city," "state," "name," and "cc"members are invalid when
//  the "class" member is equal to usr_wpt_class.  The "alt" member is valid
//  only when the "class" member is equal to apt_wpt_class.

//7.5.10. D152_Wpt_Type
//  Example products: GPS 90, GPS 95 AVD, GPS 95 XL and GPSCOM 190.
typedef struct
{
	char ident[6];			// identifier
	Semicircle_Type posn;	// position
	longword unused;		// should be set to zero
	char cmnt[40];			// comment
	float dst;				// proximity distance (meters)
	char name[30];			// facility name
	char city[24];			// city
	char state[2];			// state
	INTTYP alt;				// altitude (meters)
	char cc[2];				// country code
	char unused2;			// should be set to zero
	byte gclass;				// class
} D152_Wpt_Type;
//	The enumerated values for the "class" member of the D152_Wpt_Type
//    are shown below:
//enum
//{
//	apt_wpt_class = 0, /* airport waypoint class */
//	int_wpt_class = 1, /* intersection waypoint class */
//	ndb_wpt_class = 2, /* NDB waypoint class */
//	vor_wpt_class = 3, /* VOR waypoint class */
//	usr_wpt_class = 4 /* user defined waypoint class */
//};
// The "city," "state," "name," and "cc"members are invalid when the
// "class" member is equal to usr_wpt_class.  The "alt" member is valid
// only when the "class" member is equal to apt_wpt_class.

//7.5.11. D154_Wpt_Type
//Example products: GPSMAP 195.
typedef struct
{
	char ident[6];			// identifier 
	Semicircle_Type posn;	// position 
	longword unused;		// should be set to zero 
	char cmnt[40];			// comment 
	float dst;				// proximity distance (meters) 
	char name[30];			// facility name 
	char city[24];			// city 
	char state[2];			// state 
	INTTYP alt;				// altitude (meters) 
	char cc[2];				// country code 
	char unused2;			// should be set to zero 
	byte gclass;				// class 
	Symbol_Type smbl;		// symbol id
} D154_Wpt_Type;
// The enumerated values for the "class" member of the D154_Wpt_Type are
// shown below:
//enum
//{
//	apt_wpt_class = 0, /* airport waypoint class */
//	int_wpt_class = 1, /* intersection waypoint class */
//	ndb_wpt_class = 2, /* NDB waypoint class */
//	vor_wpt_class = 3, /* VOR waypoint class */
//	usr_wpt_class = 4, /* user defined waypoint class */
//	rwy_wpt_class = 5, /* airport runway threshold waypoint class */
//	aint_wpt_class = 6, /* airport intersection waypoint class */
//	andb_wpt_class = 7, /* airport NDB waypoint class */
//	sym_wpt_class = 8 /* user defined symbol-only waypoint class */
//};
// The "city," "state," "name," and "cc"members are invalid when the
// "class" member is equal to usr_wpt_class or sym_wpt_class.  The "alt"
// member is valid only when the "class" member is equal to apt_wpt_class.

//7.5.12. D155_Wpt_Type
// Example products: GPS III Pilot.
typedef struct
{
	char ident[6];			// identifier 
	Semicircle_Type posn;	// position 
	longword unused;		// should be set to zero 
	char cmnt[40];			// comment 
	float dst;				// proximity distance (meters) 
	char name[30];			// facility name 
	char city[24];			// city 
	char state[2];			// state 
	INTTYP alt;				// altitude (meters) 
	char cc[2];				// country code 
	char unused2;			// should be set to zero 
	byte gclass;				// class 
	Symbol_Type smbl;		// symbol id 
	byte dspl;				// display option 
} D155_Wpt_Type;
// The enumerated values for the "dspl" member of the D155_Wpt_Type are
// shown below:
//enum
//{
//	dspl_smbl_only = 1, /* Display symbol by itself */
//	dspl_smbl_name = 3, /* Display symbol with waypoint name */
//	dspl_smbl_cmnt = 5, /* Display symbol with comment */
//};
// The enumerated values for the "class" member of the D155_Wpt_Type are
// shown below:
//enum
//{
//	apt_wpt_class = 0, /* airport waypoint class */
//	int_wpt_class = 1, /* intersection waypoint class */
//	ndb_wpt_class = 2, /* NDB waypoint class */
//	vor_wpt_class = 3, /* VOR waypoint class */
//	usr_wpt_class = 4 /* user defined waypoint class */
//};
// The "city," "state," "name," and "cc"members are invalid when the
// "class" member is equal to usr_wpt_class.  The "alt" member is valid
// only when the "class" member is equal to apt_wpt_class.

//=====================================================================
typedef struct
{
	double lat; // latitude in radians
	double lon; // longitude in radians
} Radian_Type;

//7.5.13. D200_Rte_Hdr_Type
// Example products: GPS 55 and GPS 55 AVD.
typedef byte D200_Rte_Hdr_Type; // route number

//7.5.14. D201_Rte_Hdr_Type
// Example products: all products unless otherwise noted.
typedef struct
{
	byte nmbr; 			// route number
	char cmnt[20]; 		// comment
} D201_Rte_Hdr_Type;

//7.5.15. D202_Rte_Hdr_Type
// Example products: StreetPilot.
typedef struct
{
	char rte_ident[1];
	/* char rte_ident[]; null-terminated string */
} D202_Rte_Hdr_Type;

//7.5.16. D300_Trk_Point_Type
// Example products: all products unless otherwise noted.
typedef struct
{
	Semicircle_Type posn;	// position
	longword time;			// time
	byte new_trk;		// new track segment?
} D300_Trk_Point_Type;
//The "time" member provides a timestamp for the track log point.  This
// time is expressed as the number of seconds since 12:00 AM on January 1st,
// 1990.  When true, the "new_trk" member indicates that the track log
// point marks the beginning of a new track log segment.
typedef struct
{
	Semicircle_Type posn;   // position
	longword time;          // time */
	float alt;              // altitude in meters
	float dpth;             // depth in meters
	byte new_trk;        // new track segment?
} D301_Trk_Point_Type;
//
// The "time" member provides a timestamp for the track log point.  This
// time is expressed as the number of seconds since UTC 12:00 AM on
// December 31 st , 1989.
//
// The `alt' and `dpth' members may or may not be supported on a given
// unit.  A value of 1.0e25 in either of these fields indicates that this
// parameter is not supported or is unknown for this track point.
//
// When true, the "new_trk" member indicates that the track log point marks
// the beginning of a new track log segment.
//

typedef struct
{
	Semicircle_Type	posn; /* position */
	longword		time; /* time */
	float			alt; /* altitude in meters */
	float			dpth; /* depth in meters */
	float			temp; /* temp in degrees C */
	byte			new_trk; /* new track segment? */
} D302_Trk_Point_Type;

typedef struct
{
	byte dspl;           // display on the map?
	byte color;             // color (same as D108)
	char trk_ident[51];     // beginning of a string of *up to* 51 chars including the end NULL, but
							//  the structure length is sizeof(boolean)+sizeof(byte)+strlen(trk_ident)+1
/* char trk_ident[];        // null-terminated string */
} D310_Trk_Hdr_Type;
// The ` trk_ident' member has a maximum length of 51 characters
//  including the terminating NULL.

//7.5.17. D400_Prx_Wpt_Type
// Example products: GPS 55, GPS 55 AVD, GPS 75 and GPS 95 AVD.
typedef struct
{
	D100_Wpt_Type wpt;	// waypoint
	float dst;			// proximity distance (meters)
} D400_Prx_Wpt_Type;

//7.5.18. D403_Prx_Wpt_Type
// Example products: GPS 12, GPS 12 XL and GPS 48.
typedef struct
{
	D103_Wpt_Type wpt;	// waypoint
	float dst;			// proximity distance (meters) 
} D403_Prx_Wpt_Type;

//7.5.19. D450_Prx_Wpt_Type
// Example products: GPS 150, GPS 155, GNC 250 and GNC 300.
typedef struct
{
	INTTYP idx;			// proximity index
	D150_Wpt_Type wpt;	// waypoint
	float dst;			// proximity distance (meters)
} D450_Prx_Wpt_Type;

//7.5.20. D500_Almanac_Type
// Example products: GPS 38, GPS 40, GPS 45, GPS 55, GPS 75, GPS 95 and GPS II.
typedef struct
{
	INTTYP wn;				// week number (weeks) 
	float toa;			// almanac data reference time (s) 
	float af0;			// clock correction coefficient (s) 
	float af1;			// clock correction coefficient (s/s) 
	float e;			// eccentricity (-) 
	float sqrta;		// square root of semi-major axis (a) (m**1/2) 
	float m0;			// mean anomaly at reference time (r) 
	float w;			// argument of perigee (r) 
	float omg0;			// right ascension (r) 
	float odot;			// rate of right ascension (r/s) 
	float i;			// inclination angle (r) 
} D500_Almanac_Type;

//7.5.21. D501_Almanac_Type
// Example products: GPS 12, GPS 12 XL, GPS 48, GPS II Plus and GPS III.
typedef struct
{
	INTTYP wn;				// week number (weeks) 
	float toa;			// almanac data reference time (s) 
	float af0;			// clock correction coefficient (s) 
	float af1;			// clock correction coefficient (s/s) 
	float e;			// eccentricity (-) 
	float sqrta;		// square root of semi-major axis (a) (m**1/2) 
	float m0;			// mean anomaly at reference time (r) 
	float w;			// argument of perigee (r) 
	float omg0;			// right ascension (r) 
	float odot;			// rate of right ascension (r/s) 
	float i;			// inclination angle (r) 
	byte hlth;			// almanac health bits 17:24 (coded) 
} D501_Almanac_Type;

typedef char svid_type;
//7.5.22. D550_Almanac_Type
// Example products: GPS 150, GPS 155, GNC 250 and GNC 300.
typedef struct
{
	svid_type svid;		// satellite id 
	INTTYP wn;				// week number (weeks) 
	float toa;			// almanac data reference time (s) 
	float af0;			// clock correction coefficient (s) 
	float af1;			// clock correction coefficient (s/s) 
	float e;			// eccentricity (-) 
	float sqrta;		// square root of semi-major axis (a) (m**1/2) 
	float m0;			// mean anomaly at reference time (r) 
	float w;			// argument of perigee (r) 
	float omg0;			// right ascension (r) 
	float odot;			// rate of right ascension (r/s) 
	float i;			// inclination angle (r) 
} D550_Almanac_Type;
// The "svid" member identifies a satellite in the GPS constellation as
// follows:  PRN-01 through PRN-32 are indicated by "svid" equal to 0
// through 31, respectively.

//7.5.23. D551_Almanac_Type
// Example products: GPS 150 XL, GPS 155 XL, GNC 250 XL and GNC 300 XL.
typedef struct
{
	svid_type svid;		// satellite id 
	INTTYP wn;				// week number (weeks) 
	float toa;			// almanac data reference time (s) 
	float af0;			// clock correction coefficient (s) 
	float af1;			// clock correction coefficient (s/s) 
	float e;			// eccentricity (-) 
	float sqrta;		// square root of semi-major axis (a) (m**1/2) 
	float m0;			// mean anomaly at reference time (r) 
	float w;			// argument of perigee (r) 
	float omg0;			// right ascension (r) 
	float odot;			// rate of right ascension (r/s) 
	float i;			// inclination angle (r) 
	byte hlth;			// almanac health bits 17:24 (coded) 
} D551_Almanac_Type;
// The "svid" member identifies a satellite in the GPS constellation as
// follows:  PRN-01 through PRN-32 are indicated by "svid" equal to 0
// through 31, respectively.

//7.5.24. D600_Date_Time_Type
// Example products: all products unless otherwise noted.
typedef struct
{
	byte month;			// month (1-12) 
	byte day;			// day (1-31) 
	word year;			// year (1990 means 1990) 
	INTTYP hour;		// hour (0-23) 
	byte minute;		// minute (0-59) 
	byte second;		// second (0-59) 
} D600_Date_Time_Type;
// The D600_Date_Time_Type contains the UTC date and UTC time.

//7.5.25. D700_Position_Type
// Example products: all products unless otherwise noted.
typedef Radian_Type D700_Position_Type;

//7.5.26. D800_Pvt_Data_Type
// Example products: all products unless otherwise noted.
typedef struct
{
	float alt;			// altitude above WGS 84 ellipsoid (meters) 
	float epe;			// estimated position error, 2 sigma (meters) 
	float eph;			// epe, but horizontal only (meters) 
	float epv;			// epe, but vertical only (meters) 
	INTTYP fix;			// type of position fix 
	double tow;			// time of week (seconds) 
	Radian_Type posn;	// latitude and longitude (radians) 
	float east;			// velocity east (meters/second) 
	float north;		// velocity north (meters/second) 
	float up;			// velocity up (meters/second) 
	float msl_hght;		// height of WGS 84 ellipsoid above MSL (meters) 
	INTTYP leap_scnds;		// difference between GPS and UTC (seconds) 
	long wn_days;		// week number days 
} D800_Pvt_Data_Type;

#pragma pack()

//=========================================
//=== End of typedefs from Garmin Beta doc
//=========================================

//=========================================
//
// functions
//
//=========================================
void	byebye(INTTYP rt);
void	calc_sa_items(void);
INTTYP	checksum(unsigned char *m, INTTYP l);
void	chk_cmdline(void);
//void	set_sign(void);
//void	set_ns(void);
void	clear_route_dupes(void);
void	clean_proximity(void);
void 	count_tracks(void);
void 	count_routes(void);
INTTYP		comment_len(void);
void	convert_g64(unsigned char *in);
INTTYP		convert_from_GPS(INTTYP ict);
void	datumconv(double la, double lo);
void	get_datetime_str(void);
long	date2days(short mm, short dd, short yy);
void	days2date(long days, short *mm, short *dd, short *yy);
void	datetime2record(char *d);
void	decode_COM(char *b);
INTTYP		decode_month(char *s);
void	DegToUTM(double lat, double lon, char *zone, double *x, double *y);
INTTYP		determine_input(char *fname_in);
double	distance(double a, double b, double c, double d);
void	doD100(D100_Wpt_Type *a);
void 	doD105(D105_Wpt_Type *a);
void 	doD106(D106_Wpt_Type *a);
void 	doD150(D150_Wpt_Type *a);
void	doEOFRec(void);
void	doProtocolArray(void);
void	doID(void);
void	domagmaxmin(char *in);
void	doSAlmanac500(void);
void	doTime(void);
void	do_command(unsigned char *b);
void	do_input(INTTYP i);
void	do_pushed_route(void);
void	do_sa3_merge(void);
void	doasystat(void);
//long	dt2secs(short offset);
void	fill_icon(int i, unsigned char * j, INTTYP convert);
void	fill_record_from_text(unsigned char *ix, INTTYP convert);
void	fill_track(int ele, INTTYP convert);
void	fill_waypoint_from_push(int i, int j, int convert);
void	fixname(INTTYP j, unsigned char *name);
double	fm_diskLL(unsigned long l);
INTTYP 	from_sa3object(byte val);
void	getGarminDisplay(void);
void	getGarminMessage(void);
void	getGPSTime(void);
void	getGPSrecords(void);
void	getID(void);
void	GetLowrDisplay(void);
char	*getstrings(char *source, char *separators, int a);
void	getTrack(void);
void	getProtocolArray(void);
void	getWaypoints(void);
INTTYP		get_model(void);
INTTYP		getopt(INTTYP argc, char **argv, char *opts);
INTTYP		htoi(char *s);
INTTYP		instr(char *src, char *pat);
void	light_sleep(char onoff);
char *  ltrim(char *s);
void	make_bin(char *i, INTTYP r);
double	mhr(char * tt);
void	msdelay(unsigned INTTYP milliseconds);
void	navSRouteS(void);
void	navSWaypoint(void);
void	parseGPS(void);
void	poweroff(void);
void	printArray(void);
void	light_on(void);
void	light_off(void);
void	prhex(unsigned char *message, INTTYP c);
void	print_csv_i(void);
void	print_csv_w(void);
void	print_csv_t(void);
void	print_datums(void);
void	print_g64_t(void);
void	print_g64_w(void);
void	print_icons(void);
void	print_routes(void);
void	print_text_w(void);
void 	print_waypoints(INTTYP i, INTTYP j);
void 	print_tracks(void);
void	process_g45_in(void);
void	push_files(char *b);
void 	push_route_point(INTTYP r, INTTYP p);
void	readMapNote(FILE *in);
void	readMapLine(FILE *in);
void	readMapArea(FILE *in);
void	readMapCircle(FILE *in);
void	readMapSymbol(FILE *in);
void	readMapText(FILE *in);
void	read_nav_route(void);
void	read_text(void);
char *  rtrim(char *s);
void	sa3out(unsigned char *s);
void	sa3out_w(void);
void	sa4out(unsigned char *s);
void	sa4out_t(void);
void	sa4out_th(INTTYP i);
void	secs2dt(long secs/*, short offset*/);
void	SendGarminMessage(byte *message, short bytes);
void	send_g45_routes(void);
void	send_num_points(int num, char *a);
void	set_align(char *arg);
void	set_circle_parms(void);
void	set_color(char *arg);
void	set_contrast(void);
void	set_datetime(void);
void 	set_datum(char *from, char * to);
void	set_params(void);
void	set_record_icon(void);
void	set_symbol_parms(void);
void	set_size(char *arg);
void	set_line_parms(INTTYP lt);
void	setProtocolParams(void);
void	settime_record(time_t dot);
void	sortit(char **a, int *items);
void	strip_chars(char *s);
void	strip_tr_0(char *s);
unsigned long to_diskLL(double l);
byte	to_sa3object(INTTYP val);
char *  toDMlat(double a);
char *  toDMlon(double a);
char *  trim(char *s);
double  toDegrees(long a);
long	toSemiCircles(double b);
int		wpt_in_route(char *wpt, int rt);
void	UTMtoDeg(short zone, short southernHemisphere, double x, double y, double *lat, double *lon);
//
// Lowr.c routines
//
void	DoLowrWPT(void);
byte	SetLowrChkSum(byte * a, INTTYP len);
byte	ChkLowrChkSum(byte * s, INTTYP l);
void	SendLowrMessage(INTTYP command, byte *message, INTTYP bytes, char *msgtype);
void	GetLowrMessage(INTTYP exit_on_error);
//double	round(double n, int dig);
long 	atDegtoMM(double d);
double	LatMMtoDeg(long m);
long	LongDegtoMM(double d);
double	LongMMtoDeg(long m);
void	LowrParse(void);
void	OutputLowr(void);
void	GetLowrTrack(INTTYP i);
void	GetLowrTrackMem(INTTYP i);
void	GetLowrWpts(void);
void	GetLowrRts(void);
void	GetLowrIcons(void);
void	GetLowrID(void);
void	GetLowrGPSDisplay(void);
long	LatDegtoMM(double d);
INTTYP		convert_to_Lowr(INTTYP i);
INTTYP		convert_from_Lowr(INTTYP i);
void	DoLowrPlotTrailOrigin(void);

#define PROCX
#define DISABLE			0
#define ENABLE			1
#define ChkFree(a)		if((a)!=NULL)free(a)
#define MAXWAYP 	 65500
#define MAXROUTES 	  99
#define MAXPOINTS 	  99
#define MAXICON   	1001
#define MAXPROX 	  10
#define NORMAL_WP 	   0
#define NORMAL_WP_1    1
#define PROX_WP 	   6
#define ROUTE_1_WP	   8
#define ROUTE_WP	   9
#define NOFILE		-2  // file not found
#define UNKNOWN		 0  // Unknown file type
#define TEXT_IO		 1	// text/gd7
#define GARMIN		 2	// GPS i/o
#define SA3			 3	// Street Atlas 3
#define ALMANAC_D	 4	// Almanac data
#define NAVROUTE 	 5	// Navigate route
#define SA4          6	// Street Atlas 4
#define SA5          7	// Street Atlas 5
#define SA_GPL		 8  // Street Atlas GPL files
#define SA6          9	// Street Atlas 6
#define FUGAWI_WPT	10	// Fugawi waypoint file
#define FUGAWI_TRK	11	// Fugawi track file
#define OZI_EVT		12	// OziExplorer events (Icons)
#define LOWRANCE	15	// GPS i/o
#define OZI_PLT		16	// OziExplorer track file
#define OZI_WPT		17	// OziExplorer waypoint file
#define OZI_RTE		18	// OziExplorer route file
#define NONE		 0
#define SA_SYMBOL	 1
#define SA_TRACK	 2
#define SA_CIRCLE	 3

#define TRACKS		 1
#define WAYPOINTS	 2
#define ROUTES		 3
#define ID			 4
#define TIME		 5
#define POSITION	 6
#define ALMANAC		 7
#define PROXIMITY	 8
#define VOLTAGES 	 9
#define PVT			12
#define PPROTOCOLS	14
#define GERROR		99
#define LOWRERROR	99

#define TRACK1 		1
#define TRACK2 		2
#define LWRWPTS 	3
#define LWRRTS 		4
#define LWRICONS	5
#define LWRID		6
#define	COM_SETTINGS	(BITS_8 | STOP_1 | NO_PARITY)
#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define MARKPARITY          3
#define SPACEPARITY         4
#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2
#define BYTESIZE			8

extern byte SerialMessage[MAX_LENGTH];
extern byte SerialMessageOut[MAX_LENGTH];
extern byte message[MAX_LENGTH];
extern short SerialMsgLen;
//=====================================================================

extern INTTYP		gotID;					// set to 1 when the connected GPS has been
										//  ID'd
//
// used in resending last message
//
extern byte 	LastGarminGPSMsg[MAX_LENGTH];
extern short	LastGarminLen;
//
// Global variables
//
extern char gps_errmsg[128];
extern BOOL bLocalTime, bWptAlt, bTrkAlt;
extern char gps_limitName[16];

//===================================================================

extern unsigned INTTYP SWVersion;
extern int VersionSubStrings;
extern char VersionString[99];  /* model/version in string format    */
extern char *VersionSubString[10];// version substrings

typedef struct {
	float fsec;
	short ideg;
	short imin;
	UINT  bNeg;
} DEGTYP;

void getDMS(DEGTYP *p, double val);

