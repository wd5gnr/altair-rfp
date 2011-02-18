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
// Control terminal

#include "iobase.h"
#include "contterm.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#if !defined(NOTELNET)
#include <pthread.h>
#else
#define pthread_t int
#define sched_yield()
#endif

#include "ram.h"
#include "cpu.h"
#include "rfp.h"

// command line buffer
char cmdbuf[1024];
pthread_t controlthread;

extern "C" 
{
  #include "coniol.h"
}


// These are used to virtually flip switches on the front panel (even if we don't have one)
volatile int virt_switch=0;
volatile int virt_smask=0;
volatile int virt_sreset=0;

int base=0x10; // default number base (must be 0x10 or 010).
 
// Convert a string to a number
// use default radix unless number starts with # (dec), & (octal), or $ (hex)
unsigned strtonum(const char *t)
{
  int abase=base;  // assumed base
  if (*t=='$')   // hex
    {
      t++;
      abase=0x10;
    }
  else if (*t=='#')  // dec
    {
      t++;
      abase=10;
    }
  else if (*t=='&')  // oct
    {
      t++;
      abase=010;
    }
  return strtoul(t,NULL,abase);
}

// Get numeric value out of command input stream
unsigned getval(int *success=NULL)
{
  char *t=strtok(NULL," ,\t");
  if (success) *success=0;
  if (!t || !*t) return 0;
  if (success) *success=1;
  return strtonum(t);
}



// Get a command line (optional prompt)
// ends is a string that sets the allowable end of "line" (default \r\n but set can use space)
// returns -1 for escape (cancel)
int getcline(const char *prompt, const char *ends=NULL)
{
  int cp=0;
  int c;
  if (!ends) ends="\r\n";
  cmdbuf[cp]='\0';
  if (prompt) iobase::puts(iobase::CONTROL,prompt);
  while (1)
    {
      c=iobase::getchar(iobase::CONTROL);
      if (c=='\0'||c==-1) continue;
      if (c=='\x1b') return -1;  // escape cancels
      if (c!='\x7f') iobase::putchar(iobase::CONTROL,c);  // echo
      if (c=='\x7f') // if a rub out
	{
	  if (cp) // if not at start of line
	    {
	      // backspace over the last character and discard
	      cp--;
	      iobase::putchar(iobase::CONTROL,'\010');
	      iobase::putchar(iobase::CONTROL,' ');
	      iobase::putchar(iobase::CONTROL,'\010');
	    }
	  continue;
	}
      // end of line?
      if (strchr(ends,c))
	{
	  if (c!='\r') iobase::putchar(iobase::CONTROL,'\r');  // echo return if it wasn't
	  iobase::putchar(iobase::CONTROL,'\n');  // and add a new line
	  cmdbuf[cp]='\0';  // end string
	  return 0;    // done!
	}
      // add to buffer
      cmdbuf[cp++]=c;
      // if we are at the end (really?) then force an end
      if ((cp+1)==sizeof(cmdbuf)) 
	{
	  cmdbuf[cp]='\0';
	  return 0;
	}
    } 
}

// Functions that start with f_ are command handlers

void f_exit(void)
{
  // at exit should close keyboard!
  exit(0);
}

void do_help(const char *k); // we need help to do help ;-) forward ref

void f_help(void)
{
  char *t=strtok(NULL,"\r\n");
  if (!t || !*t) t=NULL;
  do_help(t);
}

void f_stop(void)
{
  virt_switch=0;
  virt_sreset=1;
  virt_smask=1;
}

void f_run(void)
{
  virt_switch=1;
  virt_sreset=1;
  virt_smask=1;
}

// let front panel switches win no matter what
void f_release(void)
{
  virt_smask=0;
}

// reset CPU
void f_reset(void)
{
  for (int i=0;i<27;i++) 
    theRFP->bps[i].setannounce(0);
  virt_switch=0x80;
  virt_sreset=0;
  virt_smask=0x80;
}

void f_step(void)
{
  virt_switch=2;
  virt_sreset=0;
  virt_smask=3;
}



