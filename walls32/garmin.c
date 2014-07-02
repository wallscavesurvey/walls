#include "gps.h"

static const double CONVERT = 11930464.7111111111;  /* 2^31 / 180 */

static long	numrecords;			// number of input records in file
static INTTYP datatype;		// The type of data stored in the current record.  This is based
						//  upon the record type used to store the data
//=====================================================================
// Data from Garmin Beta Document
//
//=====================================================================
static INTTYP  SPWptNo;
static INTTYP Product_Ver;				// 000 - all		...
static INTTYP Product_ID;
static INTTYP go_on,ok;
static INTTYP firstwaypoint;
static INTTYP firsttrackpoint;
static INTTYP haveProtocolArray;		// set to 1 if the GPS sends a protocol array

static BYTE newTrack;  		/* 255 no track records this session */
                    		/*  0 sending tracks                 */
		    				/*  1 sending first track            */
static BYTE recordtype;	// record type being sent
static BYTE *StreetPilotWptName;
static BYTE *StreetPilotLnk_ident;

//
// Defines of Known Product_ID's
//
#define GPSIIP               73
#define GPS12_12XLA          77
#define GPS12                87
#define GPS12_12XLB          96
#define GPSIIP_1             97
#define GPS12CX             116
#define GPS12XL_J           105  // Japanese
#define GPS12XL_C           106  // Chinese
#define GPS38_40_45_45XL_A   31
#define GPS38_40_45_45XL_B   41
#define GPSII                59
#define GPS38_C              56
#define GPS38_J              62
#define GPSIIIPILOT          71
#define GPSIII_ID            72
#define STREETPILOT          67
#define STREETPILOTCOLORMAP 118
#define GPS3P               119

// Garmin GPS Family for Icons
//
static INTTYP Ggps=0;
#define     G12XL           1
#define     GPSIII          2
#define     GPSIIIP         3
#define     SPILOT          4
#define     SPILOTCOLOR     5

//=====================================================================
//
// Garmin symbol numbers
//
//=====================================================================
enum
{
//
// * indicates those in the GPS III revision 2.05 and greater
//
//---------------------------------------------------------------
// Symbols for marine (group 0...0-8191...bits 15-13=000).
//---------------------------------------------------------------
	sym_anchor =			0,	// 0x0000 * white anchor symbol 
	sym_bell =				1,	// 0x0001 * white bell symbol 
	sym_diamond_grn =		2,	// 0x0002   green diamond symbol 
	sym_diamond_red =		3,	// 0x0003   red diamond symbol 
	sym_dive1 =				4,	// 0x0004   diver down flag 1 
	sym_dive2 =				5,	// 0x0005   diver down flag 2 
	sym_dollar =			6,	// 0x0006 * white dollar symbol 
	sym_fish =				7,	// 0x0007 * white fish symbol 
	sym_fuel =				8,	// 0x0008 * white fuel symbol 
	sym_horn =				9,	// 0x0009   white horn symbol 
	sym_house =			   10,	// 0x000a * white house symbol 
	sym_knife =			   11,	// 0x000b * white knife & fork symbol 
	sym_light =			   12,	// 0x000c   white light symbol 
	sym_mug =			   13,	// 0x000d * white mug symbol 
	sym_skull =			   14,	// 0x000e * white skull and crossbones symbol
	sym_square_grn =	   15,	// 0x000f   green square symbol 
	sym_square_red =	   16,	// 0x0010   red square symbol 
	sym_wbuoy =			   17,	// 0x0011   white buoy waypoint symbol 
	sym_wpt_dot =		   18,	// 0x0012 * waypoint dot 
	sym_wreck =			   19,	// 0x0013   white wreck symbol 
	sym_null =			   20,	// 0x0014   null symbol (transparent) 
	sym_mob =			   21,	// 0x0015 * man overboard symbol 
//------------------------------------------------------
// Marine navaid symbols
//------------------------------------------------------
	sym_buoy_ambr =		   22,	// 0x0016   amber map buoy symbol 
	sym_buoy_blck =		   23,	// 0x0017   black map buoy symbol 
	sym_buoy_blue =		   24,	// 0x0018   blue map buoy symbol 
	sym_buoy_grn =		   25,	// 0x0019   green map buoy symbol 
	sym_buoy_grn_red =	   26,	// 0x001a   green/red map buoy symbol 
	sym_buoy_grn_wht =	   27,	// 0x001b   green/white map buoy symbol 
	sym_buoy_orng =		   28,	// 0x001c   orange map buoy symbol 
	sym_buoy_red =		   29,	// 0x001d   red map buoy symbol 
	sym_buoy_red_grn =	   30,	// 0x001e   red/green map buoy symbol 
	sym_buoy_red_wht =	   31,	// 0x001f   red/white map buoy symbol 
	sym_buoy_violet =	   32,	// 0x0020   violet map buoy symbol 
	sym_buoy_wht =		   33,	// 0x0021   white map buoy symbol 
	sym_buoy_wht_grn =	   34,	// 0x0022   white/green map buoy symbol 
	sym_buoy_wht_red =	   35,	// 0x0023   white/red map buoy symbol 
	sym_dot =			   36,	// 0x0024   white dot symbol 
	sym_rbcn =			   37,	// 0x0025   radio beacon symbol 
//------------------------------------------------------
// ... leave space for more navaids (up to 128 total)
//------------------------------------------------------
	sym_boat_ramp =		  150,	// 0x0096 * boat ramp symbol 
	sym_camp =			  151,	// 0x0097 * campground symbol 
	sym_restrooms =		  152,	// 0x0098 * restrooms symbol 
	sym_showers =		  153,	// 0x0099 * shower symbol 
	sym_drinking_wtr =	  154,	// 0x009a * drinking water symbol 
	sym_phone =			  155,	// 0x009b * telephone symbol 
	sym_1st_aid =		  156,	// 0x009c * first aid symbol 
	sym_info =			  157,	// 0x009d * information symbol 
	sym_parking =		  158,	// 0x009e * parking symbol 
	sym_park =			  159,	// 0x009f * park symbol 
	sym_picnic =		  160,	// 0x00a0 * picnic symbol 
	sym_scenic =		  161,	// 0x00a1 * scenic area symbol 
	sym_skiing =		  162,	// 0x00a2 * skiing symbol 
	sym_swimming =		  163,	// 0x00a3 * swimming symbol 
	sym_dam =			  164,	// 0x00a4 * dam symbol 
	sym_controlled =	  165,	// 0x00a5   controlled area symbol 
	sym_danger =		  166,	// 0x00a6   danger symbol 
	sym_restricted =	  167,	// 0x00a7   restricted area symbol 
	sym_null_2 =		  168,	// 0x00a8   null symbol 
	sym_ball =			  169,	// 0x00a9 * ball symbol 
	sym_car =			  170,	// 0x00aa * car symbol 
	sym_deer =			  171,	// 0x00ab * deer symbol 
	sym_shpng_cart =	  172,	// 0x00ac * shopping cart symbol 
	sym_lodging =		  173,	// 0x00ad * lodging symbol 
	sym_mine =            174,  // 0x00ae * mine symbol
//---------------------------------------------------------------
// Symbols for land (group 1...8192-16383...bits 15-13=001).
//---------------------------------------------------------------
	sym_is_hwy =		 8192,	// 0x2000   interstate hwy symbol 
	sym_us_hwy =		 8193,	// 0x2001   us hwy symbol 
	sym_st_hwy =		 8194,	// 0x2002   state hwy symbol 
	sym_mi_mrkr =		 8195,	// 0x2003   mile marker symbol 
	sym_trcbck =		 8196,	// 0x2004 * TracBack (feet) symbol 
	sym_golf =			 8197,	// 0x2005 * golf symbol 
	sym_sml_cty =		 8198,	// 0x2006 * small city symbol 
	sym_med_cty =		 8199,	// 0x2007 * medium city symbol 
	sym_lrg_cty =		 8200,	// 0x2008 * large city symbol 
	sym_freeway =		 8201,	// 0x2009   intl freeway hwy symbol 
	sym_ntl_hwy =		 8202,	// 0x200a   intl national hwy symbol 
	sym_cap_cty =		 8203,	// 0x200b   capitol city symbol (star) 
	sym_amuse_pk =		 8204,	// 0x200c   amusement park symbol 
	sym_bowling =		 8205,	// 0x200d   bowling symbol 
	sym_car_rental =	 8206,	// 0x200e   car rental symbol 
	sym_car_repair =	 8207,	// 0x200f   car repair symbol 
	sym_fastfood =		 8208,	// 0x2010   fast food symbol 
	sym_fitness =		 8209,	// 0x2011   fitness symbol 
	sym_movie =			 8210,	// 0x2012   movie symbol 
	sym_museum =		 8211,	// 0x2013   museum symbol 
	sym_pharmacy =		 8212,	// 0x2014   pharmacy symbol 
	sym_pizza =			 8213,	// 0x2015   pizza symbol 
	sym_post_ofc =		 8214,	// 0x2016   post office symbol 
	sym_rv_park =		 8215,	// 0x2017   RV park symbol 
	sym_school =		 8216,	// 0x2018   school symbol 
	sym_stadium =		 8217,	// 0x2019   stadium symbol 
	sym_store =			 8218,	// 0x201a   dept. store symbol 
	sym_zoo =			 8219,	// 0x201b   zoo symbol 
//---------------------------------------------------------------
// Symbols for aviation (group 2...16383-24575...bits 15-13=010).
//---------------------------------------------------------------
	sym_airport =		16384,	// 0x4000 * airport symbol 
	sym_int =			16385,	// 0x4001   intersection symbol 
	sym_ndb =			16386,	// 0x4002   non-directional beacon symbol 
	sym_vor =			16387,	// 0x4003   VHF omni-range symbol 
	sym_heliport =		16388,	// 0x4004 * heliport symbol 
	sym_private =		16389,	// 0x4005 * private field symbol 
	sym_soft_fld =		16390,	// 0x4006 * soft field symbol 
	sym_tall_tower =	16391,	// 0x4007 * tall tower symbol 
	sym_short_tower =	16392,	// 0x4008 * short tower symbol 
	sym_glider =		16393,	// 0x4009 * glider symbol 
	sym_ultralight =	16394,	// 0x400a * ultralight symbol 
	sym_parachute =		16395,	// 0x400b * parachute symbol 
	sym_vortac =		16396,	// 0x400c   VOR/TACAN symbol 
	sym_vordme =		16397,	// 0x400d   VOR-DME symbol 
	sym_faf =			16398,	// 0x400e   first approach fix 
	sym_lom =			16399,	// 0x400f   localizer outer marker 
	sym_map =			16400,	// 0x4010   missed approach point 
	sym_tacan =			16401,	// 0x4011   TACAN symbol 
	sym_seaplane =		16402,	// 0x4012 * Seaplane Base 
};
//---------------------------------------------------------------
//D103_Wpt_Type symbols:
//---------------------------------------------------------------
enum
{
	smbl_dot = 				0,	//   0x00 dot symbol 
	smbl_house =			1,	//   0x01 house symbol 
	smbl_gas =				2,	//   0x02 gas symbol 
	smbl_car =				3,	//   0x03 car symbol 
	smbl_fish =				4,	//   0x04 fish symbol 
	smbl_boat =				5,	//   0x05 boat symbol 
	smbl_anchor =			6,	//   0x06 anchor symbol 
	smbl_wreck =			7,	//   0x07 wreck symbol 
	smbl_exit =				8,	//   0x08 exit symbol 
	smbl_skull =			9,	//   0x09 skull symbol 
	smbl_flag =			   10,	//   0x0a flag symbol 
	smbl_camp =			   11,	//   0x0b camp symbol 
	smbl_duck =			   12,	//   0x0c duck symbol 
	smbl_deer =			   13,	//   0x0d deer symbol 
	smbl_buoy =			   14,	//   0x0e buoy symbol 
	smbl_back_track =	   15 	//   0x0f back track symbol 
};

