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
  unsigned cycle;   // which subcycle are we in on multipart instructions
  unsigned parity(unsigned v);  // compute parity
  unsigned opcode;    // current opcode
   // temporaries for instructions
  unsigned t1,t2;
  unsigned cond;   // condition
  unsigned getM8(void);   // get M
  void setM8(unsigned v);  // set M
  void setop8(unsigned r, unsigned v);  // set an 8 bit op
  unsigned loadop8(unsigned r);  // load an 8 bit op
  unsigned  getcond(unsigned op);  // get conditional part
  RFP &rfp;  // reference back to RFP
  // modify pc or sp
  unsigned incpc(void)  { unsigned t=pc; pc++; pc&=0xFFFF; return t; }
  unsigned incsp(void)  { unsigned t=sp; sp++; sp&=0xFFFF; return t; }
  void decsp(void)  { sp--; sp&=0xFFFF; }
    
 public:
 CPU(RAM& r,RFP& rp) : ram(r), rfp(rp) { upper=0; reset(); } 
  // reset CPU
  void reset(void);
  // Are we at the start of an instruction (1) or in the middle of one? (0)
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
   // Do an opcode
   void doop(unsigned opcode);
   // set flags after instruction
   unsigned setflags(unsigned value, unsigned savemask);
   // support for trace and control
   void dump(iobase::streamtype s=iobase::TRACE, int base=0x10);
   // set or get register by name
   void setreg(const char *regstring,unsigned val);
   unsigned getreg(const char *regstring);
   // do we conert input to uppercase for SIO?
   int upper;
};

#endif
