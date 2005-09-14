/*
 * Font driver - gp_setfont.c
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

extern void gp_drawPixel8  ( int x, int y, u8 c ,  u8 *framebuffer );
extern void gp_drawPixel16 ( int x, int y, u16 c, u16 *framebuffer );

extern const unsigned char font8x8[]; // 1024x8
#define fontwidth 1024


static
void set_char8x8_16bpp (int xx,int yy,int offset,u16 mode,u16 *framebuffer) {
  unsigned int x, y, pixel;
  offset *= 8;
  for (x = 0; x < 8; x++)
    for (y = 0; y < 8; y++) {
        pixel = font8x8[x + (y * fontwidth) + offset];
        if (pixel == 0) gp_drawPixel16 ( (xx+x)%320, (yy+y)%240, mode,framebuffer);
    }
}

static
void set_char8x8_8bpp (int xx,int yy,int offset,u16 mode,u8 *framebuffer) {
  unsigned int x, y, pixel;
  offset *= 8;
  for (x = 0; x < 8; x++)
    for (y = 0; y < 8; y++) {
        pixel = font8x8[x + (y * fontwidth) + offset];
        if (pixel == 0) gp_drawPixel8 ( (xx+x)%320, (yy+y)%240, mode,(u8*)framebuffer);
    }
}


void gp_drawString (int x,int y,int len,char *buffer,u16 color,void *framebuffer) {
      char text[80];
      int l,base=0;
      char bppmode;
      bppmode = (rLCDCON1>>1)&0xF;  // 12=16Bit 11=8Bit 
      for (l=0;l<80;l++) text[l]=buffer[l];if (len>79) len=79;
      for (l=0;l<len;l++) {
         if ( text[l]>126 ) text[l]=0;
         if (bppmode==12) set_char8x8_16bpp (x+base,y,text[l],color,framebuffer);
         if (bppmode==11) set_char8x8_8bpp  (x+base,y,text[l],color,framebuffer);
         base+=8;
      }
}