//=====================================================================
//  Default Packet IDs
//
enum {
	Pid_Ack_Byte		=   6, // 0x06
	Pid_Nak_Byte		=  21, // 0x15
	Pid_Ext_Product_Data = 248, /* may not be implemented in all devices */
	Pid_Protocol_Array	= 253, // 0xfd
	Pid_Product_Rget	= 254, // 0xfe
	Pid_Product_Data	= 255  // 0xff
};

//=====================================================================
// L001 - Link Protocol 1
//
enum {
	Pid_Command_data	=  10, // 0x0a
	Pid_Xfer_Cmplt		=  12, // 0x0c
	Pid_Date_Time_Data	=  14, // 0x0e
	Pid_Position_Data	=  17, // 0x11
	Pid_Prx_Wpt_Data	=  19, // 0x13
	Pid_Records			=  27, // 0x1b
	Pid_Rte_Hdr			=  29, // 0x1d
	Pid_Rte_Wpt_Data	=  30, // 0x1e
	Pid_Almanac_Data	=  31, // 0x1f
	Pid_Trk_Data		=  34, // 0x22
	Pid_Wpt_Data		=  35, // 0x23
	Pid_Pvt_Data		=  51, // 0x33  // also seen on GPS III with DeLorme
	Pid_Rte_Link_Data = 98,
	Pid_Trk_Hdr = 99
};

//=====================================================================
// L002 - Link Protocol 2
//
enum {
	Pid_Almanac_Data2	=   4, // 0x04
	Pid_Command_Data2	=  11, // 0x0b
	Pid_Xfer_Cmplt2		=  12, // 0x0c
	Pid_Date_Time_Data2	=  20, // 0x14
	Pid_Position_Data2	=  24, // 0x18
	Pid_Records2		=  35, // 0x23
	Pid_Rte_Hdr2		=  37, // 0x25
	Pid_Rte_Wpt_Data2	=  39, // 0x27
	Pid_Wpt_Data2		=  43  // 0x2b
};

//=====================================================================
// A000 - Product Data Protocol
//
//typedef struct
//{
//	INTTYP product_ID;
//	INTTYP software_version;
//	byte a[1];  // fake for below ...
// char product_description[]; null-terminated string
// ... zero or more additional null-terminated strings
//} Product_Data_Type;

//=====================================================================
// A010 - Device Command Protocol 1
//
enum
{
	Cmnd_Abort_Transfer	=  0, // 0x00	abort current transfer
	Cmnd_Transfer_Alm 	=  1, // 0x01	transfer almanac
	Cmnd_Transfer_Posn 	=  2, // 0x02	transfer position   added by crh
	Cmnd_Transfer_Prx 	=  3, // 0x03	transfer proximity waypoints
	Cmnd_Transfer_Rte 	=  4, // 0x04	transfer routes
	Cmnd_Transfer_Time	=  5, // 0x05	transfer date/time  added by crh
	Cmnd_Transfer_Trk 	=  6, // 0x06	transfer track log
	Cmnd_Transfer_Wpt 	=  7, // 0x07	transfer waypoints
	Cmnd_Turn_Off_Pwr   =  8, // 0x08	Turn unit off
	Cmnd_Send_Voltages  = 17, // 0x11	Send voltage readings (some units)
	Cmnd_Send_Display   = 32, // 0x20   Send GPS III display
	Cmnd_Start_Pvt_Data = 49, // 0x31	start transmitting PVT
	Cmnd_Stop_Pvt_Data	= 50  // 0x32	stop transmitting PVT data
};

//=====================================================================
// A011 - Device Command Protocol 2 (I added the '2' below crh 3/23/98)
//
enum
{
	Cmnd_Abort_Transfer2=  0, // 0x00	abort current transfer
	Cmnd_Transfer_Alm2 	=  4, // 0x04	transfer almanac
	Cmnd_Transfer_Rte2	=  8, // 0x08	transfer routes
	Cmnd_Transfer_Time2 = 20, // 0x14   transfer time
	Cmnd_Transfer_Wpt2	= 21, // 0x15	transfer waypoints
	Cmnd_Turn_Off_Pwr2  = 26  // 0x19   Turn unit off
};
//=====================================================================
// Index into Protocols Array for the various protocol id's
//
//   For example, the III is index 72 which has the following data:
//
//  Product_ID		:  72	Garmin's product ID for the III
//	Product_Ver		:   0	(0 indicates all versions of the III)
//	link			:   1	Link version L001
//	Command			:  10	Command version A010
//	WaypDataRecord	: 104	Waypoint data	D104
//	RouteRecord		: 201	RouteRecord		D201
//	RouteTrkRecord	: 104	RouteTrack		D104
//	TrackDataRecord	: 300	TrackRecord		D300
//	ProxDataRecord  :  -1	ProximityData is not supported on this model gps
//	AlmanacRecord	: 501	AlmanacData		D501
//

static INTTYP		AlmanacEOF;
static INTTYP		RouteEOF;
static INTTYP		TrackEOF;
static INTTYP		ProxWaypointEOF;
static INTTYP		WaypointEOF;

//static BYTE	p1[]	= "\x06\x02\x1b\x00";	// Get 1st packet
//static BYTE	p2[]	= "\x06\x02\x0c\x00";	// Get last packet
//static BYTE	rte2[]	= "\x06\x02\x1d\x00";	// Get next route packet
//static BYTE	alm2[]	= "\x06\x02\x1f\x00";	// Get next almanac packet
//static BYTE	trk2[]	= "\x06\x02\x22\x00";	// Get next track packet
//static BYTE	wpt2[]	= "\x06\x02\x23\x00";	// Get next waypoint packet
static BYTE	ack1[]	= "\x06\x02\xff\x00";	// ack for received mod/ver info

static BYTE	Gabort[]= "\x0a\x02\x00\x00";	// Abort Transfer
static BYTE	alm1[]	= "\x0a\x02\x01\x00";	// Send almanac
static BYTE	GPSPos[]= "\x0a\x02\x02\x00";	// send position
static BYTE	pwpt1[]	= "\x0a\x02\x03\x00";	// send Proximity waypoints
static BYTE	rte1[]	= "\x0a\x02\x04\x00";	// send routes
//static BYTE	display[]="\x0a\x02\x20\x00";	// send GPS III display
static BYTE	GPStime[]="\x0a\x02\x05\x00";	// send UTC time
static BYTE	trk1[]	= "\x0a\x02\x06\x00";	// send tracks
static BYTE	wpt1[]	= "\x0a\x02\x07\x00";	// send waypoints
static BYTE	pwroff[]= "\x0a\x02\x08\x00";	// Power off command
static BYTE	VoltsM[]= "\x0a\x02\x11\x00";	// send voltages
static BYTE	PVTon[] = "\x0a\x02\x31\x00";	// start sending PVT records
static BYTE	PVToff[]= "\x0a\x02\x32\x00";	// stop sending PVT records

