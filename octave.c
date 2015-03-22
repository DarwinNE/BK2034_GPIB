/*****************************************************************************
 * THIRD OCTAVE ANALYSIS OF A SIGNAL
 *
 * Davide Bucci March 2, 2015
 * 
 * GPL v.3, tested on MacOSX 10.6.8 with NI drivers IEEE488
 *****************************************************************************/


#include <stdio.h>
#include <strings.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#include "commBK2034.h"

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
"   -cb    Acquire ch.B instead of ch.A.\n"\
"\n"\
"   -h1    Acquire H1 transfer function instead of the spectrum of ch.A.\n"\
"          In this case, the transfer function will be normalized to the\n"\
"          Of each octave: a flat H1 transfer function will give a flat\n"\
"          third of octave representation.\n"\
"\n"\
"   -h2    Acquire H2 transfer function instead of the spectrum of ch.A.\n"\
"          See -h1 for the representation.\n"\
"\n"\
"   -a     Choose the number of averages to be done on each acquisition.\n"\
"          The default value is 20.\n"\
"\n"\
"   -l     Perform only the low frequency acquisition (the higher band \n"\
"          therefore will be the 1/3 of octave comprised between\n"\
"          562 Hz and 708 Hz.\n"\
"\n"\
"   -f     Perform only the high frequency acquisition (the lowest band\n"\
"          will start from 447 Hz).\n"\
"\n"\
"   -o     Write on a file the results.\n"\
"\n"\
"   -lf    Write on a file the data collected in the first pass.\n"\
"\n"\
"   -ff    Write on a file the data collected in the second pass.\n"\
"\n"\
"   -n     Do not draw the graph on the BK2034 at the end of acquisitions.\n"\
"\n"\
"   -v     Specify vertical range in dB for the graphs (between 5 and 160).\n"\
"\n"\
"   -c     Use a calibration file. Measurement results will be normalized\n"\
"          to those read from the given file.\n"\
"\n"\
"\n\n"\
 

/* Declarations */

float *movArray(float *source, int size);
float* getOctaveLimits(void);
float* getThirdOctaveLimits(void);
float *readBandsFromFile(char *fileName, int octn, float *limits);
void writeBandsOnFile(char* filename, int octn, float *limits, float *cv);
void printBandsOnScreen(int octn, /* Number of bands */
    float *limits,               /* Limits in frequency of bands */
    float *calcvalues,           /* Power in dB/U integrated in bands */
    float vrange);                /* Vertical range in dB */

