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
#include <stdlib.h>
#include <stdarg.h>
#if !defined(NOTELNET)
#include <pthread.h>
#endif


#if defined(WIN32) 
#include <conio.h>
void init_keyboard() 
{
}

void close_keyboard()
{
}

#elif defined (CYGWIN)
#include <windows.h>
void init_keyboard()
{
}

void close_keyboard()
{
}


int kbhit(void)
{
  INPUT_RECORD buffer;
  DWORD num;
  HANDLE hStdin=GetStdHandle(STD_INPUT_HANDLE);
  PeekConsoleInput(hStdin,&buffer,1,&num);
  //  if (num>0 && buffer.EventType==KEY_EVENT) fprintf(stderr,"Peek %d\n",buffer.Event.KeyEvent.bKeyDown);
  if (num>0 && buffer.EventType==KEY_EVENT && buffer.Event.KeyEvent.bKeyDown) 
    {
      return 1;
    }
  //  if (buffer.EventType==KEY_EVENT)
  if (num>0)
    {
      INPUT_RECORD bogus;
      DWORD l;
      ReadConsoleInput(hStdin,&bogus,1,&l);
    }
  return 0;
}

int getch(void)
{
  DWORD oldmode, newmode;
  unsigned char buffer;
  DWORD num;
  if (!kbhit()) return -1;
  HANDLE hStdin=GetStdHandle(STD_INPUT_HANDLE);
  newmode=0;
  GetConsoleMode(hStdin, &oldmode);
  SetConsoleMode(hStdin, newmode);
  ReadConsole(hStdin, &buffer,1,&num,NULL);
  SetConsoleMode(hStdin, oldmode);
  if (num<=0) return -1;
  if (buffer=='\010') buffer='\x7f';
  return buffer;
}



#else
extern "C" 
{
  #include "coniol.h"
}
#endif

int iobase::init=0;
iobase *iobase::streams[5]=
  { NULL, NULL, NULL, NULL, NULL };


iobase::iobase(streamtype type)
{
   killchar=-1; 
   streams[type]=this;
   if (!init)
     {
       init=1;
       atexit(deleteall);
     }
   
}




int iobase::getchar(void) 
{
  return 0;
}

int iobase::ischar(void)
{
  return 0;
}

void iobase::putchar(int c) 
{
}

void iobase::puts(const char *s)
{
  int c;
  while (c=*s++) putchar(c);
}


iobase::~iobase()
{
  for (int i=0;i<sizeof(streams)/sizeof(streams[0]);i++)
    if (streams[i]==this) streams[i]=NULL;
}

#if !defined(NOTELNET)
extern pthread_t controlthread;
#endif

void iobase::deleteall(void)
{
  int i;
    for (i=0;i<sizeof(streams)/sizeof(streams[0]);i++)
    if (streams[i]) 
      {
	delete streams[i];
      }
  
}


console::console(iobase::streamtype st, FILE *outstream) : iobase(st)
{
  init_keyboard();
  atexit(close_keyboard);
  out=outstream;
}

console::~console()
{
  close_keyboard();
}


int console::getchar(void)
  {
    int c=getch();
    if (c==killchar) 
      {
	do c=getch(); while (c==-1);
	if (c!=killchar) exit(0);
      }
    return c;
  }

int console::ischar(void)
  {
    return kbhit();
  }
  
void console::putchar(int c)
  {
    putc(c,out);
    fflush(out);
  }

void console::puts(const char *s)
{
  int c;
  while (c=*s++) putc(c,out);
  fflush(out);
}

  
