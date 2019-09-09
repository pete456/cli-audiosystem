SOURCEFILES = main.c
EXECUTABLE = cli-recorder
LIBS = -lasound -lm

all: $(SOURCEFILES)
	gcc $(SOURCEFILES) -o $(EXECUTABLE) $(LIBS) 
