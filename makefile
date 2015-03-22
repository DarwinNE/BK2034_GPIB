
all : octave totaldoc impedance

octave : octave.c commBK2034.c commBK2034.h
	gcc octave.c commBK2034.c -m32 -framework NI488 -o octave
	
totaldoc : totaldoc.c commBK2034.c commBK2034.h
	gcc totaldoc.c commBK2034.c -m32 -framework NI488 -o totaldoc
	
impedance : impedance.c commBK2034.c commBK2034.h
	gcc impedance.c commBK2034.c -m32 -framework NI488 -o impedance
	
clean :
	rm octave 