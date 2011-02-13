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
#include "cpu.h"
#include <ctype.h>



// Reset
void CPU::reset(void)
{
  pc=0;
  cycle=0;
  // from Intel data sheet:
  // ...the ontents of the program counter is cleared.... the INTE and HLDA
  // flip flops are also reset. Note that the flags, accumulator, stack pointer
  // and registers are not cleared. However, on power up regs could be >0xFF
  // in the simulator so at least chop them
  for (int i=0;i<8;i++) regs[i]=regs[i]&0xFF;
  sp&=0xFFFF; 
}

// Set flags after an instruction
unsigned CPU::setflags(unsigned value, unsigned savemask)
{
  unsigned oldflags;
  unsigned newflags=0;
  savemask|=0x3A;  // save A, N, and the unused (N not used on 8080?)
  oldflags=regs[F]&savemask;
  if (value>=256) newflags|=1;
  value&=0xFF;
  if (value & 0x80) newflags|=0x80;
  if (value==0) newflags|=0x40;
  if (parity(value)) newflags|=4;
  newflags&=~savemask;
  regs[F]=oldflags|newflags;
  return value;
}


// Computer parity for flags
unsigned CPU::parity(unsigned v)
{
  unsigned bit=0;
  unsigned i;
  for (i=0;i<8;i++) bit^=(v&(1<<i))?1:0;
  return bit;
}


// Get 8 bits M (that is [HL])
unsigned CPU::getM8(void)
{
  unsigned a=(regs[H]<<8)+regs[L];
  return ram.read(a);
}


// set 8 bit M
void CPU::setM8(unsigned v)
{
  unsigned a=(regs[H]<<8)+regs[L];
  ram.write(a,v);
}


// get an 8 bit operand (A, B, C, ...M)
unsigned CPU::loadop8(unsigned r)
{
  if (r==6) return getM8();
  if (r==7) r--;
  return regs[r];
}


// Set an 8 bit operand (A, B, C, ... M)
void CPU::setop8(unsigned r,unsigned v)
{
  if (r==6) setM8(v);
  else 
    {
      if (r==7) r--;
      regs[r]=v;
    }
}

// Get conditional part
unsigned CPU::getcond(unsigned cc)
{
  unsigned c;
  switch (cc)
    {
    case 0: return (regs[F]&0x40)!=0x40;
    case 1: return (regs[F]&0x40)==0x40;
    case 2: return (regs[F]&1)!=1;
    case 3: return (regs[F]&1)==1;
    case 4: return (regs[F]&4)==4;
    case 5: return (regs[F]&4)!=4;
    case 6: return (regs[F]&0x80)!=0x80;
    case 7: return (regs[F]&0x80)==0x80;
    }
  
}



