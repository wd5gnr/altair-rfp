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
#include <termios.h>
#include <unistd.h>   // for read()

#include <stdio.h> // temporary for fprintf stderr debugging

static struct termios initial_settings, new_settings;
static int peek_character = -1;

static kopen=0;

void init_keyboard(void)
{
    if (kopen) return;
    tcgetattr(0,&initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~(ICRNL|INLCR);
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VERASE]=0;
    new_settings.c_cc[VKILL]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    kopen=1;
}

void close_keyboard(void)
{
  if (kopen)
    {
      int i=0;
      // for some reason you sometimes have to try a few times to get this to work
      do
	{
	  i++;
	  tcsetattr(0, TCSAFLUSH, &initial_settings);
	  tcgetattr(0,&new_settings);
	} while (i<10 && memcmp(&new_settings,&initial_settings,sizeof(struct termios)));
    kopen=0;
    }
  
}

int kbhit(void)
{
unsigned char ch;
int nread;

    if (peek_character != -1) return 1;
    new_settings.c_cc[VMIN]=0;
    tcsetattr(0, TCSANOW, &new_settings);
    nread = read(0,&ch,1);
    new_settings.c_cc[VMIN]=1;
    tcsetattr(0, TCSANOW, &new_settings);
    if(nread == 1) 
    {
        peek_character = ch;
        return 1;
    }
    return 0;
}

int getch(void)
{
char ch;

    if(peek_character != -1) 
    {
        ch = peek_character;
        peek_character = -1;
        return ch;
    }
    read(0,&ch,1);
    return ch;
}

