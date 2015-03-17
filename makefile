
all : octave totaldoc

octave : octave.c
	gcc octave.c -m32 -framework NI488 -o octave
	
totaldoc : totaldoc.c
	gcc totaldoc.c -m32 -framework NI488 -o totaldoc
	
clean :
	rm octave 