// Do an opcode
void CPU::doop(unsigned opcode)
{
  static unsigned r1,r2,op1,op2;
  int inp;
  // rather than mess with pointers to member functions
  // assume the compiler will optmizize a big switch well

  // Move is such a big swath we handle it special
  if (opcode>=0x40 && opcode<=0x7F && opcode!=0x76)
    {
      r1=opcode&7;
      r2=(opcode>>3)&7;  // r2<-r1
      setop8(r2,loadop8(r1));
      return;
    }
  
  switch (opcode)
    {
      // LXI
    case 0x01:
    case 0x11:
    case 0x21:
    case 0x31:
      r1=(opcode&0x30)>>4;
      if (r1!=3) r1*=2;
      switch (++cycle)  // which part of the LXI am I doing?
	{
	case 1: break;
	case 2: if (r1!=3) regs[r1+1]=ram.read(incpc()); else t1=ram.read(incpc()); break;
	case 3: if (r1!=3) regs[r1]=ram.read(incpc()); else sp=ram.read(incpc())*256+t1;
	  cycle=0; 
	  break;
	}
      break;
      
      // INX
    case 0x03:
    case 0x13:
    case 0x23:
      r1=(opcode&0x30)>>3;
      regs[r1+1]++;
      if (regs[r1+1]>=0x100) regs[r1]++;
      regs[r1]&=0xFF;
      regs[r1+1]&=0xFF;
      break;

      
      // INX SP
    case 0x33:
      sp++;
      sp&=0xffff;
      break;


      // DCX
    case 0x0b:
    case 0x1b:
    case 0x2b:
      r1=(opcode&0x30)>>3;
      regs[r1+1]--;
      if (regs[r1+1]>=0x100) regs[r1]--;
      regs[r1]&=0xFF;
      regs[r1+1]&=0xFF;
      break;

    case 0x3B:  // DCX SP
      sp--;
      sp&=0xFFFF;
      break;
      
      
      // DAD
    case 0x09:
    case 0x19:
    case 0x29:
      r1=(opcode&0x30)>>3;
      t1=regs[r1]*0x100+regs[r1+1];
      t1+=regs[H]*0x100+regs[L];
      regs[H]=(t1>>8)&0xFF;
      regs[L]=t1&0xFF;
      regs[F]&=0xFE;
      if (t1>0xFFFF) regs[F]|=1;
      break;
      
      // DAD SP
    case 0x39:
      t1=regs[H]*0x100+regs[L];
      t1+=sp;
      regs[H]=(t1>>8)&0xFF;
      regs[L]=t1&0xFF;
      regs[F]&=0xFE;
      if (t1>0xFFFF) regs[F]|=1;

      break;

      // LDAX
    case 0x0A:
    case 0x1A:
      t1=256*((opcode&0x10)?regs[D]:regs[B]);
      t1+=(opcode&0x10)?regs[E]:regs[C];
      regs[A]=ram.read(t1);
      break;

      // LHLD
    case 0x2A:
      switch (++cycle)
	{
	case 1: break;
	case 2: t1=ram.read(incpc()); break;
	case 3: t1+=ram.read(incpc())*256; regs[L]=ram.read(t1); regs[H]=ram.read(t1+1); cycle=0; break;
	}
      break;

      // LDA
    case 0x3A:
      switch (++cycle)
	{
	case 1: break;
	case 2: t1=ram.read(incpc()); break;
	case 3: t1+=ram.read(incpc())*256; regs[A]=ram.read(t1); cycle=0; break;
	}
      break;
      

      // STAX
    case 0x02:
    case 0x12:
      t1=(opcode&0x10)?regs[D]:regs[B];
      t1*=256;
      t1+=(opcode&0x10)?regs[E]:regs[C];
      ram.write(t1,regs[A]);
      break;
      
      // SHLD
    case 0x22:
      switch (++cycle)
	{
	case 1: break;
	case 2: t1=ram.read(incpc()); break;
	case 3: t1+=ram.read(incpc())*256; cycle=0; ram.write(t1,regs[L]); ram.write(t1+1,regs[H]);  break;
	}
      break;

      // STA
    case 0x32:
      switch (++cycle)
	{
	case 1: break;
	case 2: t1=ram.read(incpc()); break;
	case 3: t1+=ram.read(incpc())*256; cycle=0; ram.write(t1,regs[A]);  break;
	}
      break;

      // MVI
    case 0x06:
    case 0x0e:
    case 0x16:
    case 0x1e:
    case 0x26:
    case 0x2e:
    case 0x36:
    case 0x3e:
      switch (++cycle)
	{
	case 1: r1=(opcode&0x38)>>3; break;
	case 2: cycle=0; t1=ram.read(incpc()); setop8(r1,t1); break;
	}
      break;

      // RLC
    case 0x07:
      regs[F]&=0xFE;
      if (regs[A]&0x80) regs[F]|=1;
      regs[A]=((regs[A]<<1)&0xFE)|(regs[F]&1);
      break;

      // RAL
    case 0x17:
      regs[A]<<=1;
      t1=regs[F]&1;
      regs[F]&=0xFE;
      if (regs[A]&0x100) regs[F]|=1;
      if (t1) regs[A]|=1;
      regs[A]&=0xFF;
      break;
      

      // DAA
    case 0x27:
      t1=0;
      if ((regs[A]&0xF)>9 || (regs[F]&0x10)) 
	{
	  if ((regs[A]&0xF)+6>=0x10) regs[F]|=0x10; else regs[F]&=~0x10;
	  regs[A]+=6;
	}
        if ((regs[A]&0xF0)>0x90 || (regs[F]&1)) regs[A]+=0x60;
      regs[A]=setflags(regs[A],0);
      // not sure how C and AC get set here
      break;

      // RRC
    case 0x0F:
      regs[F]&=0xFE;
      if (regs[A]&1) regs[F]|=1;
      regs[A]=((regs[A]>>1)&0x7F)|((regs[F]&1)?0x80:0);
      break;
      
      // RAR
    case 0x1F:
      regs[A]|=(regs[F]&1)?0x100:0;
      regs[F]&=0xFE;
      if (regs[A]&1) regs[F]|=1;
      regs[A]>>=1;
      break;
      

      // INR
    case 0x04:
    case 0x0C:
    case 0x14:
    case 0x1C:
    case 0x24:
    case 0x2C:
    case 0x34:
    case 0x3C:
      r1=(opcode&0x38)>>3;
      op1=loadop8(r1);
      if ((op1&0xF)+1>=0x10) 
	regs[F]|=0x10;
      else
	regs[F]&=~0x10;
      setop8(r1,setflags(++op1,1));
      break;

      // DCR
    case 0x05:
    case 0x0D:
    case 0x15:
    case 0x1D:
    case 0x25:
    case 0x2D:
    case 0x35:
    case 0x3D:
      r1=(opcode&0x38)>>3;
      op1=loadop8(r1);
      if ((op1&0xF)==0) 
	regs[F]|=0x10;
      else
	regs[F]&=~0x10;  // is this right?
      setop8(r1,setflags(--op1,1));
      break;
      
      // CMA
    case 0x2f:
      regs[A]=(~regs[A])&0xFF;
      break;
      
      // STC
    case 0x37:
      regs[F]|=1;
      break;
      
      // CMC
    case 0x3F:
      regs[F]^=1;
      break;

      // ADD
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
      r1=(opcode&7);
      op1=loadop8(r1);
    l_add:
      if ((op1&0xF)+(regs[A]&0xF)>=0x10)
	regs[F]|=0x10;
      else
	regs[F]&=~10;
      regs[A]=setflags(regs[A]+op1,0);
      break;
      
      // ADC
    case 0x88:
    case 0x89:
    case 0x8A:
    case 0x8B:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x8F:
      r1=(opcode&7);
      op1=loadop8(r1);
    l_adc:
      if ((op1&0xF)+(regs[A]&0xF+(regs[F]&1))>=0x10)
	regs[F]|=0x10;
      else
	regs[F]&=~10;
      regs[A]=setflags(regs[A]+op1+(regs[F]&1),0);
      break;

      // SUB
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
      r1=(opcode&7);
      op1=loadop8(r1);
    l_sub:
      if ((op1&0xF)+(regs[A]&0xF)>=0x10)  // does this matter?
	regs[F]|=0x10;
      else
	regs[F]&=~10;
      regs[A]=(regs[A]+(~op1&0xFF)+1)^0x100;
      regs[A]=setflags(regs[A],0);
      break;
      
      // SBB
    case 0x98:
    case 0x99:
    case 0x9A:
    case 0x9B:
    case 0x9C:
    case 0x9D:
    case 0x9E:
    case 0x9F:
      r1=(opcode&7);
      op1=loadop8(r1);
    l_sbb:
      if ((op1&0xF)+(regs[A]&0xF+(regs[F]&1))>=0x10)  // does this matter?
	regs[F]|=0x10;
      else
	regs[F]&=~10;
      regs[A]=(regs[A]+(~op1&0xFF)+1-(regs[F]&1))^0x100;
      regs[A]=setflags(regs[A],0);
      break;

      
      // AND
    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
      r1=(opcode&7);
      op1=loadop8(r1);
    l_and:
      regs[F]|=0x10;
      regs[A]=setflags(regs[A]&op1,0);
      break;
      
      // XRA
    case 0xA8:
    case 0xA9:
    case 0xAA:
    case 0xAB:
    case 0xAC:
    case 0xAD:
    case 0xAE:
    case 0xAF:
      r1=(opcode&7);
      op1=loadop8(r1);
    l_xra:
      regs[F]&=~0x10;
      regs[A]=setflags(regs[A]^op1,0);
      break;

      // OR
    case 0xB0:
    case 0xB1:
    case 0xB2:
    case 0xB3:
    case 0xB4:
    case 0xB5:
    case 0xB6:
    case 0xB7:
      r1=(opcode&7);
      op1=loadop8(r1);
    l_ora:
      regs[F]&=~0x10;
      regs[A]=setflags(regs[A]|op1,0);
      break;
      
      // CMP
    case 0xB8:
    case 0xB9:
    case 0xBA:
    case 0xBB:
    case 0xBC:
    case 0xBD:
    case 0xBE:
    case 0xBF:
      r1=(opcode&7);
      op1=loadop8(r1);
    l_cmp:
      if ((op1&0xF)+(regs[A]&0xF)>=0x10)  // does this matter?
	regs[F]|=0x10;
      else
	regs[F]&=~10;
      setflags((regs[A]+(~op1&0xFF)+1)^0x100,0);
      break;

      // ADI
    case 0xC6:
      switch (++cycle)
	{
	case 1: break;
	case 2: cycle=0; op1=ram.read(incpc()); goto l_add;
	}
      break;

      // SUI
    case 0xD6:
      switch (++cycle)
	{
	case 1: break;
	case 2: cycle=0; op1=ram.read(incpc()); goto l_sub;
	}
      break;

      // ANI
    case 0xE6:
      switch (++cycle)
	{
	case 1: break;
	case 2: cycle=0; op1=ram.read(incpc()); goto l_and;
	}
      break;

      // ORI
    case 0xF6:
      switch (++cycle)
	{
	case 1: break;
	case 2: cycle=0; op1=ram.read(incpc()); goto l_ora;
	}
      break;

	  
      
      // ACI
    case 0xCE:
      switch (++cycle)
	{
	case 1: break;
	case 2: cycle=0; op1=ram.read(incpc()); goto l_adc;
	}
      break;

      // SBI
    case 0xDE:
      switch (++cycle)
	{
	case 1: break;
	case 2: cycle=0; op1=ram.read(incpc()); goto l_sbb;
	}
      break;

      // XRI
    case 0xEE:
      switch (++cycle)
	{
	case 1: break;
	case 2: cycle=0; op1=ram.read(incpc()); goto l_xra;
	}
      break;

      // CPI
    case 0xFE:
      switch (++cycle)
	{
	case 1: break;
	case 2: cycle=0; op1=ram.read(incpc()); goto l_cmp;
	}
      break;



      // JMP
    case 0xC3:
    case 0xCB:
      switch (++cycle)
	{
	case 1: break;
	case 2: t1=ram.read(incpc()); break;
	case 3: t1+=ram.read(incpc())*256; cycle=0; pc=t1; break;
	}
      break;

      // conditional jumps
      // JMP
    case 0xC2:
    case 0xCA:
    case 0xD2:
    case 0xDA:
    case 0xE2:
    case 0xEA:
    case 0xF2:
    case 0xFA:
      switch (++cycle)
	{
	case 1: cond=getcond((opcode>>3)&7); break;
	case 2: t1=ram.read(incpc()); break;
	case 3: t1+=ram.read(incpc())*256; cycle=0; if (cond) pc=t1; break;
	}
      break;

      
      // CALL
    case 0xCD:
    case 0xDD:
    case 0xED:
    case 0xFD:
      switch (++cycle)
	{
	case 1: break;
	case 2: t1=ram.read(incpc()); break;
	case 3: t1+=ram.read(incpc())*256; decsp(); ram.write(sp,pc>>8); decsp(); ram.write(sp,pc&0xFF); cycle=0; pc=t1&0xFFFF; break;
	}
      break;

      // Conditional Calls
    case 0xC4:
    case 0xCC:
    case 0xD4:
    case 0xDC:
    case 0xE4:
    case 0xEC:
    case 0xF4:
    case 0xFC:
      switch (++cycle)
	{
	case 1: cond=getcond((opcode>>3)&7); break;
	case 2: t1=ram.read(incpc()); break;
	case 3: cycle=0; t1+=ram.read(incpc())*256; if (cond) {  decsp(); ram.write(sp,pc>>8); decsp(); ram.write(sp,pc&0xFF); pc=t1; }  break;
	}
      break;
      
      
      // RET
    case 0xC9:
    case 0xD9:
      pc=ram.read(incsp());
      pc+=ram.read(incsp())<<8;
      pc&=0xffff;
      break;

      // Conditional returns
    case 0xC0:
    case 0xC8:
    case 0xD0:
    case 0xD8:
    case 0xE0:
    case 0xE8:
    case 0xF0:
    case 0xF8:
      cond=getcond((opcode>>3)&7); 
      if (cond) 
	{
	  pc=ram.read(incsp());
	  pc+=ram.read(incsp())<<8;
	  pc&=0xFFFF;
	}
      break;
      
      //POPS
    case 0xC1:
    case 0xD1:
    case 0xE1:
    case 0xF1:
      r1=(opcode&0x30)>>3;
      regs[r1+1]=ram.read(incsp());
      regs[r1]=ram.read(incsp());  
      break;
      

      // PUSHES
    case 0xC5:
    case 0xD5:
    case 0xE5:
    case 0xF5:
      r1=(opcode&0x30)>>3;
      decsp();
      ram.write(sp,regs[r1]);
      decsp();
      ram.write(sp,regs[r1+1]); 
      break;


      // XTHL
    case 0xE3:
      r1=regs[H];
      r2=regs[L];
      // check order todo
      regs[L]=ram.read(sp);
      regs[H]=ram.read((sp+1)&0xFFFF);
      ram.write(sp,r2);
      ram.write((sp+1)&0xFFFF,r1);
      break;
      
      // PCHL
    case 0xE9:
      pc=(regs[H]<<8)+regs[L];
      break;

      // SPHL
    case 0xF9:
      sp=(regs[H]<<8)+regs[L];
      break;
      

      // XCHG
    case 0xEB:
      r1=regs[H];
      r2=regs[L];
      regs[H]=regs[D];
      regs[L]=regs[E];
      regs[D]=r1;
      regs[E]=r2;
      break;

      // OUT
    case 0xD3:
      if (++cycle==1) break;
      cycle=0;
      r1=ram.read(incpc());
      if (r1==0x11) 
	{ 
	  int c=regs[A]&0x7F;
	  if (c=='_') c='\010';
	  iobase::putchar(iobase::CONSOLE,c); 
	  if (c=='\010') 
	    {
	      iobase::putchar(iobase::CONSOLE,' ');
	      iobase::putchar(iobase::CONSOLE,'\010');
	    }
	}
      
      break;
      
      // IN
    case 0xDB:
      switch (++cycle)
	{
	case 1: break;
	case 2: 
	  cycle=0; 
	  r1=ram.read(incpc());
	  switch (r1)
	    {
	    case 0x11: 
	      inp=iobase::getchar(iobase::CONSOLE);
	      regs[A]=(inp<0)?0:inp;
	      if (upper) regs[A]=toupper(regs[A]);
	      if (regs[A]==0x7F) regs[A]='_'; 
	      if (regs[A]=='\n') regs[A]='\r'; 
	      break; 
	    case 0x10: regs[A]=2+iobase::ischar(iobase::CONSOLE); break;  
	    case 0xFF: regs[A]=rfp.getSWHigh(); break;
	    }
	  

	  break;
	}
      break;
      
      // Restarts
    case 0xC7:
    case 0xCF:
    case 0xD7:
    case 0xDF:
    case 0xE7:
    case 0xEF:
    case 0xF7:
    case 0xFF:
      r1=(opcode&0x38);
      decsp();
      ram.write(sp,pc>>8); 
      decsp();
      ram.write(sp,pc&0xFF); 
      pc=r1;
      break;
      


      // EI/DI (no interrupts in this version so nothing)
    case 0xF3:
    case 0xFB:

      // NOP
    case 0x00:
    case 0x08:
    case 0x10:
    case 0x18:
    case 0x20:
    case 0x28:
    case 0x30:
    case 0x38:
      break;
  // HLT
 case 0x76:
   pc--;
   break;


    }
   

}



