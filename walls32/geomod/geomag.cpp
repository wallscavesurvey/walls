/* PROGRAM MAGPOINT (GEOMAG DRIVER) */
/************************************************************************
     Contact Information

     Software and Model Support
     	National Geophysical Data Center
     	NOAA EGC/2
     	325 Broadway
     	Boulder, CO 80303 USA
     	Attn: Susan McLean or Stefan Maus
     	Phone:  (303) 497-6478 or -6522
     	Email:  Susan.McLean@noaa.gov or Stefan.Maus@noaa.gov
		Web: http://www.ngdc.noaa.gov/seg/WMM/

     Sponsoring Government Agency
	   National Geospatial-Intelligence Agency
    	   PRG / CSAT, M.S. L-41
    	   3838 Vogel Road
    	   Arnold, MO 63010
    	   Attn: Craig Rollins
    	   Phone:  (314) 263-4186
    	   Email:  Craig.M.Rollins@Nga.Mil

      Original Program By:
        Dr. John Quinn
        FLEET PRODUCTS DIVISION, CODE N342
        NAVAL OCEANOGRAPHIC OFFICE (NAVOCEANO)
        STENNIS SPACE CENTER (SSC), MS 39522-5001
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void E0000(int IENTRY, int *maxdeg, float alt,float glat,float glon, float time, float *dec, float *dip, float *ti, float *gv);
void geomag(int *maxdeg);
void geomg1(float alt, float glat, float glon, float time, float *dec, float *dip, float *ti, float *gv);
char geomag_introduction(float epochlowlim);

void main()
{
//  extern void geomag(), geomg1();
//  char geomag_introduction(float);
  static int maxdeg;
  static float altm, dlat, dlon;
  static float ati, adec, adip;
  static float alt, time, dec, dip, ti, gv;
  static float time1, dec1, dip1, ti1;
  static float time2, dec2, dip2, ti2;
  char answer, ans;
  char decd[5], dipd[5],d_str[81],modl[20];
  char goodbye[81];
  float x1,x2,y1,y2,z1,z2,h1,h2;
  float ax,ay,az,ah;
  float rTd=0.017453292;
  float epochlowlim,epochuplim;
  float epochrange = 5.0;
  
  FILE *wmmtemp;
  wmmtemp = fopen("WMM.COF","r");
  fgets(d_str, 80, wmmtemp);
  sscanf(d_str,"%f%s",&epochlowlim,modl);
  fclose(wmmtemp);
  strcpy (goodbye,"\n End of WMM Point Calculation Program \n");
  ans = geomag_introduction(epochlowlim);
  
  if ((ans == 'y') || (ans == 'Y')) 
    {
      
/* INITIALIZE GEOMAG ROUTINE */
      
    S1:
      
      maxdeg = 12;
      
      geomag(&maxdeg);
      
      
      printf("\n\n\nENTER LATITUDE IN DECIMAL DEGREES ");
      printf("\n(North latitude positive, South latitude negative \n");
      printf("i.e. 25.5 for 2.5 degrees 30 minutes north.) \n");
      scanf("%f", &dlat);
      
      printf("ENTER LONGITUDE IN DECIMAL DEGREES");
      printf("(East longitude positive, West negative \n"); 
      printf("i.e.- 100.0 for 100.0 degrees west.)\n");
      scanf("%f", &dlon);
      
      printf("ENTER ALTITUDE IN METERS\n");
      scanf("%f", &altm);
      alt = altm/1000;
      
      epochuplim = epochlowlim + epochrange;
      printf("ENTER TIME IN DECIMAL YEAR (%-7.2f - %-7.2f)\n",epochlowlim,epochuplim);
      scanf("%f",&time);
      
      geomg1(alt,dlat,dlon,time,&dec,&dip,&ti,&gv);
      time1 = time;
      dec1 = dec;
      dip1 = dip;
      ti1 = ti;
      time = time1 + 1.0;
      
      geomg1(alt,dlat,dlon,time,&dec,&dip,&ti,&gv);
      time2 = time;
      dec2 = dec;
      dip2 = dip;
      ti2 = ti;
      
/*COMPUTE X, Y, Z, AND H COMPONENTS OF THE MAGNETIC FIELD*/
      
      x1=ti1*(cos((dec1*rTd))*cos((dip1*rTd)));
      x2=ti2*(cos((dec2*rTd))*cos((dip2*rTd)));
      y1=ti1*(cos((dip1*rTd))*sin((dec1*rTd)));
      y2=ti2*(cos((dip2*rTd))*sin((dec2*rTd)));
      z1=ti1*(sin((dip1*rTd)));
      z2=ti2*(sin((dip2*rTd)));
      h1=ti1*(cos((dip1*rTd)));
      h2=ti2*(cos((dip2*rTd)));

