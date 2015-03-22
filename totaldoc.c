/*****************************************************************************
 * TOTAL DOCUMENTATION
 *
 * Davide Bucci March 17, 2015
 * 
 * GPL v.3, tested on MacOSX 10.6.8 with NI drivers IEEE488
 *****************************************************************************/

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>

#include "commBK2034.h"

#define HELP_STR \
"This software communicates with a Bruel&Kjaer 2034 double channel FFT\n"\
"spectrum analyzer and records or sends back to the instrument its current\n"\
"state, which comprises the measurement settings, the data acquired as well\n"\
"as the visualization settings.\n"\
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
"   -r     Record a total documentation of the BK 2034 on a given file.\n"\
"              (2034 -> file)\n"\
"\n"\
"   -t     Send back a total documentation to the BK 2034 from a given file.\n"\
"              (file -> 2034)\n"\
"\n\n"




/** Main routine. Parse and execute command options (see the HELP_STR string
    which contains the help of the program.
*/
int main(int argc, char**argv) 
{
    int   primaryAddress = 3;  /* Primary address of the device           */
    int   secondaryAddress = 0; /* Secondary address of the device        */

    
    char *storagefile=NULL;
    int i;
    int nbytes;
    char buffer[1001];
    char command[1001];
    char display_spec[1001];
    char measurement_spec[1001];

    char *data=NULL;
    FILE *filep=NULL;
    bool read=true;
    
    printf("\nTotal documentation of a B&K 2034 via GPIB\n\n");
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
            } else if(strcmp(argv[i], "-r")==0) { /* -r BK2034 -> file */
            	read=true;
                if(argc>i+1) {
                    storagefile=argv[++i];
                } else {
                    fprintf(stderr, "-r requires the file name.\n");
                }
            } else if(strcmp(argv[i], "-t")==0) { /* -t file -> BK2034 */
            	read=false;
                if(argc>i+1) {
                    storagefile=argv[++i];
                } else {
                    fprintf(stderr, "-t requires the file name.\n");
                }
            }
        }
    }
    
    printf("Configuration settings:\n");
    printf("Primary address: %d, secondary: %d \n", primaryAddress, 
        secondaryAddress);

    init2034(0, primaryAddress, secondaryAddress);
    identify2034();
	
	if(storagefile==NULL) {
		fprintf(stderr, "You should use -r or -t options to specify an "
			"action, as well as a filename.\n");
		return 1;
	}
	
	if(read) {
		/* Send a TOTAL DOCUMENTATION command. Apparently, that technique was 
	   		used to save on a compact cassette the data read by the BK2034 for
	   		a later use. */
		sprintf(command, "TOTAL_DOCUMENTATION\n");
    	
    	writeGPIB(command);
    
    	/* Read the data back. At first a TD */
		
		readGPIB(buffer, 3);
		buffer[2]='\0';
	
		if(strcmp(buffer, "TD")!=0) {
			fprintf(stderr, "TD has not been received from the instrument "
					"(%s).\n",buffer);
		}

		/* Then the Display Specification data */
		readGPIB(display_spec, 206);
		/* The Measurement Specification data */
		readGPIB(measurement_spec, 340);
		/* The number of bytes */
		readGPIB(buffer, 2);
		buffer[2]='\0';
	
		nbytes=buffer[0]+buffer[1]*256;
		printf("Reading %d bytes from the B&K 2034.\n", nbytes);
		data=(char*)calloc(nbytes, 1);
		readGPIB(data, nbytes);
		
		filep=fopen(storagefile, "w");
		if(filep==NULL) {
			fprintf(stderr, "Could not open output file.\n");
			if(data!=NULL)
				free(data);
			return 1;
		}
		
		fprintf(filep, "TD ");
		fwrite(display_spec, 206, 1, filep); 
		fwrite(measurement_spec, 340, 1, filep);
		fwrite(buffer, 2, 1, filep);
		fwrite(data, nbytes, 1, filep);
		fprintf(filep, "\n");
			
		readGPIB(buffer, 1);

		fclose(filep);
		free(data);
		printf("File %s written\n",storagefile);
	} else {
		/*  Read all the contents of a file and send them to the BK2034.
			The file does not need to contain a TOTAL DOCUMENTATION command,
			even if this will be the most frequent case where the utility 
			will be used.
		*/
		filep=fopen(storagefile, "r");
		
		if(filep==NULL) {
			fprintf(stderr, "Could not open input file.\n");
			return 1;
		}
		printf("Open file %s for read.\n", storagefile);
		int nbytes;
		fseek(filep , 0 , SEEK_END);
  		nbytes = ftell(filep);
  		rewind(filep);
		
		char *filebuffer = (char*) malloc (sizeof(char)*nbytes);
  		if (filebuffer == NULL) {
  			fprintf(stderr, "Could not allocate enough memory to read the "
  				"input file.\n");
  			return 2;
  		}

  		int result = fread (filebuffer,1, nbytes,filep);
  		if (result != nbytes) {
  			fprintf(stderr, "Could not read correctly the input file.\n");
  			return 3;
  		}
		
		writenGPIB(filebuffer, nbytes);
     	fclose(filep);
	}
	
	closeCommIEEE();
       
    return 0;
}

