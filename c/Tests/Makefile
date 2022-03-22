COMPILEFLAGS = -Wall -g
LINKERFLAGS = 
CC = gcc

PRGFLAGS = -D USELIBFEC

OBJS = testsmp.o

LINK_TARGET = test

REBUILDABLES = $(OBJS) $(LINK_TARGET)

all : $(LINK_TARGET) $(debughelper)

.PHONY: clean
clean : 
	$(RM) $(REBUILDABLES)

#Link the program
$(LINK_TARGET) : $(OBJS)
	$(CC) $(PRGFLAGS) $(LINKERFLAGS) $(COMPILEFLAGS) -o $@ $^ ../fecmp/libfecmp.a ../libsmp.a

#Compile the sourcefiles
%.o : %.c
	$(CC) $(PRGFLAGS) $(LINKERFLAGS) $(COMPILEFLAGS) -o $@ -c $<

testsmp.o : ../libsmp.a ../fecmp/libfecmp.a