static BYTE	almt[]	= "\x0c\x02\x01\x00";	// Almanac data base terminator
static BYTE	prxt[]	= "\x0c\x02\x03\x00";	// Proximity data base terminator
static BYTE	rtet[]	= "\x0c\x02\x04\x00";	// Route data base terminator
static BYTE	trkt[]	= "\x0c\x02\x06\x00";	// Track data base terminator
static BYTE	wptt[]	= "\x0c\x02\x07\x00";	// Waypoint data base terminator

static BYTE	nak1[]	= "\x15\x02\xff\x00";	// nak for received mod/ver info

static BYTE	bgnxfr[]= "\x1b\x02\x00\x00";   // 0000 filled in with number

static BYTE	lighton[]=  "\x0f\x01\x00";		// send light on (some units)
static BYTE	lightoff[]= "\x0f\x01\x01";		// send light off (some units)
static BYTE	lightiiP[]= "\x0f\x01\x80";		// send light on II+
static BYTE	contrast[]= "\x0b\x01\x08";		// set contrast
static BYTE	m1[]	  = "\xfe\x01\x20";		// Get the model and version number

//=====================================================================
// Current product info			Examples		Range allowed by doc
//=====================================================================
static INTTYP D105orD106;			// 1 if current waypoint is from/to a StreetPilot

static INTTYP PhysicalProtocol;	// 000 = P000	P000

static INTTYP LinkProtocol;		// 001 = L001	L001,L002
 
static INTTYP CommandProtocol;		// 010 = A010	A010,A011

static INTTYP WaypXferProtocol;	// 100 = A100	A100
static INTTYP WaypDataRecord;	// 104 = D104	D100,D101,D102,D103,D104,D105,D106,D150
								//				D151,D152,D154,D155

static INTTYP RouteXferProtocol;	// 200 = A200	A200
static INTTYP RouteHdrType;		// 201 = D201	D200,D201,D202
static INTTYP RouteDataRecord;		// 104 = D104	D100,D101,D102,D103,D104,D105,D106,D150
								//				D151,D152,D154,D155

static INTTYP TrackXferProtocol;	// 300 = A300	A300
static INTTYP TrackDataRecord;		// 300 = D300	D300

static INTTYP ProxXferProtocol;	// 000 = None	A400
static INTTYP ProxDataRecord;		// 000 = None	D101,D102,D152,D400,D403,D450

static INTTYP AlmanacXferProtocol;	// 500 = A500	A500
static INTTYP AlmanacDataRecord;	// 501 = D501	D500,D501,D550,D551

static INTTYP UTCXferProtocol;		// 600 = A600	A600
static INTTYP UTCDataRecord;		// 600 = D600	D600

static INTTYP PositionXferProtocol;// 700 = A700	A700
static INTTYP PositionDataRecord;	// 700 = D700	D700

static PVTXferProtocol;		// 000 = None	A800
static PVTDataRecord;		// 000 = None	D800

//=====================================================================
// Product ID vs Protocols supported.  These products do not currently
//  support PVT data transfer (A800-D800).  They do not support the
//  Protocol Capability Protocol, either
//
static INTTYP Protocols[55][12] = {
// ID Ver Link Cmnd        Wpt          Rte        Trk       Prx          Alm
// -- --- ---- ----        ---        --------   --------  --------       ---
 { 13,  0,   1,  10,       100,       201, 100,  300, 300, 400, 400,      500},
 { 14,  0,   1,  10,       100,       200, 100,  300, 300, 400, 400,      500},
 { 15,  0,   1,  10,       151,       200, 151,  300, 300, 400, 400,      500},
 { 18,  0,   1,  10,       100,       201, 100,  300, 300, 400, 400,      500},
 { 20,  0,   2,  11,       150,       201, 150,   -1,  -1, 400, 450,      550},
 { 22,  0,   1,  10,       152,       201, 152,  300, 300, 400, 152,      500},
 { 23,  0,   1,  10,       100,       201, 100,  300, 300, 400, 400,      500},
 { 24,  0,   1,  10,       100,       201, 100,  300, 300, 400, 400,      500},
 { 29,-400,  1,  10,       101,       201, 101,  300, 300, 400, 101,      500},
 { 29, 400,  1,  10,       102,       200, 102,  300, 300, 400, 102,      500},
 { 31,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 33,  0,   2,  11,       150,       201, 150,   -1,  -1, 400, 450,      550},
 { 34,  0,   2,  11,       150,       201, 150,   -1,  -1, 400, 450,      550},
 { 35,  0,   1,  10,       100,       201, 100,  300, 300, 400, 400,      500},
 { 36,-300,  1,  10,       152,       201, 152,  300, 300, 400, 152,      500},
 { 36, 300,  1,  10,       152,       201, 152,  300, 300,  -1,  -1,      500},
 { 39,  0,   1,  10,       151,       201, 151,  300, 300,  -1,  -1,      500},
 { 41,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 42,  0,   1,  10,       100,       201, 100,  300, 300, 400, 400,      500},
 { 44,  0,   1,  10,       101,       201, 101,  300, 300, 400, 101,      500},
 { 45,  0,   1,  10,       152,       201, 152,  300, 300,  -1,  -1,      500},
 { 47,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 48,  0,   1,  10,       154,       201, 154,  300, 300,  -1,  -1,      501},
 { 49,  0,   1,  10,       102,       201, 102,  300, 300, 400, 102,      501},
 { 50,  0,   1,  10,       152,       201, 152,  300, 300,  -1,  -1,      501},
 { 52,  0,   2,  11,       150,       201, 150,   -1,  -1, 400, 450,      550},
 { 53,  0,   1,  10,       152,       201, 152,  300, 300,  -1,  -1,      501},
 { 55,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 56,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 59,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 61,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 62,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 64,  0,   2,  11,       150,       201, 150,   -1,  -1, 400, 450,      551},
 { 71,  0,   1,  10,       155,       201, 155,  300, 300,  -1,  -1,      501},
 { 72,  0,   1,  10,       104,       201, 104,  300, 300,  -1,  -1,      501},
 { 73,  0,   1,  10,       103,       201, 103,  300, 300,  -1,  -1,      501},
 { 74,  0,   1,  10,       100,       201, 100,  300, 300,  -1,  -1,      500},
 { 76,  0,   1,  10,       102,       201, 102,  300, 300, 400, 102,      501},
 { 77,-301,  1,  10,       100,       201, 100,  300, 300, 400, 400,      501},
 { 77,-350,  1,  10,       103,       201, 103,  300, 300, 400, 403,      501},
 { 77,-361,  1,  10,       103,       201, 103,  300, 300,  -1,  -1,      501},
 { 77, 361,  1,  10,       103,       201, 103,  300, 300, 400, 403,      501},
 { 87,  0,   1,  10,       103,       201, 103,  300, 300, 400, 403,      501},
 { 95,  0,   1,  10,       103,       201, 103,  300, 300, 400, 403,      501},
 { 96,  0,   1,  10,       103,       201, 103,  300, 300, 400, 403,      501},
 { 97,  0,   1,  10,       103,       201, 103,  300, 300,  -1,  -1,      501},
 { 98,  0,   2,  11,       150,       201, 150,   -1,  -1, 400, 450,      551},
 {100,  0,   1,  10,       103,       201, 103,  300, 300, 400, 403,      501},
 {105,  0,   1,  10,       103,       201, 103,  300, 300, 400, 403,      501},
 {106,  0,   1,  10,       103,       201, 103,  300, 300, 400, 403,      501},
 { -1, -1,   -1,  -1,       -1,       -1, -1,    -1,   -1,  -1,  -1,      -1}
};
static INTTYP ProtocolIndex;	// index into above table which holds the valid
						//  protocols for this connected device
/* -------------------------------------------------------------------------- */
D200_Rte_Hdr_Type 	D200;
D201_Rte_Hdr_Type 	*D201;
D202_Rte_Hdr_Type 	*D202;
D400_Prx_Wpt_Type	*D400_Prx_Wpt;
D403_Prx_Wpt_Type	*D403_Prx_Wpt;
D450_Prx_Wpt_Type	*D450_Prx_Wpt;

short SerialMsgLen;
INTTYP gLine=0;					//  GPS III Scan line (0-99) being read

/* -------------------------------------------------------------------------- */
void settime_record(time_t dot)
{
	struct tm *today;

	if(dot<=0 || dot==0x7FFFFFFF)	{
		record.monthn=0;
		//dot = time(NULL) - UnixSecsMinusGpsSecs;
	}
	else {
		//dot is GPS time: seconds since UTC 12:00 AM on December 31st, 1989.
		if(bLocalTime) {
			dot+=UnixSecsMinusGpsSecs; //Unix time: seconds since midnight (00:00:00), January 1, 1970
			today=localtime(&dot);
			record.monthn=today->tm_mon+1,
		    record.dayofmonth=today->tm_mday;
		    record.year=today->tm_year+1900;
			record.ht=today->tm_hour;
			record.mt=today->tm_min;
			record.st=today->tm_sec;
		}
		else secs2dt((long)dot);
		//strcpy(record.datetime,secs2dt(dot,0)); //gPrefs.offset==0
	}
} // settime_record()

