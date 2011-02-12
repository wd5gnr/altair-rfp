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
/*
 * @author Al Williams alw@al-williams.com
 * @version 1.1
 * 
 * Revised for use with Micro8800 RFP
 *
 */
 
/** @file
 * @author Al Williams alw@al-williams.com
 * @version 1.1
 * @date 17 Feb 2004
 */



#ifndef _AWC_GP3LIB_H
#define _AWC_GP3LIB_H

/** Open the port. 
*/
int rfp_openport(char *port);  // returns -1 for error
/** Close the port.
 */
void rfp_closeport();
/** Write a byte 
 * @param byte The byte to write
 * @return 1 for success
 */
int rfp_write(int byte);   // returns 1 for success
/** Read a byte 
 * This function blocks until a byte is received
 * @return Character or -1 in case of error
 */
unsigned int rfp_read(void);  // returns character or -1 for error

/** Emit a character to the RFP and wait for echo acknowledge
 * @return 0 on success, -1 on error
 */
int rfp_emit(int byte);

/** Emit a command to the RFP and wait for echo acknowledge
 * Command must either expect no input or the command that
 * generates input must be LAST
 * @return 0 on success, -1 on error
 */
int rfp_cmd(char *s);


/** @} */

#endif


