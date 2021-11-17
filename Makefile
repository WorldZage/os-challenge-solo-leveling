
# Source: https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
IDIR = include
CC=gcc
CFLAGS = -I$(IDIR) -O3 -Wall

ODIR=obj
LDIR = lib

LIBS=-lcrypto -pthread

_DEPS = auxstructs.h prioritycode.h hashcode.h messages.h 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = servercode.o hashcode.o prioritycode.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

servercode: $(OBJ)
	$(CC) -o server $^ $(CFLAGS) $(LIBS)

$(OBJ): $(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


# $@ = lefthandside of : in a rule
# $^ = right hand side of : in a rule.
# use $@ after -o to name the compiled code "servercode" (same as the rule).
# 

.PHONY: clean


clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~