/* -------------------------------------------------------------------------- */
long toSemiCircles(double b)
{
	return((long)(b*CONVERT));
} // toSemiCircles(double b)

/* -------------------------------------------------------------------------- */
double toDegrees(long a)
{
	double t;

	t=(double)(a);
	return ((double)(t/CONVERT));
} // toDegrees(long a)

/* -------------------------------------------------------------------------- */
//
// This routine is common to almost all waypoint data records of all Garmin Units.
//  The only formats not using the basic D100 format are D105,106 and D150
//
/* -------------------------------------------------------------------------- */
void readD100(D100_Wpt_Type *D100_Wpt, INTTYP wpt)
{
	D101_Wpt_Type 		*D101_Wpt;
	D102_Wpt_Type 		*D102_Wpt;
	D103_Wpt_Type 		*D103_Wpt;
	D104_Wpt_Type 		*D104_Wpt;
	D107_Wpt_Type       *D107_Wpt; // 12CX
	D151_Wpt_Type 		*D151_Wpt;
	D152_Wpt_Type 		*D152_Wpt;
	D154_Wpt_Type 		*D154_Wpt;
	D155_Wpt_Type 		*D155_Wpt;

	record.la = toDegrees(D100_Wpt->posn.lat); //359554047
	record.lo = toDegrees(D100_Wpt->posn.lon); //-1168650945
	record.altitude=NO_ALTITUDE;

	//set_ns();
/*
 * waypoint name
 */
	strncpy(record.ident,D100_Wpt->ident,6);
	record.ident[6]=0; trim(record.ident);
/*
 * Comment, up to 40 characters
 */
	strncpy(record.comment,D100_Wpt->cmnt,40);
	record.comment[40]=0; trim(record.comment);

/*
 * Date/Time  Always unused in D104, so set it to system time
 */
// why? settime_record(0);
//
// process rest of records
//
	switch(wpt) {
		case 100:
		case 400: break;
		case 101:
			    D101_Wpt=(D101_Wpt_Type *)D100_Wpt;
				record.ProxDist = D101_Wpt->dst;
				record.icon_smbl= D101_Wpt->smbl;
				break;

		case 102:
			    D102_Wpt=(D102_Wpt_Type *)D100_Wpt;
				record.ProxDist = D102_Wpt->dst;
				record.icon_smbl= D102_Wpt->smbl;
				break;

		case 103:
		case 403:
			    D103_Wpt=(D103_Wpt_Type *)D100_Wpt;
				record.icon_smbl  = D103_Wpt->smbl;
				record.icon_dspl = D103_Wpt->dspl;
//
// Translate from GPS 12 & II+ Icons
//
//	Program   12/II+      Display mode
//	   3        0         Symbol with Waypoint Name
//	   1        1         Symbol
//	   5        2         Symbol with Waypoint Comment
//
				switch(record.icon_dspl){
					case 0: record.icon_dspl=3;break;
					case 2: record.icon_dspl=5;break;
				}
				break;

		case 104:
			    D104_Wpt=(D104_Wpt_Type *)D100_Wpt;
				record.ProxDist   = D104_Wpt->dst;
				record.icon_smbl  = D104_Wpt->smbl;
				record.icon_dspl = D104_Wpt->dspl;
				break;

		case 107:
			D107_Wpt=(D107_Wpt_Type *)D100_Wpt;
			record.ProxDist   = D107_Wpt->dst;
			record.icon_smbl  = D107_Wpt->smbl;
			record.icon_dspl  = D107_Wpt->dspl;
			switch(record.icon_dspl){
				case 0: record.icon_dspl=3;break;
				case 2: record.icon_dspl=5;break;
			}
			record.ProxDist = D107_Wpt->dst;
			record.color = D107_Wpt->color;
			switch(record.color) {
				case 0: break;  // no change
				case 1: break;  // no change
				case 2:
					record.color=3;
					break;
				case 3:
					record.color=2;
					break;
			}
			break;

		case 151:
			    D151_Wpt=(D151_Wpt_Type *)D100_Wpt;
				record.ProxDist = D151_Wpt->dst;
				strncpy(record.name, D151_Wpt->name, 30);
				strncpy(record.city, D151_Wpt->city, 24);
				strncpy(record.state, D151_Wpt->state, 2);
				record.altitude=record.alt = D151_Wpt->alt;
				strncpy(record.cc, D151_Wpt->cc,2);
				record.gclass = D151_Wpt->gclass;
				break;

		case 152:
			    D152_Wpt=(D152_Wpt_Type *)D100_Wpt;
				record.ProxDist = D152_Wpt->dst;
				strncpy(record.name, D152_Wpt->name, 30);
				strncpy(record.city, D152_Wpt->city, 24);
				strncpy(record.state, D152_Wpt->state, 2);
				record.altitude=record.alt = D152_Wpt->alt;
				strncpy(record.cc, D152_Wpt->cc,2);
				record.gclass = D152_Wpt->gclass;
				break;

		case 154:
			    D154_Wpt=(D154_Wpt_Type *)D100_Wpt;
				record.ProxDist = D154_Wpt->dst;
				strncpy(record.name, D154_Wpt->name, 30);
				strncpy(record.city, D154_Wpt->city, 24);
				strncpy(record.state, D154_Wpt->state, 2);
				record.altitude=record.alt = D154_Wpt->alt;
				strncpy(record.cc, D154_Wpt->cc,2);
				record.gclass = D154_Wpt->gclass;
				record.icon_smbl = D154_Wpt->smbl;
				break;

		case 155:
			    D155_Wpt=(D155_Wpt_Type *)D100_Wpt;
				record.ProxDist = D155_Wpt->dst;
				strncpy(record.name, D155_Wpt->name, 30);
				strncpy(record.city, D155_Wpt->city, 24);
				strncpy(record.state, D155_Wpt->state, 2);
				record.altitude=record.alt = D155_Wpt->alt;
				strncpy(record.cc, D155_Wpt->cc,2);
				record.gclass = D155_Wpt->gclass;
				record.icon_smbl = D155_Wpt->smbl;
				record.icon_dspl = D155_Wpt->dspl;
				break;

		default: sprintf(gps_errmsg,"Unknown Data protocol in readD100");
				byebye(-1);
	} // switch(wpt)
	record.name[30]=0; trim(record.name);
	record.city[24]=0; trim(record.city);
	record.state[2]=0; trim(record.state);
	record.cc[2]=0; trim(record.cc);
} // readD100

/* -------------------------------------------------------------------------- */
void readD105(D105_Wpt_Type *D105_Wpt)
{
   // StreetPilot Waypoint
/*
 * Date/Time  Not used in D105, so set it to system time
 */
 	//settime_record(0);

	SPWptNo++;						// starts at 1
	record.la = toDegrees(D105_Wpt->posn.lat);
	record.lo = toDegrees(D105_Wpt->posn.lon);
	record.altitude=NO_ALTITUDE;
	//set_ns();
	record.icon_smbl=D105_Wpt->smbl;
	memset(StreetPilotWptName,0,MAX_LENGTH);
	memset(record.comment,0,40);
	memset(record.ident,0,6);
	strncpy((char*) StreetPilotWptName, D105_Wpt->wpt_ident,255);
	strncpy(record.comment,(char*)StreetPilotWptName,40);
	sprintf(record.ident,"SPW%03d",SPWptNo);
} // readD105

// 7.5.6. D105_Wpt_Type
//typedef struct
//{
//	Semicircle_Type posn;	// position
//	Symbol_Type smbl;		// symbol id
//	char wpt_ident[1]; 		// null-terminated string -- length isn't
//	                        //   really specified, I added 1 for a place
//							//   holder for the decode of D105 waypoints
//} D105_Wpt_Type;

// 7.5.7. D106_Wpt_Type
//  Example products: StreetPilot (route waypoints).
//typedef struct
//{
//	byte gclass;				// class
//	byte subclass[13];		// subclass
//	Semicircle_Type posn;	// position
//	Symbol_Type smbl;		// symbol id
//	char wpt_ident[1];  	// null-terminated string -- length isn't
//	                        //   really specified, I added 10 for a place
//							//   holder for the decode of D106 waypoints
///* char lnk_ident[]; null-terminated string */
//} D106_Wpt_Type;

