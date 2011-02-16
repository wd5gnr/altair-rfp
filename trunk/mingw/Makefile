# Source files live where?
SRC=..


CC=i586-mingw32msvc-gcc
CXX=i586-mingw32msvc-g++
CFLAGS=
CPPFLAGS=-g -D NOTELNET
LDFLAGS=
LOADLIBES=
LDLIBS=
VPATH=$(SRC)
SRCS=rs232w.c rfp.cpp cpu.cpp iobase.cpp outfile.cpp contterm.cpp iotelnet.cpp breakpoint.cpp options.cpp
OBJ0=$(SRCS:.cpp=.o)
OBJS=$(OBJ0:.c=.o)

all : altairrfp.exe

altairrfp.exe : $(OBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) -o altairrfp.exe $(OBJS)

include makefile.dep

clean :
	rm *.o *.d altairrfp.exe