// Do a step
void CPU::step(void)
{
  if (!cycle) opcode=ram.read(incpc());
  doop(opcode);
}

// Dump state
void CPU::dump(iobase::streamtype s, int base)
{

  iobase::printf(s,
		 base==0x10?"PC=%04X (%02X)  A=%02X F=%02X B=%02X C=%02X D=%02X E=%02X H=%02X L=%02X SP=%04X\r\n":
		 "PC=%06o (%03o)  A=%03o F=%03o B=%03o C=%03o D=%03o E=%03o H=%03o L=%03o SP=%06o\r\n",
	  pc, ram.read(pc,0), regs[A],regs[F],regs[B],regs[C],regs[D],regs[E],
	  regs[H],regs[L],sp);

}

// Get a register by name
unsigned CPU::getreg(const char *regstring)
{
  // first letter is A, B, D, H, S, or P
  int c=toupper(*regstring);
  int idx=-1;
  unsigned v;
  
  switch (c)
    {
    case 'A':
      idx=6;
      break;
      
    case 'B':
      idx=0;
      break;
      
    case 'D':
      idx=2;
      break;
      
    case 'H':
      idx=4;
      break;
      
    case 'S':
      v=sp;
      break;
      
    case 'P':
      v=pc;
      break;
    }

  if (idx!=-1) v=regs[idx]<<8+regs[idx+1];
  return v;
  
   
}

// set a register by name
void CPU::setreg(const char *regstring,unsigned val)
{
  // first letter is A, B, D, H, S, or P
  int c=toupper(*regstring);
  val&=0xFFFF;
  unsigned vh=val>>8, vl=val&0xFF;
  
  switch (c)
    {
    case 'A':
      regs[6]=vh;
      regs[7]=vl;
      break;
      
    case 'B':
      regs[0]=vh;
      regs[1]=vl;
      break;
      
    case 'D':
      regs[2]=vh;
      regs[3]=vl;
      break;
      
    case 'H':
      regs[4]=vh;
      regs[5]=vl;
      break;
      
    case 'S':
      sp=val;
      break;
      
    case 'P':
      pc=val;
      break;
    }

}