/* -------------------------------------------------------------------------- */
void readD106(D106_Wpt_Type *D106_Wpt)
{
	unsigned char *p;
	INTTYP i;
/*
 * Date/Time  Not unused in D106, so set it to system time
 */
	//settime_record(0);
	SPWptNo++;						// starts at 1
	record.la = toDegrees(D106_Wpt->posn.lat);
	record.lo = toDegrees(D106_Wpt->posn.lon);
	record.altitude=NO_ALTITUDE;
	//set_ns();
	record.icon_smbl=D106_Wpt->smbl;
	memset(StreetPilotWptName,0,MAX_LENGTH);
	memset(StreetPilotLnk_ident,0,MAX_LENGTH);
	memset(record.comment,0,40);
	memset(record.ident,0,6);
	strncpy((char*) StreetPilotWptName, D106_Wpt->wpt_ident,255);
	strncpy(record.comment,(char*)StreetPilotWptName,40);
	sprintf(record.ident,"SPW%03d",SPWptNo);
	record.gclass = D106_Wpt->gclass;
	for(i=0;i<13;i++) record.subclass[i]=D106_Wpt->subclass[i];
	strncpy((char*)StreetPilotWptName, D106_Wpt->wpt_ident,255);
	p=(unsigned char *)&D106_Wpt->wpt_ident;

	while(*p)
		p++; // look for null of string
	p++; // skip over null
	strncpy((char*)StreetPilotLnk_ident,(char*) p, 255);
} // readD106

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
// D108_Wpt_Type
//  Example products: eMap
//typedef struct 														// size
//{
//	byte			wpt_class; 		// class (see below) 				1
//	byte			color; 			// color (see below) 				1 
//	byte			dspl; 			// display options (see below) 		1
//	byte			attr; 			// attributes (see below) 			1
//	Symbol_Type		smbl; 			// waypoint symbol 					2
//	byte			subclass[18]; 	// subclass 						18
//	Semicircle_Type	posn; 			// 32 bit semicircle 				8
//	float 			alt; 			// altitude in meters 				4
//	float 			dpth; 			// depth in meters 					4
//	float 			dist; 			// proximity distance in meters		4
//	char 			state[2]; 		// state 							2
//	char 			cc[2]; 			// country code 					2
// char 			ident[]; 			variable length string 			1-51
// char 			comment[]; 			waypoint user comment 			1-51
// char 			facility[]; 		facility name 					1-31
// char 			city[]; 			city name 						1-25
// char 			addr[]; 			address number 					1-51
// char 			cross_road[]; 		intersecting road label 		1-51
//} D108_Wpt_Type;

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

/* -------------------------------------------------------------------------- */
//
//  in points to a string which contains null terminated values.
//
void SplitOutD108Data(char *in, char *a, char *b, char *c, char *d, char *e, char *f)
{
	int done;
	char *p;

	p=a;
	done =0;
	while(!done) {
		if(*in==0)  done=1;
		*p++ = *in++;
	} //!done

	p=b;
	done =0;
	while(!done) {
		if(*in==0)  done=1;
		*p++ = *in++;
	} //!done

	p=c;
	done =0;
	while(!done) {
		if(*in==0)  done=1;
		*p++ = *in++;
	} //!done

	p=d;
	done =0;
	while(!done) {
		if(*in==0)  done=1;
		*p++ = *in++;
	} //!done

	p=e;
	done =0;
	while(!done) {
		if(*in==0)  done=1;
		*p++ = *in++;
	} //!done

	p=f;
	done =0;
	while(!done) {
		if(*in==0)  done=1;
		*p++ = *in++;
	} //!done

} // SplitOutD108Data

/* -------------------------------------------------------------------------- */
void readD108(D108_Wpt_Type *D108_Wpt)
{
	//eMap --
	//Date/Time  Not unused in D108, so set it to system time
	//why? settime_record(0);
	record.gclass = D108_Wpt->wpt_class;
 	record.la = toDegrees(D108_Wpt->posn.lat);
 	record.lo = toDegrees(D108_Wpt->posn.lon);
 	//set_ns();
 	record.icon_smbl=D108_Wpt->smbl;
 	memcpy(record.subclass,D108_Wpt->subclass,18);
	record.altitude=D108_Wpt->alt;

	SplitOutD108Data(D108_Wpt->ident,record.ident,
	                                 record.comment,
									 record.facility,
									 record.city,
	                                 record.addr,
									 record.cross_road);
} // readD108

/* -------------------------------------------------------------------------- */
void readD109(D109_Wpt_Type *D109_Wpt)
{
	//eMap --
	//Date/Time  Not unused in D109, so set it to system time
	// why? settime_record(0);
	record.gclass = D109_Wpt->wpt_class;
 	record.la = toDegrees(D109_Wpt->posn.lat);
 	record.lo = toDegrees(D109_Wpt->posn.lon);
 	//set_ns();
 	record.icon_smbl=D109_Wpt->smbl;
 	memcpy(record.subclass,D109_Wpt->subclass,18);
	record.altitude=D109_Wpt->alt;

	SplitOutD108Data(D109_Wpt->ident,record.ident,
	                                 record.comment,
									 record.facility,
									 record.city,
	                                 record.addr,
									 record.cross_road);
} // readD109

/* -------------------------------------------------------------------------- */
void readD110(D110_Wpt_Type *D110_Wpt)
{
	settime_record(D110_Wpt->time);
	record.gclass = D110_Wpt->wpt_class;
 	record.la = toDegrees(D110_Wpt->posn.lat);
 	record.lo = toDegrees(D110_Wpt->posn.lon);
 	//set_ns();
 	record.icon_smbl=D110_Wpt->smbl;
 	memcpy(record.subclass,D110_Wpt->subclass,18);
	record.altitude=D110_Wpt->alt;

	SplitOutD108Data(D110_Wpt->ident,record.ident,
	                                 record.comment,
									 record.facility,
									 record.city,
	                                 record.addr,
									 record.cross_road);
} // readD110

/* -------------------------------------------------------------------------- */
void readD150(D150_Wpt_Type *D150_Wpt)
{
	strncpy(record.ident, D150_Wpt->ident, 6);
	strncpy(record.cc, D150_Wpt->cc,2);
	record.gclass = D150_Wpt->gclass;
	record.la = D150_Wpt->posn.lat;
	record.lo = D150_Wpt->posn.lon;
	//set_ns();
	record.altitude=record.alt = D150_Wpt->alt;
	record.altitude=NO_ALTITUDE;
	strncpy(record.city, D150_Wpt->city, 24);
	strncpy(record.state, D150_Wpt->state, 2);
	strncpy(record.name, D150_Wpt->name, 30);
	strncpy(record.comment, D150_Wpt->cmnt, 40);
} // readD150

/* -------------------------------------------------------------------------- */
static void doWaypoint(INTTYP wpt)
{
	BYTE *pp;

	set_record_icon();
	pp = (BYTE *)(&SerialMessage[3]); 
	datatype=wpt;

	switch(wpt) {
		case 100:
		case 101:
		case 102:
		case 103:
		case 104:
		case 107:
		case 151:
		case 152:
		case 154:
		case 155:
		case 400:
		case 403:
				readD100((D100_Wpt_Type*)pp, wpt);
				break;

		case 105:
				D105orD106=1;
				readD105((D105_Wpt_Type *)pp);
				break;

		case 106:
				D105orD106=1;
				readD106((D106_Wpt_Type *)pp);
				break;

		case 108:
				readD108((D108_Wpt_Type *)pp);
				break;
		case 109:
				readD109((D109_Wpt_Type *)pp);
				break;
		case 110:
				readD110((D110_Wpt_Type *)pp);
				break;
		case 150:
		case 450:
				readD150((D150_Wpt_Type *)pp);
				break;
		default:
			sprintf(gps_errmsg,"Unknown waypoint protocol: D%d",wpt);
			byebye(-1);
	} // switch(wpt)
//
// convert symbols if ID is of a 12/II+ gps
//
	//Don't bother (DMcK): if(Ggps==G12XL) record.icon_smbl=convert_from_12xl(record.icon_smbl);
	//sprintf(Text,"W;%d;%s;%s",datatype,record.ident,record.comment);
	//rtrim(Text);
	push_waypoint();
	D105orD106=0;
} // doWaypoint

/* -------------------------------------------------------------------------- */
static void doTrack()
{
	D300_Trk_Point_Type *D300_Trk_Point=(D300_Trk_Point_Type *)&SerialMessage[3];
	D301_Trk_Point_Type *D301_Trk_Point;
	D302_Trk_Point_Type *D302_Trk_Point;
	BOOL bNew;

	if(TrackDataRecord==300) { //GPS III, etc.
		bNew = D300_Trk_Point->new_trk!=0;
		record.altitude=NO_ALTITUDE;
	}
	else if(TrackDataRecord==310) { //GPS V
		D301_Trk_Point = (D301_Trk_Point_Type *)D300_Trk_Point;
		bNew = D301_Trk_Point->new_trk!=0;
		record.altitude=D301_Trk_Point->alt;
	}
	else if(TrackDataRecord==312) { //60csx
		D302_Trk_Point = (D302_Trk_Point_Type *)D300_Trk_Point;
		bNew = D302_Trk_Point->new_trk!=0;
		record.altitude=D302_Trk_Point->alt;
	}
	else {
		sprintf(gps_errmsg,"Unknown track data protocol: D%d",TrackDataRecord);
		byebye(-1);
	}

	datatype=TrackDataRecord;

	settime_record((time_t)D300_Trk_Point->time);

	record.type = bNew?'N':'T';
	record.la = toDegrees(D300_Trk_Point->posn.lat);
	record.lo = toDegrees(D300_Trk_Point->posn.lon);
	push_track();
} // doTrack

