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
#include "iobase.h"
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

char cmdbuf[1024];
pthread_t controlthread;

extern "C" 
{
  #include "coniol.h"
}


volatile int virt_switch=0;
volatile int virt_smask=0;
volatile int virt_sreset=0;

int base=0x10;

// Convert a string to a number
// use default radix unless number starts with # (dec), & (octal), or $ (hex)
unsigned strtonum(const char *t)
{
  int abase=base;
  if (*t=='$') 
    {
      t++;
      abase=0x10;
    }
  else if (*t=='#')
    {
      t++;
      abase=10;
    }
  else if (*t=='&')
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
      if (c=='\x1b') return -1;
      if (c!='\x7f') iobase::putchar(iobase::CONTROL,c);  // echo
      if (c=='\x7f') 
	{
	  if (cp) 
	    {
	      cp--;
	      iobase::putchar(iobase::CONTROL,'\010');
	      iobase::putchar(iobase::CONTROL,' ');
	      iobase::putchar(iobase::CONTROL,'\010');
	    }
	  continue;
	}
      if (strchr(ends,c))
	{
	  if (c!='\r') iobase::putchar(iobase::CONTROL,'\r');
	  iobase::putchar(iobase::CONTROL,'\n');
	  cmdbuf[cp]='\0';
	  return 0;
	}
      cmdbuf[cp++]=c;
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
  //  close_keyboard();
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
  virt_switch=0x80;
  virt_sreset=0;
  virt_smask=0x80;
}

void f_step(void)
{
  virt_switch=2;
  virt_sreset=0;
  virt_smask=2;
}



void f_save(void)
{
  char *t=strtok(NULL,"\r\n");
  if (t)
      thecpu->ram.save(t);
}

void f_load(void)
{
  char *t=strtok(NULL,"\r\n");
  if (t)
      thecpu->ram.load(t);
}

// disp memory
void f_disp(void)
{
  unsigned add,end;
  int i,j;
  add=getval();
  end=getval();
  if (!end) end=256;
  end+=add;
  j=0;
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
  
  newv=getval(&ok);
  if (base==0x10)
    fmt="%s=%04X\r\n";
  else 
    fmt="%s=%06o\r\n";
  if (!ok)
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
  thecpu->setreg(regstring,newv);
}


void f_hex(void) 
{
  base=0x10;
}

void f_oct(void)
{
  base=010;
}



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
  //  theRFP->bps[26].action=0x40+26;
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
  virt_smask=2;
  while (virt_smask) sched_yield();;
#endif
  f_regs();
}


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
		     "bp list [X] - show all breakpoints or one particular breakpoint\r\n"
		     "bp help - this message\r\n");
      return;
    }
  if (!strcasecmp(tag,"list"))
    {
      // list all breakpoints
      int i;
      // static member for breakpoint?
      iobase::printf(iobase::CONTROL,"ID ON  COND\t\t\tCOUNT\tACTION\r\n");
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
      char *tmp;
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
      tmp=strtok(NULL," \t,");
      if (!tmp || !*tmp) goto bperr;
      v=strtonum(tmp);
      tmp=strtok(NULL," \t,");
      if (tmp && *tmp) mask=strtonum(tmp);
      theRFP->bps[bp].init(1,tag,v,mask);
      return;
    }
  if (!strcasecmp(tag,"onchange"))
    {
      unsigned mask=0xFFFF;
      char *tmp;
      tag=strtok(NULL," \t,");
      if (!tmp || !*tmp) goto bperr;
      tmp=strtok(NULL," \t,");
      if (tmp && *tmp) mask=strtonum(tmp);
      theRFP->bps[bp].init(1,tag,0x10000,mask);
      return;
    }
  if (!strcasecmp(tag,"action"))
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
  if (!strcasecmp(tag,"count"))
    {
      unsigned v;
      tag=strtok(NULL," \t,");
      if (!tag||!*tag) goto bperr;
      v=strtonum(tag);
      theRFP->bps[bp].setcount(v);
      return;
    }
  if (!strcasecmp(tag,"on"))
    {
      theRFP->bps[bp].setstate(1);
      return;
    }
  if (!strcasecmp(tag,"off"))
    {
      theRFP->bps[bp].setstate(0);
      return;
    }
  if (!strcasecmp(tag,"once"))
    {
      int n=0;
      tag=strtok(NULL," \t,");
      if (!tag || !*tag || !strcasecmp(tag,"on")) n=1;
      theRFP->bps[bp].oneshot=n;
      return;
    }
  if (!strcasecmp(tag,"resume"))
    {
      theRFP->bps[bp].setstate(-1);
      return;
    }
  goto bphelp;
}

void f_resume(void)
{
  for (int i=0;i<27;i++) 
      if (theRFP->bps[i].getstate()==1) theRFP->bps[i].setstate(-1);
}




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
    { "load", f_load, "load file - Load RAM with file" },
    { "n", f_n, "n - step + regs command"  },
    { "oct", f_oct,  "oct - Set default radix to octal (override # -decimal, & - octal, $ - hex)" },
    { "reg", f_reg,  "reg register [value] - Display/set register (AF, BC, DE, HL, SP, PC for 8080" },
    { "regs", f_regs, "regs - Show all registers" },
    { "release", f_release, "release - Release all control switches to front panel or default" },
    { "reset", f_reset, "reset - Reset CPU" },
    { "resume", f_resume, "resume - Continue after breakpoint"   },
    { "run", f_run, "run - Run/resume program"  },
    { "save", f_save, "save filename - Save RAM to file"   },
    { "set", f_set, "set address - Set RAM (Esc to quit)"   },
    { "step", f_step, "step - Single step program"  },
    { "stop", f_stop, "stop - Stop program execution" }
      
  };

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
