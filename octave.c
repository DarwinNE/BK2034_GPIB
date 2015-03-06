/*****************************************************************************
 *
 * Compiling and linking this application from command line:
 *        gcc octave.c -m32 -framework NI488 -o octave
 *
 * Davide Bucci March 2, 2015
 * 
 * GPL v.3, tested on MacOSX 10.6.8 with NI drivers IEEE488
 *****************************************************************************/

#ifdef __MACH__
#include <NI488/ni488.h>
#else
#include "ni488.h"
#endif
#include <stdio.h>
#include <strings.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

int Device = 0;                   /* Device unit descriptor                  */
int BoardIndex = 0;               /* Interface Index (GPIB0=0,GPIB1=1,etc.)  */

/* Declarations */
void waitFor (unsigned int secs);
void GpibError(char *msg);
float *movArray(float *source, int size);
float* getOctaveLimits(void);
float* getThirdOctaveLimits(void);
void drawBandsOn2034(int octn, float *limits, float *calcvalues);
int getBandsFrom2034(FILE *outputf,float *limits,int npoints,float maxfreq,	
	float *calcvalues, int scv);
void identify2034(void);
void init2034(int boardIndex, int primaryAddress, int secondaryAddress);
void startMeasurement2034(void);
float readMaxFrequency2034(void);
void waitUntilFinished2034(void);
void closeCommIEEE(void);


/** Main routine. If a filename is provided as an argument, all the 
	data are written there.
*/
int main(int argc, char**argv) 
{
   	int   primaryAddress = 3;  /* Primary address of the device           */
   	int   secondaryAddress = 0; /* Secondary address of the device         */

	printf("\nThird-octave analysis with B&K 2034 via GPIB\n\n");
	printf("Primary address: %d, secondary: %d \n", primaryAddress, 
		secondaryAddress);
	printf("Davide Bucci, 2015\n\n");
	
	init2034(0, primaryAddress, secondaryAddress);

	FILE *outputf=NULL;
	if(argc>1) {
		outputf=fopen(argv[1],"w");
	}
	
	identify2034();
	startMeasurement2034();
	waitUntilFinished2034();
	
	float maxfreq=readMaxFrequency2034();
   	
   	int npoints=801;
	float data[801];
	
	float *limits=getThirdOctaveLimits();
	int sc=100;
	float calcvalues[sc];
	
	int i;
	float freq;
	if(outputf!=NULL) {
		fprintf(outputf, "# FFT spectrum on a %f frequency range\n",maxfreq);
	} 
	int nbands;
	
	nbands=getBandsFrom2034(outputf, limits, npoints, maxfreq, calcvalues,sc);
	
	if(nbands>0) 
		drawBandsOn2034(nbands, limits, calcvalues);

	if(outputf!=NULL)
		fclose(outputf);
		
	free(limits);
	
	closeCommIEEE();
	   
   	return 0;
}

/** Wait for a given number of seconds.
*/
void waitFor (unsigned int secs) 
{
    unsigned int retTime = time(0) + secs;     // Get finishing time.
    while (time(0) < retTime);    // Loop until it arrives.
}


/** Move an array of floating point data
*/
float *movArray(float *source, 		/* Pointer to the source */
	int size)						/* Number of elements */
{
	float *dest=calloc(sizeof(float),size);
	int i;
	for(i=0; i<size; ++i)
		dest[i]=source[i];
	
	return dest;
}

/** Creates an array containing limits of octave frequencies.
	A pointer with an array is returned.
	The caller should free the pointer when it is not needed anymore.
*/
float* getOctaveLimits(void)
{
	float limits[]= {11,22,44,88,177,355,710,1420,2840,5680,11360,22720};
	return movArray(limits, sizeof(limits));
}

/** Creates an array containing limits of 1/3 octave frequencies.
	The caller should free the pointer when it is not needed anymore.
*/
float* getThirdOctaveLimits(void)
{
	float limits[]= {17.8,22.4,28.2,35.5,44.7,56.2,70.8,89.1,112,141,
		178,224,282,355,447,562,708,891,1122,1413,1778,2239,2818,3548,4467,
		5623,7079,8913,11220,14130,17780};
	return movArray(limits, sizeof(limits)/sizeof(float));
}