// TODO: save and load need start address and count (see f_disp)
void f_save(void)
{
  unsigned start=0;
  unsigned len0=thecpu->ram.getlen(), len;
  char *t=strtok(NULL,"\r\n");
  len=len0;
  if (!t||!*t) f_help();
  if (*t=='@')
    {
      start=strtonum(t+1);
      t=strtok(NULL,"\r\n");
    }
  if (!t||!*t) f_help();
  if (*t=='-')
    {
      len=strtonum(t+1);
      if (len>len0) len=len0;
      t=strtok(NULL,"\r\n");
    }
  if (!t||!*t) f_help;
  thecpu->ram.save(t,start,len);
}

void f_load(void)
{
  unsigned start=0;
  unsigned len0=thecpu->ram.getlen(), len;
  char *t=strtok(NULL,"\r\n");
  len=len0;
  if (!t||!*t) f_help();
  if (*t=='@')
    {
      start=strtonum(t+1);
      t=strtok(NULL,"\r\n");
    }
  if (!t||!*t) f_help();
  if (*t=='-')
    {
      len=strtonum(t+1);
      if (len>len0) len=len0;
      t=strtok(NULL,"\r\n");
    }
  if (!t||!*t) f_help;

      thecpu->ram.load(t);
}

// disp memory
void f_disp(void)
{
  unsigned add,end;
  int i,j;
  add=getval();  // get address
  end=getval();  // get count
  if (!end) end=256;  // default count to 256
  end+=add;
  j=0;
  // do the print in the proper base
  while (add+j*16<=end)
    {
      iobase::printf(iobase::CONTROL,base==0x10?"%04X: ":"%06o: ",add+j*16);
      for (i=0;i<16;i++) 
	iobase::printf(iobase::CONTROL,base==0x10?"%02X ":"%03o ",thecpu->ram.read(add+j*16+i,0));
      iobase::printf(iobase::CONTROL,"\r\n");
      j++;
    }
}


// set memory
void f_set(void)
{
  unsigned add,n;
  int rv;
  add=getval();
  do 
    {
      iobase::printf(iobase::CONTROL,base==0x10?"%04X: ":"%06o: ",add);
      rv=getcline(NULL,"\r\n \t");
      if (rv>=0) thecpu->ram.write(add++,strtonum(cmdbuf),0);
    } while (rv>=0);
  iobase::printf(iobase::CONTROL,"\r\n");
}


// let CPU do most of the work because it knows what registers it has

void f_reg(void)
{
  unsigned v,newv;
  char *regstring;
  const char *fmt;
  int ok;
  if (!thecpu) return;
  regstring=strtok(NULL," ,\t");
  if (regstring==NULL) 
    {
      iobase::printf(iobase::CONTROL,"reg register_name [value]\r\n");
      return;  
    }
  // if there is a new value we have to set it
  newv=getval(&ok);
  if (base==0x10)
    fmt="%s=%04X\r\n";
  else 
    fmt="%s=%06o\r\n";
  if (!ok)  // display only
    {
      char *t=regstring;
      while (*t) 
	{
	  *t=toupper(*t);
	  t++;
	}
      v=thecpu->getreg(regstring);
      iobase::printf(iobase::CONTROL,fmt,regstring,v);
      return;
    }
  // set value instead of display
  thecpu->setreg(regstring,newv);
}

// radix changers

void f_hex(void) 
{
  base=0x10;
}

void f_oct(void)
{
  base=010;
}


// show registers
void f_regs(void)
{
  if (!thecpu) return;
  thecpu->dump(iobase::CONTROL,base);
}


void f_n(void)
{
#if 1  // this is one way to do things like this
  // this has the advantage of only stopping on whole instructions
  // set private breakpoint
  theRFP->bps[26].init(1,"pc",0x10000,0xFFFF);
  // do not set OUR breakpoint to hold so 26 is the right #
  for (int i=0;i<26;i++) 
    if (theRFP->bps[i].getstate()==1) theRFP->bps[i].setstate(-1);
  virt_switch=1;
  virt_sreset=0;
  virt_smask=1;
  while(theRFP->bps[26].check()==-1) { sched_yield(); virt_smask=1;  }
  virt_smask=0;
  theRFP->bps[26].setstate(0);
#else
  // and another way
  // this way steps one cycle at a time like step or the step button
  virt_switch=2;
  virt_sreset=0;
  virt_smask=3;
  while (virt_smask) sched_yield();;
#endif
  f_regs();
}


