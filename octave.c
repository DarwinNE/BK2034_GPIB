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
#include <stdbool.h>

#define HELP_STR \
"This software communicates with a Bruel&Kjaer 2034 double channel FFT\n"\
"spectrum analyzer to perform 1/3 of octave analysis of an input signal\n"\
"The 2034 acquires 801 points for each FFT spectrum, so to cover the\n"\
"whole audio range, the spectrum should be acquired twice. The first\n"\
"time, this is done in a band from 0 Hz to 800 Hz and the second time\n"\
"the higher frequency limit is increased to 25.6 kHz.\n"\
"The measurement is done by considering that a Power Spectral Density is\n"\
"acquired by the instrument. This means that the signal acquired has its\n"\
"power spectral density which is larger of the FFT bands of the instrument.\n"\
"In other words, this works correctly when using a pink or white noise,\n"\
"but not a signal having a line spectrum.\n\n"\
"The output can be written on the screen of the PC running this utility,\n"\
"or can be plotted in a graphical way on the screen of the BK 2034 itself.\n"\
"It is also possible to choose a file where to write the results.\n"\
"\n"\
"The following options are available:\n"\
"\n"\
"   -h     Show this help\n"\
"\n"\
"   -b     Change the board interface index (GPIB0=0,GPIB1=1,etc.)\n"\
"\n"\
"   -p     Change the primary address of the BK 2034.\n"\
"\n"\
"   -s     Change the secondary address of the BK 2034.\n"\
"\n"\
"   -a     Choose the number of averages to be done on each acquisition.\n"\
"          The default value is 20.\n"\
"          \n"\
"   -l     Perform only the low frequency acquisition (the higher band \n"\
"          therefore will be the 1/3 of octave comprised between\n"\
"          562 Hz and 708 Hz.\n"\
"          \n"\
"   -f     Perform only the high frequency acquisition (the lowest band\n"\
"          will start from 447 Hz).\n"\
"          \n"\
"   -o     Write on a file the results.\n"\
"          \n"\
"   -n     Do not draw the graph on the BK2034 at the end of acquisitions.\n"\
"          \n\n"\
 

int Device = 0;                   /* Device unit descriptor                  */
int BoardIndex = 0;               /* Interface Index (GPIB0=0,GPIB1=1,etc.)  */

/* Declarations */
void waitFor (unsigned int secs);
void GpibError(char *msg);
float *movArray(float *source, int size);
float* getOctaveLimits(void);
float* getThirdOctaveLimits(void);
void configureMaxFreq2034(char *maxfreq);
void drawBandsOn2034(int octn, float *limits, float *calcvalues);
void printBandsOnScreen(int octn, float *limits, float *calcvalues);
void writeBandsOnFile(char* filename, int octn, float *limits, float *cv);
int getBandsFrom2034(float *limits,int npoints,float maxfreq, 
    float *calcvalues, int scv);
void identify2034(void);
void init2034(int boardIndex, int primaryAddress, int secondaryAddress);
void configureAcquisitionAndGraph2034(int navg);
void startMeasurement2034(void);
float readMaxFrequency2034(void);
void waitUntilFinished2034(int wnavg);
void closeCommIEEE(void);


