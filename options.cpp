#include "options.h"
#include "iobase.h"
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// C linkage
int baudselect=0;

// static C++ members
 int options::runonly=1;
 int options::baud=options::B9600;
 int options::forcetrace=0;
 unsigned options::skip=0;
 unsigned options::memsize=0x10000;
 int options::upper=0;
 int options::softonly=1;
  // need to make these smarter instead of just large
 char options::fn[1024];
 char options::port[1024];
 int options::killchar=-1;
 int options::cstream;
 char options::tstream[1024];
 char options::dstream[1024];
 char options::estream[1024];
 int options::xstream;

int options::process_options(int argc, char *argv[])
{
  int c;
  *estream=*tstream=*dstream=*port=*fn='\0';
  xstream=cstream=0;
  // no command line?
  if (argc==1)
    {
    help:
      fprintf(stderr,
"altairrfp " VERSION_STRING " by Al Williams http://www.hotsolder.com\n"
"Usage: altairrfp [-p port_name] [-b baudcode] [-l skipupdates] [-m memorysize] [-r] [-t] [-k char]\n"
"[-u] [-f load_file] [-C telnetport] [-E stream] [-T stream] [-D stream] [-X telnetport]\n"
	      "\tskipupdates: Skips updating LEDs in run mode to speed execution\n"
	      "\tmemorysize: RAM size in decimal (default=65536)\n"
	      "\t-r forces the CPU to run and ignores front panel switches (faster execution)\n"
	      "\t-t forces tracing on regardless of the state of the protect switch\n"
	      "\t-p Sets port name to use for remote front panel\n"
	      "\tbaudcodes: 0=>9600, 1=>19200, 2=>57k, 3=>115k; default=0\n"
	      "\t-u forces input to uppercase\n"
	      "\t-k sets a character used to exit the emulator\n"
	      "\t-C sets the console telnet port (if omitted, the standard I/O is used)\n"
	      "\t-E -T -D - sets the error, trace, and debug streams. All of these default to the console. If the argument is numeric it is taken as a telnet port. If the argument is a string, it is taken as a file name. Existing files will be overwritten.\n"
	      "\t-X sets the control terminal telnet port. By default there is no control terminal\n"
	      "\n\tNote: DO NOT USE THE RESET SWITCH. AUX acts as reset/save. Prot turns on tracing.\nWarning: Make sure Altair internal terminal is off before connecting external serial port\n");
      
      return 1;
    }
  // process options
  opterr = 0;
  while ((c = getopt (argc, argv, "k:p:rtb:l:m:hf:uC:T:D:E:X:")) != -1)
         switch (c)
           {
	   case 'E':
	     strcpy(estream,optarg);
	     break;
	     
	   case 'X':
	     if (runonly==-1) runonly=0;  // don't force run if control port
	     xstream=atoi(optarg);
	     break;
	     
	   case 'C':
	     cstream=atoi(optarg);
	     break;
	     
	   case 'T':
	     strcpy(tstream,optarg);
	     break;
	     
	   case 'D':
	     strcpy(tstream,optarg);
	     break;
	     
	   case 'k':
	     killchar=optarg[0];
	     break;
	     
	   case 'p':
	     softonly=0;
	     strcpy(port,optarg);
	     break;
	     
           case 'r':
             runonly = 1;
             break;
	   case 'h':
	   case '?':
	     goto help;
	     break;
	     
	   case 'u':
	     upper=1;
	     break;
	     
           case 'b':
             baud=atoi(optarg);
	     if (baud<0||baud>3) goto help;
             break;

	   case 'l':
	     skip=atoi(optarg);
	     break;

	   case 't':
	     forcetrace=1;
	     break;
	     
	   case 'm':
	     memsize=atoi(optarg);
	     break;
	   case 'f':
	     strcpy(fn,optarg);
	     break;
           }
     
  // set run only if set specifically or if no front panel
  if (runonly==-1) runonly=softonly;
  // set up C linkage
  baudselect=baud;
  return 0;
}