/** Main routine. Parse and execute command options (see the HELP_STR string
    which contains the help of the program.
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
    float vrange=40.0f;
    t_style acquisition_c=CH_A;
    int i;
    char *calfile=NULL;
    char *lowfile=NULL;
    char *highfile=NULL;
    
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
                        setBoardGPIB(wbi);
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
            } else if(strcmp(argv[i], "-v")==0) { /* -p primaryAddress */
                if(argc>i+1) {
                    sscanf(argv[++i], "%f", &vrange);
                    if (vrange<4 || vrange>161) {
                        fprintf(stderr, "Invalid vertical range.\n");
                        vrange=40;
                    }
                } else {
                    fprintf(stderr, "-p requires the address.\n");
                }
            } else if(strcmp(argv[i], "-l")==0) { /* -l low (first) pass  */
                secondPass=false;
            } else if(strcmp(argv[i], "-f")==0) { /* -l high (second) pass  */
                firstPass=false;
            } else if(strcmp(argv[i], "-n")==0) { /* -n do not draw on 2034  */
                drawOn2034=false;
            } else if(strcmp(argv[i], "-h1")==0) { /* -h1 H1 transfer fct.  */
                acquisition_c=H1;
            } else if(strcmp(argv[i], "-h2")==0) { /* -h2 H2 transfer fct.   */
                acquisition_c=H2;
            } else if(strcmp(argv[i], "-cb")==0) { /* -cb channel B.  */
                acquisition_c=CH_B;
            } else if(strcmp(argv[i], "-o")==0) { /* -o write on a file */
                if(argc>i+1) {
                    fileName=argv[++i];
                } else {
                    fprintf(stderr, "-o requires the file name.\n");
                }
            } else if(strcmp(argv[i], "-lf")==0) { /* -lf write on a file */
                if(argc>i+1) {
                    lowfile=argv[++i];
                } else {
                    fprintf(stderr, "-lf requires the file name.\n");
                }
            }else if(strcmp(argv[i], "-ff")==0) { /* -o write on a file */
                if(argc>i+1) {
                    highfile=argv[++i];
                } else {
                    fprintf(stderr, "-ff requires the file name.\n");
                }
            } else if(strcmp(argv[i], "-c")==0) { /* -c calfile */
                if(argc>i+1) {
                    calfile=argv[++i];
                } else {
                    fprintf(stderr, "-c requires the file name.\n");
                }
            }else {
                fprintf(stderr, "Unrecognized option: %3s\n", argv[i]);
            }
        }
    }
    
    printf("Configuration settings:\n");
    printf("Primary address: %d, secondary: %d \n", primaryAddress, 
        secondaryAddress);
    printf("Number of averages: %d\n", wnavg);
    
    init2034(0, primaryAddress, secondaryAddress);
    reset2lev2034();
    identify2034();
    configureAcquisitionAndGraph2034(wnavg, acquisition_c, true);

    /*float testl[]={5,0,-5,-15,-20,-35,6, 10,-7};
    printBandsOnScreen(9, limits, testl, 50);
    drawBandsOn2034(9, limits, testl, 50);
    exit(0);*/

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
            calcvaluesLO,sc,acquisition_c,
            lowfile);
        
    	nbands=nbands1;
        calcvalues=calcvaluesLO;
    
    }
    
    if(secondPass) {
        printf("Second read, high frequency range.\n");

        /* The second measurement is done up to 25.6kHz, to get information for
            the high frequency range of the spectrum. */
        configureMaxFreq2034("25.6k");
        startMeasurement2034();
        waitUntilFinished2034(wnavg);
    
        nbands2=getBandsFrom2034(limits, npoints,
            readMaxFrequency2034(), 
            calcvaluesHI,sc,acquisition_c,
            highfile);
    
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
    
    /* Perform a calibration operation if required. */
    float *calib=NULL;
    if(calfile!=NULL) {
    	calib = readBandsFromFile(calfile, nbands, limits);
    	if(calib!=NULL) {
    		for(i=0; i<nbands;++i) {
    			calcvalues[i]-=calib[i];
    		}
    	}
    }
    printBandsOnScreen(nbands, limits, calcvalues, vrange);
    
    if(drawOn2034)
        drawBandsOn2034(nbands, limits, calcvalues, vrange);
    
    
    if(fileName!=NULL) {
        writeBandsOnFile(fileName, nbands, limits, calcvalues);
    }

    free(limits);
    if(calib!=NULL)
    	free(calib);
    
    closeCommIEEE();
       
    return 0;
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

/** Writes the calculated bands on a file.
*/
float *readBandsFromFile(
    char *fileName,
    int octn, /* Number of bands */
    float *limits)               /* Limits in frequency of bands */
{
    int i;
    float octaveinflimit;
    float octavesuplimit;
    float centraloct;
    float co;
    
    float *calib=calloc(sizeof(float),octn);
    
    FILE *fin=fopen(fileName, "r");
    if(fin==NULL) {
        fprintf(stderr, "Could not open input file (%s)!\n", fileName);
        return NULL;
    } 
    char c;
    do {
  		c = fgetc(fin);
	} while (c != '\n');
	
    for(i =0; i<octn; ++i) {
        octaveinflimit=limits[i];
        octavesuplimit=limits[i+1];
        centraloct=sqrt(octavesuplimit*octaveinflimit);
        fscanf(fin, "%f%f", &co, &calib[i]);
        printf("%f   %f\n", co, calib[i]);
        if(1==0) {
        	fprintf(stderr, "Input file does not seem to have enough bands\n");
        	fclose(fin);
        	return NULL;
        } else if(abs(co-centraloct)>1) {
        	fprintf(stderr, "Incompatible input file.\n");
        	fclose(fin);
        	return NULL;
        }
    }
    fclose(fin);
    printf("File %s read.\n", fileName);
    return calib;
}
        

/** Shows on the PC screen way the band frequency analysis.
*/
void printBandsOnScreen(int octn, /* Number of bands */
    float *limits,               /* Limits in frequency of bands */
    float *calcvalues,           /* Power in dB/U integrated in bands */
    float vrange)                /* Vertical range in dB */
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
           max-vrange, max);
    printf("      +----------------------------------------------------------------------+\n");
    for(i =0; i<octn; ++i) {
        octaveinflimit=limits[i];
        octavesuplimit=limits[i+1];
        centraloct=sqrt(octavesuplimit*octaveinflimit);
        printf("%-6.0f: ", centraloct);
        
        lenline=(calcvalues[i]-max+vrange)/(vrange)*ncol;
        
        for(k=0; k<lenline;++k) 
            printf("*");
            
        for(; k<ncol;++k) {
            if((k+1)%10==0) 
                printf("|");
            else
                printf(" ");
        }
            
        printf("\n");
    }
    printf("      +----------------------------------------------------------------------+\n");

}