/*  COMPUTE ANNUAL CHANGE FOR TOTAL INTENSITY  */
      ati = ti2 - ti1;

/*  COMPUTE ANNUAL CHANGE FOR DIP & DEC  */
      adip = (dip2 - dip1) * 60.;
      adec = (dec2 - dec1) * 60.;


/*  COMPUTE ANNUAL CHANGE FOR X, Y, Z, AND H */
      ax = x2-x1;
      ay = y2-y1;
      az = z2-z1;
      ah = h2-h1;


      if (dec1 < 0.0) { 
		  strcpy (decd,"WEST");
      }
      else 
        { 
          strcpy(decd,"EAST");
        }

      if (dip1 < 0.0) 
        {
          strcpy(dipd,"DOWN");
        }
      else 
        {
          strcpy(dipd,"UP  ");
        }


      printf("\n Results For \n");
      printf("\n LATITUDE:    = %-7.2f Degrees",dlat); // put decision for West, East ,north, South
      printf("\n LONGITUDE:   = %-7.2f Degrees\n",dlon);
      printf("\n ALTITUDE     = %.2f  METERS",altm);
      printf("\n DATE         = %-5.1f\n",time1);

      printf("\n\t\t\t      OUTPUT\n\t\t\t      ------");

      printf("\n\nMAIN FIELD COMPONENTS\t\t\t   ANNUAL CHANGE");
      printf("\n---------------------\t\t\t   -------------\n");

      printf("\n TI          = %-7.0f nT\t\t   TI  = %-6.0f  nT/yr",ti1,ati);
      printf("\n HI          = %-7.0f nT\t\t   HI  = %-6.0f  nT/yr",h1,ah);
      printf("\n X           = %-7.0f nT\t\t   X   = %-6.0f  nT/yr ",x1,ax);
      printf("\n Y           = %-7.0f nT\t\t   Y   = %-6.0f  nT/yr ",y1,ay);
      printf("\n Z           = %-7.0f nT\t\t   Z   = %-6.0f  nT/yr ",z1,az);
      printf("\n DEC         = %-7.2f DEG %s\t\t DEC   = %-6.2f  MIN/yr",dec1,decd,adec); 
      printf("\n DIP         = %-7.2f DEG %s\t\t DIP   = %-6.2f  MIN/yr",dip1,dipd,adip); 


      printf("\n\n\nDO YOU NEED MORE POINT DATA? (y or n)\n");
      scanf("%s", &answer);
      if ((answer =='y')||(answer == 'Y')) goto S1;
      else 
        { 
          printf("%s",goodbye);
        }
	  
    }

  else
    {
      printf("%s",goodbye);
    }
  
  exit(0);
  
}
/*************************************************************************/