// Breakpoint (manu subcommands)
void f_bp(void)
{
  char *tag=strtok(NULL," \t,");
  int bp;
  if (!tag || !*tag || !strcasecmp(tag,"help"))
    {
    bphelp:
      // do bp help here
      iobase::printf(iobase::CONTROL,
		     "Breakpoints range from A-Z (not case sensitive; X in the list below represents any breakpoint letter)\r\n"
		     "bp X set target value [mask] - set regular breakpoint\r\n"
		     "   (for target use @address or register name)\r\n"
		     "bp X onchange target [mask] - set a break on change\r\n"
		     "bp X action (stop|trace|enable X|disable X) - set breakpoint action\r\n"
		     "   Enable and disable allow you to change state of any breakpoint\r\n"
		     "bp X count n - set the breakpoint counter (0=immediate)\r\n"
		     "bp X (on|off) - enable or disable a breakpoint\r\n"
		     "bp X once [(on|off)] - set oneshot mode; default is on\r\n"
		     "bp X resume - allow execution to continue after breakpoint\r\n"
		     "bp list [X] - show all breakpoints or one particular breakpoint (X=* for only enabled)\r\n"
		     "bp help - this message\r\n");
      return;
    }
  if (!strcasecmp(tag,"list"))
    {
      // list all breakpoints (or just one)
      int i;
      // print header
      breakpoint::header(iobase::CONTROL);
      tag=strtok(NULL," \t,");
      if (tag && *tag)
	{
	  *tag=toupper(*tag);
	  if (*tag>='A' && *tag<='Z')
	    {
	      theRFP->bps[*tag-'A'].dump(iobase::CONTROL,base);
	      return;
	    }
	}
      for (i=0;i<26;i++)
	{
	  if (tag && *tag=='*' && theRFP->bps[i].getstate()==0) continue;
	  theRFP->bps[i].dump(iobase::CONTROL,base);
	}
      return;
    }
  // everything but list and help take a BPID first
  bp=toupper(*tag)-'A';
  tag=strtok(NULL," \t,");
  if (!tag || !*tag) goto bphelp;
  if (!strcasecmp(tag,"set"))
    {
      char *tmp;   // set a match breakpoint
      unsigned v;
      unsigned mask;
      mask=0xFFFF;
      tag=strtok(NULL," \t,");
      if (!tag || !*tag) 
	{
	bperr:
	  iobase::printf(iobase::CONTROL,"?error\r\n");
	  return;
	}
      // portable strupr
      for (tmp=tag;*tmp;tmp++) *tmp=toupper(*tmp);
      tmp=strtok(NULL," \t,");
      if (!tmp || !*tmp) goto bperr;
      v=strtonum(tmp);
      tmp=strtok(NULL," \t,");
      if (tmp && *tmp) mask=strtonum(tmp);
      theRFP->bps[bp].init(1,tag,v,mask);
      return;
    }
  if (!strcasecmp(tag,"onchange"))  // set a change breakpoint
    {
      unsigned mask=0xFFFF;
      char *tmp;
      tag=strtok(NULL," \t,");
      if (!tmp || !*tmp) goto bperr;
      // portable strupr
      for (tmp=tag;*tmp;tmp++) *tmp=toupper(*tmp);
      tmp=strtok(NULL," \t,");
      if (tmp && *tmp) mask=strtonum(tmp);
      theRFP->bps[bp].init(1,tag,0x10000,mask);
      return;
    }
  if (!strcasecmp(tag,"action"))  // set action
    {
      int act=-1;
      tag=strtok(NULL," \t,");
      if (!strcasecmp(tag,"stop")) act=0;
      if (!strcasecmp(tag,"trace")) act=1;
      if (!strcasecmp(tag,"enable")) act=0x80;
      if (!strcasecmp(tag,"disable")) act=0x40;
      if (act==-1) goto bperr;
      if (act>=0x40)
	{
	  int c;
	  tag=strtok(NULL," \t,");
	  if (!tag||!*tag) goto bperr;
	  c=toupper(*tag);
	  if (c<'A'||c>'Z') goto bphelp;
	  act+=c-'A';
	}
      theRFP->bps[bp].action=act;
      return;
    }
  if (!strcasecmp(tag,"count"))  // set count
    {
      unsigned v;
      tag=strtok(NULL," \t,");
      if (!tag||!*tag) goto bperr;
      v=strtonum(tag);
      theRFP->bps[bp].setcount(v);
      return;
    }
  if (!strcasecmp(tag,"on"))  // enable 
    {
      theRFP->bps[bp].setstate(1);
      return;
    }
  if (!strcasecmp(tag,"off"))  // disable
    {
      theRFP->bps[bp].setstate(0);
      return;
    }
  if (!strcasecmp(tag,"once"))  // set one shot flag
    {
      int n=0;
      tag=strtok(NULL," \t,");
      if (!tag || !*tag || !strcasecmp(tag,"on")) n=1;
      theRFP->bps[bp].oneshot=n;
      return;
    }
  if (!strcasecmp(tag,"resume"))   // resume from this breakpoint (note: bp X resume is not the same as just resume; see f_resume)
    {
      theRFP->bps[bp].setstate(-1);
      return;
    }
  goto bphelp;
}