/** Shows in a graphical way the band frequency analysis.
*/
void drawBandsOn2034(int octn, /* Number of bands */
	float *limits, 				 /* Limits in frequency of bands */
	float *calcvalues)           /* Power in dB/U integrated in bands */
{
	char command[1001];
	int i;
	
	sprintf(command, "WRITE_TEXT CLEAR_HOME\n");
   	ibwrt(Device, command, strlen(command));
   	
   	sprintf(command, "WRITE_TEXT 1,5,\"OCTAVE ANALYSIS, DAVIDE BUCCI 2015\"\n");
   	ibwrt(Device, command, strlen(command));
   	
   	sprintf(command, "WRITE_TEXT 2,5,\"20 dB/DIV, TOP: 20 dB\"\n");
   	ibwrt(Device, command, strlen(command));
   	
   	// 401 pts/line, bipolar display, show scales
   	sprintf(command, "CONTROL_PROCESS DISPLAY_MODE 1,0,1\n");
   	ibwrt(Device, command, strlen(command));
   	
   	sprintf(command, "PLOT_FOREGROUND 0,0\n");
   	ibwrt(Device, command, strlen(command));

   	int screenwidth=401;
   	float octwidth=(float)screenwidth/(float)octn;
   	int baseline=150;
   	float mult=50/20;
   	int val;
   	int countthirdoct=0;
   	float octaveinflimit;
   	float octavesuplimit;
   	float centraloct;
   	
   	for(i =0; i<octn; ++i) {
   		val=(int)(calcvalues[i]*mult)+baseline;
   		sprintf(command, "PLOT_CONTINUE_FOREGROUND %d,%d,%d,%d\n", 
   			(int)(octwidth*i),val,(int)(octwidth*(i+1)-1),val);
   		ibwrt(Device, command, strlen(command));
   		
   		octaveinflimit=limits[i];
		octavesuplimit=limits[i+1];
		centraloct=sqrt(octavesuplimit*octaveinflimit);
		if((++countthirdoct)%3==2) {
   			sprintf(command, "PLOT_CONTINUE_FOREGROUND %d,%d,\"%d\"\n", 
   				(int)(octwidth*(i+0.1f)),20,(int)round(centraloct));
   		}
   		ibwrt(Device, command, strlen(command));
   	}
}

/** Read the data of all displayed points from the 2034, save it in a file
	(only if outputf!=NULL). Returns the number of bands covered or a
	negative number if something bad happened.
	Data will be put in calcvalues, which should have been allocated before
	calling to this function.
*/
int getBandsFrom2034(FILE *outputf, 	/* Pointer to the output file desc. */
	float *limits, 						/* Pointer to band limits */
	int npoints, 						/* Total number of points to read */
	float maxfreq,						/* Max frequency span */
	float *calcvalues,					/* Pt. to the table to be completed */
	int scv)							/* Size of calcvalues array. */
{
	char command[1001];
	char  Buffer[1001];
	
	float centraloct=16.0f;
	int ptinoctave=0;
	int octn=0;
	int i;
	float freq;
	
	float octaveinflimit=limits[octn];
	float octavesuplimit=limits[octn+1];
	float value;
	float accum=1.0e-10f;
	float total=1.0e-10f;
	float deltaf=maxfreq/((float)npoints-1);
	/* This increases the execution speed of the transfer via GPIB */
   	sprintf(command, "CONTROL_PROCESS MAXIMUM_INTERFACE_ACTIVITY\n");
   	ibwrt(Device, command, strlen(command)); 
   	
	for (i=0; i<npoints; ++i) {
		freq=deltaf*(float)(i);
		sprintf(command, "AF IR,%d\n", i);
   		ibwrt(Device, command, strlen(command)); 
   		ibrd(Device, Buffer, sizeof(Buffer));   
   		if (ibsta & ERR) {
			GpibError("ibrd Error");	
   		}
  		Buffer[ibcntl] = '\0'; 
		sscanf(Buffer+1,"%f", &value);
	
	recalc:
		if(freq>=octaveinflimit && freq<octavesuplimit) {
			/* Calculation of the power in each interval. Expects data
			   in dB, so a conversion is done to sup up the power. */
			float lindata=powf(10.0f,value/10.0f);
			accum+=lindata*deltaf;
			total+=lindata*deltaf;
			++ptinoctave;
		} else if(freq>octavesuplimit) {
			/* We have got to the following interval */
			accum=10*log10(accum);
			printf("Octave %f Hz - %f Hz around %f Hz: %f dB/YREF*Hz\n", 
				octaveinflimit, octavesuplimit,centraloct, accum);
			calcvalues[octn]=accum;
			octaveinflimit=limits[++octn];
			octavesuplimit=limits[octn+1];
			centraloct=sqrt(octavesuplimit*octaveinflimit);
			ptinoctave=0;
			if(octn>=scv) {
				fprintf(stderr, "getBandsFrom2034, size of calcvalues too "
					"small\n");
				return -1;
			}
			
			/* The accumulator is not set to zero to avoid divide by
			   zero errors if no point fall inside the band */
			accum=1.0e-10f;
			goto recalc;			// BUAHAHAHA!!!     ]:-D
		}

		if(outputf!=NULL) {
			fprintf(outputf, "%f %s",freq, Buffer+1);
		} 
	}
	printf("Total power: %f dB/YREF*Hz\n",10*log10(total));
	sprintf(command, "CONTROL_PROCESS NORMAL_INTERFACE_ACTIVITY\n");
   	ibwrt(Device, command, strlen(command));
   	
   	return octn;
}

