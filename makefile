
all : octave

octave : octave.c
	gcc octave.c -m32 -framework NI488 -o octave
	
clean :
	rm octave 