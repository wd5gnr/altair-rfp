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
#ifndef _RFP_H
#define _RFP_H
extern "C" 
{
  #include "rs232.h"
}

#include "iobase.h"


class RAM;
class RFP;

#include "breakpoint.h"

class RFP 
{
 protected:
  int ready;
  int soft;  // if 1, then software only
  void sendcmd2(char c, unsigned u);
  unsigned read2(char c);
  unsigned add;
  unsigned dat;
  unsigned status;
  unsigned oldhadd;
  unsigned oldstat;
 public:
  RFP(char *port, int software=0);
  ~RFP();
  iobase *io;
  breakpoint bps[27];  // note one extra for private use
  int isReady(void)   { return ready;  }
  unsigned getID(void);
  void setAhigh(unsigned a);
  void setAlow(unsigned a);
  void setCS(int bit=1);
  void setDB(unsigned d);
  unsigned getDB(void);
  void setIO(int bit=1);
  void setLED(unsigned l);
  unsigned getSWLow(void);
  unsigned getSWHigh(void);
  unsigned getSWFunc(void);
  void setRW(int bit=1);
  void releaseDB(void);
  // high level
  void setstate(void);
  void execute(RAM& ram);
};

  

extern CPU *thecpu;
extern RFP *theRFP;
  
#endif
