SOURCEFILES = main.c
EXECUTABLE = cli-audioprocessor
LIBS = -lasound -lm

all: $(SOURCEFILES)
	gcc $(SOURCEFILES) -o $(EXECUTABLE) $(LIBS) 
