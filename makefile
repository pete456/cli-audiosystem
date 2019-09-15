SOURCEFILES = main.c
EXECUTABLE = cli-audiosystem
LIBS = -lasound -lm

all: $(SOURCEFILES)
	gcc $(SOURCEFILES) -o $(EXECUTABLE) $(LIBS) 
debug: $(SOURCEFILES)
	gcc -DDEBUG $(SOURCEFILES) -o $(EXECUTABLE) $(LIBS) 
