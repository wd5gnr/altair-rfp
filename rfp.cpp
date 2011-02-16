/***********************************************************************
This file is part of Altairrfp, an Altair 8800 simulator.
Altairrfp can work standalone or with the Briel Micro8800
computer in remote mode as a front panel.

For more information, see http://www.hotsolder.com (Altairrfp)
or http://www.brielcomputers.com (Micro8800)

Altairrfp (c) 2011 by Al Williams. 

    Altairrfp is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Altairrfp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Altairrfp.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/
// Main file

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rfp.h"
#include "cpu.h"
#include "iobase.h"
#include "outfile.h"
#include <ctype.h>
#include "iotelnet.h"
#include "options.h"
#if !defined(NOTELNET)
#include <pthread.h>
// linkage to control terminal routine
extern void *controlterm(void *nothing);
extern pthread_t controlthread;
#else
#define sched_yield()
#endif

// Globals to make it easy to find the CPU and the front panel
CPU *thecpu=NULL;
RFP *theRFP=NULL;


RFP::RFP(char *port, int software)
{
  soft=software;
  add=dat=0;
  oldhadd=0x100;  // impossible values for cache
  oldstat=0x100;
  status=5;
  if (software)  // if software==1 then no real front panel
      ready=1;
  else
    ready=(rfp_openport(port)>=0);
  if (ready && !soft) rfp_emit('R');  // reset 
  setstate();
  theRFP=this;
  for (int b=0;b<26;b++) bps[b].id='A'+b;
}

RFP::~RFP() 
{
  if (ready && !soft) rfp_closeport();
  ready=0;
}

// send cmd with byte
void RFP::sendcmd2(char c, unsigned u)
{
  char cmd[8];
  if (soft) return;
  cmd[0]=c;
  sprintf(cmd+1,"%02X",u);
  rfp_cmd(cmd);
}

// Get firmware ID
unsigned RFP::getID(void) 
{
  // note that this returns 0 if software=1
  return read2('?');
}

// Set high address 
void RFP::setAhigh(unsigned a)
{
  if (a==oldhadd||soft) return;
  sendcmd2('A',a);
  oldhadd=a;
}


// set low address
void RFP::setAlow(unsigned a)
{
  sendcmd2('a',a);
}

// set CS
void RFP::setCS(int bit)
{
  if (soft) return;
  rfp_emit(bit?'C':'c');
}

// Set IO
void RFP::setIO(int bit)
{
  if (soft) return;
  rfp_emit(bit?'I':'i');
}

// Set RW
void RFP::setRW(int bit)
{
  if (soft) return;
  rfp_emit(bit?'W':'w');
}

// Release data bus
void RFP::releaseDB(void)
{
  if (soft) return;
  rfp_emit('Z');
}

// output to databus
void RFP::setDB(unsigned d)
{
  sendcmd2('D',d);
}

// Set LEDs
void RFP::setLED(unsigned l)
{
  if (oldstat==l) return; // don't send the same thing twice
  sendcmd2('L',l);
  oldstat=l;
}


// Get two characters
unsigned RFP::read2(char c)
{
  char co[3];
  unsigned u;
  if (soft) return 0;
  co[2]='\0';
  rfp_emit(c);
  co[0]=rfp_read();
  co[1]=rfp_read();
  sscanf(co,"%x",&u);
  return u;
}

  
// Used by contterm to flip switches
extern volatile int virt_switch, virt_smask, virt_sreset;

// process real switches (func) with virtual switches
unsigned int virtsw(int func)
{
  if (virt_smask)
    {
      func&=~virt_smask;
      func|=virt_switch&virt_smask;
      virt_smask=virt_sreset;
    }
  return func;
}


// Get databus
unsigned RFP::getDB(void)
{
  return read2('d');
}

// Get low switches
unsigned RFP::getSWLow(void)
{
  return read2('S');
}

// Get high switches
unsigned RFP::getSWHigh(void)
{
  if (soft) return 0x08;  // A11 on for Basic
  return read2('T');
}

// Get Function switches
unsigned RFP::getSWFunc(void)
{
  return read2('U');
}

// Set full machine state 
void RFP::setstate(void)
{
  setAhigh(add>>8);
  setAlow(add&0xFF);
  setLED(status);
  setDB(dat);
}


#if 0
// options from command line
int runonly=-1;
// 0=9600, 1=19200, 2=56700, 3=115200, 4=230400
int baud=0;
int forcetrace=0;
unsigned skip=0;
unsigned memsize=0x10000;
int upper=0;
int softonly=1;
char fn[1024];
char port[1024];
//fix me^
int killchar=-1;
int cstream;
char tstream[1024];
char dstream[1024];
char estream[1024];
int xstream;
 
// add more
#endif


// PROGRAM STARTS HERE!
int main(int argc, char *argv[])
{
  options::process_options(argc, argv);
  // create I/O streams
  iobase *io=new console(iobase::CONSOLE);
  if (*options::estream)
    {
      if (isdigit(*options::estream)) 
	new iotelnet(iobase::ERROROUT,atoi(options::estream));
      else
	new outfile(iobase::ERROROUT,options::estream);
    }
  else iobase::dup(iobase::ERROROUT,iobase::CONSOLE);
  if (*options::dstream)
    {
      if (isdigit(*options::dstream)) 
	new iotelnet(iobase::DEBUG,atoi(options::dstream));
      else 
	  new outfile(iobase::DEBUG,options::dstream);
    }
  else iobase::dup(iobase::DEBUG,iobase::CONSOLE);
  if (options::cstream)
    {
      io=new iotelnet(iobase::CONSOLE,options::cstream);
      if (!*options::estream) iobase::dup(iobase::ERROROUT,iobase::CONSOLE);
      if (!*options::dstream) iobase::dup(iobase::DEBUG,iobase::CONSOLE);
    }

  // default trace is a duplicate of the console
  iobase::dup(iobase::TRACE,iobase::CONSOLE);
  if (*options::tstream)
    {
      if (isdigit(*options::tstream)) 
	new iotelnet(iobase::TRACE,atoi(options::tstream));
      else
	  new outfile(iobase::TRACE,options::tstream);
    }
#if !defined(NOTELNET)
  if (options::xstream)
    {
      iobase *control= new iotelnet(iobase::CONTROL,options::xstream);
      // wait for first control terminal
      while (!control->ready()) sched_yield();
      pthread_create(&controlthread,NULL,controlterm,NULL);
    }
#endif
  io->killchar=options::killchar;
  // create RFP and RAM
  RFP rfp(options::port,options::softonly);
  RAM ram(rfp,options::memsize,*options::fn?options::fn:NULL);

  rfp.io=io;
  // wait for connect from console
#if !defined(NOTELNET)
  while (!io->ready())  sched_yield(); 
  
#endif  // note: "normal" console is always ready

  // if we are ready then execute
  if (rfp.isReady())  
    {
      rfp.execute(ram); 
    }
  
  else iobase::printf(iobase::ERROROUT,"Can't open front panel\n");
}



// This is the main part of the simulator
void RFP::execute(RAM& ram)
{
  int tracing=0;
  dat=ram.read(add);
  setstate();
  // create CPU
  CPU cpu(ram,*this);
  thecpu=&cpu;
  thecpu->upper=options::upper;
  while (1)
    {
      // reaad function switches
      int func=options::runonly?1:getSWFunc();
      func=virtsw(func);
      // see if we are tracing
      tracing=options::forcetrace||((func&0x40)==0x40);
      if ((func&0x80))  // reset?
	{
	cpureset:
	  int cmd;
	  if (options::xstream==0) 
	    {
	      iobase::printf(iobase::CONTROL,"\n<R>eset, <S>ave, e<X>it, or <C>ontinue? ");
	      do
		{
		  cmd=iobase::getchar(iobase::CONTROL);
		  iobase::putchar(iobase::CONTROL,cmd);
		} while (cmd!='R' && cmd!='r' && cmd!='S' && cmd!='s' && cmd!='c' && cmd!='C' && cmd!='X' && cmd!='x');
	  iobase::putchar(iobase::CONTROL,'\n');
	    }
	  else  cmd='R'; 
	  if (cmd=='x'||cmd=='X') exit(0);
	  if (cmd=='r'||cmd=='R')  cpu.reset();
	  else if (cmd=='s' || cmd=='S') 
	    {
#if defined(WIN32)
	      ram.save("altairsave.bin");
	      iobase::printf(iobase::CONTROL,"Saved to altairsave.bin\n");
#else
	      ram.save("/tmp/altairsave.bin");
	      iobase::printf(iobase::CONTROL,"Saved to /tmp/altairsave.bin\n");
#endif

	    }
	  while (getSWFunc()&0x80);  // wait for release
	}
      else if (func&1)  // running
	{ 
	  // we are running so attend to that first
	  ram.statusct=0;
	  ram.statusskip=options::skip;
	  while (func&1) 
	    {
	      // main run loop
	      // figure out breakpoint status
	      int action=-1;
	      tracing=options::forcetrace||((func&0x40)==0x40);
	      if (cpu.isInst()) for (int b=0;b<27;b++)
		{
		  int act;
		  act=bps[b].check();
		  if (act==1) tracing=1;  // trace point
		  // enable a breakpoint
		  if (act&0x80) bps[action&0x3F].setstate(1);
		  // disable a breakpoint
		  if (act&0x40) bps[action&0x3F].setstate(0);
		  if (act==0)  // stop
		    {
		      action=act;
		      break;
		    }
		}
	      // if action==-1 then keep going 
	      if (action!=0) // not a stop
		{
		  cpu.step();  // do a step
		  add=cpu.pc; // set the new address
		  dat=ram.read(add); // get the address
		  // trace if required
		  if (tracing && cpu.isInst()) cpu.dump();
		} 
	      else   // if at breakpoint, release
		sched_yield();
	      
	      // check to see if it is still running
	      func=virtsw(options::runonly?1:(ram.statusct==0?getSWFunc():1));
	      if (func&0x80) goto cpureset;  // reset during run
	    }
	  ram.statusskip=0;  // only skip during run
	}
      else if (func & 2)  // step
	{
	  // step
	  // tell CPU to do next instruction
	  cpu.step();
	  add=cpu.pc;
	  dat=ram.read(add);
	  // could do dumps etc conditionally on tracing 
	  if (tracing) cpu.dump();
	  while (getSWFunc()&2);  // wait for release
	}
      else if (func & 4)   // examine
	{
	  // examine
	  unsigned hi, lo;
	  hi=getSWHigh();
	  lo=getSWLow();
	  add=(hi<<8)+lo;
	  dat=ram.read(add);
	  // could do dumps etc
	  while (getSWFunc()&4);  // wait for release
	}
      else if (func & 8)  
	{
	  // ex next
	  dat=ram.read(++add);
	  while (getSWFunc()&8);  // wait for release
	}
      else if (func & 16)
	{
	  // deposit
	  ram.write(add,dat=getSWLow());
	  while (getSWFunc()&16);  // wait for release
	}
      else if (func & 32)
	{
	  // dep next
	  ram.write(++add,dat=getSWLow());
	  while (getSWFunc()&32);  // wait for release
	}
    }
}

      


      

      







