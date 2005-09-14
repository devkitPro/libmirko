/*
 * Draws a nice SelectBox - gp_sbox.c
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

#include <string.h>

#include "gp32.h"
#include "sbox.h"

extern void gp_drawPixel8  ( int x, int y, u8 c ,  u8 *framebuffer );
extern void gp_drawPixel16 ( int x, int y, u16 c, u16 *framebuffer );
extern void gp_drawString  ( int x, int y, int len,char *buffer,u16 color,void *framebuffer);

int gp_drawSelectbox( SELECTBOX *sbox,u16 *framebuffer ) {

//    char buffer[80];
    int i,j,x,y;
    int select=0;
    int entrys;
    int largest=0;

    x=sbox->x;
    y=sbox->y;
    entrys = sbox->entrys;

    // get the largest entry
    for (i=0;i<entrys;i++) if ( strlen(sbox->text[i]) > largest ) largest=strlen(sbox->text[i]);

    // Draw a nice box
    { int xx,yy;
      int breite;
      int hoehe;

      breite = strlen(sbox->banner)*8;
      hoehe  = 8;
      // Draw banner background
      for (yy=y;yy<hoehe +y;yy++)
      for (xx=x;xx<breite+x;xx++) gp_drawPixel16 (xx,yy,(sbox->boxcolor),framebuffer);

      breite = largest*8;
      hoehe  = entrys*8;
      // Draw select background
      for (yy=y+8;yy<hoehe +y+8;yy++)
      for (xx=x;xx<breite+x;xx++) gp_drawPixel16 (xx,yy,sbox->boxcolor,framebuffer);
    }


    // Display Banner
    gp_drawString(x,y,strlen(sbox->banner),sbox->banner,sbox->bannercolor,framebuffer);


    while (1) {
      for (i=0;i<entrys;i++) {
         if (i==select)
         gp_drawString(x,8+y+(i*8),strlen(sbox->text[i]),sbox->text[i],0xFFFF,framebuffer);
         else
         gp_drawString(x,8+y+(i*8),strlen(sbox->text[i]),sbox->text[i],sbox->textcolor,framebuffer);
       }

      if (gp_getButton()&BUTTON_DOWN) select++;
      if (gp_getButton()&BUTTON_UP  ) select--;
      if (select<0) select = 0;
      if (select>entrys-1) select = entrys-1;
      for (j=0;j<1500000;j++) j=j;
      if ( gp_getButton()&BUTTON_A) break;
    }

return select;
}