/** Main routine. If a filename is provided as an argument, all the 
    data are written there.
*/
int main(int argc, char**argv) 
{
    int   primaryAddress = 3;  /* Primary address of the device           */
    int   secondaryAddress = 0; /* Secondary address of the device        */

    float *limits=getThirdOctaveLimits();
    int sc=100;
    float calcvaluesLO[sc];
    float calcvaluesHI[sc];
    int npoints=801;
    int wnavg =20;
    bool firstPass=true;
    bool secondPass=true;
    char *fileName=NULL;
    bool drawOn2034=true;
    
    int i;
    
    
    /*float testl[]={5,0,-5,-15,-20,-35,6, 10,-7};
    printBandsOnScreen(9, limits, testl);
    exit(0);
    */
    
    printf("\nThird-octave analysis with B&K 2034 via GPIB\n\n");
    printf("Davide Bucci, 2015\n\n");
    
    
    /* If necessary, process all command line functions */
    if(argc>1) {
    	for(i=1; i<argc;++i) {
    		if(strcmp(argv[i], "-h")==0) { /* -h show a help and exit */
    			printf(HELP_STR);
    			return 0;
			} else if(strcmp(argv[i], "-a")==0) { /* -a number of averages */
        		if(argc>i+1) {
        			int wnavg_p;
        			sscanf(argv[++i], "%d", &wnavg_p);
        			if (wnavg_p<1 || wnavg_p>32767) {
        				fprintf(stderr, "Invalid number of averages (%d).\n",
        					wnavg_p);
        			} else {
        				wnavg=wnavg_p;
        			}
        		} else {
        			fprintf(stderr, "-a requires the number of averages.\n");
				}
        	} else if(strcmp(argv[i], "-b")==0) { /* -b board index */
        		if(argc>i+1) {
        			int wbi;
        			sscanf(argv[++i], "%d", &wbi);
        			if (wbi<0 || wbi>31) {
        				fprintf(stderr, "Invalid board index (%d).\n",
        					wbi);
        			} else {
        				BoardIndex=wbi;
        			}
        		} else {
        			fprintf(stderr, "-b requires the board index.\n");
				}
        	} else if(strcmp(argv[i], "-p")==0) { /* -p primaryAddress */
        		if(argc>i+1) {
        			int wpa;
        			sscanf(argv[++i], "%d", &wpa);
        			if (wpa<0 || wpa>31) {
        				fprintf(stderr, "Invalid address (%d).\n",
        					wpa);
        			} else {
        				primaryAddress=wpa;
        			}
        		} else {
        			fprintf(stderr, "-p requires the address.\n");
				}
        	} else if(strcmp(argv[i], "-s")==0) { /* -p secondaryAddress */
        		if(argc>i+1) {
        			int wsa;
        			sscanf(argv[++i], "%d", &wsa);
        			if (wsa<0 || wsa>31) {
        				fprintf(stderr, "Invalid address (%d).\n",
        					wsa);
        			} else {
        				secondaryAddress=wsa;
        			}
        		} else {
        			fprintf(stderr, "-s requires the address.\n");
				}
			} else if(strcmp(argv[i], "-l")==0) { /* -l low (first) pass  */
				secondPass=false;
			} else if(strcmp(argv[i], "-f")==0) { /* -l high (second) pass  */
				firstPass=false;
			} else if(strcmp(argv[i], "-n")==0) { /* -n do not draw on 2034  */
				drawOn2034=false;
			} else if(strcmp(argv[i], "-o")==0) { /* -o write on a file */
        		if(argc>i+1) {
        			fileName=argv[++i];
        		} else {
        			fprintf(stderr, "-o requires the file name.\n");
				}
			} else {
				fprintf(stderr, "Unrecognized option: %3s\n", argv[i]);
			}
        }
    }
    
    printf("Configuration settings:\n");
    printf("Primary address: %d, secondary: %d \n", primaryAddress, 
        secondaryAddress);
    printf("Number of averages: %d\n", wnavg);
    
	init2034(0, primaryAddress, secondaryAddress);
    identify2034();
    configureAcquisitionAndGraph2034(wnavg);

    int nbands;
    int nbands1=0;
    int nbands2=0;
    float *calcvalues;
    
    if(firstPass) {
    	printf("First read, low frequency range.\n");
    	/* The first measurement is done up to 800 Hz, to get information for
       		the low frequency range of the spectrum. */
    	configureMaxFreq2034("800");
    	startMeasurement2034();
    	waitUntilFinished2034(wnavg);
    
		nbands1=getBandsFrom2034(limits, npoints, 
			readMaxFrequency2034(), 
			calcvaluesLO,sc);
	}
	
	if(secondPass) {
		printf("Second read, low frequency range.\n");

		/* The second measurement is done up to 25.6kHz, to get information for
       		the high frequency range of the spectrum. */
    	configureMaxFreq2034("25.6k");
    	startMeasurement2034();
    	waitUntilFinished2034(wnavg);
    
    
		nbands2=getBandsFrom2034(limits, npoints,
			readMaxFrequency2034(), 
			calcvaluesHI,sc);
	}
	
	if(secondPass) {
		float freq=limits[0];
		i=0;
    	while(freq<700.0f) {
    		if(firstPass) 
    			calcvaluesHI[i]=calcvaluesLO[i];
    		else
    			calcvaluesHI[i]=-100.0f;
    		freq=limits[++i];
    	}
    	nbands=nbands2;
    	calcvalues=calcvaluesHI;
    } 
    
    if(drawOn2034)
    	drawBandsOn2034(nbands, limits, calcvalues);
    
    printBandsOnScreen(nbands, limits, calcvalues);
    
    if(fileName!=NULL) {
    	writeBandsOnFile(fileName, nbands, limits, calcvalues);
    }

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
float *movArray(float *source,      /* Pointer to the source */
    int size)                       /* Number of elements */
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
        5623,7079,8913,11220,14130,17780,22390,28210};
    return movArray(limits, sizeof(limits)/sizeof(float));
}

