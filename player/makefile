# alacconvert make

CFLAGS = -g -O3 -c -I/usr/local/include
LFLAGS = -Wall -L$(CODECDIR) -lalac -lout123 -L/usr/local/lib
CC = cc
C++ = c++

SRCDIR = .
CODECDIR = ../codec
OBJDIR = ./obj
INCLUDES = -I../codec -I/usr/local/include -I$(CODECDIR) -I../convert-utility

all:		cafplay m4aplay

cafplay:	cafplay.cpp
	$(C++) $(INCLUDES) $(LFLAGS) cafplay.cpp ../convert-utility/CAFFileALAC.cpp -o cafplay

m4aplay:	m4aplay.c alac_wrapper.o
	$(CC) $(INCLUDES) $(LFLAGS) -lstdc++ m4aplay.c alac_wrapper.o -o m4aplay

alac_wrapper.o : alac_wrapper.cpp
	$(C++) $(INCLUDES) $(CFLAGS) alac_wrapper.cpp

clean:
	-rm $(OBJS) cafplay m4aplay *.o

