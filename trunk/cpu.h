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
#ifndef __CPU_H
#define __CPU_H
#include <stdio.h>  // need STDERR
#include "ram.h"
#include "rfp.h"

class CPU
{
 protected:
  unsigned cycle;
  unsigned parity(unsigned v);
  unsigned opcode;
   // temporaries for instructions
  unsigned t1,t2;
  unsigned cond;
  unsigned getM8(void);
  void setM8(unsigned v);
  void setop8(unsigned r, unsigned v);
  unsigned loadop8(unsigned r);
  unsigned  getcond(unsigned op);
  RFP &rfp;
  unsigned incpc(void)  { unsigned t=pc; pc++; pc&=0xFFFF; return t; }
  unsigned incsp(void)  { unsigned t=sp; sp++; sp&=0xFFFF; return t; }
  void decsp(void)  { sp--; sp&=0xFFFF; }
    
 public:
 CPU(RAM& r,RFP& rp) : ram(r), rfp(rp) { reset(); } 
  void reset(void);
  int isInst(void) { return cycle==0;   }
  
  // registers
  // af, bc, de, hl
  unsigned  regs[8];
  enum regnames  { B=0, C, D, E, H, L, A, F   };
  unsigned pc, sp;
  // reference to memory
   RAM &ram;
  // step
   void step(void);
   void doop(unsigned opcode);
   unsigned setflags(unsigned value, unsigned savemask);
   // support for trace and control
   void dump(iobase::streamtype s=iobase::TRACE, int base=0x10);
   void setreg(const char *regstring,unsigned val);
   unsigned getreg(const char *regstring);
     
};

#endif
