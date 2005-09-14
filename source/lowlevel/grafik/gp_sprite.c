/*
 * Sprite display driver - gp_sprite.c
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
//#include "sprite.h"

extern void gp_drawPixel16 ( int x, int y, u16 c, u16 *framebuffer );

static
void setpixel(short x, short y, u16 color, u16 *framebuffer ) {
     if ( !((x<0) || (x>319) || (y<0) || (y>239)) )
       gp_drawPixel16(x,y,color,framebuffer);
}

// result = ALPHA * ( srcPixel - destPixel ) + destPixel             0.0 - 1.0
// result = ( ALPHA * ( srcPixel - destPixel ) ) / 256 + destPixel   0   - 255
// result   - Is the alpha blended color
// srcPixel - Is the foreground pixel
// destPixel- Is the background pixel

// alpha 0-31
void gp_drawSpriteHTB( u16 *sprite, short put_x, short put_y, u16 *framebuffer, u16 trans,u8 alpha) {
     int x,y;
     int xx,yy;
     SHEADER *sheader;
     u16   *spriteraw;
     u16   color;
     u16   SR,SG,SB;
     u16   DR,DG,DB;

     sheader = (SHEADER*) sprite;
     spriteraw = sprite + 6;
     if (alpha > 31 ) alpha = 31;
     for (yy=0; yy<(sheader->size_y); yy++) {
        y=put_y+yy;
        if ( !((y<0) || (y>239)) ) {
         for (xx=0; xx<(sheader->size_x); xx++) {
           color = *(spriteraw++);
           if ( color != trans ) {
              x=put_x+xx;
              //y=put_y+yy;
              //if ( !((x<0) || (x>319) || (y<0) || (y>239)) ) {
              if ( !((x<0) || (x>319)) ) {
                 SB=(color>> 1)&31;
                 SG=(color>> 6)&31;
                 SR=(color>>11)&31;
                 color = *(framebuffer +(239-y)+(240*x) );
                 DB=(color>> 1)&31;
                 DG=(color>> 6)&31;
                 DR=(color>>11)&31;
                 color = ( (((alpha*(SR-DR))>>5)+DR)<<11 ) | ((((alpha*(SG-DG))>>5)+DG)<<6) | ((((alpha*(SB-DB))>>5)+DB)<<1);
                 *(framebuffer +(239-y)+(240*x) ) = color;
              }
           }
         }
        } else spriteraw+=sheader->size_x;
     }
}

void gp_drawSpriteHT( u16 *sprite, short put_x, short put_y, u16 *framebuffer, u16 trans) {
     int xx,yy,i=0;
     SHEADER *sheader;
     u16   color;

     sheader = (SHEADER*) sprite;
     for (yy=0; yy<(sheader->size_y); yy++)
        for (xx=0; xx<(sheader->size_x); xx++) {
           color = sprite[6+i++];
           if ( color != trans ) 
             setpixel(put_x+xx,put_y+yy,color,framebuffer);
        }
}

void gp_drawSpriteH( u16 *sprite, short put_x, short put_y, u16 *framebuffer) {
     int xx,yy,i=0;
     SHEADER *sheader;
     
     sheader = (SHEADER*) sprite;
     for (yy=0; yy<(sheader->size_y); yy++)
     for (xx=0; xx<(sheader->size_x); xx++) setpixel(put_x+xx,put_y+yy,sprite[6+i++],framebuffer);
}



/* please only use the ...H (header) functions above, thanx ... */

void gp_drawSpriteT( u16 *sprite, short put_x, short put_y, u16 *framebuffer, u16 trans, u16 xsize, u16 ysize) {
     short xx,yy;
     int   i=0;
     u16   color;

     for (yy=0;yy<ysize ;yy++)
     for (xx=0;xx<xsize ;xx++) {
         color = sprite[i++];
         if ( color != trans )
           setpixel(put_x+xx,put_y+yy,color,framebuffer);
     }
}

void gp_drawSprite( u16 *sprite, short put_x, short put_y, u16 *framebuffer, u16 xsize, u16 ysize) {
     short xx,yy;
     int   i=0;

     for (yy=0;yy<ysize ;yy++)
     for (xx=0;xx<xsize ;xx++) setpixel(put_x+xx,put_y+yy,sprite[i++],framebuffer);
}