/* -------------------------------------------------------------------------- */
void doTime(void)
{
	D600_Date_Time_Type *dtime = (D600_Date_Time_Type *)&SerialMessage[3];

	record.year=dtime->year;
	record.monthn=dtime->month;
	record.dayofmonth=dtime->day;
	record.ht=dtime->hour;
	record.mt=dtime->minute;
	record.st=dtime->second;
	get_datetime_str();
} // doTime

/* -------------------------------------------------------------------------- */
void getGPSTime(void)
{
	*record.datetime=0;
	SendGarminMessage(GPStime,4);	/* ask for time */
	getGarminMessage(); 		/* get ack	*/
	getGarminMessage();		/* get time	*/
} /* getGPSTime */

/* -------------------------------------------------------------------------- */
void doID(void)
{
	BYTE *x;  
	BYTE *q;
	char *p;

	INTTYP i;

	Product_Data_Type *dd;

	if(gotID==1) return;
	dd=(Product_Data_Type *)&SerialMessage[3];

	x=&SerialMessage[2]; 			/* data length */

	Product_ID=dd->product_ID;
	SWVersion=dd->software_version;

	Ggps=0;
	switch(Product_ID) {
	    case GPS38_40_45_45XL_A:
	    case GPS38_40_45_45XL_B:
	    case GPSII:
	    case GPS38_C:
	    case GPS38_J:
	    case GPSIIP:
	    case GPS12_12XLA:
	    case GPS12:
	    case GPS12CX:
	    case GPS12_12XLB:
	    case GPSIIP_1:
	    case GPS12XL_J:
	    case GPS12XL_C:
	        Ggps=G12XL;
	        break;
	
	    case GPSIII_ID:
	    case GPSIIIPILOT:
	        Ggps=GPSIII;
	        break;
	
	    case GPS3P:
	        Ggps=GPSIIIP;
	        break;
	
	    case STREETPILOT:
	    case STREETPILOTCOLORMAP:
	        Ggps=SPILOT;
	        break;
	}
//
// decode null terminated strings
//
	q=dd->a;
	p=getstrings((char*)q,"",0);


	if(p != NULL) {
		strcpy(VersionString,p);
		trim(VersionString);
		q+=(strlen(p)+1);
	}

	i=0;
	while(q < (dd->a + *x - 4)) {
		p=getstrings(NULL,"",1);
		if(p != NULL) {
			VersionSubString[i]=strdup(p);
			q+=(strlen(p)+1);
		}
		i++;
		VersionSubString[i]=NULL;
	}
	VersionSubStrings=i;
	gotID=1;
} /* doID */

/* -------------------------------------------------------------------------- */
void doEOFRec(void)
{
	AlmanacEOF=RouteEOF=TrackEOF=ProxWaypointEOF=WaypointEOF=0;
	switch(SerialMessage[3]) {
		case 1: AlmanacEOF=1; break;
		case 3: ProxWaypointEOF=1;break;
		case 4: RouteEOF=1; break;
		case 6: TrackEOF=1; break;
		case 7: WaypointEOF=1; break;
		default: break;
	}
} /* doEOFRec */

/* -------------------------------------------------------------------------- */
void AsyncSend(BYTE *message, int bytes)
{
	AsyncOut(message,bytes);
} // AsyncSend()

/* -------------------------------------------------------------------------- */
void SendGarminMessage(BYTE *message, short bytes)
{
	short i;
	INTTYP n;
	BYTE csum = 0;
	BYTE *p = SerialMessageOut;

	ok=-1;	
//
// Frame start byte
//
	*p++ = 0x10;    /* Start of message */
//
// Transfer message buffer to transmit buffer duplicating any 0x10 bytes
//
	for (i=0; i<bytes; i++) {
		*p++ = message[i];
		csum -= message[i];
		if (message[i] == 0x10)
			*p++ = 0x10;
	}
//
// Add in checksum
//
	*p++ = csum;
	if (csum == 0x10) *p++ = 0x10;
//
// Add frame trailer
//
	*p++ = 0x10;
	*p++ = 0x03;
//
// Message length in bytes
//
	n =(int)(p - SerialMessageOut);

//
// 'virus..check?  a 0x2b record apparently causes the erasure of
//  the GPS's program.
//
	if(SerialMessageOut[1]==0x2b) byebye(GPS_ERR_SENDRECORD);
//
// Save message
//
	if(strcmp(message,ack1)) {
		LastGarminLen=bytes;  		// Length w/o header, checksum and trailer
		memcpy(LastGarminGPSMsg,message,bytes);
	}
	AsyncSend(SerialMessageOut, n);
} /* SendGarminMessage() */

//-------------------------------------------------------------------------- 
/*
 * checksums an incoming GPS record *after* repeating 0x10 characters are
 *  removed
 */
INTTYP checksum(unsigned char *m,INTTYP l)
{
	INTTYP i;
	unsigned char cs;

	cs=0;
	for(i=1;i<l-3;i++) {
		cs-=m[i];
	}
	if(cs==m[l-3]) return 1;
	return 0;
} /* checksum */

/* -------------------------------------------------------------------------- */
// Garmin message type characters
//  ACK	     0x06	 last record received was ok
//  ALM 	 0x1f	 Almanac record
//  EOFREC   0x0c	 EOF leadin for transters
//  GPSpos	 0x11	 GPS Position record
//  PROX  	 0x13	 Proximity Waypoint record
//  MYID	 0xff	 Version/ID record
//  NAK	     0x15	 last record received was bad
//  NREC	 0x1b	 number of records to receive
//  RTE_NAM  0x1d	 Route name record
//  RTE_WPT  0x1e	 Route waypoint record
//  TIMEREC  0x0e	 UTC Time record 
//  TRK 	 0x22	 Track record
//  WPT 	 0x23	 Waypoint record
//  Volts	 0x28	 Voltages
//  SENDID   0xfe    Send your ID.  Probably a bogus record
//
// L001 - Link Protocol 1
//
//enum {
//	Pid_Command_data	=  10, // 0x0a
//	Pid_Xfer_Cmplt		=  12, // 0x0c			EOFREC
//	Pid_Date_Time_Data	=  14, // 0x0e			TIMEREC
//	Pid_Position_Data	=  17, // 0x11			GPSpos
//	Pid_Prx_Wpt_Data	=  19, // 0x13			PROX
//	Pid_Records			=  27, // 0x1b			NREC
//	Pid_Rte_Hdr			=  29, // 0x1d			RTE_NAM
//	Pid_Rte_Wpt_Data	=  30, // 0x1e			RTE_WPT
//	Pid_Almanac_Data	=  31, // 0x1f			ALM
//	Pid_Trk_Data		=  34, // 0x22			TRK
//	Pid_Wpt_Data		=  35, // 0x23			WPT
//	Pid_Pvt_Data		=  51, // 0x33
//	Pid_Protocol_Array	= 253  // 0xfd
//};
//
////
//// L002 - Link Protocol 2
////
//enum {
//	Pid_Almanac_Data2	=  4, // 0x04
//	Pid_Command_Data2	= 11, // 0x0b
//	Pid_Xfer_Cmplt2		= 12, // 0x0c			EOFREC
//	Pid_Date_Time_Data2	= 20, // 0x14
//	Pid_Position_Data2	= 24, // 0x18
//	Pid_Records2		= 35, // 0x23
//	Pid_Rte_Hdr2		= 37, // 0x25
//	Pid_Rte_Wpt_Data2	= 39, // 0x27
//	Pid_Wpt_Data2		= 43  // 0x2b
//};
//
/* -------------------------------------------------------------------------- */
void parseGPS(void)
{	
//
// SerialMessage[0] is always 0x10 in a valid message string
//
// Product data, ACK and NAK are common to all Garmin Units
//
	switch(SerialMessage[1]) { // All units support ACK, NAK, Pid_Protocol_Data
		case Pid_Product_Data:		/* Version/ID record */
			//Pid_Product_Data record
			doID();
			return;

		case Pid_Ext_Product_Data:
			{
			}
			return;

		case Pid_Protocol_Array:
			doProtocolArray();
			//Pid_Protocol_Array record
			return;

		case Pid_Ack_Byte:
			//Pid_Ack_Byte record
			ok=1;
			return;

		case Pid_Nak_Byte:
			//Pid_Nak_Byte record
			ok=0;
			return;

	//	case Pid_Display_Data:
			//Pid_Display_Data record
			//doDisplay();
			//return;

	} // switch(SerialMessage[1]) #1

	if(LinkProtocol==1) {

		switch (SerialMessage[1]) {

		case Pid_Xfer_Cmplt:
			//Pid_Xfer_Cmplt record
			doEOFRec();
			return;
		
		case Pid_Rte_Hdr:	/* Route name record */
			//Pid_Rte_Hdr record
			//doRouteName();
			return;

		case Pid_Rte_Wpt_Data:	/* Route waypoint record */
			//Pid_Rte_Wpt_Data record
			doWaypoint(RouteDataRecord);
			return;

		case Pid_Almanac_Data:		/* Almanac record */
			//Pid_Almanac_Data record
			//doAlmanac();
			return;

		case Pid_Trk_Data:		/* Track record */
			//Pid_Trk_Data record
			doTrack();
			return;

		case Pid_Trk_Hdr:		/* Track header record (GPS V)*/
			strcpy(record.ident,&SerialMessage[5]);
			return;

		case Pid_Prx_Wpt_Data:		/* Waypoint record */
			//Pid_Prx_Wpt_Data record
			//doProxWaypoint();
			return;

		case Pid_Wpt_Data:		/* Waypoint record */
			//Pid_Wpt_Data record
			doWaypoint(WaypDataRecord);
			return;

		case Pid_Records:
			//Pid_Records # of Records record
			numrecords=*((WORD *)(SerialMessage+3));
			return;

		case Pid_Date_Time_Data:
			//Pid_Date_Time_Data record
			doTime();
			return;

		case Pid_Position_Data:
			//Pid_Position_Data record
			//doGPSPos();
			return;

		case Volts:
			//Voltages record
			//doGPSVolts();
			return;

		case Pid_Pvt_Data:
			//PVT data record
			//doPVT();
			return;

		case SENDID:
			//Received unexpected SEND ID
			return;

		default:
			 //New Garmin Record ID found
			return;
 	    } // switch(SerialMessage[1]) #2
	}

	if(LinkProtocol==2)
		switch(SerialMessage[1]) {

		case Pid_Xfer_Cmplt2:
			//Pid_Xfer_Cmplt2 record
			doEOFRec();
			return;
		
		case Pid_Wpt_Data2:		/* Waypoint record */
			//Pid_Wpt_Data2 record
			doWaypoint(WaypDataRecord);
			return;

		case Pid_Rte_Hdr2:	/* Route name record */
			//Pid_Rte_Hdr2 record
			//doRouteName();
			return;

		case Pid_Rte_Wpt_Data2:	/* Route waypoint record */
			//Pid_Rte_Wpt_Data2 record
			doWaypoint(RouteDataRecord);
			return;

		case Pid_Almanac_Data2:		/* Almanac record */
			//Pid_Almanac_Data2 record
			//doAlmanac();
			return;

		case Pid_Records2:
			//Pid_Records2 # of Records record
			numrecords=*((WORD *)(SerialMessage+3));
			return;

		case Pid_Date_Time_Data2:
			//Pid_Date_Time_Data2 record
			doTime();
			return;

		case Pid_Position_Data2:
			//Pid_Position_Data2 record
			//doGPSPos();
			return;

		} // switch(SerialMessage[1]) #2

		if(go_on==1) return;

		if(LinkProtocol==-1) byebye(GPS_ERR_PROTVIOL);
		byebye(GPS_ERR_PROTUNK);

} /* parseGPS */

