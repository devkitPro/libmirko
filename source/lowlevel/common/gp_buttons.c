/*
 * Button control driver - gp_buttons.c
 *
 * Copyright (C) 2003,2004 Mirko Roller <mirko@mirkoroller.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Changelog:
 *
 *  21 Aug 2004 - Mirko Roller <mirko@mirkoroller.de>
 *   first release 
 *
 */

#include "gp32.h"

void gp_initButton() {
  rPBCON=0x0; // Init, all Input
  //rPECON=0x0;
}
      
u16 gp_getButton() {
  u16 back=0;
  u32 pressedb = ((~rPBDAT >> 8) & 0xFF);
  u32 pressedc = ((~rPEDAT >> 6) & 0x3 );  // Bit 6&7  // 1=start 2=select
  
  back = (pressedc<<8)|(pressedb<<0);
   
  return back;
}