// Resume from all breakpoints
void f_resume(void)
{
  for (int i=0;i<27;i++) 
      if (theRFP->bps[i].getstate()==1) theRFP->bps[i].setstate(-1);
}


// command table -- string, function, help text

struct cmdentry
{
  const char *cmd;
  void (*func)(void);
  const char *help;
} cmds[]=
  {
    {"bp",f_bp,"bp a_z command - Breakpoint commands (bp help for more)"  },
    { "disp", f_disp, "display address [count] - Show memory" },
    { "exit", f_exit , "exit - End simulator" },
    { "help", f_help , "help [keyword] - Get help" },
    { "hex", f_hex, "hex - Set default radix to hex (override # -decimal, & - octal, $ - hex)"  },
    { "load", f_load, "load [@start] [-len] file - Load RAM with file" },
    { "n", f_n, "n - step + regs command"  },
    { "oct", f_oct,  "oct - Set default radix to octal (override # -decimal, & - octal, $ - hex)" },
    { "reg", f_reg,  "reg register [value] - Display/set register (AF, BC, DE, HL, SP, PC for 8080" },
    { "regs", f_regs, "regs - Show all registers" },
    { "release", f_release, "release - Release all control switches to front panel or default" },
    { "reset", f_reset, "reset - Reset CPU" },
    { "resume", f_resume, "resume - Continue after breakpoint"   },
    { "run", f_run, "run - Run/resume program"  },
    { "save", f_save, "save [@start] [-len] filename - Save RAM to file"   },
    { "set", f_set, "set address - Set RAM (Esc to quit)"   },
    { "step", f_step, "step - Single step program"  },
    { "stop", f_stop, "stop - Stop program execution" }
      
  };

// Print a breakpoint when it hits (called by the breakpoint)
void print_bp(char id)
{
  if (id=='Z'+1) return;
  iobase::printf(iobase::CONTROL,"\r\nBreakpoint %c hit at ",id);
  iobase::printf(iobase::CONTROL,base==0x10?"%04X\r\n":"%06o\r\n",thecpu->getreg("PC"));
}


// actually print help for a command or all help for k==NULL
void do_help(const char *k)
{
  int i;
  for (i=0;i<sizeof(cmds)/sizeof(cmds[0]);i++)
    {
      if (!k || !strcasecmp(k,cmds[i].cmd))
	{
	  iobase::printf(iobase::CONTROL,"%s\r\n",cmds[i].help);
	  if (k) return;
	}
    }
}



// This is the thread function that does the control terminal

void *controlterm(void *nothing)
{
  while (1)
    {
      int i, found;
      char *ctoken;
      do 
	{
	  getcline("? ");
	} while (!*cmdbuf);
      found=0;
      ctoken=strtok(cmdbuf," \t");
      if (ctoken) for (i=0;i<sizeof(cmds)/sizeof(cmds[0]);i++)
	if (!strcasecmp(ctoken,cmds[i].cmd))
	  {
	    found=1;
	    cmds[i].func();
	    break;
	  }
      if (!found) iobase::printf(iobase::CONTROL,"Unknown command\r\n");
    }
  
}
