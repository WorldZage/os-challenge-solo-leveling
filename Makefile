
# Source: https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
IDIR = include
CC=gcc
CFLAGS = -I$(IDIR)

ODIR=obj
LDIR = lib

LIBS=-lcrypto -pthread

_DEPS = hashcode.h messages.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = servercode.o hashcode.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

servercode: $(OBJ)
	$(CC) -o server $^ $(CFLAGS) $(LIBS)

# $@ = lefthandside of : in a rule
# $^ = right hand side of : in a rule.
# use $@ after -o to name the compiled code "servercode" (same as the rule).


.PHONY: clean


clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~