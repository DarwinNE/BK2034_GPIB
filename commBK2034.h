#ifndef COMMBK2034_H
#define COMMBK2034_H

void setBoardGPIB(int board);

void writeGPIB(char *command);
void writenGPIB(char *command, size_t numbytes);
char *readGPIB(char *buffer, size_t maxlen);


void GpibError(char *msg);
void init2034(int boardIndex, int primaryAddress, int secondaryAddress);
void closeCommIEEE(void);
void identify2034(void);



typedef enum tag_style {CH_A, CH_B, H1, H2} t_style;


void waitFor (unsigned int secs);

void reset2lev2034(void);
void configureMaxFreq2034(char *maxfreq);
void drawBandsOn2034(int octn, float *limits, float *calcvalues, float vrange);
int getBandsFrom2034(float *limits,int npoints,float maxfreq, 
    float *calcvalues, int scv, t_style acq, char *filename);
void configureAcquisitionAndGraph2034(int navg, t_style s);
void startMeasurement2034(void);
float readMaxFrequency2034(void);
void waitUntilFinished2034(int wnavg);
int getDataPoints2034(float *freqpt, float *storage, int npoints);



#endif