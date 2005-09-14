/*
 * Draw a line  - gp_line.c
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


extern void gp_drawPixel16 ( int x, int y, u16 c, u16 *framebuffer );

static
void setpixel(short x, short y, u16 color, u16 *framebuffer ) {
     if ( !((x<0) || (x>319) || (y<0) || (y>239)) )
       gp_drawPixel16(x,y,color,framebuffer);
}

void gp_drawLine16(int x0, int y0, int x1, int y1, u16 color, u16 *framebuffer) {
        int dy = y1 - y0;
        int dx = x1 - x0;
        int stepx, stepy;

        if (dy < 0) { dy = -dy;  stepy = -1;   } else { stepy = 1; }
        if (dx < 0) { dx = -dx;  stepx = -1;   } else { stepx = 1; }
        dy <<= 1;
        dx <<= 1;

        setpixel(x0,y0,color,framebuffer);
        if (dx > dy) {
            int fraction = dy - (dx >> 1);
            while (x0 != x1) {
                if (fraction >= 0) {
                    y0 += stepy;
                    fraction -= dx;
                }
                x0 += stepx;
                fraction += dy;
                setpixel(x0,y0,color,framebuffer);
            }
        } else {
            int fraction = dx - (dy >> 1);
            while (y0 != y1) {
                if (fraction >= 0) {
                    x0 += stepx;
                    fraction -= dy;
                }
                y0 += stepy;
                fraction += dx;
                setpixel(x0,y0,color,framebuffer);
            }
        }
}