/** Read an identification string from the 2034.
	It is supposed to be "BK,+02034,+00000,+00000,+00002", but the
	exact contents of it are not checked. If there is a 
	transmission error, exit.
*/
void identify2034(void)
{
	char command[1001];
	char  Buffer[1001];
   	// Get the device identification.
   	strncpy(command, "IDENTIFY\n", sizeof(command));
   	ibwrt(Device, command, strlen(command));  

   	ibrd(Device, Buffer, sizeof(Buffer));   
   	if (ibsta & ERR) {
		GpibError("ibrd Error");
		exit(1);	/* A little brutal but effective */
   	}
  	Buffer[ibcntl] = '\0';         /* Null terminate the ASCII string         */

   	printf("Identification received: %s", Buffer);
	
}
	

/** Initialize the IEEE488 interface and device
*/
void init2034(int boardIndex, int primaryAddress, int secondaryAddress)
{
   	char  Buffer[1001];
   	char command[1001];

   	Device = ibdev(               /* Create a unit descriptor handle         */
         boardIndex,              /* Board Index (GPIB0 = 0, GPIB1 = 1, ...) */
         primaryAddress,          /* Device primary address                  */
         secondaryAddress,        /* Device secondary address                */
         T3s,                     /* Timeout setting (T3s = 3 seconds)     */
         1,                       /* Assert EOI line at end of write         */
         0);                      /* EOS termination mode                    */
   	if (ibsta & ERR) {            /* Check for GPIB Error                    */
      	GpibError("ibdev Error"); 
   	}

   	ibclr(Device);                 /* Clear the device                       */
   	if (ibsta & ERR) {
      	GpibError("ibclr Error");
   	}
   	/* Set the line terminator for the device */
	strncpy(command, "DEFINE_TERMINATOR \n", sizeof(command));
   	ibwrt(Device, command, strlen(command));  
   	
   	/* Show upper graph and measurement settings */
   	sprintf(command, "DISPLAY_FORMAT UM\n");
   	ibwrt(Device, command, strlen(command)); 
   	
   	/* System settings for MEASUREMENT */
   	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION MC 0\n");
   	ibwrt(Device, command, strlen(command)); 
   	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION MM 0\n");
   	ibwrt(Device, command, strlen(command)); 
   	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION FS 6.4k\n");
   	ibwrt(Device, command, strlen(command)); 
   	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION ZB 1\n");
   	ibwrt(Device, command, strlen(command));  
	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AT 1\n");
	ibwrt(Device, command, strlen(command)); 
	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AN 50\n");
	ibwrt(Device, command, strlen(command)); 
	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AA 0\n");
	ibwrt(Device, command, strlen(command)); 
	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AW 1\n");
	ibwrt(Device, command, strlen(command));
   	
   	/* System settings for DISPLAY */
	sprintf(command, "EDIT_DISPLAY_SPECIFICATION FU 14\n");
	ibwrt(Device, command, strlen(command)); 
 
	sprintf(command, "EDIT_DISPLAY_SPECIFICATION YU 1\n");
	ibwrt(Device, command, strlen(command)); 
	sprintf(command, "EDIT_DISPLAY_SPECIFICATION SU 2\n");
	ibwrt(Device, command, strlen(command)); 
	sprintf(command, "EDIT_DISPLAY_SPECIFICATION ID 2\n");
	ibwrt(Device, command, strlen(command)); 
	sprintf(command, "EDIT_DISPLAY_SPECIFICATION DS 0\n");
	ibwrt(Device, command, strlen(command)); 

}

/** Start a measurement campaign on the 2034.
*/
void startMeasurement2034(void)
{
	char command[1001];
	sprintf(command, "KEY_PUSH G 0\n"); /* RECORD Cont */
	ibwrt(Device, command, strlen(command));
	//sprintf(command, "KEY_PUSH A 0\n");	/* AVERAGING Start */
	//ibwrt(Device, command, strlen(command));
	printf("Please start 50 measurements for averaging.\n");
}

