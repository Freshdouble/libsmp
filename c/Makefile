COMPILEFLAGS = -Wall -Werror -g
CC = gcc
armgcc = arm-none-eabi-gcc

OBJS = libsmp.o
LINK_TARGET = libsmp.a
LINK_TARGET_SHARED = libsmp.so

REBUILDABLES = $(OBJS) $(LINK_TARGET)

all : $(LINK_TARGET) buildlibs
	$(MAKE) -C Tests

all_cleanbuild : clean all

buildlibs: $(LINK_TARGET)
	$(MAKE) -C fecmp

buildsharedlib: $(LINK_TARGET_SHARED)

.PHONY: clean
clean : 
	$(RM) $(REBUILDABLES)
	$(MAKE) -C fecmp clean
	$(MAKE) -C Tests clean
	$(RM) *.so
	$(RM) *.a

#Link the shared library
$(LINK_TARGET_SHARED) :
	gcc -shared -g -DCREATE_ALLOC_LAYER -DSHAREDLIB -o $@ libsmp.c

#Link the static library
$(LINK_TARGET) : $(OBJS)
	ar rcs $@ $^

#Compile the sourcefiles
%.o : %.c
	$(CC) $(COMPILEFLAGS) -o $@ -c $<

libsmp.o : libsmp.h
libsmp.so : libsmp.h
