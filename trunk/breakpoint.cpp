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
#include "breakpoint.h"
#include "cpu.h"
#include "contterm.h"
#include <string.h>

// See important comments in breakpoint.h

breakpoint::breakpoint()
{
  lasthit=0;   // initial conditions
  state=0;
  oneshot=0;
  count=0;
  ttype=0;
  address=0;
  reg[0]='\0';
  mask=0xFFFF;
  value=0;
  action=0;
  announced=0;
  id='?';
}


void breakpoint::init(int st, const char *target,  unsigned v, unsigned msk)
{
  state=st;    // set up without constructing new
  value=v;
  mask=msk;
  announced=0;
  lasthit=0;
  count=0;
  countreset=0;
  oneshot=0;
  action=0;
  if (*target=='@')   // target of @xxx is an address
    {
      address=strtonum(target+1);
      ttype=0;
    }
  else  // must be a register
    {
      strcpy(reg,target);
      ttype=1;
    }
  if (!thecpu) lastvalue=0;  // what else can you do? No CPU means no last value
  else if (value>=0x10000)  // change bp?
    {
      if (ttype==0)
	lastvalue=thecpu->ram.read(address,0);  // prime lastvalue
      else
	lastvalue=thecpu->getreg(reg);  // prime with register
    }
}


breakpoint::breakpoint(int st, const char *target,  unsigned v, unsigned msk)
{
  init(st,target,v,msk);
}

// returns action or -1 if not happening
int breakpoint::check(void)
{
  int hit=0;
  unsigned target;
  if (state==0) return -1;  // ignore disabled breakpoint
  // get current value
  if (ttype==0)
    target=thecpu->ram.read(address,0);
  else
    target=thecpu->getreg(reg);
  // if value == 0x10000 then this is a change bp
  if (value<0x10000)
    {
      target&=mask;
      hit=target==value;   // simple match bp
    }
  else
    {
      hit=((target&mask)!=(lastvalue&mask));  // change bp
    }
  // so at this point we kind of know if we have a hit or not
  // but we need to check state, and count
  if (!hit && state==-1 && value<0x10000)  // this is a match bp on hold that released
    {
      state=oneshot?0:1;
      announced=0;
      lasthit=0;
      return -1;  // take off of hold
    }
  if (hit && state==-1 && value>=0x10000) // this is a change bp on hold that ought to release
    {
      state=oneshot?0:1;
      announced=0;
      lastvalue=target;
      lasthit=0;
      return -1;
    }
  // reset counting breakpoints that are resuming just in case
  if (hit && state==-1 && countreset) count=countreset;
  // if on hold then ignore any hit
  if (hit && state==-1) return -1;  // we are on hold so no hit
  if (!hit && countreset)    // if we are counting and this wasn't a hit
    {
      lasthit=0;            // remember we have to count the next hit
      if (!count) count=countreset;  // if count was zero, put it back
    }
  
  // check for counting supression
  if (hit)
    {
      // is this a counting bp?
      if (countreset)
	if (count && !lasthit)   // are we counting?
	{
	  count--;
	  if (count!=0) hit=0; else  lasthit=1; 
	}
    }
  
  // Check if we should reset a one shot
  if (oneshot) 
    {
      if (lasthit && !hit)  state=0;
      lasthit=hit;
    }
      
  // if we have a hit here we are at a breakpoint
  // but if we are stopping and haven't already 
  // announced then we ask contterm.cpp to print for us
  if (hit && !announced && action==0)
    {
      announced=1;
      print_bp(id);
    }
  // done!
  return hit?action:-1;
}

// accessors/setters
void breakpoint::setcount(unsigned c)
{
  count=countreset=c;
}

unsigned breakpoint::getcount(void) 
{
  return count;
}

// Changing state needs to clear announce flag
void breakpoint::setstate(int s)
{
  state=s;
  if (s!=1) announced=0;
}

int breakpoint::getstate(void) 
{
  return state;
}

// Dump out breakpoint for the list code in contterm
void breakpoint::dump(iobase::streamtype s, int base)
{
  char tstring[16];
  char mstring[32];
  if (ttype==0) sprintf(tstring,base==0x10?"@%04X":"@%06o",address); else strcpy(tstring,reg);
  if (value<0x10000) sprintf(mstring,base==0x10?"MASK %04X == %04X":"MASK %06o == %06o",mask,value);
  else sprintf(mstring,base==0x10?"MASK %04X CHANGE":"MASK %06o CHANGE",mask);
  iobase::printf(s,
		 base==0x10?"%c: %s %s %s\t%04X%s"
		 :"%c: %s %s %s \t%06o%s",
		 id,state?"ON ":"OFF",tstring,mstring,count,oneshot?"ONCE":"    ");
  if (action==0) strcpy(tstring,"STOP ");
  if (action==1) strcpy(tstring,"TRACE");
  if (action&0x80) sprintf(tstring,"ENA %c",(action&0x3F)+'A');
  if (action&0x40) sprintf(tstring,"DIS %c",(action&0x3F)+'A');
  iobase::printf(s,"%s\r\n",tstring);
}

