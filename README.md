# BK2034_GPIB
Tools for controlling a Bruel &amp; Kjaer 2034 via the GPIB (IEEE488) bus

You must have a National Instrument GPIB IEEE488 interface (mine is a NI GPIB-USB-HS) with the drivers installed in your system.
The utilities have been developed and tested with MacOSX 10.6.9 and 10.9.5 with the National Instruments IEEE488 framework.


|File          | Description |
|--------------|-------------|
|octave.c      | Performs a read of the spectrum and calculates the 1/3 of octave representation. |
|totaldoc.c    | Read or send back a "total documentation" of the 2034 (backup of configuration and measurements). |
|impedance.c   | Perform an impedance measurement via a resistive bridge. |
|record.c      | Record data on the display of the instrument. |
|commBK2034.c  | Several routines for the GPIB control of the 2034. |



## octave

Third-octave analysis with B&K 2034 via GPIB

Davide Bucci, 2015

This software communicates with a Bruel&Kjaer 2034 double channel FFT
spectrum analyzer to perform 1/3 of octave analysis of an input signal
The 2034 acquires 801 points for each FFT spectrum, so to cover the
whole audio range, the spectrum should be acquired twice. The first
time, this is done in a band from 0 Hz to 800 Hz and the second time
the higher frequency limit is increased to 25.6 kHz.
The measurement is done by considering that a Power Spectral Density is
acquired by the instrument. This means that the signal acquired has its
power spectral density which is larger of the FFT bands of the instrument.
In other words, this works correctly when using a pink or white noise,
but not a signal having a line spectrum.

The output can be written on the screen of the PC running this utility,
or can be plotted in a graphical way on the screen of the BK 2034 itself.
It is also possible to choose a file where to write the results.

The following options are available:

| Option | Action
|--------|------------------------------------------------------------|
|  -h    | Show this help
|  -b    | Change the board interface index (GPIB0=0,GPIB1=1,etc.)
|  -p    | Change the primary address of the BK 2034.
|  -s    | Change the secondary address of the BK 2034.
|  -cb   | Acquire ch.B instead of ch.A.
|  -h1   | Acquire H1 transfer function instead of the spectrum of ch.A. In this case, the transfer function will be normalized to the width of each octave: a flat H1 transfer function will give a flat third of octave representation.
|  -h2   | Acquire H2 transfer function instead of the spectrum of ch.A. See -h1 for the representation.
|  -a    | Choose the number of averages to be done on each acquisition. The default value is 20.
|  -l    | Perform only the low frequency acquisition (the higher band  therefore will be the 1/3 of octave comprised between 562 Hz and 708 Hz.
|  -f    | Perform only the high frequency acquisition (the lowest band will start from 447 Hz).
|  -o    | Write on a file the results.
|  -lf   | Write on a file the data collected in the first pass.
|  -ff   | Write on a file the data collected in the second pass.
|  -n    | Do not draw the graph on the BK2034 at the end of acquisitions.
|  -v    | Specify vertical range in dB for the graphs (between 5 and 160).
|  -c    | Use a calibration file. Measurement results will be normalized to those read from the given file.

## totaldoc

Total documentation of a B&K 2034 via GPIB

Davide Bucci, 2015

This software communicates with a Bruel&Kjaer 2034 double channel FFT
spectrum analyzer and records or sends back to the instrument its current
state, which comprises the measurement settings, the data acquired as well
as the visualization settings.


The following options are available:

| Option | Action
|--------|------------------------------------------------------------|
|  -h    | Show this help
|  -b    | Change the board interface index (GPIB0=0,GPIB1=1,etc.)
|  -p    | Change the primary address of the BK 2034.
|  -s    | Change the secondary address of the BK 2034.
|  -r    | Record a total documentation of the BK 2034 on a given file (2034 -> file).
|  -t    | Send back a total documentation to the BK 2034 from a given file  (file -> 2034).

## impedance

Impedance measurement with a B&K 2034 via GPIB

Davide Bucci, 2015

This software communicates with a Bruel&Kjaer 2034 double channel FFT
spectrum analyzer and performs an impedance measurement in the 0-25.6kHz
band of the instrument. The measurement may be done by using the internal
signal generator of the instrument, as a pseudo-random noise generator.
The output of the generator should be used to fed the device under test,
by putting a series resistor of known value R. By default, the program
considers a value of R of 1000 Ω. Connect the input of channel A to the
output of the signal generator and use channel B to probe the voltage
at the terminals of the device. The output impedance of the generator
(by default 0 Ω) can be taken into account separately.


The following options are available:

| Option | Action
|--------|------------------------------------------------------------|
|  -h    | Show this help
|  -b    | Change the board interface index (GPIB0=0,GPIB1=1,etc.)
|  -p    | Change the primary address of the BK 2034.
|  -s    | Change the secondary address of the BK 2034.
|  -a    | Choose the number of averages to be done on each acquisition. The default value is 20.
|  -o    | Write on a file the results.
|  -r    | Changes the resistance R to be used in the divider (in Ω)
|  -g    | Changes the generator impedance (in Ω)

## record

Record data from a B&K 2034 via GPIB

Davide Bucci, 2015

This software communicates with a Bruel&Kjaer 2034 double channel FFT
spectrum analyzer and records what it is shown on the upper display.
No processing is done on the data.


The following options are available:

| Option | Action
|--------|------------------------------------------------------------|
|  -h    | Show this help
|  -b    | Change the board interface index (GPIB0=0,GPIB1=1,etc.)
|  -p    | Change the primary address of the BK 2034.
|  -s    | Change the secondary address of the BK 2034.
|  -o    | Write on a file the results.
