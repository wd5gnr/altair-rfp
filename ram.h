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
#ifndef __RAM_H
#define __RAM_H
#include <stdio.h>
#include "iobase.h"
#include "rfp.h"

// Class representing memory (no implementation file at all)

class RAM
{
 protected:
  unsigned len;
  unsigned char *memory;
  RFP& rfp;
  // set the front panel LEDs if possible
  void setstatus(unsigned a, unsigned status=0xFF) 
  {
    if (statusskip && statusct--) return;
    statusct=statusskip;
    rfp.setAhigh(a>>8); 
    rfp.setAlow(a&0xFF); 
    if (status!=0xFF) rfp.setLED(status);
    rfp.setDB(a<len?memory[a]:0xFF);
  }
  
 public:
 RAM(RFP &r, unsigned siz=0x10000, char *filen=NULL) : rfp(r) { memory=new unsigned char[len=siz]; statusct=0;  statusskip=0;
    if  (filen) load(filen);  };
  ~RAM() { delete memory; }
  // track infrequent updates
  unsigned statusct;
  unsigned statusskip;
  void load(const char *filen,unsigned off=0, unsigned flen=0xFFFF)   // todo: more error checking
  {
           FILE *f=fopen(filen,"rb");
	   if (!f) 
	     {
	       iobase::printf(iobase::ERROROUT,"Failed to read %s\n",filen);
	       return;
	     }
	   int dbg=fread(memory+off,len>flen?flen:len,1,f);
           fclose(f);
  }
  void save(const char *filen, unsigned off=0, unsigned flen=0xFFFF)
  {
    FILE *f=fopen(filen,"wb");
    fwrite(memory+off,len>flen?flen:len,1,f);
    fclose(f);
  }
  
  // todo set MR or MW leds
  unsigned read(unsigned a,int setled=1) { if (setled) setstatus(a); return a<len?memory[a]:0xFF; }
  void write(unsigned a, unsigned v, int setled=1) { if (a<len) memory[a]=v; if (setled) setstatus(a); } ;
};


#endif
  

   
  
