/*
 * DMA access - gp_memory.c
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


int gp_get32MBCHECK() {
   if ( ( rBANKSIZE & 7) == 0) return 1;
   else return 0;
}

void gp_waitDMA (u8 channel) {
         // Wait until dma controller is ready
         if (channel ==0 ) while ((rDSTAT0 >> 20) & 3);
         if (channel ==1 ) while ((rDSTAT1 >> 20) & 3);
         if (channel ==2 ) while ((rDSTAT2 >> 20) & 3);
         if (channel ==3 ) while ((rDSTAT3 >> 20) & 3);
}

// DMA 2 and 3 is used for Sound, beware of this
void gp_setDMA( u32 s, u32 d, u32 size,u8 channel ) {

         // DMA Control register
         int DMD_HS = 0;  // 0=demand 1=handshake mode
         int SYNC   = 1;  // 0=sync to PCLK/APB 1=HCLK/AHB
         int INT    = 0;  // 0=irq disable 1=irq enable (when transfer is done)
         int TSZ    = 0;  // transfer size 0=one 1=four units
         int SERVMODE=1;  // 0=single service 1=whole service
         int HWSRCSEL=0;  // 0=nXDREQ0 DMA Source
         int SWHW_SEL=0;  // 0=SW request mode 1=source selected by bit HWSRCSEL
         int RELOAD = 1;  // 0=auto reload 1=turned off
         int DSZ    = 1;  // 0=8bit 1=16bit 2=32bit  data size
         int TC     = (size); // Transfercount DSZ*TSZ*TC

         u32 rdcon  = (DMD_HS<<30)|(SYNC<<29)|(INT<<28)|(TSZ<<27)|(SERVMODE<<26)
                      |(HWSRCSEL<<24)|(SWHW_SEL<<23)|(RELOAD<<22)|(DSZ<<20)|(TC<<0);

         // DMA channel on in Software mode
         gp_waitDMA(channel);
         switch (channel) {
           case 0 : {  rDISRC0 = s; rDIDST0 = d; rDCON0 = rdcon; rDMASKTRIG0 = 3;  break; }
           case 1 : {  rDISRC1 = s; rDIDST1 = d; rDCON1 = rdcon; rDMASKTRIG1 = 3;  break; }
           case 2 : {  rDISRC2 = s; rDIDST2 = d; rDCON2 = rdcon; rDMASKTRIG2 = 3;  break; }
           case 3 : {  rDISRC3 = s; rDIDST3 = d; rDCON3 = rdcon; rDMASKTRIG3 = 3;  break; }
         }

}

