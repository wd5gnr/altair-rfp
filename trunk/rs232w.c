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
/** @file
 * @author Al Williams alw@al-williams.com
 * @version 1.1
 * @date 17 Feb 2004
*/

#include <windows.h>
#include <stdio.h>
#include "rs232.h"

/** If DEBUGOUT=1 then the platform-specific code will generate
 * interesting debug messages.
 * 
 */
#define DEBUGOUT 0
#if DEBUGOUT
#define DEBUG(tag,x) printf("DEBUG: %s=%02X\n",tag,x)
#else
#define DEBUG(tag,x)
#endif

/* This is the part that is platform specific */
/* This is for Windows */

static HANDLE comport;  // file descriptor

extern int baud;

static char *bauds[]=
  {
    "baud=9600 parity=N data=8 stop=1",
    "baud=19200 parity=N data=8 stop=1",
    "baud=57600 parity=N data=8 stop=1",
    "baud=115200 parity=N data=8 stop=1",
    "baud=230400 parity=N data=8 stop=1"
  };

  


/* Open port at 9600 baud */
int rfp_openport(char *port)
{
	DCB dcb;
	COMMTIMEOUTS cto;
	DWORD rv;
	comport=CreateFile(port,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if (comport) 
	{
	   SetupComm(comport,128,128);
	   cto.ReadIntervalTimeout=0;
	   cto.ReadTotalTimeoutMultiplier=100;
	   cto.ReadTotalTimeoutConstant=0;
	   cto.WriteTotalTimeoutMultiplier=0;
	   cto.WriteTotalTimeoutConstant=100;
	   SetCommTimeouts(comport,&cto);
	   dcb.DCBlength=sizeof(dcb);
	   GetCommState(comport,&dcb);
	   rv=BuildCommDCB(bauds[baud],&dcb);
	   dcb.fBinary=TRUE;
	   dcb.fDtrControl=DTR_CONTROL_DISABLE;
	   dcb.fOutxDsrFlow=FALSE;
	   dcb.fNull=FALSE;
	   dcb.fRtsControl=RTS_CONTROL_ENABLE;
	   dcb.fOutxCtsFlow=FALSE;
	   dcb.fDsrSensitivity=FALSE;
	   dcb.fOutX=FALSE;
	   dcb.fInX=FALSE;
           rv=SetCommState(comport,&dcb);
	   if (!rv) 
	   {
		   CloseHandle(comport);
		   comport=NULL;
	   }
	}
	return comport!=NULL?0:-1;
}

/* Close the port when done */
void rfp_closeport()
{
  CloseHandle(comport);
}

/* Write a byte to the FP */
int rfp_write(int byte)
{
  char c=(char)byte;
  DWORD n=0;
  DEBUG("write",byte);
  WriteFile(comport,&c,1,&n,NULL);
  return n==1;
}

/* Read a byte from the fp */
unsigned int rfp_read(void)
{
  char c;
  DWORD n;
  unsigned int rv;
  ReadFile(comport,&c,1,&n,NULL);
  rv=((unsigned int)c) & 0xFF;;  
  DEBUG("read",rv);
  return (n==1)?rv:-1;
}

int rfp_emit(int byte)
{
  DEBUG("emit", byte);
  if (rfp_write(byte)<0) return -1;
  DEBUG("emit1",0);
  if (rfp_read()!=byte) return -1;
  DEBUG("emit2",0);
  return 0;
}

// Send a command that needs no response (or response is last of string) 
int rfp_cmd(char *s)
{
  while (*s) if (rfp_emit(*s++)<0) return -1;
  return 0;
}






