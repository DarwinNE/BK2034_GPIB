/*****************************************************************************
 *
 * Compiling and linking this application from command line:
 *        gcc totaldoc.c -m32 -framework NI488 -o totaldoc
 *
 * Davide Bucci March 17, 2015
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
#include <stdlib.h>
#include <stdbool.h>

#define HELP_STR \
"This software communicates with a Bruel&Kjaer 2034 double channel FFT\n"\
"spectrum analyzer and records or send back to the instrument its current\n"\
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


int Device = 0;                   /* Device unit descriptor                  */
int BoardIndex = 0;

void GpibError(char *msg);
void init2034(int boardIndex, int primaryAddress, int secondaryAddress);
void closeCommIEEE(void);
void identify2034(void);


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
	}
	
	if(read) {
		/* Send a TOTAL DOCUMENTATION command. Apparently, that technique was 
	   		used to save on a compact cassette the data read by the BK2034 for
	   		a later use. */
		sprintf(command, "TOTAL_DOCUMENTATION\n");
    	ibwrt(Device, command, strlen(command));
    
    	/* Read the data back. At first a TD */
		ibrd(Device, buffer, 3);
		buffer[2]='\0';
	
		if(strcmp(buffer, "TD")!=0) {
			fprintf(stderr, "TD has not been received from the instrument "
					"(%s).\n",buffer);
		}

		/* Then the Display Specification data */
		ibrd(Device, display_spec, 206);
		/* The Measurement Specification data */
		ibrd(Device, measurement_spec, 340);
		/* The number of bytes */
		ibrd(Device, buffer, 2);
		buffer[2]='\0';
	
		nbytes=buffer[0]+buffer[1]*256;
		printf("Reading %d bytes from the B&K 2034.\n", nbytes);
		data=(char*)calloc(nbytes, 1);
		ibrd(Device, data, nbytes);
		
		filep=fopen(storagefile, "w");
		if(filep==NULL) {
			fprintf(stderr, "Could not open output file.\n");
			if(data!=NULL)
				free(data);
			return 1;
		}
		
		fprintf(filep, "TD %s%s%s%s\n", display_spec, measurement_spec, 
			buffer, data);
		fclose(filep);
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
		int ch;
		while((ch = fgetc(filep)) != EOF) {
			ibwrt(Device, (char*) &ch, 1);
		}
     	fclose(filep);
	}
	
	closeCommIEEE();
       
    return 0;
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