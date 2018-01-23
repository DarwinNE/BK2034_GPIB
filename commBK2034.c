
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include "GPIB.h"
#include "commBK2034.h"

#define BUFSIZE 1001

/** Configure the acquisition of the BK 2034 to acquire the
    wanted spectrum and number of averages.
*/
void configureAcquisitionAndGraph2034(int navg, t_style s, bool logY)
{
    char command[BUFSIZE];

    /* Show upper graph and measurement settings */
    sprintf(command, "DISPLAY_FORMAT UM\n");
    writeGPIB(command);

    /* System settings for MEASUREMENT */
    /* Configure the acquisition channel, at first. */
    switch(s) {
        default:
        case CH_A:
            sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION MC 0\n");
            break;
        case CH_B:
            sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION MC 1\n");
            break;
        case H1:
        case H2:
            sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION MC 2\n");
            break;
    }
    writeGPIB(command);

    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION MM 0\n");
    writeGPIB(command);
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION ZB 1\n");
    writeGPIB(command);
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AT 1\n");
    writeGPIB(command);
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AN %d\n",navg);
    writeGPIB(command);
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AA 0\n");
    writeGPIB(command);
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION AW 1\n");
    writeGPIB(command);

    /* System settings for DISPLAY */

    switch(s) {
        default:
        case CH_A:
            sprintf(command, "EDIT_DISPLAY_SPECIFICATION FU 14\n");
            break;
        case CH_B:
            sprintf(command, "EDIT_DISPLAY_SPECIFICATION FU 15\n");
            break;
        case H1:
            sprintf(command, "EDIT_DISPLAY_SPECIFICATION FU 17\n");
            break;
        case H2:
            sprintf(command, "EDIT_DISPLAY_SPECIFICATION FU 19\n");
            break;
    }

    writeGPIB(command);

    if(logY) {
        sprintf(command, "EDIT_DISPLAY_SPECIFICATION YU 1\n");
        writeGPIB(command);
        //writeGPIB("EDIT_DISPLAY_SPECIFICATION YL 1\n");
    } else {
        sprintf(command, "EDIT_DISPLAY_SPECIFICATION YU 0\n");
        /* Linear axes */
        //writeGPIB(command);
        //writeGPIB("EDIT_DISPLAY_SPECIFICATION YL 0\n");
    }

    switch(s) {
        default:
        case CH_A:
            sprintf(command, "EDIT_DISPLAY_SPECIFICATION SU 2\n");
            writeGPIB(command);
            break;
        case CH_B:
            sprintf(command, "EDIT_DISPLAY_SPECIFICATION SU 2\n");
            writeGPIB(command);
            break;
        case H1:
            break;
        case H2:
            break;
    }

    sprintf(command, "EDIT_DISPLAY_SPECIFICATION ID 2\n");
    writeGPIB(command);
    sprintf(command, "EDIT_DISPLAY_SPECIFICATION DS 0\n");
    writeGPIB(command);

}

/** Configure the maximum frequency span to be used for measurements
    on the 2034.
*/
void configureMaxFreq2034(char *maxfreq)
{
    char command[BUFSIZE];
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION FS %s\n", maxfreq);
    writeGPIB(command);
}

/** Start a measurement campaign on the 2034.
*/
void startMeasurement2034(void)
{
    char command[BUFSIZE];
    sprintf(command, "KEY_PUSH G 0\n"); /* RECORD Cont */
    writeGPIB(command);
    sprintf(command, "KEY_PUSH C 0\n");   /* AVERAGING Stop */
    writeGPIB(command);
    sprintf(command, "KEY_PUSH A 0\n");   /* AVERAGING Start */
    writeGPIB(command);
    printf("Please start measurements for averaging.\n");
}

/** Read the maximum frequency present in the FFT done by the instrument.
*/
float readMaxFrequency2034(void)
{
    char  buffer[BUFSIZE];
    char command[BUFSIZE];
    sprintf(command, "EDIT_MEASUREMENT_SPECIFICATION FREQUENCY_SPAN\n");
    writeGPIB(command);
    readGPIB(buffer, BUFSIZE);

    float maxfreq;
    sscanf(buffer, "%f", &maxfreq);
    printf("Frequency span: %5.1f Hz\n", maxfreq);
    return maxfreq;
}

