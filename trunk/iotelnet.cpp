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
// Issues:
// TODO: Move to portable C++ socket class so it will work with mingw etc.

// This is the telnet IO stream
#include <stdio.h>
#include <string.h>  // for bzero
#include <fcntl.h>
#include <sys/types.h> 
#if !defined(NOTELNET)
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include "iotelnet.h"
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>


// Construct telnet on given stream and port #
iotelnet::iotelnet(streamtype st,int portno) : iobase(st)
{
#if defined(NOTELNET)
  iobase::printf(iobase::ERROROUT,"Telnet not supported\n");
  exit(1);
#else
  killchar=cbuffer=-1;
  skipct=0;
  port=portno;
  signal(SIGPIPE,SIG_IGN);
  // fire off thread     
  pthread_create(&worker,NULL,connect,(void *)this);
  //  pthread_detach(worker);
#endif
}

// Get a connection
void *iotelnet::connect(void *_this)
{
#if !defined(NOTELNET)
  iotelnet *obj=(iotelnet *)_this;
  struct sockaddr_in serv_addr;
  int sockfd;
  struct sockaddr_in cli_addr;
  socklen_t clilen;
  signal(SIGPIPE,SIG_IGN);
  obj->newsockfd=-1;
  obj->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (obj->sockfd < 0) 
    {
      iobase::printf(iobase::DEBUG,"socket failed %d\n",perror);
      return NULL;
    }
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(obj->port);
  if (bind(obj->sockfd, (struct sockaddr *) &serv_addr,
	   sizeof(serv_addr)) < 0) 
    {
      iobase::printf(iobase::DEBUG,"bind failed %d\n",errno);
      return NULL;
    }

  // wait for a connection
  while (1)
    {
      obj->newsockfd=-1;
      // wait for connection
      listen(obj->sockfd,5);
      clilen = sizeof(cli_addr);
      obj->newsockfd = accept(obj->sockfd, 
			 (struct sockaddr *) &cli_addr, 
			 &clilen);
      // flip to single character no echo
      send(obj->newsockfd,"\xff\xfb\x01\xff\xfb\x03\xff\xfc\x22",9,0);
       fcntl(obj->newsockfd,F_SETFL,O_NONBLOCK);
       // wait for telnet client to disconnect
      while (obj->newsockfd>0);
    }
#endif
}

iotelnet::~iotelnet()
{
  // better to let the threads die 
  // otherwise you get all this strange
  // race behavior as everything tries
  // to shut down together
}

// get character (raw, no pushback)
int iotelnet::getch(void)
{
#if !defined(NOTELNET)
  unsigned char ch;
  int c=-1;
  if (newsockfd>0) 
    if (read(newsockfd,&ch,1)<=0)
      {
	if (errno!=EAGAIN&&errno!=EWOULDBLOCK)
	  {
	    close(newsockfd);
	    newsockfd=-1;
	  }
      }
    else  c=ch;    
  return c;
#else
  return -1;
#endif
}

  

// This is the getchar you want to use; calls getch
int iotelnet::getchar(void)
{
  int c=-1;
#if !defined(NOTELNET)
  unsigned char ch,cc[2];
 getagn:
  if (cbuffer!=-1)  // if there is a pushback buffer, use it
    {
      ch=c=cbuffer;
      cbuffer=-1;
    }
  else 
    ch=c=getch();  // otherwise get character
  
  // consume IAC responses
  // not sure this will always work but should be ok for the kinds of things we do
  if (skipct==2 && c==0xff)
    {
      skipct=0;
    }
  else if (c!=-1 && skipct!=0) 
    {
      skipct--;
      return -1;
    }
  else if (c==0xff) 
    {
      skipct=2;
      return -1;
    }
  
  // Check for kill character
  if (killchar!=-1 && c==killchar)
    {
      do { c=getch();  } while (c==-1);
      if (c!=killchar) exit(0);
    }
#endif
  return c;
}

// is character waiting
int iotelnet::ischar(void)
{
  if (cbuffer==-1) cbuffer=getch();
  return cbuffer!=-1;
}

// put a character
void iotelnet::putchar(int c)
{
#if !defined(NOTELNET)
  char cc[2];
 putagain:
  cc[0]=cc[1]=c;
  if (newsockfd>0) 
    if (write(newsockfd,&c,(c!=0xFF)?1:2)<=0)   // escape 0xFF for telnet
      {
	if (errno!=EAGAIN&&errno!=EWOULDBLOCK)
	  {
	    close(newsockfd);
	    newsockfd=-1;
	  }
	else goto putagain;  // yeah bad form 
      }
#endif
}

// Is the stream ready?
int iotelnet::ready(void)
{
#if !defined(NOTELNET)
  return newsockfd>0;
#else
  return 0;
#endif
}


