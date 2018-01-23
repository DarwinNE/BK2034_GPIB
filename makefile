
all : octave totaldoc impedance record

octave : octave.c commBK2034.c commBK2034.h GPIB.c GPIB.h
	gcc octave.c commBK2034.c GPIB.c -m32 -framework NI488 -o octave
	
totaldoc : totaldoc.c commBK2034.c commBK2034.h GPIB.c GPIB.h
	gcc totaldoc.c commBK2034.c GPIB.c -m32 -framework NI488 -o totaldoc
	
impedance : impedance.c commBK2034.c commBK2034.h GPIB.c GPIB.h
	gcc impedance.c commBK2034.c GPIB.c -m32 -framework NI488 -o impedance

record : record.c commBK2034.c commBK2034.h GPIB.c GPIB.h
	gcc record.c commBK2034.c GPIB.c -m32 -framework NI488 -o record

	
clean :
	rm octave totaldoc impedance record