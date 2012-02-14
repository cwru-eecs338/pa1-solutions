# Makefile for Assignment 1

# This should be already defined
# as an environment variable,
# but you can uncomment this if
# you're having trouble:
#CC = gcc

# -g : allows use of GNU Debugger
# -Wall : show all warnings
FLAGS = -g -Wall -Werror
LIBS = # None yet...
SOURCE = prog.c
OUTPUT = main

all: $(SOURCE)
	@# Call the compiler with source & output arguments
	$(CC) $(LIBS) $(FLAGS) -o $(OUTPUT) $(SOURCE)
	@# Make the output file executable
	chmod u+x $(OUTPUT)

# 'clean' rule for remove non-source files
# To use, call 'make clean'
# Note: you don't have to call this in between every
#       compilation, it's only in case you need to
#       clean out your directory for some reason.
clean:
	@# Using the '@' sign suppresses echoing
	@# the line while the command is run
	@rm -f $(OUTPUT)

test: all
	./main
