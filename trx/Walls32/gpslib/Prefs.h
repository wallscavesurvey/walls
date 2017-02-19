/*
   Note: The order of the seven items DMSITEM, DMMITEM, DDDITEM, UTMITEM, 
   BNGITEM ITMITEM and SUIITEM must not be changed. The function 
   doRadioButtons() in the file Prefs.c depends upon this order.
*/

#define OKITEM		1	/* OK button */
#define CANCELITEM	2	/* Cancel button */
#define LISTITEM	3	/* Datum list */
#define DESCITEM	4	/* Datum description static text item */
#define DMSITEM		5	/* D¡MM'SS.S" format radio button */
#define DMMITEM		6	/* D¡MM.MMM' format radio button */
#define DDDITEM		7	/* D.DDDDD format radio button */
#define UTMITEM		8	/* UTM format radio button */
#define BNGITEM		9	/* British Grid format radio button */
#define ITMITEM		10	/* Irish Grid format radio button */
#define SUIITEM		11	/* Swiss Grid format radio button */ 
#define CREATITEM	12	/* Text file creator edit text item */ 
#define	OFFSETITEM	13	/* Local time - UTC offset edit text item */
#define DUMMYITEM	14  /* used so that one can set up the default item. */

/* Resource ID's */

#define DIALOGID	129

/* Maximum length of description field */

#define DESC_LEN	30

enum FORMAT {DMS, DMM, DDD, UTM, BNG, ITM, KKJ};

struct PREFS{
	short  datum;
	enum   FORMAT format;
	double offset;
	char   Device[255];
};

void ChangePrefs(void);
