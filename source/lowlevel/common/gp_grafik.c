/*
 * Common grafik setup - gp_grafik.c
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
 *  17 Dez 2004 - Mirko Roller <mirko@mirkoroller.de>
 *   added gp_initFramebufferBP, thanx to Reesy
 *  10 Jan 2005 - Mirko Roller <mirko@mirkoroller.de>
 *   added a new Blu+ detection
 *  24 Jan 2005 - Mirko Roller <mirko@mirkoroller.de>
 *   added Reesy setFramebuffer function
 */

#include "gp32.h"

volatile char _SDKVERSION[]="- SDK 0.96 ,copyright 2005 by Mirko Roller  mirko@mirkoroller.de -";

static
void gp_waitVsync() {
  // The LINECNT is downcount from 319 to 0
  while ((rLCDCON1 >> 18) !=   1);
  // changeing Framebuffer
  while ((rLCDCON1 >> 18) != 319);
  // Framebuffer is changed
}

void gp_drawPixel8 ( int x, int y, u8 c, u8 *framebuffer ) {
     *(framebuffer +(239-y)+(240*x) ) = c;
}

void gp_drawPixel16 ( int x, int y, u16 c, u16 *framebuffer ) {
     *(framebuffer +(239-y)+(240*x) ) = c;
}

void gp_setFramebuffer(void *add,int vsync) {
   u32 addr = (u32) add;
   u32 LCDBANK  =  addr >> 22;
   u32 LCDBASEU = (addr & 0x3FFFFF) >> 1;
   u32 LCDBASEL;
   u16 OFFSIZE = 0;
   u16 PAGEWIDTH;
   u16 vidmode = ((rLCDCON1>>1) & 15)-8;   // result in 0,1,2,3,4
   const u8 faktor[]= {15,30,60,120,240};

   LCDBASEL  = LCDBASEU + 320*faktor[vidmode];
   PAGEWIDTH = faktor[vidmode];

   if (vsync) {
     while(1) {if(((rLCDCON5>>17)&3)==2) break;} // wait for active line  
     while(1) {if(((rLCDCON5>>17)&3)!=2) break;} // wait for active line to end - start of front porch and hsync
   }
   rLCDSADDR1 = (LCDBANK<<21) | (LCDBASEU<<0) ;
   rLCDSADDR2 = (LCDBASEL<<0) ;
   rLCDSADDR3 = (OFFSIZE<<11) | (PAGEWIDTH<<0) ;
   if (vsync) gp_waitVsync();
}
/*
void gp_setFramebuffer(void *add,int vsync) {
  u32 addr = (u32) add;
  u32 LCDBANK  =  addr >> 22;
  u32 LCDBASEU = (addr & 0x3FFFFF) >> 1;
  u32 LCDBASEL;
 
  u16 vidmode = ((rLCDCON1>>1) & 15)-8;   // result in 0,1,2,3,4
  const u16 faktor[]= {15,30,60,120,240,480,480};
 
  LCDBASEL  = LCDBASEU + 320*faktor[vidmode];
 
  // wait until lcd is in front porch or hsync before updating lcd regs
  // as screen will be distorted 
  while(1) {if(((rLCDCON5>>17)&3)==2) break;} // wait for active line  
  while(1) {if(((rLCDCON5>>17)&3)!=2) break;} // wait for active line to end - start of front porch and hsync

  // now switch
  rLCDSADDR1 = (LCDBANK<<21) | (LCDBASEU<<0);
  rLCDSADDR2 = (LCDBASEL<<0);
  //rLCDSADDR3 = (OFFSIZE<<11) | (PAGEWIDTH<<0); // only update this once now when framebuffer is initialised
  if (vsync) gp_waitVsync();
}
*/

short gp_initFramebufferN(void *add,u16 bitmode,u16 refreshrate) {
   u32 addr = (u32) add;
   u32 GPHCLK = gp_getHCLK();
   {  u16 BPPMODE = 12;     // default 16 Bit
      u16 CLKVAL = 3;       // default 16 Bit
      u16 ENVID = 1;
      u16 MMODE = 0;
      u16 PNRMODE = 3;
      switch (bitmode) {
         //case 16 : BPPMODE=12; break;
         case  8 : BPPMODE=11; break;
         case  4 : BPPMODE=10; break;
         case  2 : BPPMODE= 9; break;
         case  1 : BPPMODE= 8; break;
      }
      // Get the correct CLKVAL for refreshrate
      // works in all bitmodes now :)
      if (refreshrate <  50) refreshrate = 50;
      if (refreshrate > 120) refreshrate =120;
      CLKVAL = (GPHCLK/(83385*2*refreshrate))-1;
      if (CLKVAL == 0) CLKVAL=1;
      refreshrate = GPHCLK / (83385*2*(CLKVAL+1));
      rLCDCON1 = (CLKVAL<<8) | (MMODE<<7) | (PNRMODE<<5) | (BPPMODE<<1) | (ENVID<<0) ;
   }
   {  u16 LINEVAL = 320-1;
      u16 VBPD = 1;
      u16 VFPD = 2;
      u16 VSPW = 1;
      rLCDCON2 = 0;
      rLCDCON2 = (VBPD<<24) | (LINEVAL<<14) | (VFPD<<6) | (VSPW<<0) ;
   }
   {  u16 HBPD = 6;
      u16 HFPD = 2;
      u16 HOZVAL = 240-1;
      rLCDCON3 = 0;
      rLCDCON3 = (HBPD<<19) | (HOZVAL<<8) | (HFPD<<0) ;
   }
   {  u16 ADDVAL = 0;
      u16 HSPW = 4;
      u16 MVAL = 0;
      u16 PALADDEN = 0;
      rLCDCON4 = 0;
      rLCDCON4 = (PALADDEN<<24) | (ADDVAL<<16) | (MVAL<<8) | (HSPW<<0) ;
   }
   {  u16 BSWP = 0;
      u16 ENLEND = 0;
      u16 HWSWP = 1;
      u16 INVENDLINE = 0;
      u16 INVVCLK = 1;
      u16 INVVD = 0;
      u16 INVVDEN = 0;
      u16 INVVFRAME = 1;
      u16 INVVLINE = 1;
      if (bitmode<16) { BSWP=1;HWSWP=0; }
      rLCDCON5 = 0;
      rLCDCON5 = (INVVCLK<<10) | (INVVLINE<<9) | (INVVFRAME<<8) | (INVVD<<7) | (INVVDEN<<6)
               | (INVENDLINE<<4) | (ENLEND<<2) | (BSWP<<1) | (HWSWP<<0) ;
   }
   gp_setFramebuffer((u32*)addr,1);

return refreshrate;
}

