# Source files live where?
SRC=..

CC=gcc
CXX=g++
CFLAGS=
CPPFLAGS=-g -DCYGWIN
LDFLAGS=-lpthread
LOADLIBES=
LDLIBS=
VPATH=$(SRC)
SRCS=rs232.c rfp.cpp cpu.cpp iobase.cpp outfile.cpp contterm.cpp iotelnet.cpp breakpoint.cpp coniol.cpp options.cpp
OBJ0=$(SRCS:.cpp=.o)
OBJS=$(OBJ0:.c=.o)

all : altairrfp

altairrfp: $(OBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -o altairrfp $(OBJS)

include makefile.dep

clean :
	rm *.o *.d altairrfp

