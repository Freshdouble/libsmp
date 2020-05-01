OBJS = \
	berlekamp.o \
	crcgen.o \
	galois.o \
	libfecmp.o \
	rs.o

LINK_TARGET = libfecmp.a
CC = gcc

COMPILEFLAGS = -g -Wall

REBUILDABLES = $(OBJS) $(LINK_TARGET)

all : $(LINK_TARGET)

.PHONY: clean
clean : 
	$(RM) $(REBUILDABLES)

#Link the library
$(LINK_TARGET) : $(OBJS)
	ar rcs $@ $^ ../libsmp.a

#Compile the sourcefiles
%.o : %.c
	$(CC) $(COMPILEFLAGS) -L../ -lsmp -o $@ -c $<

berlekamp.o : ecc.h
crcgen.o : ecc.h
galois.o : ecc.h
rs.o : ecc.h
libfecmp.o : ecc.h libfecmp.h