short gp_initFramebufferBP(void *add,u16 bitmode,u16 refreshrate) {
  u32 addr = (u32) add;
  u32 GPHCLK = gp_getHCLK();
  {  u16 BPPMODE = 12;     // default 16 Bit
     u16 CLKVAL = 3;       // default 16 Bit
     u16 ENVID = 1;
     u16 MMODE = 0;
     u16 PNRMODE = 3;
     switch (bitmode) {
        //case 16 : BPPMODE=12; break;
        case  8 : BPPMODE=11; break;
        case  4 : BPPMODE=10; break;
        case  2 : BPPMODE= 9; break;
        case  1 : BPPMODE= 8; break;
     }
     // Get the correct CLKVAL for refreshrate
     // works in all bitmodes now :)
     if (refreshrate <  50) refreshrate = 50;
     if (refreshrate > 120) refreshrate =120;
     CLKVAL = (GPHCLK/(109850*2*refreshrate))-1;
     if (CLKVAL == 0) CLKVAL=1;
     refreshrate = GPHCLK / (109850*2*(CLKVAL+1));
     rLCDCON1 = (CLKVAL<<8) | (MMODE<<7) | (PNRMODE<<5) | (BPPMODE<<1) | (ENVID<<0);
  }
  {  u16 LINEVAL = 320-1;
     u16 VBPD = 8;
     u16 VFPD = 2;
     u16 VSPW = 5;
     rLCDCON2 = 0;
     rLCDCON2 = (VBPD<<24) | (LINEVAL<<14) | (VFPD<<6) | (VSPW<<0);
  }
  {  u16 HBPD = 50;
     u16 HFPD = 2;
     u16 HOZVAL = 240-1;
     rLCDCON3 = 0;
     rLCDCON3 = (HBPD<<19) | (HOZVAL<<8) | (HFPD<<0);
  }
  {  u16 ADDVAL = 0;
     u16 HSPW = 30;
     u16 MVAL = 0;
     u16 PALADDEN = 0;
     rLCDCON4 = 0;
     rLCDCON4 = (PALADDEN<<24) | (ADDVAL<<16) | (MVAL<<8) | (HSPW<<0);
  }
  {  u16 BSWP = 0;
     u16 ENLEND = 0;
     u16 HWSWP = 1;
     u16 INVENDLINE = 0;
     u16 INVVCLK = 1;
     u16 INVVD = 0;
     u16 INVVDEN = 0;
     u16 INVVFRAME = 1;
     u16 INVVLINE = 1;
     if (bitmode<16) { BSWP=1;HWSWP=0; }
     rLCDCON5 = 0;
     rLCDCON5 = (INVVCLK<<10) | (INVVLINE<<9) | (INVVFRAME<<8) | (INVVD<<7) | (INVVDEN<<6)
              | (INVENDLINE<<4) | (ENLEND<<2) | (BSWP<<1) | (HWSWP<<0);
  }
  gp_setFramebuffer((u32*)addr,1);

return refreshrate;
}

// Here we must decide what machine we are running on.
// 1st we check the bios version
// 2nd we check if the User pressed the R or L Button ( this will override bios check )
//   R = Blu+
//   L = gp32 normal, flu, blu
// Date  0x0C7B004C bit 0-15
// 0x1018 also seems to contain a date code (in the 1.5.7e firmware it's 2001-11-01).
short gp_initFramebuffer(void *add,u16 bitmode,u16 refreshrate) {
  u32 bios=0;
  u32 biosv=*(int*)0x1014; 
  if (biosv<0x01030606) bios=0; else bios=1;
  if (gp_getButton()&BUTTON_L) bios=0;
  if (gp_getButton()&BUTTON_R) bios=1;
  if (bios==1) return (gp_initFramebufferBP(add,bitmode,refreshrate));
  return (gp_initFramebufferN (add,bitmode,refreshrate));
}

void gp_setPalette( unsigned char pos,u16 color) {
   u32 *palette = (u32*) PALETTE;
   // writing to palette is only allowed, if VSTATUS 
   // is not in active mode
   while ((rLCDCON5>>19) == 2);
   palette[pos]=color;
}