static void E0000(int IENTRY, int *maxdeg, float alt, float glat, float glon, float time, float *dec, float *dip, float *ti, float *gv)
{
  static int maxord,i,icomp,n,m,j,D1,D2,D3,D4;
  static float c[13][13],cd[13][13],tc[13][13],dp[13][13],snorm[169],
    sp[13],cp[13],fn[13],fm[13],pp[13],k[13][13],pi,dtr,a,b,re,
    a2,b2,c2,a4,b4,c4,epoch,gnm,hnm,dgnm,dhnm,flnmj,otime,oalt,
    olat,olon,dt,rlon,rlat,srlon,srlat,crlon,crlat,srlat2,
    crlat2,q,q1,q2,ct,st,r2,r,d,ca,sa,aor,ar,br,bt,bp,bpp,
    par,temp1,temp2,parp,bx,by,bz,bh;
  static char model[20], c_str[81], c_new[5];
  static float *p = snorm;
  char answer;
  
  FILE *wmmdat;

  switch(IENTRY){case 0: goto GEOMAG; case 1: goto GEOMG1;}
  
 GEOMAG:
  wmmdat = fopen("WMM.COF","r");
  
/* INITIALIZE CONSTANTS */
  maxord = *maxdeg;
  sp[0] = 0.0;
  cp[0] = *p = pp[0] = 1.0;
  dp[0][0] = 0.0;
  a = 6378.137;
  b = 6356.7523142;
  re = 6371.2;
  a2 = a*a;
  b2 = b*b;
  c2 = a2-b2;
  a4 = a2*a2;
  b4 = b2*b2;
  c4 = a4 - b4;
  
/* READ WORLD MAGNETIC MODEL SPHERICAL HARMONIC COEFFICIENTS */
  c[0][0] = 0.0;
  cd[0][0] = 0.0;
  
  fgets(c_str, 80, wmmdat);
  sscanf(c_str,"%f%s",&epoch,model);
 S3:
  fgets(c_str, 80, wmmdat);
/* CHECK FOR LAST LINE IN FILE */
  for (i=0; i<4 && (c_str[i] != '\0'); i++)
    {
      c_new[i] = c_str[i];
      c_new[i+1] = '\0';
    }
  icomp = strcmp("9999", c_new);
  if (icomp == 0) goto S4;
/* END OF FILE NOT ENCOUNTERED, GET VALUES */
  sscanf(c_str,"%d%d%f%f%f%f",&n,&m,&gnm,&hnm,&dgnm,&dhnm);
  if (m <= n)
    {
      c[m][n] = gnm;
      cd[m][n] = dgnm;
      if (m != 0)
        {
          c[n][m-1] = hnm;
          cd[n][m-1] = dhnm;
        }
    }
  goto S3;

/* CONVERT SCHMIDT NORMALIZED GAUSS COEFFICIENTS TO UNNORMALIZED */
 S4:
  *snorm = 1.0;
  for (n=1; n<=maxord; n++)
    {
      *(snorm+n) = *(snorm+n-1)*(float)(2*n-1)/(float)n;
      j = 2;
      for (m=0,D1=1,D2=(n-m+D1)/D1; D2>0; D2--,m+=D1)
        {
          k[m][n] = (float)(((n-1)*(n-1))-(m*m))/(float)((2*n-1)*(2*n-3));
          if (m > 0)
            {
              flnmj = (float)((n-m+1)*j)/(float)(n+m);
              *(snorm+n+m*13) = *(snorm+n+(m-1)*13)*sqrt(flnmj);
              j = 1;
              c[n][m-1] = *(snorm+n+m*13)*c[n][m-1];
              cd[n][m-1] = *(snorm+n+m*13)*cd[n][m-1];
            }
          c[m][n] = *(snorm+n+m*13)*c[m][n];
          cd[m][n] = *(snorm+n+m*13)*cd[m][n];
        }
      fn[n] = (float)(n+1);
      fm[n] = (float)n;
    }
  k[1][1] = 0.0;
  
  otime = oalt = olat = olon = -1000.0;
  fclose(wmmdat);
  return;
  
/*************************************************************************/

 GEOMG1:
  
  dt = time - epoch;
  if (otime < 0.0 && (dt < 0.0 || dt > 5.0))
    {      
      printf("\n\n WARNING - TIME EXTENDS BEYOND MODEL 5-YEAR LIFE SPAN");
      printf("\n CONTACT NGDC FOR PRODUCT UPDATES:");
      printf("\n         National Geophysical Data Center");
      printf("\n         NOAA EGC/2");
      printf("\n         325 Broadway");
      printf("\n         Boulder, CO 80303 USA");
      printf("\n         Attn: Susan McLean or Stefan Maus");
      printf("\n         Phone:  (303) 497-6478 or -6522");
      printf("\n         Email:  Susan.McLean@noaa.gov");
      printf("\n         or");
      printf("\n         Stefan.Maus@noaa.gov");
      printf("\n         Web: http://www.ngdc.noaa.gov/seg/WMM/");
      printf("\n\n EPOCH  = %.3lf",epoch);
      printf("\n TIME   = %.3lf",time);
      printf("\n Do you wish to continue (y or n) ?");
      scanf("%s",&answer);
      if ((answer == 'n') || (answer == 'N'))
        {
          printf("\n Do you wish to enter more point data (y or n)?");
          scanf("%s",&answer);
          if ((answer == 'y')||(answer == 'Y')) goto GEOMG1;
          else exit (0);
        }
    }

  pi = 3.14159265359;
  dtr = pi/180.0;
  rlon = glon*dtr;
  rlat = glat*dtr;
  srlon = sin(rlon);
  srlat = sin(rlat);
  crlon = cos(rlon);
  crlat = cos(rlat);
  srlat2 = srlat*srlat;
  crlat2 = crlat*crlat;
  sp[1] = srlon;
  cp[1] = crlon;

/* CONVERT FROM GEODETIC COORDS. TO SPHERICAL COORDS. */
  if (alt != oalt || glat != olat)
    {
      q = sqrt(a2-c2*srlat2);
      q1 = alt*q;
      q2 = ((q1+a2)/(q1+b2))*((q1+a2)/(q1+b2));
      ct = srlat/sqrt(q2*crlat2+srlat2);
      st = sqrt(1.0-(ct*ct));
      r2 = (alt*alt)+2.0*q1+(a4-c4*srlat2)/(q*q);
      r = sqrt(r2);
      d = sqrt(a2*crlat2+b2*srlat2);
      ca = (alt+d)/r;
      sa = c2*crlat*srlat/(r*d);
    }
  if (glon != olon)
    {
      for (m=2; m<=maxord; m++)
        {
          sp[m] = sp[1]*cp[m-1]+cp[1]*sp[m-1];
          cp[m] = cp[1]*cp[m-1]-sp[1]*sp[m-1];
        }
    }
  aor = re/r;
  ar = aor*aor;
  br = bt = bp = bpp = 0.0;
  for (n=1; n<=maxord; n++)
    {
      ar = ar*aor;
      for (m=0,D3=1,D4=(n+m+D3)/D3; D4>0; D4--,m+=D3)
        {
/*
   COMPUTE UNNORMALIZED ASSOCIATED LEGENDRE POLYNOMIALS
   AND DERIVATIVES VIA RECURSION RELATIONS
*/
          if (alt != oalt || glat != olat)
            {
              if (n == m)
                {
                  *(p+n+m*13) = st**(p+n-1+(m-1)*13);
                  dp[m][n] = st*dp[m-1][n-1]+ct**(p+n-1+(m-1)*13);
                  goto S50;
                }
              if (n == 1 && m == 0)
                {
                  *(p+n+m*13) = ct**(p+n-1+m*13);
                  dp[m][n] = ct*dp[m][n-1]-st**(p+n-1+m*13);
                  goto S50;
                }
              if (n > 1 && n != m)
                {
                  if (m > n-2) *(p+n-2+m*13) = 0.0;
                  if (m > n-2) dp[m][n-2] = 0.0;
                  *(p+n+m*13) = ct**(p+n-1+m*13)-k[m][n]**(p+n-2+m*13);
                  dp[m][n] = ct*dp[m][n-1] - st**(p+n-1+m*13)-k[m][n]*dp[m][n-2];
                }
            }
        S50:
/*
    TIME ADJUST THE GAUSS COEFFICIENTS
*/
          if (time != otime)
            {
              tc[m][n] = c[m][n]+dt*cd[m][n];
              if (m != 0) tc[n][m-1] = c[n][m-1]+dt*cd[n][m-1];
            }
/*
    ACCUMULATE TERMS OF THE SPHERICAL HARMONIC EXPANSIONS
*/
          par = ar**(p+n+m*13);
          if (m == 0)
            {
              temp1 = tc[m][n]*cp[m];
              temp2 = tc[m][n]*sp[m];
            }
          else
            {
              temp1 = tc[m][n]*cp[m]+tc[n][m-1]*sp[m];
              temp2 = tc[m][n]*sp[m]-tc[n][m-1]*cp[m];
            }
          bt = bt-ar*temp1*dp[m][n];
          bp += (fm[m]*temp2*par);
          br += (fn[n]*temp1*par);
/*
    SPECIAL CASE:  NORTH/SOUTH GEOGRAPHIC POLES
*/
          if (st == 0.0 && m == 1)
            {
              if (n == 1) pp[n] = pp[n-1];
              else pp[n] = ct*pp[n-1]-k[m][n]*pp[n-2];
              parp = ar*pp[n];
              bpp += (fm[m]*temp2*parp);
            }
        }
    }
  if (st == 0.0) bp = bpp;
  else bp /= st;
/*
    ROTATE MAGNETIC VECTOR COMPONENTS FROM SPHERICAL TO
    GEODETIC COORDINATES
*/
  bx = -bt*ca-br*sa;
  by = bp;
  bz = bt*sa-br*ca;
/*
    COMPUTE DECLINATION (DEC), INCLINATION (DIP) AND
    TOTAL INTENSITY (TI)
*/
  bh = sqrt((bx*bx)+(by*by));
  *ti = sqrt((bh*bh)+(bz*bz));
  *dec = atan2(by,bx)/dtr;
  *dip = atan2(bz,bh)/dtr;
/*
    COMPUTE MAGNETIC GRID VARIATION IF THE CURRENT
    GEODETIC POSITION IS IN THE ARCTIC OR ANTARCTIC
    (I.E. GLAT > +55 DEGREES OR GLAT < -55 DEGREES)

    OTHERWISE, SET MAGNETIC GRID VARIATION TO -999.0
*/
  *gv = -999.0;
  if (fabs(glat) >= 55.)
    {
      if (glat > 0.0 && glon >= 0.0) *gv = *dec-glon;
      if (glat > 0.0 && glon < 0.0) *gv = *dec+fabs(glon);
      if (glat < 0.0 && glon >= 0.0) *gv = *dec+glon;
      if (glat < 0.0 && glon < 0.0) *gv = *dec-fabs(glon);
      if (*gv > +180.0) *gv -= 360.0;
      if (*gv < -180.0) *gv += 360.0;
    }
  otime = time;
  oalt = alt;
  olat = glat;
  olon = glon;
  return;
}

