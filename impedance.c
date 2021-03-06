/*****************************************************************************
 * IMPEDANCE MEASUREMENT IN THE AUDIO RANGE
 *
 * Davide Bucci March 22, 2015
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
"spectrum analyzer and performs an impedance measurement in the 0-25.6kHz\n"\
"band of the instrument. The measurement can be done by using the internal\n"\
"signal generator of the instrument, as a pseudo-random noise generator.\n"\
"The output of the generator should be used to fed the device under test,\n"\
"by putting a series resistor of known value R. By default, the program\n"\
"considers a value of R of 1000 Ω. Connect the input of channel A to the\n"\
"output of the signal generator and use channel B to probe the voltage\n"\
"at the terminals of the device. The output impedance of the generator\n"\
"(by default 0 Ω) can be taken into account.\n"\
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
"   -a     Choose the number of averages to be done on each acquisition.\n"\
"          The default value is 20.\n"\
"\n"\
"   -o     Write on a file the results.\n"\
"\n"\
"   -r     Changes the resistance R to be used in the divider (in Ω)\n"\
"\n"\
"   -g     Changes the generator impedance (in Ω)\n"\
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

    printf("\nImpedance measurement with a B&K 2034 via GPIB\n\n");
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
            } else if(strcmp(argv[i], "-r")==0) { /* -r external res. */
                if(argc>i+1) {
                    int wsa;
                    sscanf(argv[++i], "%f", &externalRes);
                } else {
                    fprintf(stderr, "-r requires a resistance.\n");
                }
            } else if(strcmp(argv[i], "-g")==0) { /* -r internal res. */
                if(argc>i+1) {
                    int wsa;
                    sscanf(argv[++i], "%f", &internalRes);
                } else {
                    fprintf(stderr, "-g requires a resistance.\n");
                }
            } else if(strcmp(argv[i], "-a")==0) { /* -a number of averages */
                if(argc>i+1) {
                    int wnavg_p;
                    sscanf(argv[++i], "%d", &wnavg_p);
                    if (wnavg_p<1 || wnavg_p>32767) {
                        fprintf(stderr, "Invalid number of averages (%d).\n",
                            wnavg_p);
                    } else {
                        navg=wnavg_p;
                    }
                } else {
                    fprintf(stderr, "-a requires the number of averages.\n");
                }
            } else {
                fprintf(stderr, "Unknown option %4s \n", argv[i]);
                return 1;
            }
        }
    }

    printf("Configuration settings:\n");
    printf("Primary address: %d, secondary: %d \n", primaryAddress,
        secondaryAddress);

    init2034(0, primaryAddress, secondaryAddress);
    reset2lev2034();

    identify2034();


    configureAcquisitionAndGraph2034(navg, H1, false);



    startMeasurement2034();
    waitUntilFinished2034(navg);

    int npoints=801;
    float realpoints[npoints];
    float imagpoints[npoints];

    float freqpoints[npoints];

    float _Complex impedance[npoints];
    float module;
    float phase;

    float rtot=internalRes+externalRes;

    printf("Considering a total resistance of %f Ω.\n", rtot);

    float _Complex h=1.0f;

    printf("Reading real part of data\n");

    /* Real part */
    writeGPIB("EDIT_DISPLAY_SPECIFICATION FC 0\n");
    getDataPoints2034(freqpoints, realpoints, npoints);

    printf("Reading imaginary part of data\n");

    /* Imaginary part */
    writeGPIB("EDIT_DISPLAY_SPECIFICATION FC 1\n");
    getDataPoints2034(freqpoints, imagpoints, npoints);

    FILE *fout=NULL;
    if(fileName!=NULL) {
        fout=fopen(fileName, "w");
        if(fout==NULL) {
            fprintf(stderr,"Could not open the output file.\n");
            return 1;
        }
        fprintf(fout, "#freq/Hz module/Ω phase/rad\n");
    }
    for(i=0; i<npoints; ++i) {
        h=realpoints[i]+imagpoints[i]*I;
        impedance[i] = rtot*h/(1-h);
        module=cabsf(impedance[i]);
        phase=cargf(impedance[i]);

        if(fileName==NULL) {
            printf("freq=%f, module=%f, phase=%f rad\n",
                freqpoints[i], module, phase);
        } else {
            fprintf(fout, "%f %f %f\n",
                freqpoints[i], module, phase);
        }
    }

    closeCommIEEE();

    return 0;
}