/* -------------------------------------------------------------------------- */
void send_num_points(int num1, char *bb)
{
/*
 * send number of records to send
 */
	*((short*)(bgnxfr+2)) = num1;
	SendGarminMessage(bgnxfr, 4);
	getGarminMessage();	/* make sure GPS receiver responded */
} /* send_num_points */

/* -------------------------------------------------------------------------- */
/*
 *	getGarminMessage() returns the number of bytes in the message. If the
 *	timeout period has lapsed without completing a message, 0 is returned.
 *  
 *  The string SerialMessage contains the entire data string returned
 *   from the GPS
 */
void getGarminMessage(void)
{
	extern short gInRefNum;
	INTTYP fails, GPSchecksum;
	time_t t;
	BYTE c, *p, last;
	short length, ne;
	
	int Garmin_NAKS_Sent=0;

redoit:
	p = SerialMessage;
	length = SerialMsgLen = ne = last = 0;
	memset(SerialMessage,0,MAX_LENGTH);
	t = time(NULL);
	fails=0;
	for (;;) {
		if ((time(NULL)-t) > TIMEOUT) {
			if(Walls_CB(0)==-1) byebye(GPS_ERR_CANCELLED);
			if(++fails>3) {
				byebye(GPS_ERR_NORESPOND);
				return;
			}
			t=time(NULL);
			//
			// Re-send last message
			//
			SendGarminMessage(LastGarminGPSMsg,LastGarminLen);
		}
		while (AsyncInStat()) {
			fails=0;
			if (length >= MAX_LENGTH-1) {
				sprintf(gps_errmsg,"GPS communication length > %d",length);
				byebye(-1);
			}
			c=(BYTE)AsyncIn();
			t = time(NULL);  /* reset time */

			if(c==0x03 && last==0x10)
				ne = 1;
			else
				ne = 0;
			
			if (c == 0x10 && last == 0x10)
				last = 0;
			else {
				last = *p++ = c;
				++length;
			}

			if (*(p-1) == 0x03 && *(p-2) == 0x10 && ne) {
				GPSchecksum=checksum(SerialMessage,length);
				if(GPSchecksum==0) {
					SendGarminMessage(nak1,4);   /* send NAK */
					if(++Garmin_NAKS_Sent<7) goto redoit;
					byebye(GPS_ERR_MAXNAKS);
				}
				ack1[2]=SerialMessage[1];	// setup proper ack response
				if(SerialMessage[1]!=0x06) // Don't ACK an ACK!
					SendGarminMessage(ack1,4);   /* send ACK */
				SerialMsgLen=length;
				parseGPS();
				return;
			} //if (*(p-1) == 0x03 && *(p-2) == 0x10 && ne) 
		} // while 
	} // for (;;)
} /* getGarminMessage() */

/* -------------------------------------------------------------------------- */
/*
 * Gets records from the GPS.  recordtype holds the type of record
 *  the program expects to see
 */
void getGPSrecords(void)
{
	INTTYP cc,i;
//
// get number of records to send
//
	getGarminMessage();

	if(numrecords==-2) return;
	if(numrecords<0) byebye(GPS_ERR_NUMRECS);
	if(numrecords==0) {
		getGarminMessage(); /* get EOF */
		return;
	}
	nak1[2]=recordtype;			// setup proper NAK response
	for(i=1;i<=numrecords;i++) {
		sprintf(gps_errmsg,"%s:%5d of %d",(recordtype==TRK)?"Track pts":"Waypoints",i,numrecords);
		if(cc=Walls_CB(gps_errmsg)) {
			SendGarminMessage(Gabort,4); // Abort the transfer
			if(cc==-1) byebye(GPS_ERR_CANCELLED);
			numrecords=0;
			break;
		}
		getGarminMessage(); /* get record */
		/*
		if(recordtype==WPT && bWptFound==2) {
			SendGarminMessage(Gabort,4); // Abort the transfer
			bWptFound=1;
			WaypointEOF=TRUE;
			break;
		}
		*/
	}
} /* getGPSrecords */

/* -------------------------------------------------------------------------- */
#pragma pack(1)
typedef struct
{
	byte tag;
	word data;
} Protocol_Data_Type;
#pragma pack()

Protocol_Data_Type (*a)[1];

// Decodes Protocol Capability Array
void doProtocolArray(void)
{
	INTTYP i;

// GPS 12XL ver 3.53:
// Received:
// 10   fd   3c   50P  00   00   4cL  01   00   41A  0a   00   41A  64d  00   44D  67g  00
// 41A  c8   00   44D  c9   00   44D  67g  00   41A  2c   01   44D  2c   01   41A  90   01
// 44D  93   01   41A  f4   01   44D  f5   01   41A  58X  02   44D  58X  02   41A  bc   02
// 44D  bc   02   41A  20   03   44D  20   03   d0   10   03  
// Pid_Protocol_Array record

// GPS II+ ver 2.07:
// Received:
// 10   fd   366  50P  00   00   4cL  01   00   41A  0a   00   41A  64d  00   44D  67g  00
// 41A  c8   00   44D  c9   00   44D  67g  00   41A  2c   01   44D  2c   01   41A  f4   01
// 44D  f5   01   41A  58X  02   44D  58X  02   41A  bc   02   44D  bc   02   41A  20   03
// 44D  20   03   80   10   03  
// Pid_Protocol_Array record
//
	a=(void *)&SerialMessage[3];
//
// 0 is a valid value, so Preset values to -1
//
	CommandProtocol=WaypXferProtocol=WaypDataRecord=RouteXferProtocol=RouteHdrType=-1;
	RouteDataRecord=TrackXferProtocol=TrackDataRecord=ProxXferProtocol=ProxDataRecord=-1;
	AlmanacXferProtocol=AlmanacDataRecord=UTCXferProtocol=UTCDataRecord=-1;
	PositionXferProtocol=PositionDataRecord=PVTXferProtocol=PVTDataRecord=-1;
	LinkProtocol=PhysicalProtocol=-1;

	for(i=0;i<(SerialMsgLen-6)/3;i++) {
		switch(a[i]->tag) {
			case 'A':
/////=================================================================================================
					if(a[i]->data >= 10 && a[i]->data <= 99) {
							CommandProtocol=a[i]->data;
					}
					else 
					if((a[i]->data>=100) && (a[i]->data<=199)) {
							WaypXferProtocol=a[i]->data;
							i++; // if(a[++i]->tag!='D') printArray();// exits
							WaypDataRecord=a[i]->data;
					} // 100-199: WaypXfrProtocol
					else
					if((a[i]->data>=200) && (a[i]->data<=299)) {
							RouteXferProtocol=a[i]->data;
  							i++; //if(a[++i]->tag!='D') printArray();// exits
							RouteHdrType=a[i]->data;
 							i++; //if(a[++i]->tag!='D') printArray();// exits
							RouteDataRecord=a[i]->data;
					} // 200-299: RouteXfrProtocol & RouteHdrType
					else
					if((a[i]->data>=300) && (a[i]->data<=399)) {
							TrackXferProtocol=a[i]->data;
 							i++; //if(a[++i]->tag!='D') printArray();// exits
							TrackDataRecord=a[i]->data;
					} // 300-399: TrackDataRecord
					else
					if((a[i]->data>=400) && (a[i]->data<=499)) {
							ProxXferProtocol=a[i]->data;
 							i++; //if(a[++i]->tag!='D') printArray();// exits
							ProxDataRecord=a[i]->data;
					} // ProxXferProtocol & ProxDataRecord
					else
					if((a[i]->data>=500) && (a[i]->data<=599)) {
							AlmanacXferProtocol=a[i]->data;
 							i++; //if(a[++i]->tag!='D') printArray();// exits
							AlmanacDataRecord=a[i]->data;
					} // AlmanacXferProtocol & AlmanacDataRecord
					else
					if((a[i]->data>=600) && (a[i]->data<=699)) {
							UTCXferProtocol=a[i]->data;
 							i++; //if(a[++i]->tag!='D') printArray();// exits
							UTCDataRecord=a[i]->data;
					} // UTCXferProtocol & UTCDataRecord
					else
					if((a[i]->data>=700) && (a[i]->data<=799)) {
							PositionXferProtocol=a[i]->data;
 							i++; //if(a[++i]->tag!='D') printArray();// exits
							PositionDataRecord=a[i]->data;
					} // PositionXferProtocol & PositionDataRecord
					else
					if((a[i]->data>=800) && (a[i]->data<=899)) {
							PVTXferProtocol=a[i]->data;
 							i++; //if(a[++i]->tag!='D') printArray();// exits
							PVTDataRecord=a[i]->data;
					} // PVTXferProtocol & PVTDataRecord
					//else {
						//sprintf(gps_errmsg,"Undocumented Protocol: A%03d, ignored.", a[i]->data);
					//}
					break;

			case 'L': 
					LinkProtocol=a[i]->data;
					break;

			case 'P': PhysicalProtocol=a[i]->data; break;
		  } // switch(a[i]->tag)
	}
	haveProtocolArray=1;
} // doProtocolArray()

