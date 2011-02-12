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
#ifndef __BREAKPOINT_H
#define __BREAKPOINT_H

class CPU;
class RFP;
#include "iobase.h"


class breakpoint
{
 protected:
  unsigned lasthit;  // used for one shot
  unsigned lastvalue;
  unsigned count;
  unsigned countreset;
  int announced;
  int state;  // 1= active, 0=inactive, -1=hold
 public:
  char id;
  int oneshot;
  int ttype;
  unsigned address;
  char reg[16];
  unsigned mask;    // if mask==0 then on change bp
  unsigned value;
  int action; // 0= stop, 1=trace, 0x80+# = enable 0x40+# = disable
  breakpoint();
  breakpoint(int st,const char *target, unsigned v, unsigned msk=0xFFFF);
  void setannounce(int b=1)  { announced=b; }
  void init(int st,const char *target, unsigned v, unsigned msk=0xFFFF);
  void setcount(unsigned c);
  unsigned getcount(void);
  void setstate(int s);
  int getstate(void);
  int check(void);
      //-- if tempinactive and hit do nothing, if tempinative and no hit set active, if iactive and hit that's a real hit if inactive do nothing
  void dump(iobase::streamtype,int base);
  


};


#endif