/** Shows in a graphical way the band frequency analysis.
*/
void drawBandsOn2034(int octn, /* Number of bands */
    float *limits,               /* Limits in frequency of bands */
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


/** Writes the calculated bands on a file.
*/
void writeBandsOnFile(
	char *fileName,
	int octn, /* Number of bands */
    float *limits,               /* Limits in frequency of bands */
    float *calcvalues)           /* Power in dB/U integrated in bands */
{
    int i;
    float octaveinflimit;
    float octavesuplimit;
    float centraloct;
    
    FILE *fout=fopen(fileName, "w");
    if(fout==NULL) {
    	fprintf(stderr, "Could not open output file (%s)!\n", fileName);
    	return;
    } 
    fprintf(fout, 
    	"# 1/3 octave analysis. Center band in Hz, PSD in dB/YREF*Hz\n");
    for(i =0; i<octn; ++i) {
        octaveinflimit=limits[i];
        octavesuplimit=limits[i+1];
        centraloct=sqrt(octavesuplimit*octaveinflimit);
        fprintf(fout, " %f  %f \n", centraloct, calcvalues[i]);
    }
    fclose(fout);
	printf("File %s written.\n", fileName);
}
    	

/** Shows on the PC screen way the band frequency analysis.
*/
void printBandsOnScreen(int octn, /* Number of bands */
    float *limits,               /* Limits in frequency of bands */
    float *calcvalues)           /* Power in dB/U integrated in bands */
{
    int i;
    int val;
    int countthirdoct=0;
    float octaveinflimit;
    float octavesuplimit;
    float centraloct;
    float max=-100.0f;
    
    printf("\n Final results of 1/3 octave analysis.\n");
    
    for(i =0; i<octn; ++i) {
        octaveinflimit=limits[i];
        octavesuplimit=limits[i+1];
        centraloct=sqrt(octavesuplimit*octaveinflimit);
        printf("Band %6.1f Hz - %6.1f Hz around %6.1f Hz: %5.2f dB/YREF*Hz\n", 
                octaveinflimit, octavesuplimit,centraloct, calcvalues[i]);
        if(calcvalues[i]>max)
        	max=calcvalues[i];
        
    }
    printf("\n    1/3 octave analysis, plot\n\n");
    int ncol=70;
    int k;
    int lenline;
    
    printf("     %2.1f                                                                %2.1f\n",
    	   max-40, max);
 	printf("      +----------------------------------------------------------------------+\n");
    for(i =0; i<octn; ++i) {
        octaveinflimit=limits[i];
        octavesuplimit=limits[i+1];
        centraloct=sqrt(octavesuplimit*octaveinflimit);
        printf("%-6.0f: ", centraloct);
        
        lenline=(calcvalues[i]-max+40)/40*ncol;
        
        for(k=0; k<lenline;++k) 
        	printf("*");
        	
        printf("\n");
    }
   	printf("      +----------------------------------------------------------------------+\n");

}

/** Read the data of all displayed points from the 2034. Returns the number 
	of bands covered or a negative number if something bad happened.
    Data will be put in calcvalues, which should have been allocated before
    calling to this function.
*/
int getBandsFrom2034(
    float *limits,                      /* Pointer to band limits */
    int npoints,                        /* Total number of points to read */
    float maxfreq,                      /* Max frequency span */
    float *calcvalues,                  /* Pt. to the table to be completed */
    int scv)                            /* Size of calcvalues array. */
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
            printf("Band %6.1f Hz - %6.1f Hz around %6.1f Hz: %5.2f dB/YREF*Hz\n", 
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
            goto recalc;            // BUAHAHAHA!!!     ]:-D
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
        exit(1);    /* A little brutal but effective */
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
    /* Reset the device */
    strncpy(command, "SYSTEM_RESET 2\n", sizeof(command));
    ibwrt(Device, command, strlen(command));  
    
    
}

/** Configure the acquisition of the BK 2034 to acquire the 
	wanted spectrum and number of averages.
*/
void configureAcquisitionAndGraph2034(int navg)
{
    char command[1001];

    /* Show upper graph and measurement settings */
    sprintf(command, "DISPLAY_FORMAT UM\n");
    ibwrt(Device, command, strlen(command)); 
    
    /* System settings for MEASUREMENT */
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION MC 0\n");
    ibwrt(Device, command, strlen(command)); 
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION MM 0\n");
    ibwrt(Device, command, strlen(command)); 
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION ZB 1\n");
    ibwrt(Device, command, strlen(command));  
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AT 1\n");
    ibwrt(Device, command, strlen(command)); 
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AN %d\n",navg);
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

/** Configure the maximum frequency span to be used for measurements
	on the 2034. 
*/
void configureMaxFreq2034(char *maxfreq)
{
    char command[1001];
	sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION FS %s\n", maxfreq);
    ibwrt(Device, command, strlen(command)); 
}

/** Start a measurement campaign on the 2034.
*/
void startMeasurement2034(void)
{
    char command[1001];
    sprintf(command, "KEY_PUSH G 0\n"); /* RECORD Cont */
    ibwrt(Device, command, strlen(command));
    sprintf(command, "KEY_PUSH C 0\n");   /* AVERAGING Stop */
    ibwrt(Device, command, strlen(command));
    sprintf(command, "KEY_PUSH A 0\n");   /* AVERAGING Start */
    ibwrt(Device, command, strlen(command));
    printf("Please start measurements for averaging.\n");
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
void waitUntilFinished2034(int wnavg)
{
    char  Buffer[1001];
    char command[1001];
    float averagingnum;
    float wstar=50.0f;
    float complete=0.0f;
    int i;
    
    printf("[                                                  ]\r");
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
    
    	complete=averagingnum/wnavg;
    	printf("\r[");
    	for(i=0; i<(int)(complete*wstar);++i) {
            printf("*");
            fflush(stdout);
        }
    } while (averagingnum<wnavg);
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
