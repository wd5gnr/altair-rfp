#ifndef _OPTIONS_H
#define _OPTIONS_H

#define VERSION_STRING "1.1"

// Options for (mostly) rfp.cpp all pulled off the command line
// We have to have a global for baud
// because rs232.c is a C file _and_ that means the
// baud rate table there has to sync with this too
// Ditto for rs232w.c
//
// Set defaults in options.cpp
#ifdef __cplusplus
class options
{
 public:
  enum baudrate 
  {
    B9600=0, B19200, B56700, B115200, B230400
  };
  static int runonly;   // -r 
  static int baud;      // baudrate
  static int forcetrace;  // -t
  static unsigned skip;  // cycles to skip updates when running 
  static unsigned memsize;  // RAM size
  static int upper;  // force terminal to upper case
  static int softonly;  // no front panel?
  // need to make these smarter instead of just large
  static char fn[1024];  // load file name
  static char port[1024];  // front panel port
  // notes about killchar
  // First, shells expand things like ~ or * so be sure you
  // escape anything like that
  // Second, remember to kill you have to press the character
  // and then something else
  // pressing it twice just sends the kill character
  static int killchar;  // -k kill character 
  static int cstream;  // console
  static char tstream[1024];  // trace
  static char dstream[1024];  // debug
  static char estream[1024];  // error
  static int xstream;   // control
  // actually set everything up
  static int process_options(int argc, char *argv[]);
};

#endif
// for the C code!

extern int baudselect;


#endif
  
	     
