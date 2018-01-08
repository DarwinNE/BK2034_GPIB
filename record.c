/*****************************************************************************
 * SIMPLE RECORDING OF THE ACTUAL DISPLAY
 *
 * Davide Bucci March 28, 2015
 * 
 * GPL v.3, tested on MacOSX 10.6.8 with NI drivers IEEE488
 *****************************************************************************/

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <complex.h>

#include "commBK2034.h"

#define HELP_STR \
"This software communicates with a Bruel&Kjaer 2034 double channel FFT\n"\
"spectrum analyzer and records what it is shown on the upper display.\n"\
"No processing is done on the data.\n"\
"\n"\
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
"   -o     Write on a file the results.\n"\
"\n\n"


/** Main routine. Parse and execute command options (see the HELP_STR string
    which contains the help of the program.
*/
int main(int argc, char**argv) 
{
    int   primaryAddress = 3;  /* Primary address of the device           */
    int   secondaryAddress = 0; /* Secondary address of the device        */

   	int i=0;
   	
   	float internalRes = 0.0; 
   	float externalRes = 1000.0;
   	
   	int navg=20;
   	char *fileName=NULL;
   	 
    printf("\nRecord data from a B&K 2034 via GPIB\n\n");
    printf("Davide Bucci, 2015\n\n");
    
    /* If necessary, process all command line functions */
    if(argc>1) {
        for(i=1; i<argc;++i) {
            if(strcmp(argv[i], "-h")==0) { /* -h show a help and exit */
                printf(HELP_STR);
                return 0;
            } else if(strcmp(argv[i], "-b")==0) { /* -b board index */
                if(argc>i+1) {
                    int wbi;
                    sscanf(argv[++i], "%d", &wbi);
                    if (wbi<0 || wbi>31) {
                        fprintf(stderr, "Invalid board index (%d).\n", wbi);
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
                        fprintf(stderr, "Invalid address (%d).\n", wpa);
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
                        fprintf(stderr, "Invalid address (%d).\n", wsa);
                    } else {
                        secondaryAddress=wsa;
                    }
                } else {
                    fprintf(stderr, "-s requires the address.\n");
                }
            } else if(strcmp(argv[i], "-o")==0) { /* -o write on a file */
                if(argc>i+1) {
                    fileName=argv[++i];
                } else {
                    fprintf(stderr, "-o requires the file name.\n");
                }
            } 
        }
    }
    
    printf("Configuration settings:\n");
    printf("Primary address: %d, secondary: %d \n", primaryAddress, 
        secondaryAddress);

    init2034(0, primaryAddress, secondaryAddress);
	
    identify2034();
	
	int npoints=801;
	float points[npoints];
	
	float freqpoints[npoints];

	printf("Reading data\n");
	
	getDataPoints2034(freqpoints, points, npoints);

	FILE *fout=NULL;
	if(fileName!=NULL) {
		fout=fopen(fileName, "w");
		if(fout==NULL) {
			fprintf(stderr,"Could not open the output file.\n");
			return 1;
		}
		fprintf(fout, "#freq/Hz read\n");
	} else {
		fout=stdout;
	}
	for(i=0; i<npoints; ++i) {
		fprintf(fout, "%f %f\n",
				freqpoints[i], points[i]);
	}

	closeCommIEEE();
       
    return 0;
}
