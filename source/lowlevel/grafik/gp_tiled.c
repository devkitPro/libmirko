/*
 * Tiled grafik driver - gp_tiled.c
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


#include <gp32.h>
//#include <sprite.h>

void gp_drawTiled16 (u16 *sprite_data, u16 trans, u16 x_len, u16 plot_number, s16 plot_x, s16 plot_y, u16 *framebuffer) {

      SHEADER *sheader;
      sheader = (SHEADER*)sprite_data;
      u16 tiled_x = sheader->size_x;
      u16 tiled_y = sheader->size_y;
      int y,x,offset;
      offset = plot_number*x_len;
      for (y=0;y<tiled_y;y++)
      for (x=0;x<x_len  ;x++) {
         if ( (plot_x+x>=0) && (plot_x+x<320) && (plot_y+y>=0) && (plot_y+y<240) ) {
            u16 point = sprite_data[6+x+offset+(y*tiled_x)];
            if (point != trans) gp_drawPixel16(plot_x+x ,plot_y+y, point, framebuffer);
         }
      }
}