/** Polls the averaging number status until 50 averages have been
    computed. Shows a sort of progress bar during the measurement.
*/
void waitUntilFinished2034(int wnavg)
{
    char  buffer[BUFSIZE];
    char command[BUFSIZE];
    float averagingnum;
    float wstar=50.0f;
    float complete=0.0f;
    int i;

    sprintf(command, "PROMPT 'PLEASE START MEASUREMENTS NOW'\n");
    writeGPIB(command);
    printf("[                                                  ]\r");
    do {
        waitFor(1);
        sprintf(command, "CURRENT_STATUS A_N_R\n");
        writeGPIB(command);
        readGPIB(buffer, BUFSIZE);

        sscanf(buffer, "%f", &averagingnum);

        complete=averagingnum/wnavg;
        printf("\r[");

        /* Erase the prompt line on the 2034 */
        if(averagingnum>0) {
            sprintf(command, "PROMPT\n");
            writeGPIB(command);
        }
        for(i=0; i<(int)(complete*wstar);++i) {
            printf("*");
            fflush(stdout);
        }
    } while (averagingnum<wnavg);
    printf("]\n");
}

/** Wait for a given number of seconds.
*/
void waitFor (unsigned int secs)
{
    unsigned int retTime = time(0) + secs;     // Get finishing time.
    while (time(0) < retTime);    // Loop until it arrives.
}
/** Shows in a graphical way the band frequency analysis.
*/
void drawBandsOn2034(int octn, /* Number of bands */
    float *limits,               /* Limits in frequency of bands */
    float *calcvalues,           /* Power in dB/U integrated in bands */
    float vrange)                /* Vertical range in dB */
{
    char command[BUFSIZE];
    int i;

    sprintf(command, "WRITE_TEXT CLEAR_HOME\n");
    writeGPIB(command);

    sprintf(command, "WRITE_TEXT 1,5,\"OCTAVE ANALYSIS, DAVIDE BUCCI 2015\"\n");
    writeGPIB(command);



    int screenwidth=401;
    float octwidth=(float)screenwidth/(float)octn;
    int baseline=200;
    float mult=50.0f/(vrange/4.0f);

    int val;
    int countthirdoct=0;
    float octaveinflimit;
    float octavesuplimit;
    float centraloct;

    float max=-200;
    /* Search for the maximum value */
    for(i =0; i<octn; ++i)
        if(calcvalues[i]>max)
            max=calcvalues[i];

    sprintf(command, "WRITE_TEXT 2,5,\"V. range: %4.2f dB, TOP: %4.2f dB\"\n",
        vrange, max);

    writeGPIB(command);

    // 401 pts/line, bipolar display, show scales
    sprintf(command, "CONTROL_PROCESS DISPLAY_MODE 1,0,1\n");
    writeGPIB(command);

    sprintf(command, "PLOT_FOREGROUND 0,0\n");
    writeGPIB(command);

    for(i =0; i<octn; ++i) {
        val=(int)((calcvalues[i]-max)*mult)+baseline;

        sprintf(command, "PLOT_CONTINUE_FOREGROUND %d,%d,%d,%d\n",
            (int)(octwidth*i),val,(int)(octwidth*(i+1)-1),val);
        writeGPIB(command);

        octaveinflimit=limits[i];
        octavesuplimit=limits[i+1];
        centraloct=sqrt(octavesuplimit*octaveinflimit);
        if((++countthirdoct)%3==2) {
            int corr=octwidth/2-6;
            if(centraloct<100) {
                corr+=2;
            } else if(centraloct<1000) {
                corr+=6;
            } else if(centraloct<10000) {
                corr+=9;
            } else {
                corr+=12;
            }
            sprintf(command, "PLOT_CONTINUE_FOREGROUND %d,%d,\"%d\"\n",
                (int)(octwidth*(i+0.1f)-corr),20,
                (int)round(centraloct));
        }
        writeGPIB(command);
    }
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
    int scv,                            /* Size of calcvalues array. */
    t_style acq,                        /* Acquisition style */
    char *filename)                     /* Name of the file where to store raw
                                           data (NULL: do not store anything)*/
{
    char command[BUFSIZE];
    char  buffer[BUFSIZE];

    float centraloct=16.0f;
    int ptinoctave=0;
    int octn=0;
    int i;
    float freq;
    FILE *fout=NULL;

    float octaveinflimit=limits[octn];
    float octavesuplimit=limits[octn+1];
    float value;
    float accum=1.0e-10f;
    float total=1.0e-10f;
    float deltaf=maxfreq/((float)npoints-1);
    /* This increases the execution speed of the transfer via GPIB */
    sprintf(command, "CONTROL_PROCESS MAXIMUM_INTERFACE_ACTIVITY\n");
    writeGPIB(command);

    if(filename!=NULL)
        fout=fopen(filename, "w");


    for (i=0; i<npoints; ++i) {
        freq=deltaf*(float)(i);
        sprintf(command, "AF IR,%d\n", i);
        writeGPIB(command);
        readGPIB(buffer, BUFSIZE);
        sscanf(buffer+1,"%f", &value);

        if(fout!=NULL) {
            fprintf(fout,"%f %f\n",freq, value);
        }
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

            float bandwidth=octavesuplimit-octaveinflimit;
            float realbandwidth=ptinoctave*deltaf;
            float correction=realbandwidth/bandwidth;

            if(acq==H1 || acq==H2) {
                accum/= realbandwidth;
            } else {
                accum *= correction;
            }

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
    if (fout!=NULL)
        fclose(fout);

    printf("Total power: %f dB/YREF*Hz\n",10*log10(total));
    sprintf(command, "CONTROL_PROCESS NORMAL_INTERFACE_ACTIVITY\n");
    writeGPIB(command);

    return octn;
}

/** Read the data of all displayed points from the 2034.
    The array pointed by storage should be allocated to contain the wanted
    number of points. Same for freqs.
    Returns the number of points read from the analyzer. If the returned
    value is different to npoints, this indicate that there has been a
    problem somewhere.
*/
int getDataPoints2034(
    float *freqs,       /* Array where the frequency points are stored */
    float *storage,     /* Array where read data will be stored */
    int npoints)        /* Number of points to be read */
{
    char command[BUFSIZE];
    char buffer[BUFSIZE];

    float value;

    int i;

    float maxfreq=readMaxFrequency2034();
    float deltaf=maxfreq/((float)npoints-1);

    /* This increases the execution speed of the transfer via GPIB */
    sprintf(command, "CONTROL_PROCESS MAXIMUM_INTERFACE_ACTIVITY\n");
    writeGPIB(command);

    for (i=0; i<npoints; ++i) {
        freqs[i]=deltaf*(float)(i);
        sprintf(command, "AF IR,%d\n", i);
        writeGPIB(command);
        readGPIB(buffer, BUFSIZE);
        sscanf(buffer+1,"%f", &value);
        storage[i]=value;
        if(i%15==0) {
            printf(".");
            fflush(stdout);
        }
    }
    printf("\n");
    sprintf(command, "CONTROL_PROCESS NORMAL_INTERFACE_ACTIVITY\n");
    writeGPIB(command);

    return npoints;
}

/** Initialize the IEEE488 interface and device
*/
void init2034(int boardIndex, int primaryAddress, int secondaryAddress)
{
    char  buffer[BUFSIZE];
    char command[BUFSIZE];

    initGPIB(boardIndex, primaryAddress, secondaryAddress);

    /* Set the line terminator for the device */
    strncpy(command, "DEFINE_TERMINATOR \n", sizeof(command));
    writeGPIB(command);
}

/** Read an identification string from the 2034.
    It is supposed to be "BK,+02034,+00000,+00000,+00002", but the
    exact contents of it are not checked. If there is a
    transmission error, exit.
*/
void identify2034(void)
{
    char command[BUFSIZE];
    char  buffer[BUFSIZE];
    // Get the device identification.
    strncpy(command, "IDENTIFY\n", sizeof(command));
    writeGPIB(command);

    if(readGPIB(buffer, BUFSIZE)==NULL) {
        exit(1);    /* A little brutal but effective */
    }

    printf("Identification received: %s", buffer);
}

/** Level 2 reset of the 2034
*/
void reset2lev2034(void)
{
    char command[BUFSIZE];
    /* Reset the device */
    strncpy(command, "SYSTEM_RESET 2\n", sizeof(command));
    writeGPIB(command);
}

