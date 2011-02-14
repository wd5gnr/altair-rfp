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
#include "iobase.h"
#if !defined(NOTELNET)
#include <pthread.h>
#else
#define pthread_t int
#endif

// Telnet I/O stream

class iotelnet : public iobase
{
 protected:
  // sockets and ports
  volatile int newsockfd;
  int sockfd;
  int port;
  pthread_t worker;
  static void *connect(void *);
  int cbuffer;
  int skipct;
  int getch(void);
 public:
  iotelnet(streamtype st, int portno);
  virtual ~iotelnet();
  void putchar(int c);
  int getchar(void);
  int ischar(void);
  int ready(void);
};