/* -------------------------------------------------------------------------- */
void setProtocolParams()
{
	INTTYP svi;
	INTTYP i;
	
	if(haveProtocolArray==1) return;

	i=0;
	while(Protocols[i][0] != Product_ID ) {
		if(Protocols[i][0]==-1) {
			byebye(GPS_ERR_PROTOCOL);
		}
		i++;
	}
// assume that the product ID found in the array matches.  Set the program
//  protocols to the protocols found in the table.
//
	ProtocolIndex=i;

//
//  If Version is != 0 then this is the first of at least 2 sub-codes.
//  If our version is less than the absolute value of the table entry then we use it.
//
	while(Product_ID == Protocols[i][0]) {
		if((svi=Protocols[i][1]) < 0) {
			svi*=(-1);
			if(SWVersion < svi) goto process;
			svi=Protocols[++i][1];
			if(svi < 0) {
				svi*=(-1);
				if(SWVersion < svi) goto process;
			}
			else if(SWVersion >= svi) goto process;
			i++;
			if(i>60) {
				byebye(GPS_ERR_PROTOCOL); //Was 9
			}
		} // if((svi=Protocols[i][1]) < 0)
		else break;
	} // while(Product_ID == Protocols[i][0])
//
// set program protocols
//
process:;
	if(Protocols[i][0] != Product_ID) {
		byebye(GPS_ERR_PROTOCOL);
	}
	Product_Ver 		= Protocols[i][1];
	LinkProtocol		= Protocols[i][2];
	CommandProtocol		= Protocols[i][3];

	WaypXferProtocol	= 100;
	WaypDataRecord 		= Protocols[i][4];

	RouteXferProtocol	= 200;
	RouteHdrType 		= Protocols[i][5];
	RouteDataRecord 	= Protocols[i][6];

	TrackXferProtocol	= Protocols[i][7];
	TrackDataRecord 	= Protocols[i][8];

	ProxXferProtocol	= Protocols[i][9];
	ProxDataRecord		= Protocols[i][10];

	AlmanacXferProtocol	= 500;
	AlmanacDataRecord 	= Protocols[i][11];

	UTCXferProtocol		= 600;
	UTCDataRecord		= 600;

	PositionXferProtocol= 700;
	PositionDataRecord	= 700;

	PVTXferProtocol		= -1;
	PVTDataRecord		= -1;

 	if(LinkProtocol==1) bgnxfr[0]=Pid_Records;
	if(LinkProtocol==2) bgnxfr[0]=Pid_Records2;

	switch(CommandProtocol) {
		case 10:
				Gabort[2]	= Cmnd_Abort_Transfer;
				alm1[2]		= Cmnd_Transfer_Alm;
				almt[2]		= Cmnd_Transfer_Alm;
				GPSPos[2]	= Cmnd_Transfer_Posn;
				pwpt1[2]	= Cmnd_Transfer_Prx;
				prxt[2] 	= Cmnd_Transfer_Prx;
				rte1[2]		= Cmnd_Transfer_Rte;
				rtet[2]		= Cmnd_Transfer_Rte;
				GPStime[2]	= Cmnd_Transfer_Time;
				trk1[2]		= Cmnd_Transfer_Trk;
				trkt[2]		= Cmnd_Transfer_Trk;
				wpt1[2]		= Cmnd_Transfer_Wpt;
				wptt[2]		= Cmnd_Transfer_Wpt;
				pwroff[2]	= Cmnd_Turn_Off_Pwr;
				VoltsM[2]	= Cmnd_Send_Voltages;
				PVTon[2] 	= Cmnd_Start_Pvt_Data;
				PVToff[2]	= Cmnd_Stop_Pvt_Data;
				break;
		case 11:
				Gabort[2]	= Cmnd_Abort_Transfer2;
				alm1[2]		= Cmnd_Transfer_Alm2;
				almt[2]		= Cmnd_Transfer_Alm2;
				rte1[2]		= Cmnd_Transfer_Rte2;
				rtet[2]		= Cmnd_Transfer_Rte2;
				wpt1[2]		= Cmnd_Transfer_Wpt2;
				wptt[2]		= Cmnd_Transfer_Wpt2;
				GPStime[2]	= Cmnd_Transfer_Time2;
				pwroff[2]	= Cmnd_Turn_Off_Pwr2;
				break;
		default: sprintf(gps_errmsg,"Unknown Command Protocol: A%03d",CommandProtocol);
				 byebye(-1);
	} // switch(CommandProtocol)

} // setProtocolParams

/* -------------------------------------------------------------------------- */
void getID(void)
{
	if(gotID==1) return;

	//Misc init --
	SPWptNo=0;
	D105orD106=0;
	firstwaypoint=1; //for sending only
	firsttrackpoint=1;
	LinkProtocol=-1;
	newTrack=255;
	haveProtocolArray=0;
	ProtocolIndex=-1;
	go_on=1;

	SendGarminMessage(Gabort,4);	// JIC Abort the transfer
	getGarminMessage();						// get ack to  abort
	SendGarminMessage(m1,3);		// Send your ID
	ok=0;
	getGarminMessage();			// get ack to Send your ID
	if(ok==0) {						// try again
		SendGarminMessage(m1,3);	// Send your ID
		getGarminMessage();					// get ack to Send your ID
	}
	getGarminMessage();			// read the ID
	delay(300); 				// allow unit to respond with Protocol Array if supported
	if(AsyncInStat()) {			// Assume that if we have a character then
		getGarminMessage();		// this should be either the Protocol Array or Ext_Product_Data that preceeds it.
		if(SerialMessage[1]==Pid_Ext_Product_Data) {
			delay(300);  // allow unit to respond with Protocol Array
			if(AsyncInStat()) {
				getGarminMessage();		// This should be the Protocol Array
			}
		}
	}
	if(haveProtocolArray==0) {
		setProtocolParams(); // If not, try the table
	}
	go_on=0;
	gotID=1;
	bTrkAlt=(TrackDataRecord==310 || TrackDataRecord==312);
	bWptAlt=(WaypDataRecord==109 || WaypDataRecord==110 || WaypDataRecord==108 || (WaypDataRecord>=150 && WaypDataRecord<=155));
} // getID

/* -------------------------------------------------------------------------- */
void getWaypoints(void)
{
	recordtype=WPT;
	SendGarminMessage(wpt1,4);
	getGarminMessage();	/* make sure GPS receiver responded */
	getGPSrecords();
	if(numrecords<=0) return;
	getGarminMessage(); /* get WaypointEOF */
	if(!WaypointEOF) byebye(GPS_ERR_NOWPEOF);
} /* getWaypoints */

/* -------------------------------------------------------------------------- */
void getTrack(void)
{
	recordtype=TRK;
	SendGarminMessage(trk1,4);
	getGarminMessage();	/* make sure GPS receiver responded */
	getGPSrecords();
	if(numrecords<=0) return;
	getGarminMessage(); /* get TrackEOF */
	if(!TrackEOF) byebye(GPS_ERR_NOTRKEOF);
} /* getTrack */