/*************************************************************************/

void geomag(int *maxdeg)
{
  E0000(0,maxdeg,0.0,0.0,0.0,0.0,NULL,NULL,NULL,NULL);
}

/*************************************************************************/

void geomg1(float alt, float glat, float glon, float time, float *dec, float *dip, float *ti, float *gv)
{
  E0000(1,NULL,alt,glat,glon,time,dec,dip,ti,gv);
}

/*************************************************************************/

char geomag_introduction(float epochlowlim)
{
  char help;
  static char ans;
  
  printf("\n\n Welcome to the World Magnetic Model (WMM) %7.2f", epochlowlim);
  printf("\n This program estimates the strength and direction of ");
  printf("\n Earth's main magnetic field for a given point/area.");
  printf("\n Enter h for help and contact information or c to continue.");
  printf ("\n )");
  scanf("%s",&help);
  
  if ((help == 'h') || (help == 'H'))
    {
      printf("\n Help information ");
      
      printf("\n The World Magnetic Model (WMM) for %7.2f", epochlowlim);
      printf("\n is a model of Earth's main magnetic field.  The WMM");
      printf("\n is recomputed every five (5) years, in years divisible by ");
      printf("\n five (i.e. 2005, 2010).  See the contact information below");
      printf("\n to obtain more information on the WMM and associated software.");
      printf("\n ");
      printf("\n Input required is the location in geodetic latitude and");
      printf("\n longitude (positive for northern latitudes and eastern ");
      printf("\n longitudes), geodetic altitude in meters, and thedate of "); 
      printf("\n interest in years.");
      
      printf("\n\n\n The program computes the estimated magnetic Declination");
      printf("\n (D) which is sometimes called MAGVAR, Inclination (I), Total");
      printf("\n Intensity (F or TI), Horizontal Intensity (H or HI), Vertical");
      printf("\n Intensity (Z), and Grid Variation (GV). Declination and Grid");
      printf("\n Variation are measured in units of degrees and are considered"); 
      printf("\n positive when east or north.  Inclination is measured in units");
      printf("\n of degrees and is considered positive when pointing up (out of");
      printf("\n the Earth).  The WMM is reference to the WGS-84 ellipsoid and");
      printf("\n is valid for 5 years after the base epoch.");
      
      printf("\n\n\n It is very important to note that a  degree and  order 12 model,");
      printf("\n such as WMM, describes only the long  wavelength spatial magnetic ");
      printf("\n fluctuations due to  Earth's core.  Not included in the WMM series");
      printf("\n models are intermediate and short wavelength spatial fluctuations ");
      printf("\n that originate in Earth's mantle and crust. Consequently, isolated");
      printf("\n angular errors at various  positions on the surface (primarily over");
      printf("\n land, incontinental margins and  over oceanic seamounts, ridges and");
      printf("\n trenches) of several degrees may be expected.  Also not included in");
      printf("\n the model are temporal fluctuation of magnetospheric and ionospheric");
      printf("\n origin. . On the days during and immediately following magnetic storms,");
      printf("\n temporal fluctuations can cause substantial deviationsof the geomagnetic");
      printf("\n field  from model  values.  If the required  declination accuracy  is");
      printf("\n more stringent than the WMM  series of models provide, the user is");
      printf("\n advised to request special (regional or local) surveys be performed");
      printf("\n and models prepared. Please make requests of this nature to the");
      printf("\n National Geospatial-Intelligence Agency (NGA) at the address below.");
      
      printf("\n\n\n Contact Information");
      
      printf("\n  Software and Model Support");
      printf("\n	National Geophysical Data Center");
      printf("\n	NOAA EGC/2");
      printf("\n	325 Broadway");
      printf("\n	Boulder, CO 80303 USA");
      printf("\n	Attn: Susan McLean or Stefan Maus");
      printf("\n	Phone:  (303) 497-6478 or -6522");
      printf("\n	Email:  Susan.McLean@noaa.gov or Stefan.Maus@noaa.gov ");
      
      printf("\n\n\n Continue with program? (y or n) \n");
      scanf("%s", &ans);
    }
  else
    {
		ans = 'y';
      
    }
  
  return(ans);
}
