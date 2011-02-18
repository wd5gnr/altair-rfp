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

// This class represents a single breakpoint
class breakpoint
{
 protected:
  unsigned lasthit;  // used for one shot, etc.
  unsigned lastvalue;  // used for change breakpoints
  unsigned count;    // count must be 0 to fire 
  unsigned countreset;  // reset value for counts
  int announced;    // if 1 supress more messages
  int state;  // 1= active, 0=inactive, -1=hold
 public:
  char id;   // A-Z
  int oneshot;  // if 1, this breakpoint disables after it fires
  int ttype;    // do we match/change an address or a register?
  unsigned address;  // address to match/monitor
  char reg[16];      // register name to match/monitor (name so we can be CPU independent)
  unsigned mask;    // value is masked (ANDed) against this
  unsigned value;  // value == 0x10000 means we are looking for change!
  int action; // 0= stop, 1=trace, 0x80+# = enable 0x40+# = disable
  breakpoint();
  breakpoint(int st,const char *target, unsigned v, unsigned msk=0xFFFF);
  void setannounce(int b=1)  { announced=b; }
  void init(int st,const char *target, unsigned v, unsigned msk=0xFFFF);
  void setcount(unsigned c);
  unsigned getcount(void);
  void setstate(int s);
  int getstate(void);
  // check to see if breakpoint is active
  // note this happens over and over, so it isn't like the breakpoint fires 
  // and then goes to zero. It stays "hit" until the condition is cleared
  // which means we have to do things like set state to -1 (hold) to resume
  int check(void);
  // dump breakpoint info to stream in base
  void dump(iobase::streamtype,int base);
  static void header(iobase::streamtype s)
  {
      iobase::printf(s,"ID ON  COND\t\t\tCOUNT\tACTION\r\n");

  }
  


};


#endif