/** Read the maximum frequency present in the FFT done by the instrument.
*/
float readMaxFrequency2034(void)
{
	char  Buffer[1001];
   	char command[1001];
	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION FREQUENCY_SPAN\n");
   	ibwrt(Device, command, strlen(command)); 
   	ibrd(Device, Buffer, sizeof(Buffer));
   	
   	if (ibsta & ERR) {
		GpibError("ibrd Error");	
   	}
  	Buffer[ibcntl] = '\0';         /* Null terminate the ASCII string  */

	float maxfreq;
	sscanf(Buffer, "%f", &maxfreq);
   	printf("Frequency span: %f\n", maxfreq);
   	return maxfreq;
}

/** Polls the averaging number status until 50 averages have been 
	computed. Shows a sort of progress bar during the measurement.
*/
void waitUntilFinished2034(void)
{
	char  Buffer[1001];
   	char command[1001];
	float averagingnum;
	float oldaveragingnum;
	printf("[                                                  ]\r");
	printf("[");
	do {
   		waitFor(1);
		sprintf(command, "CURRENT_STATUS A_N_R\n");
   		ibwrt(Device, command, strlen(command)); 
   		ibrd(Device, Buffer, sizeof(Buffer));
   	
   		if (ibsta & ERR) {
			GpibError("ibrd Error");	
   		}
  		Buffer[ibcntl] = '\0';         /* Null terminate the ASCII string  */

		sscanf(Buffer, "%f", &averagingnum);
		
		while(oldaveragingnum++<averagingnum) {
			printf("*");
			fflush(stdout);
		}

		oldaveragingnum=averagingnum;
	} while (averagingnum<50.0f);
	printf("]\n");
}



/**  Un-initialization - Done only once at the end of your application.
*/
void closeCommIEEE(void)
{

   ibonl(Device, 0);              /* Take the device offline                 */
   if (ibsta & ERR) {
      GpibError("ibonl Error");	
   }

   ibonl(BoardIndex, 0);          /* Take the interface offline              */
   if (ibsta & ERR) {
      GpibError("ibonl Error");	
   }
}

/*****************************************************************************
 *                      Function GPIBERROR
 * This function will notify you that a NI-488 function failed by
 * printing an error message.  The status variable IBSTA will also be
 * printed in hexadecimal along with the mnemonic meaning of the bit
 * position. The status variable IBERR will be printed in decimal
 * along with the mnemonic meaning of the decimal value.  The status
 * variable IBCNTL will be printed in decimal.
 *
 * The NI-488 function IBONL is called to disable the hardware and
 * software.
 *
 * The EXIT function will terminate this program.
 *****************************************************************************/
void GpibError(char *msg) {

    printf ("%s\n", msg);

    printf ("ibsta = 0x%x  <", ibsta);
    if (ibsta & ERR )  printf (" ERR");
    if (ibsta & TIMO)  printf (" TIMO");
    if (ibsta & END )  printf (" END");
    if (ibsta & SRQI)  printf (" SRQI");
    if (ibsta & RQS )  printf (" RQS");
    if (ibsta & CMPL)  printf (" CMPL");
    if (ibsta & LOK )  printf (" LOK");
    if (ibsta & REM )  printf (" REM");
    if (ibsta & CIC )  printf (" CIC");
    if (ibsta & ATN )  printf (" ATN");
    if (ibsta & TACS)  printf (" TACS");
    if (ibsta & LACS)  printf (" LACS");
    if (ibsta & DTAS)  printf (" DTAS");
    if (ibsta & DCAS)  printf (" DCAS");
    printf (" >\n");

    printf ("iberr = %d", iberr);
    if (iberr == EDVR) printf (" EDVR <System Error>\n");
    if (iberr == ECIC) printf (" ECIC <Not Controller-In-Charge>\n");
    if (iberr == ENOL) printf (" ENOL <No Listener>\n");
    if (iberr == EADR) printf (" EADR <Address error>\n");
    if (iberr == EARG) printf (" EARG <Invalid argument>\n");
    if (iberr == ESAC) printf (" ESAC <Not System Controller>\n");
    if (iberr == EABO) printf (" EABO <Operation aborted>\n");
    if (iberr == ENEB) printf (" ENEB <No GPIB board>\n");
    if (iberr == EOIP) printf (" EOIP <Async I/O in progress>\n");
    if (iberr == ECAP) printf (" ECAP <No capability>\n");
    if (iberr == EFSO) printf (" EFSO <File system error>\n");
    if (iberr == EBUS) printf (" EBUS <GPIB bus error>\n");
    if (iberr == ESTB) printf (" ESTB <Status byte lost>\n");
    if (iberr == ESRQ) printf (" ESRQ <SRQ stuck on>\n");
    if (iberr == ETAB) printf (" ETAB <Table Overflow>\n");

    printf ("\n");
    printf ("ibcntl = %ld\n", ibcntl);
    printf ("\n");

    /* Call ibonl to take the device and interface offline */
    //ibonl (Device,0);
    //ibonl (BoardIndex,0);
}
