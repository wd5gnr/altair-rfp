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
 * Revised for use with Micro 8800 RFP
*/

/* NOTE: If program hangs, try froma  shell:
  stty -F /dev/ttyPORT -crtscts clocal cbreak
*/
 
#define TEST 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // for memset
#include <termios.h>
#include <fcntl.h>
#include "rs232.h"

/* Must operate at */
//#define BAUDRATE B115200
#define DEBUGOUT 0
#if DEBUGOUT
#define DEBUG(tag,x) printf("DEBUG: %s=%02X\n",tag,x)
#else
#define DEBUG(tag,x)
#endif

int BRTABLE[]=
  {
    B9600,
    B19200,
    B57600,
    B115200,
    B230400
  };

extern int baud;  // from command line
#define BAUDRATE (BRTABLE[baud])

    



/* This is the part that is platform specific */
/* This is for Linux or Cygwin */

static int fd;  // file descriptor
static struct termios oldtio; /* terminal info */


/* Open port */
int rfp_openport(char *port)
{
  struct termios tio;
  int i;
  // work around buggy serial port drivers
  // that hang when no RTSCTS handshaking present
  fd=open(port,O_RDWR|O_NOCTTY|O_NONBLOCK);
  if (fd<0)
    {
      return -1;
    }
  tcgetattr(fd,&oldtio); /* save old settings */
  memset(&tio,0,sizeof(tio));
  // Set up for 8N1, no processing */
  tio.c_cflag=BAUDRATE|CS8|CLOCAL|CREAD;
  tio.c_iflag=IGNPAR;
  tio.c_oflag=0;
  tio.c_lflag=0;
  for (i=0;i<NCCS;i++) tio.c_cc[i]=0;
  tio.c_cc[VMIN]=1;
  tcflush(fd,TCIFLUSH);
  tcsetattr(fd,TCSANOW,&tio); // make settings take  
  close(fd);
  // reopen as blocking
  fd=open(port,O_RDWR|O_NOCTTY);
  if (fd<0)
    {
      return -1;
    }
  // may not need this anymore, but cheap
  memset(&tio,0,sizeof(tio));
  // Set up for 8N1, no processing */
  tio.c_cflag=BAUDRATE|CS8|CLOCAL|CREAD;
  tio.c_iflag=IGNPAR;
  tio.c_oflag=0;
  tio.c_lflag=0;
  for (i=0;i<NCCS;i++) tio.c_cc[i]=0;
  tio.c_cc[VMIN]=1;
  tcflush(fd,TCIFLUSH);
  tcsetattr(fd,TCSANOW,&tio); // make settings take  
  return 0;
}

/* Close the port when done */
void rfp_closeport()
{
  tcsetattr(fd,TCSANOW,&oldtio);
  close(fd);
}

/* Write a byte to the FP */
int rfp_write(int byte)
{
  char c=(char)byte;
  DEBUG("write",byte);
  return write(fd,&c,1);
}

/* Read a byte from the FP */
unsigned int rfp_read(void)
{
  char c;
  unsigned int rv;
  read(fd,&c,1);
  rv=((unsigned int)c) & 0xFF;;  
  DEBUG("read",rv);
  return rv;
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



#if TEST

void err(char *msg)
{
  fprintf(stderr,"%s\n",msg);
  exit(1);
}


int main(int argc, char *argv[])
{
  char c0,c1;
  printf("Open port\n");
  if (rfp_openport(argv[1])<0) err("Can't open port");
  printf("Reset\n");
  rfp_emit('R');
  printf("Query\n");
  rfp_emit('?');
  printf("Reading response\n");
  c0=rfp_read();
  c1=rfp_read();
  printf("%c%c\n",c0,c1);
  printf("Set LEDs to AA\n");
  rfp_emit('L');
  rfp_emit('A');
  rfp_emit('A');
  printf("Closing\n");
  rfp_closeport();
  printf("Done\n");
  return 0;
}

#endif

