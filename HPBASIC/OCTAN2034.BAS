10    !                T H I R D O C T A V E 2 0 3 4
20    !
30    ! This program allows to control a Bruel&Kjaer 2034 spectrum analyzer
40    ! to perform 1/3 of octave analysis for acoustic research.
50    ! Acquisition can be fully configured and data retrieved can be
60    ! stored on files for further analysis, employed as calibration
70    ! to perform differential measurements. They can be plotted on
80    ! screen and on a plotter such as the HP7470A and sent by RS232
90    ! to another computer (interface 9, baud rate set by hardware, no
100   ! handshaking control).
110   !
120   ! Version: 1.0
130   ! Author.: Davide Bucci
140   ! Date...: January, 23, 2018
150   !
160   INTEGER I,Octn,Ptinoctave,Oa,Action,Knobpos,Nlines,Outputareasize
170   PRINTER IS CRT
180   CONTROL 9,12;128+32+16! Deactivate hardware control for RS232
190   DIM Graphtitle$[80]
200   DIM Onechar$[1]
210   Plotteraddress=704
211   Outputareasize=18    ! Number of lines in the output area.
212                        ! Change to 19 if you have a 80*26 char display.
213                        ! or 41 for a 128*48 display
220   Graphtitle$="1/3 oct. analysis"
230   DIM Xlabel$[80]
240   DIM Ylabel$[80]
250   Xlabel$="Center of 1/3 oct. in Hz"
260   Ylabel$="Density in dB"
270   Ymax=0
280   Ymin=-140
290   Ytick=20
300   Navg=20
310   Lowfreq=1
320   Highfreq=1
330   Docal=0
340   Comm2034=1
350   Gpibaddress=703
360   Acq=0
370   ABORT 7
380   DIM Filename$[80]
390   DIM Reply$[1000]
400   DIM Bands(801)
410   DIM Limits(32)
420   DIM Calcvalues(32)
430   DIM Calibration(32)
440   DATA 17.8,22.4,28.2,35.5,44.7,56.2,70.8,89.1,112,141
450   DATA 178,224,282,335,447,562,708,891,1122,1413,1778,2239,2818,3548,4467
460   DATA 5623,7079,8913,11220,14130,17780,22390,28210
470   READ Limits(*)
480   !
490   ! If the BK2034 is not present, an Init operation will timeout
500   ! We do not need to wait very long here. However, it is wise to
510   ! increase the timeout after init as some operations will take
520   ! longer than 1 second on the analyzer to be done.
530   !
540   ON TIMEOUT 7,1 GOSUB Gpibtimeout
550   CLEAR Gpibaddress
560   ASSIGN @Bkanalyzer TO Gpibaddress
570   GOSUB Init2034
580   ON TIMEOUT 7,10 GOSUB Gpibtimeout
590   GOSUB Titlebanner
600   ON KBD GOSUB Keyhandler
601   ON KNOB .2 GOSUB Knobhandler
610   ON KEY 0 LABEL "IDENTIFY" GOSUB Send_identify
620   ON KEY 2 LABEL "Acquire 1/3 o." GOSUB Thirdoct
630   ON KEY 1 LABEL "Reset lev. 2" GOSUB Reset2lev2034
640   ON KEY 3 LABEL "Plot 1/3 oct" GOSUB Plotbands
650   ON KEY 4 LABEL "Conf. plot" GOSUB Confplott
660   ON KEY 5 LABEL "Conf. acq." GOSUB Confacqt
670   ON KEY 6 LABEL "Storage" GOSUB Storagemenut
680   ON KEY 7 LABEL "Show data" GOSUB Showdata
690   ON KEY 8 LABEL "Plot 2034" GOSUB Drawbands2034
700   ON KEY 9 LABEL "RS232 com." GOSUB Communicationt
710   ON ERROR GOSUB Errorhandling
720   Action=0
730 Idleloop:   !
740   !
750   ! This is the main loop of the program that implements the
760   ! spooling of a state machine. If Action=0 then nothing is done
770   ! A positive action is done in the idle loop. A negative action is
780   ! done during the handling of the interrupt and interrupts a positive
790   ! action (for example during a menu).
800   !
810   SELECT Action
820   CASE 9
830     GOTO Communication
840   CASE 4
850     GOTO Confplot
860   CASE 5
870     GOTO Confacq
880   CASE 6
890     GOTO Storagemenu
900   END SELECT
910   GOTO Idleloop
920 ! Status indicators
930 Communicationt: !
940   Action=9
950   RETURN 
960 Confplott: !
970   Action=4
980   RETURN 
990 Confacqt: !
1000  Action=5
1010  RETURN 
1020 Storagemenut: !
1030  Action=6
1040  RETURN 
1050!
1060! Main error handling routine
1070!
1080 Errorhandling:! Main error handler
1090  Errorl=1
1100  PRINT "E";
1110  Errorn=ERRN
1120  RETURN 
1130  GOTO Idleloop
1140  !
1150  ! Timeout on the GPIB bus
1160  !
1170 Gpibtimeout:   !
1180  DISP "Timeout on the GPIB bus."
1190  Comm2034=0
1200  RETURN 
1210 Send_identify: !
1220!
1230! Send an IDENTIFICATION command on the GPIB and show the result
1240!
1250  IF Comm2034=0 THEN 
1260    DISP "Communication with B&K2034 not available."
1270    WAIT .5
1280    GOSUB Titlebanner
1290    RETURN 
1300  END IF
1310  GOSUB Titlebanner
1320  PRINT "Sending 'IDENTIFY' to ";Gpibaddress
1330  OUTPUT @Bkanalyzer;"IDENTIFY"
1340  ENTER @Bkanalyzer;Reply$
1350  PRINT Reply$
1360  RETURN 
1370!
1380! Initialize the 2034
1390!
1400 Init2034: !
1410  Comm2034=1
1420  PRINT "Init 2034..."
1430  OUTPUT @Bkanalyzer;"DEFINE_TERMINATOR "! A carriage return is added
1440  RETURN 
1450!
1460! Reset (level 2) of the 2034
1470!
1480 Reset2lev2034: !
1490  GOSUB Init2034
1500  OUTPUT @Bkanalyzer;"SYSTEM_RESET 2"
1510  RETURN 
1520!
1530! Configure the acquisition of the BK2034 to acquire the
1540! wanted spectrum and number of averages.
1550!
1560 Configacq2034: !
1570  GOSUB Init2034
1580  Channel=0! CHANNEL A
1590  Logy=1
1600  OUTPUT @Bkanalyzer;"DISPLAY_FORMAT UM"
1610  IF Comm2034=0 THEN RETURN   ! Exit in case of communication problems
1620  SELECT Acq
1621  CASE 0
1630    OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION MC 0"
1640  CASE 1
1650    OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION MC 1"
1651  CASE 2,3
1652    OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION MC 2"
1660  END SELECT
1670  OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION MM 0"
1680  OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION ZB 1"
1690  OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION AT 1"
1700  OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION AN ";Navg
1710  OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION AA 0"
1720  OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION AW 1"
1730    ! System settings for DISPLAY
1740  SELECT (Acq)
1750  CASE 0
1760    OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION FU 14"
1770  CASE 1
1780    OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION FU 15"
1790  CASE 2
1800    OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION FU 17"
1810  CASE 3
1820    OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION FU 19"
1830  END SELECT
1840  IF Logy THEN 
1850    OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION YU 1"
1860  ELSE
1870    OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION YU 0"
1880  END IF
1890  SELECT Acq
1900  CASE 0
1910    OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION SU 2"
1920  CASE 1
1930    OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION SU 2"
1940  END SELECT
1950  OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION ID 2"
1960  OUTPUT @Bkanalyzer;"EDIT_DISPLAY_SPECIFICATION DS 0"
1970  RETURN 
1980  !
1990  ! 1/3 of octave analysis
2000  !
2010 Thirdoct:    !
2020  Action=-2
2030  IF Comm2034=0 THEN 
2040    DISP "Communication with B&K not active, aborting acquisition."
2050    WAIT .5
2060    GOSUB Titlebanner
2070    RETURN 
2080  END IF
2090  GOSUB Titlebanner
2100  GOSUB Configacq2034
2110  Octn=0
2120  IF Lowfreq THEN 
2130    PRINT "First acquisition, low frequency range."
2140    Maxfreq=800
2150    GOSUB Configmaxf
2160    GOSUB Startmeas
2170    GOSUB Waituntilf
2180    GOSUB Getbands
2190    GOSUB Calcoct
2200  END IF
2210  IF Highfreq THEN 
2220    PRINT "Second acquisition, high frequency range."
2230    Maxfreq=25600
2240    GOSUB Configmaxf
2250    GOSUB Startmeas
2260    GOSUB Waituntilf
2270    GOSUB Getbands
2280    GOSUB Calcoct
2290  END IF
2291  DISP "Acquisition complete."
2300  RETURN 
2310!
2320! Configure the maximum frequency span to be used for measurements
2330! with the 2034
2340 Configmaxf:!
2350  OUTPUT @Bkanalyzer;"EDIT_MEASUREMENT_SPECIFICATION FS";Maxfreq
2360  RETURN 
2370  !
2380  ! Start the measurement
2390  !
2400 Startmeas:  !
2410  OUTPUT @Bkanalyzer;"KEY_PUSH G 0"   ! RECORD Cont
2420  OUTPUT @Bkanalyzer;"KEY_PUSH C 0"   ! AVERAGING Stop
2430  OUTPUT @Bkanalyzer;"KEY_PUSH A 0"   ! AVERAGING Start
2440  PRINT "Please push 'Start' control key on the BK2034 to start measuring."
2450  RETURN 
2460 !
2470 ! Polls the averaging number status until the wanted number of
2480 ! averages has been reached on the instrument
2490 !
2500 Waituntilf: !
2510  OUTPUT @Bkanalyzer;"PROMPT 'PLEASE START MEASUREMENTS NOW'"
2520  PRINT 
2530  REPEAT
2540    WAIT 1
2550    OUTPUT @Bkanalyzer;"CURRENT_STATUS A_N_R"
2560    ENTER @Bkanalyzer;Reply$
2570    Av=VAL(Reply$)
2580    DISP "Number of averages: ";Av;" on a total of ";Navg
2590  UNTIL Av>=Navg
2600  PRINT 
2601  OUTPUT @Bkanalyzer;"PROMPT ''"
2610  PRINT "Complete!"
2620  RETURN 
2630  !
2640  ! Read the data of all displayed points on the 2034. Return the
2650  ! number of bands covered in Nbands, or a negative number if
2660  ! something bad happened. Data will be put in Bands() array,
2670  ! that should have been DIM'ed to 801.
2680  !
2690 Getbands:  !
2700  OUTPUT @Bkanalyzer;"CONTROL_PROCESS MAXIMUM_INTERFACE_ACTIVITY"
2710  PRINT "Reading data... ";
2720  FOR I=1 TO 801
2730    Deltaf=Maxfreq/800
2740    OUTPUT @Bkanalyzer;"AF IR,";VAL$(I-1)
2750    ENTER @Bkanalyzer;Reply$
2760    Bands(I)=VAL(Reply$[2])
2770  NEXT I
2780  OUTPUT @Bkanalyzer;"CONTROL_PROCESS NORMAL_INTERFACE_ACTIVITY"
2790  PRINT "Done!"
2800  RETURN 
2810  !
2820  ! Perform the third of octave analysis
2830  !
2840 Calcoct:   !
2850  Accum=1.E-20
2860  Ptinoctave=0
2870  Total=Accum
2880  Deltaf=Maxfreq/800
2890  Octaveinflimit=Limits(Octn)
2900  Octavesuplimit=Limits(Octn+1)
2910  FOR I=1 TO 801
2920    Freq=Deltaf*I
2930 Recalc:   !
2940    IF Freq>=Octaveinflimit AND Freq<Octavesuplimit THEN 
2950      Lindata=10^(Bands(I)/10)! Converts dB into linear
2960      Accum=Accum+Lindata*Deltaf
2970      Total=Total+Lindata*Deltaf
2980      Ptinoctave=Ptinoctave+1
2990    ELSE
3000      IF Freq>Octavesuplimit THEN 
3010        Bandwidth=Octavesuplimit-Octaveinflimit
3020        Realbandwidth=Ptinoctave*Deltaf
3030        Correction=Realbandwidth/Bandwidth
3040        IF (Acq=2 OR Acq=3) THEN  ! If H1 or H2 normalize bandwidth
3050          Accum=Accum/Realbandwidth
3060        ELSE
3070          Accum=Accum*Correction
3080        END IF
3090        Accum=10*LGT(Accum)
3100        PRINT ".";
3110        Calcvalues(Octn)=Accum
3120        Octn=Octn+1
3130        Octaveinflimit=Limits(Octn)
3140        Octavesuplimit=Limits(Octn+1)
3150        Centraloct=SQR(Octavesuplimit*Octaveinflimit)
3160        Ptinoctave=0
3170        Accum=1.E-20 ! The accumulator is not set to zero to avoid
3180                     ! divide by zero errors if no point falls inside
3190                     ! the band
3200        GOTO Recalc  ! BUAHAHAHA!!!    ]:-)
3210      END IF
3220    END IF
3230  !
3240  NEXT I
3250  PRINT "  DONE!"
3260  RETURN 
3270  !
3280  ! Plot the graph on the screen of the third of octave acquisition
3290  !
3300 Plotbands:   !
3310  Action=-3
3320  PLOTTER IS CRT,"INTERNAL"
3330  CLEAR SCREEN
3340  GINIT
3350  GRAPHICS ON
3360 Plotbandsg:  ! Generic routine, not only for the screen
3370  Xmax=31
3380  Xgdu_max=100*MAX(1,RATIO)
3390  Ygdu_max=100*MAX(1,1/RATIO)
3400  PEN 1
3410  VIEWPORT 0,Xgdu_max,0,Ygdu_max
3420  LORG 6
3430  FOR I=-.3 TO .3
3440    MOVE Xgdu_max/2+I,Ygdu_max
3450    LABEL Graphtitle$
3460  NEXT I
3470  DEG
3480  LDIR 90
3490  CSIZE 3.5
3500  MOVE 0,Ygdu_max/2
3510  LABEL Ylabel$
3520  LORG 4
3530  LDIR 0
3540  MOVE Xgdu_max/2,.08*Ygdu_max
3550  LABEL Xlabel$
3560  VIEWPORT .1*Xgdu_max,.95*Xgdu_max,.15*Ygdu_max,.9*Ygdu_max
3570  FRAME
3580  WINDOW 0,Xmax,Ymin,Ymax
3590  CLIP OFF
3600  CSIZE 3
3610  LORG 6
3620  FOR I=1 TO Xmax STEP 3
3630    Center=INT(SQRT(Limits(I)*Limits(I+1))+.5)
3640    MOVE I,Ymin-(Ymax-Ymin)/200
3650    LABEL USING "#,K";Center
3660  NEXT I
3670  LORG 8
3680  FOR I=Ymin TO Ymax STEP Ytick
3690    MOVE -.05,I
3700    LABEL USING "#,K";I
3710  NEXT I
3720  CLIP ON
3730  AXES 1,Ytick/5,0,Ymin,1,5,2
3740  !GRID 3,10,Ymax,Ymin,2,5,2
3750  PEN 2
3760  FOR I=0 TO Octn
3770    IF Docal THEN 
3780      Value=Calcvalues(I)-Calibration(I)
3790    ELSE
3800      Value=Calcvalues(I)
3810    END IF
3820    MOVE I-.5,Value
3830    DRAW I+.5,Value
3840  NEXT I
3850  PENUP
3860  RETURN 
3870  !
3880  ! Configure the plotting characteristics
3890  !
3900 Confplot:  !
3910  GOSUB Titlebanner
3920  PRINT CHR$(132);"Plotting configuration";CHR$(128)
3930  PRINT 
3940  PRINT "1 - Maximum Y value..........: ";Ymax
3950  PRINT "2 - Minimum Y value..........: ";Ymin
3960  PRINT "3 - M. divisions on Y axis...: ";Ytick
3970  PRINT "4 - Title....................: ";Graphtitle$
3980  PRINT "5 - Y scale label............: ";Ylabel$
3990  PRINT "6 - X scale label............: ";Xlabel$
4000  PRINT "7 - Plotter address..........: ";Plotteraddress
4010  PRINT "8 - Dump on plotter the graph"
4020  PRINT 
4030  PRINT "Q - Quit the plotting configuration "
4040  PRINT "Which choice"
4050 Askagainplot:  !
4060  GOSUB Kinput
4070  SELECT Choice$
4080  CASE "2"
4090    INPUT "Enter the minimum value of the vertical range",Ymin
4100  CASE "1"
4110    INPUT "Enter the maximum value of the vertical range",Ymax
4120  CASE "3"
4130    INPUT "Enter the size of the major divisions on Y axis",Ytick
4140  CASE "7"
4150    DISP "Change the current plotter address (";Plotteraddress;")";
4160    INPUT Plotteraddress
4170  CASE "4"
4180    INPUT "Enter a new graph title",Graphtitle$
4190  CASE "5"
4200    INPUT "Enter a new legend for the Y axis",Ylabel$
4210  CASE "6"
4220    INPUT "Enter a new legend for the X axis",Xlabel$
4230  CASE "8"
4240    ON ERROR GOTO Cancelplot
4250    PLOTTER IS Plotteraddress,"HPGL"
4260    GSEND "IN"
4270    DISP "Plotter configured. Plotting now."
4280    GOSUB Plotbandsg
4290    PLOTTER IS CRT,"INTERNAL"
4300  CASE "q","Q"
4310    GOSUB Titlebanner
4320    Action=0
4330    GOTO Idleloop
4340  CASE "" ! Change action state
4350    GOTO Idleloop
4360  CASE ELSE
4370    DISP "Invalid option"
4380    WAIT .5
4390    DISP ""
4400    GOTO Askagainplot
4410  END SELECT
4420  GOTO Confplot
4430 Cancelplot:  !
4440  DISP "Errror during plotting."
4450  ON ERROR GOSUB Errorhandling
4460  GOTO Confplot
4470  !
4480  ! Show the title banner of the program
4490  !
4500 Titlebanner: !
4510  CLEAR SCREEN
4520  GRAPHICS OFF
4530  PRINT "      ";CHR$(129);
4540  PRINT " Third of octave analysis routine for Bruel & Kjaer 2034 "
4550  PRINT CHR$(128);
4560  PRINT "      ";CHR$(129);
4570  PRINT "   Copyright Davide Bucci 2015 (HP-BASIC version 2018)   "
4580  PRINT CHR$(128);
4590  PRINT 
4600  IF Comm2034 THEN 
4610    PRINT CHR$(132);"B&K2034 com.:";CHR$(128);" active   ";
4620  ELSE
4630    PRINT CHR$(132);"B&K2034 com.:";CHR$(128);" none     ";
4640  END IF
4650  IF Octn>0 THEN 
4660    PRINT CHR$(132);"Data:";CHR$(128);" present   ";
4670  ELSE
4680    PRINT CHR$(132);"Data:";CHR$(128);" none      ";
4690  END IF
4700  IF Docal THEN 
4710    PRINT CHR$(132);"Calibration:";CHR$(128);" active"
4720  ELSE
4730    PRINT CHR$(132);"Calibration:";CHR$(128);" off   "
4740  END IF
4750  PRINT 
4760  RETURN 
4770  !
4780  ! Configure the acquisition
4790  !
4800 Confacq:   !
4810  GOSUB Titlebanner
4820  PRINT 
4830  PRINT CHR$(132);"Acquisition configuration";CHR$(128)
4840  PRINT 
4850  PRINT "1 - Number of averages...............: ";Navg
4860  PRINT "2 - Low freq. acquisition............: ";
4870  IF Lowfreq THEN 
4880    PRINT "Yes"
4890  ELSE
4900    PRINT "No"
4910  END IF
4920  PRINT "3 - High freq. acquisition...........: ";
4930  IF Highfreq THEN 
4940    PRINT "Yes"
4950  ELSE
4960    PRINT "No"
4970  END IF
4980  PRINT "4 - Use current data as calibration"
4990  IF Docal THEN 
5000    PRINT "5 - Deactivate calibration"
5010  ELSE
5020    PRINT "5 - Activate calibration"
5030  END IF
5040  PRINT "6 - Address of B&K 2034..............: ";Gpibaddress
5060  PRINT "7 - Acquisition type.................: ";
5070  SELECT Acq
5080  CASE 0
5090    PRINT "Channel A"
5100  CASE 1
5110    PRINT "Channel B"
5120  CASE 2
5130    PRINT "H1"
5140  CASE 3
5150    PRINT "H2"
5160  END SELECT
5161  PRINT 
5170  PRINT "Q - Exit the acquisition configuration"
5180  PRINT "Which choice"
5190 Askagainacq:  !
5200  GOSUB Kinput
5210  SELECT Choice$
5220  CASE "1"
5230 Askavg:  !
5240    INPUT "Enter the number of averages for each acquisition",Navg
5250    IF Navg<1 OR Navg>32000 THEN 
5260      DISP "Invalid number of averages, must be between 1 and 32000"
5270      WAIT 1
5280      GOTO Askavg
5290    END IF
5300  CASE "2"
5310 Asklowfreq:      !
5320    INPUT "Perform a low frequency acquisition (yes/no)",Answer$
5330    SELECT Answer$
5340    CASE "yes","Yes","YES"
5350      Lowfreq=1
5360    CASE "no","No","NO"
5370      Lowfreq=0
5380    CASE ELSE
5390      DISP "I have not understood. Type 'yes' or 'no'."
5400      WAIT 1
5410      GOTO Asklowfreq
5420    END SELECT
5430  CASE "3"
5440 Askhighfreq:     !
5450    INPUT "Perform a high frequency acquisition (yes/no)",Answer$
5460    SELECT Answer$
5470    CASE "yes","Yes","YES"
5480      Highfreq=1
5490    CASE "no","No","NO"
5500      Highfreq=0
5510    CASE ELSE
5520      DISP "I have not understood. Type 'yes' or 'no'."
5530      WAIT 1
5540      GOTO Askhighfreq
5550    END SELECT
5560  CASE "4"
5570    FOR I=0 TO Octn-1
5580      Calibration(I)=Calcvalues(I)
5590    NEXT I
5600    Docal=1
5610  CASE "5"
5620    IF Docal THEN 
5630      Docal=0
5640    ELSE
5650      Docal=1
5660    END IF
5670  CASE "q","Q"
5680    GOSUB Titlebanner
5690    Action=0
5700    GOTO Idleloop
5710  CASE "6"
5720    Ogpibadd=Gpibaddress
5730    INPUT "Enter the new address of the Bruel&Kjaer 2034",Gpibaddress
5740    ON ERROR GOTO Invalidadd
5750    ASSIGN @Bkanalyzer TO Gpibaddress
5760    ON ERROR GOSUB Errorhandling
5770  CASE "7"
5780    INPUT "Acquisition type? (1-ChA, 2-ChB, 3-H1, 4-H2)",I
5790    IF I<1 OR I>4 THEN 
5800      DISP "Invalid option"
5810    ELSE
5820      Acq=I-1
5830    END IF
5840  CASE ""
5850    GOTO Idleloop
5860  CASE ELSE
5870    DISP "Ivalid option"
5880    WAIT .5
5890    DISP ""
5900    GOTO Askagainacq
5910  END SELECT
5920  GOTO Confacq
5930 Invalidadd:  !
5940  DISP "Invalid address, retry."
5950  BEEP 
5960  WAIT 1
5970  Gpibaddress=Ogpibadd
5980  GOTO Confacq
5990  !
6000  ! Storage menu
6010  !
6020 Storagemenu:  !
6030  GOSUB Titlebanner
6040  CAT 
6050  PRINT 
6060  PRINT CHR$(132);"Mass storage menu";CHR$(128)
6070  PRINT 
6080  PRINT "1 - Load data from file"
6090  PRINT "2 - Save data in file"
6100  PRINT "3 - Change default mass storage: ";Ms$
6110  PRINT "4 - Delete a file"
6120  PRINT 
6130  PRINT "Q - Exit from storage menu"
6140  PRINT "Which choice"
6150 Askagainstorage:  !
6160  GOSUB Kinput
6170  SELECT Choice$
6180  CASE "1"
6190    GOSUB Loaddata
6200    IF Errorl THEN GOTO Storagemenu
6210    Action=0
6220    GOSUB Titlebanner
6230    GOTO Idleloop
6240  CASE "2"
6250    GOSUB Savedata
6260    IF Errorl THEN GOTO Storagemenu
6270    Action=0
6280    GOSUB Titlebanner
6290    GOTO Idleloop
6300  CASE "3"
6310    INPUT "Write the mass storage device to use",Ms$
6320    Errorl=0
6330    IF Errorl=0 THEN MASS STORAGE IS Ms$
6340    IF Errorl THEN DISP "Invalid MS: ";ERRM$
6350  CASE "4"
6360    INPUT "Enter the name of the file to delete",Filename$
6370    PRINT "Are you sure you want to delete ";Filename$;"?"
6380    INPUT "(yes, no)",Answer$
6390    IF Answer$="yes" OR Answer$="YES" OR Answer$="Yes" THEN 
6400      Errorl=0
6410      IF Errorl=0 THEN PURGE Filename$
6420      IF Errorl THEN DISP "Error during PURGE command: ";ERRM$
6430      WAIT 1
6440    END IF
6450  CASE "Q","q"
6460    GOSUB Titlebanner
6470    Action=0
6480    GOTO Idleloop
6490  CASE "" ! Change state
6500    GOTO Idleloop
6510  CASE ELSE
6520    DISP "Invalid option"
6530    WAIT .5
6540    DISP ""
6550    GOTO Askagainstorage
6560  END SELECT
6570  GOTO Storagemenu
6580  !
6590  ! Save data from disk
6600  !
6610 Savedata:   !
6620  INPUT "Enter the name of the file where to save",Filename$
6630  Errorl=0
6640  IF Errorl=0 THEN CREATE ASCII Filename$,10
6650  IF Errorl THEN 
6660    DISP "Could not create file ";Filename$;" ";ERRM$
6670    RETURN 
6680  END IF
6690  ON ERROR GOTO Saveerror
6700  ASSIGN @Path1 TO Filename$
6710 Savedataint:  !
6720  OUTPUT @Path1;"1/3 of octave analysis results file"
6730  OUTPUT @Path1;"Number of averages: ";Navg
6740  OUTPUT @Path1;"Min. val. of Y axis: ";Ymin
6750  OUTPUT @Path1;"Max. val. of Y axis: ";Ymax
6760  OUTPUT @Path1;"Ticks in Y axis: ";Ytick
6770  OUTPUT @Path1;"Title of the graph: ";Graphtitle$
6780  OUTPUT @Path1;"X axis label: ";Xlabel$
6790  OUTPUT @Path1;"Y axis label: ";Ylabel$
6800  OUTPUT @Path1;"Low freq. acq: ";Lowfreq
6810  OUTPUT @Path1;"High freq. acq: ";Highfreq
6820  OUTPUT @Path1;"Number of valid data points: ";Octn
6830  OUTPUT @Path1;"1/3 oct. data: ";Calcvalues(*)
6840  OUTPUT @Path1;"Calibration: ";Docal
6850  OUTPUT @Path1;"Calibration data: ";Calibration(*)
6860  ASSIGN @Path1 TO *
6870 Endsave:  !
6880  IF Errorl=0 THEN DISP Filename$;" written on ";Ms$
6890  IF Errorl THEN 
6900    DISP "Error: ";ERRM$
6910    WAIT 1
6920  END IF
6930  ON ERROR GOSUB Errorhandling
6940  WAIT 1
6950  RETURN 
6960 Saveerror:  !
6970  Errorn=1
6980  GOTO Endsave
6990  !
7000  ! Load data on a file
7010  !
7020 Loaddata:   !
7030  INPUT "Enter the name of the file to read",Filename$
7040  Errorl=0
7050  IF Errorl=0 THEN ASSIGN @Path1 TO Filename$
7060  IF Errorl THEN 
7070    DISP "Could not access to file ";Filename$
7080    WAIT 1
7090    RETURN 
7100  END IF
7110  ON ERROR GOTO Loaderror
7120 Loaddataint:  !
7130  ENTER @Path1;Reply$
7140  ENTER @Path1;Reply$;Navg
7150  ENTER @Path1;Reply$;Ymin
7160  ENTER @Path1;Reply$;Ymax
7170  ENTER @Path1;Reply$;Ytick
7180  ENTER @Path1;Reply$;Graphtitle$
7190  ENTER @Path1;Reply$;Xlabel$
7200  ENTER @Path1;Reply$;Ylabel$
7210  ENTER @Path1;Reply$;Lowfreq
7220  ENTER @Path1;Reply$;Highfreq
7230  ENTER @Path1;Reply$;Octn
7240  ENTER @Path1;Reply$;Calcvalues(*)
7250  ENTER @Path1;Reply$;Docal
7260  ENTER @Path1;Reply$;Calibration(*)
7270  ASSIGN @Path1 TO *
7280 Endload:  !
7290  IF Errorl=0 THEN DISP "Data retrieved."
7300  IF Errorl THEN DISP "Error: ";ERRM$;" but I tried to get some data."
7310  ON ERROR GOSUB Errorhandling
7320  WAIT 1
7330  RETURN 
7340 Loaderror:  ! Handle reading errors
7350  Errorl=1
7360  GOTO Endload
7370  !
7380  ! Show data
7390  !
7400 Showdata:  !
7410  Action=-7
7420  GOSUB Titlebanner
7430  PRINT 
7440  IF Octn=0 THEN 
7450    PRINT "No data"
7460    RETURN 
7470  END IF
7480  FOR I=0 TO Octn-1
7490    IF Docal THEN 
7500      Value=Calcvalues(I)-Calibration(I)
7510    ELSE
7520      Value=Calcvalues(I)
7530    END IF
7540    PRINT "From";Limits(I);" Hz to ";Limits(I+1);" Hz";
7550    PRINT " power:";
7560    PRINT USING "#,K";INT(Value*100+.5)/100
7570    IF Docal THEN 
7580      PRINT " dB"
7590    ELSE
7600      PRINT " dB/YREF/Hz"
7610    END IF
7620  NEXT I
7630  RETURN 
7640  !
7650  ! Show graphically the result of the band analysis on the 2034
7660  !
7670 Drawbands2034:  !
7680  Action=-8
7690  IF Comm2034=0 THEN 
7700    DISP "Communication with B&K2034 not available."
7710    RETURN 
7720  END IF
7730  OUTPUT @Bkanalyzer;"WRITE_TEXT CLEAR_HOME"
7740  OUTPUT @Bkanalyzer;"WRITE_TEXT 1,5,'1/3 OCTAVE ANALYSIS";
7750  OUTPUT @Bkanalyzer;", DAVIDE BUCCI 2015-2018'"
7760  IF Octn=0 THEN 
7770    OUTPUT @Bkanalyzer;"WRITE_TEXT 3,5,'NO ANALYSIS AVAILABLE'"
7780    RETURN 
7790  END IF
7800  Vrange=80
7810  Screenwidth=401
7820  Octwidth=Screenwidth/Octn
7830  Baseline=200
7840  Mult=50/Vrange*4
7850  Maxv=-200
7860 ! Search for the maximum value
7870  FOR I=0 TO Octn-1
7880    IF Docal THEN 
7890      Valt=Calcvalues(I)-Calibration(I)
7900    ELSE
7910      Valt=Calcvalues(I)
7920    END IF
7930    IF Valt>Maxv THEN 
7940      Maxv=Valt
7950    END IF
7960  NEXT I
7970  Maxv=INT(Maxv*100+.5)/100
7980  OUTPUT @Bkanalyzer;"WRITE_TEXT 2,5,'V. RANGE:";Vrange;"dB, TOP:";Maxv;
7990  OUTPUT @Bkanalyzer;"dB'"
8000  OUTPUT @Bkanalyzer;"CONTROL_PROCESS DISPLAY_MODE 1,0,1"
8010  OUTPUT @Bkanalyzer;"PLOT_FOREGROUND 0,0"
8020  Countthirdoct=0
8030  FOR I=0 TO Octn-1
8040    IF Docal THEN 
8050      Calcv=Calcvalues(I)-Calibration(I)
8060    ELSE
8070      Calcv=Calcvalues(I)
8080    END IF
8090    Valv=((Calcv-Maxv)*Mult)+Baseline+.5
8100    OUTPUT @Bkanalyzer;"PLOT_CONTINUE_FOREGROUND ";INT(Octwidth*I);",";
8110    OUTPUT @Bkanalyzer;INT(Valv);",";INT(Octwidth*(I+1)-1);",";INT(Valv)
8120    Octaveinflimit=Limits(I)
8130    Octavesuplimit=Limits(I+1)
8140    Centraloct=SQR(Octavesuplimit*Octaveinflimit)
8150    Countthirdoct=Countthirdoct+1
8160    IF Countthirdoct MOD 3=2 THEN 
8170      Corr=Octwidth/2-6
8180      IF Centraloct<100 THEN 
8190        Corr=Corr+2
8200      END IF
8210      IF Centraloct>=100 AND Centraloct<1000 THEN 
8220        Corr=Corr+6
8230      END IF
8240      IF Centraloct>=1000 AND Centraloct<10000 THEN 
8250        Corr=Corr+9
8260      END IF
8270      IF Centraloct>10000 THEN 
8280        Corr=Corr+12
8290      END IF
8300      Posx=Octwidth*(I+1)-Corr+.5
8310      OUTPUT @Bkanalyzer;"PLOT_CONTINUE_FOREGROUND ";INT(Posx);",20,";
8320      OUTPUT @Bkanalyzer;"'";INT(Centraloct+.5);"'"
8330    END IF
8340  NEXT I
8350  RETURN 
8360 !
8370 ! Communication menu
8380 !
8390 Communication: !
8400  GOSUB Titlebanner
8410  PRINT 
8420  PRINT CHR$(132);"Communication with RS232 menu";CHR$(128)
8430  PRINT 
8440  PRINT "1 - Miniterminal"
8450  PRINT "2 - Send current data"
8460  PRINT "3 - Receive data"
8470  PRINT 
8480  PRINT "Q - Exit menu"
8490  PRINT "Which choice"
8500 Askagaincomm:  !
8510  Oa=Action
8520  GOSUB Kinput
8530  SELECT Choice$
8540  CASE "1"
8550    GOSUB Miniterm
8560  CASE "2"
8570    GOSUB Sendrs232
8580    GOTO Askagaincomm
8590  CASE "3"
8600    GOSUB Receivers232
8610    GOSUB Titlebanner
8620    Action=0
8630  CASE "q","Q"
8640    GOSUB Titlebanner
8650    Action=0
8660  CASE ""
8670    GOTO Idleloop
8680  CASE ELSE
8690    DISP "Invalid option."
8700    WAIT .5
8710    DISP ""
8720    GOTO Askagaincomm
8730  END SELECT
8740  IF Oa<>Action THEN GOTO Idleloop
8750  GOTO Communication
8760 !
8770 ! Keyboard input, return user choice in variable Choice$
8780 !
8790 Kinput: !
8800  Choice$=""
8810  Oa=Action
8820  IF Choice$="" AND Action=Oa THEN 8820
8830  RETURN 
8840 Keyhandler:  !
8850  Choice$=KBD$
8860  RETURN 
8861 Knobhandler:  !
8862  Knobpos=KNOBY
8863  IF Knobpos>0 THEN Knobpos=Outputareasize+Knobpos
8864  STATUS 1,3;Nlines
8865  IF Nlines+Knobpos<1 THEN Knobpos=Nlines+1
8869  CONTROL 1,1;Knobpos
8871  OUTPUT 1;CHR$(0);
8872               RETURN 
8873  !
8880  ! Miniterminal on RS232
8890  !
8900 Miniterm:  !
8910  CLEAR SCREEN
8920  Oa=Action
8930  Addcr=1
8940  GOSUB Disptermline
8950  Choice$=""
8960  Warning=0
8970  ASSIGN @Bufin TO BUFFER [1000]
8980  ASSIGN @Rs232in TO 9
8990  ASSIGN @Rs232out TO 9
9000  TRANSFER @Rs232in TO @Bufin;CONT
9010  ASSIGN @Bufout TO BUFFER [100]
9020  TRANSFER @Bufout TO @Rs232out;CONT
9030  OUTPUT @Bufout;" "
9040 Termloop: ! Main terminal loop
9050  IF Oa<>Action THEN               ! Action has changed (show another thing)
9060    GOTO Exittermloop
9070  END IF
9080  IF LEN(Choice$)>0 THEN 
9090    SELECT Choice$
9100    CASE CHR$(255)&"E"             ! Control code for return
9110      Choice$=CHR$(13)&CHR$(10)
9120    CASE CHR$(255)&"B"             ! Control code for backspace
9130      Choice$=CHR$(8)
9140    CASE CHR$(17)                  ! Control code for CTRL-Q
9150      GOTO Exittermloop
9160    CASE CHR$(4)                   ! Control code for CTRL-D
9170      IF Addcr THEN 
9180        Addcr=0
9190      ELSE
9200        Addcr=1
9210      END IF
9220      GOSUB Disptermline
9230    CASE CHR$(255)&"U"             ! Caps lock
9240      STATUS 2,0;I
9250      IF I THEN 
9260        CONTROL 2,0;0
9270      ELSE
9280        CONTROL 2,0;1
9290      END IF
9300      GOTO Termloop
9310    CASE ELSE                      ! Do not send control chars
9320      IF Choice$[1,1]=CHR$(255) THEN 
9330        Choice$=""
9340        GOTO Termloop
9350      END IF
9360    END SELECT
9370    PRINT CHR$(132);Choice$;CHR$(128);
9380    OUTPUT @Bufout;Choice$;
9390    Choice$=""
9400  END IF
9410  STATUS @Bufin,4;I
9420  IF I>0 THEN 
9430    IF I>900 THEN 
9440      DISP "WARNING: Receive buffer is more than 90% full!"
9450      Warning=1
9460    ELSE
9470      IF Warning THEN 
9480        Warning=0
9490        DISP ""
9500      END IF
9510    END IF
9520    ENTER @Bufin USING "#,A";A$
9530    IF A$=CHR$(13) AND Addcr THEN 
9540      A$=CHR$(13)&CHR$(10)
9550    END IF
9560    PRINT A$;
9570  END IF
9580  GOTO Termloop
9590 Disptermline:  !
9600  DISP "CTRL+Q quit miniterminal. ";
9610  IF Addcr THEN 
9620    DISP "CTRL+D disable add CR to LF."
9630  ELSE
9640    DISP "CTRL+D enable add CR to LF."
9650  END IF
9660  RETURN 
9670 Exittermloop:  !
9680  ABORTIO @Rs232in
9690  ABORTIO @Rs232out
9700  ASSIGN @Rs232 TO *
9710  ASSIGN @Bufin TO *
9720  ASSIGN @Bufout TO *
9730  RETURN 
9740  !
9750  ! Send data via RS232
9760  !
9770 Sendrs232:  !
9780  DISP "Sending data via RS232 (interface 9)"
9790  ASSIGN @Path1 TO 9
9800  GOSUB Savedataint
9810  DISP ""
9820  RETURN 
9830  !
9840  ! Receive via RS232
9850  !
9860 Receivers232:  !
9870  DISP "Receive data via RS232 (interface 9)"
9880  ON ERROR GOTO Rs232imp
9890  ASSIGN @Rs232 TO 9
9900  ASSIGN @Path1 TO BUFFER [1000]
9910  TRANSFER @Rs232 TO @Path1;CONT
9920  ON ERROR GOTO Errrs232
9930  ENTER @Path1;Reply$
9940  PRINT Reply$
9950  ENTER @Path1;Reply$
9960  PRINT Reply$
9970  GOSUB Extract
9980  Navg=VAL(Reply$)
9990  ENTER @Path1;Reply$
10000 GOSUB Extract
10010 Ymin=VAL(Reply$)
10020 ENTER @Path1;Reply$
10030 GOSUB Extract
10040 Ymax=VAL(Reply$)
10050 ENTER @Path1;Reply$
10060 GOSUB Extract
10070 Ytick=VAL(Reply$)
10080 ENTER @Path1;Reply$
10090 GOSUB Extract
10100 Graphtitle$=Reply$
10110 ENTER @Path1;Reply$
10120 GOSUB Extract
10130 Xlabel$=Reply$
10140 ENTER @Path1;Reply$
10150 GOSUB Extract
10160 Ylabel$=Reply$
10170 ENTER @Path1;Reply$
10180 GOSUB Extract
10190 Lowfreq=VAL(Reply$)
10200 ENTER @Path1;Reply$
10210 GOSUB Extract
10220 Highfreq=VAL(Reply$)
10230 ENTER @Path1;Reply$
10240 GOSUB Extract
10250 Octn=VAL(Reply$)
10260 ENTER @Path1;Reply$
10270 GOSUB Extract
10280 ENTER Reply$;Calcvalues(*)
10290 ENTER @Path1;Reply$
10300 GOSUB Extract
10310 Docal=VAL(Reply$)
10320 ENTER @Path1;Reply$
10330 GOSUB Extract
10340 ENTER Reply$;Calibration(*)
10350 Exitrs232: !
10360 ABORTIO @Rs232
10370 ASSIGN @Path1 TO *
10380 ASSIGN @Rs232 TO *
10390 ON ERROR GOSUB Errorhandling
10400 WAIT 1
10410 RETURN 
10420 Extract: !
10430 PRINT Reply$
10440 I=POS(Reply$,":")
10450 Reply$=Reply$[I+1]
10460 RETURN 
10470 Errrs232:!
10480 DISP "There was a communication error or the data was not formatted"
10490 GOTO Exitrs232
10500 Rs232imp:!
10510 DISP "Access to RS232 (interface 9) is impossible."
10520 RETURN 
10530 END
