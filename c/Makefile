COMPILEFLAGS = -Wall -Werror -g
CC = gcc
armgcc = arm-none-eabi-gcc

OBJS = libsmp.o
LINK_TARGET = libsmp.a

REBUILDABLES = $(OBJS) $(LINK_TARGET)

all : $(LINK_TARGET) buildlibs
	$(MAKE) -C Tests

all_cleanbuild : clean all

buildlibs: $(LINK_TARGET)
	$(MAKE) -C fecmp

.PHONY: clean
clean : 
	$(RM) $(REBUILDABLES)
	$(MAKE) -C fecmp clean
	$(MAKE) -C Tests clean

#Link the library
$(LINK_TARGET) : $(OBJS)
	ar rcs $@ $^

#Compile the sourcefiles
%.o : %.c
	$(CC) $(COMPILEFLAGS) -o $@ -c $<

libsmp.o : libsmp.h
