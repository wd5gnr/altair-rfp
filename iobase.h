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
#ifndef _IOBASE_H
#define _IOBASE_H
#include <stdio.h>
#include <stdarg.h>

#undef putchar
#undef getchar

class iobase
{
 protected:
  static iobase *streams[];
  static int init;
  static void deleteall(void);
 public:
 enum streamtype 
  {
    ERROROUT,
    CONSOLE,
    TRACE,
    CONTROL,
    DEBUG
  };
  static int getchar(streamtype st) 
  {
    if (!streams[st]) return -1;
    return streams[st]->getchar();
  }
  static int ischar(streamtype st)
  {
    if (!streams[st]) return 0;
    return streams[st]->ischar();
  }
  static void putchar(streamtype st,int c)
  {
    if (!streams[st]) return;
    streams[st]->putchar(c);
  }

  int killchar;
  virtual int ready(void) { return 1;  }
  static int ready(streamtype st)
  {
    if (!streams[st]) return 1;
    return streams[st]->ready();
  }
  
    
  virtual int getchar(void);
  virtual int ischar(void);
  virtual void putchar(int c);
  
  static void printf(streamtype st, const char *fmt,...)
  {
    if (!streams[st]) return;
    char buffer[1024];
    va_list args;
    va_start(args,fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);
    streams[st]->puts(buffer);
  }
  static void puts(streamtype st, const char *s)
  {
    if (!streams[st]) return;
    streams[st]->puts(s);
  }
  virtual void puts(const char *s);
  iobase(streamtype type);
  virtual ~iobase();
  static void dup(streamtype dst, streamtype src)
  {
    streams[dst]=streams[src];
  }
  
};


class console : public iobase
{
 protected:
  FILE *out;
 public:
  console(iobase::streamtype st,FILE *outstream=stderr) ;
  virtual ~console();
  int getchar(void);
  int ischar(void);
  void putchar(int c);
  void puts(const char *s);
};

#endif
