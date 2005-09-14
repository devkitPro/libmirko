/*
 * Blitting lib - gp_clipped.c
 *
 * Copyright (C) 2003,2004,2005  CARTIER Matthieu <matkeupon@wanadoo.fr>
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
 *  25 Jan 2005 - CARTIER Matthieu <matkeupon@wanadoo.fr>
 *   first release
 *
*/


//Some global variables and macros
#include "gp_asmlib.h"

#define	screen_width	320
#define	screen_height	240

#define	mulzoom(nb)	( ( (nb) * zoomf) >> 10)
#define	divzoom(nb)	( ( (nb) * zoominv) >> 10)

int zoomf, zoominv;

void changezoom(int newzoom) {
	zoomf = newzoom;
	zoominv = 1048576 / zoomf + 1;
}

//No zoom

//void FastTransBlit(const int numsurface, int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans) {
void gp_FastTransBlit(void *framebuffer, int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans) {
	int xmin, ymin, xmax, ymax;
	int height2 = ( (height + 3) >> 2) << 2;

	if(dx < 0) {
		xmin = -dx;
	} else xmin = 0;
	if( (dx++ + width) > screen_width) {
		xmax = screen_width - dx;
	} else xmax = width - 1;
	if(dy < 0) {
		ymax = height + dy - 1;
	} else ymax = height - 1;
	if( (dy + height) > screen_height) {
		ymin = dy + height - screen_height;
	} else ymin = 0;
	if( (xmin > xmax) || (ymin > ymax) ) return;

	//unsigned char *dst4 = gpDraw[numsurface].ptbuffer + (dx + xmax) * screen_height - height - dy + 1 + ymin;
	unsigned char *dst4 = framebuffer + (dx + xmax) * screen_height - height - dy + 1 + ymin;
	src += (xmax * height2 + ymin);
	ASMFastTransBlit(src, dst4, xmax - xmin, ymax - ymin, height2, trans);
}

//void FastSolidBlit(const int numsurface, int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans, const int coul) {
void gp_FastSolidBlit(void *framebuffer, int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans, const int coul) {
	int xmin, ymin, xmax, ymax;
	int height2 = ( (height + 3) >> 2) << 2;

	if(dx < 0) {
		xmin = -dx;
	} else xmin = 0;
	if( (dx++ + width) > screen_width) {
		xmax = screen_width - dx;
	} else xmax = width - 1;
	if(dy < 0) {
		ymax = height + dy - 1;
	} else ymax = height - 1;
	if( (dy + height) > screen_height) {
		ymin = dy + height - screen_height;
	} else ymin = 0;
	if( (xmin > xmax) || (ymin > ymax) ) return;

	//unsigned char *dst4 = gpDraw[numsurface].ptbuffer + (dx + xmax) * screen_height - height - dy + 1 + ymin;
	unsigned char *dst4 = framebuffer + (dx + xmax) * screen_height - height - dy + 1 + ymin;
	src += (xmax * height2 + ymin);
	ASMFastSolidBlit(src, dst4, xmax - xmin, ymax - ymin, height2, trans, coul);
}

//Zoom

//void ZoomBlit(const int numsurface, const int dx, const int dy, const int width, const int height, const unsigned char *src) {
void gp_ZoomBlit(void *framebuffer, const int dx, const int dy, const int width, const int height, const unsigned char *src) {
	int x2 = divzoom(dx);
	int y2 = divzoom(dy);
	int w2 = divzoom(dx + width) - x2;
	int h2 = divzoom(dy + height) - y2;
	int xmin, ymin, xmax, ymax;
	int height2 = ( (height + 3) >> 2) << 2;

	if(x2 < 0) {
		xmin = -x2;
	} else xmin = 0;
	if( (x2++ + w2) > screen_width) {
		xmax = screen_width - x2;
	} else xmax = w2 - 1;
	if(y2 < 0) {
		ymax = h2 + y2 - 1;
	} else ymax = h2 - 1;
	if( (y2 + h2) > screen_height) {
		ymin = y2 + h2 - screen_height;
	} else ymin = 0;
	if( (xmin > xmax) || (ymin > ymax) ) return;

	//unsigned char *dst4 = gpDraw[numsurface].ptbuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
	unsigned char *dst4 = framebuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
	src += mulzoom(xmin) * height2 + mulzoom(ymin);
	ASMZoomBlit(src, dst4, xmax - xmin, ymax - ymin, height2, zoomf);
}

//void ZoomTransBlit(const int numsurface, const int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans) {
void gp_ZoomTransBlit(void *framebuffer, const int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans) {
	int x2 = divzoom(dx);
	int y2 = divzoom(dy);
	int w2 = divzoom(dx + width) - x2;
	int h2 = divzoom(dy + height) - y2;
	int xmin, ymin, xmax, ymax;
	int height2 = ( (height + 3) >> 2) << 2;

	if(x2 < 0) {
		xmin = -x2;
	} else xmin = 0;
	if( (x2++ + w2) > screen_width) {
		xmax = screen_width - x2;
	} else xmax = w2 - 1;
	if(y2 < 0) {
		ymax = h2 + y2 - 1;
	} else ymax = h2 - 1;
	if( (y2 + h2) > screen_height) {
		ymin = y2 + h2 - screen_height;
	} else ymin = 0;
	if( (xmin > xmax) || (ymin > ymax) ) return;

	//unsigned char *dst4 = gpDraw[numsurface].ptbuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
	unsigned char *dst4 = framebuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
	src += mulzoom(xmin) * height2 + mulzoom(ymin);
	ASMZoomTransBlit(src, dst4, xmax - xmin, ymax - ymin, height2, zoomf, trans);
}

//void ZoomSolidBlit(const int numsurface, const int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans, const int coul) {
void gp_ZoomSolidBlit(void *framebuffer, const int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans, const int coul) {
	int x2 = divzoom(dx);
	int y2 = divzoom(dy);
	int w2 = divzoom(dx + width) - x2;
	int h2 = divzoom(dy + height) - y2;
	int xmin, ymin, xmax, ymax;
	int height2 = ( (height + 3) >> 2) << 2;

	if(x2 < 0) {
		xmin = -x2;
	} else xmin = 0;
	if( (x2++ + w2) > screen_width) {
		xmax = screen_width - x2;
	} else xmax = w2 - 1;
	if(y2 < 0) {
		ymax = h2 + y2 - 1;
	} else ymax = h2 - 1;
	if( (y2 + h2) > screen_height) {
		ymin = y2 + h2 - screen_height;
	} else ymin = 0;
	if( (xmin > xmax) || (ymin > ymax) ) return;

	//unsigned char *dst4 = gpDraw[numsurface].ptbuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
	unsigned char *dst4 = framebuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
	src += mulzoom(xmin) * height2 + mulzoom(ymin);
	ASMZoomSolidBlit(src, dst4, xmax - xmin, ymax - ymin, height2, zoomf, trans, coul);
}



//These can be flipped horizontally with cote (cote = 1 => normal, cote = -1 => flipped

//void ZoomTransBlit(const int numsurface, const int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans, const int cote) {
void gp_ZoomTransBlitFlip(void *framebuffer, const int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans, const int cote) {
	int x2 = divzoom(dx);
	int y2 = divzoom(dy);
	int w2 = divzoom(width);
	int h2 = divzoom(height);
	int xmin, ymin, xmax, ymax;
	int height2 = ( (height + 3) >> 2) << 2;

	if(y2 < 0) {
		ymax = h2 + y2 - 1;
	} else ymax = h2 - 1;
	if( (y2 + h2) > screen_height) {
		ymin = y2 + h2 - screen_height;
	} else ymin = 0;
	if(ymin > ymax) return;

	unsigned char *dst4;
	if(cote > 0) {
		if(x2 < 0) {
			xmin = -x2;
		} else xmin = 0;
		if( (x2++ + w2) > screen_width) {
			xmax = screen_width - x2;
		} else xmax = w2 - 1;
		if(xmin > xmax) return;

		//dst4 = gpDraw[numsurface].ptbuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
		dst4 = framebuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
		src += mulzoom(xmin) * height2 + mulzoom(ymin);
		ASMZoomTransBlit(src, dst4, xmax - xmin, ymax - ymin, height2, zoomf, trans);
	} else {
		if(x2 < 0) {
			xmax = w2 + x2 - 1;
		} else xmax = w2 - 1;
		if( (x2 + w2) > screen_width) {
			xmin = x2 + w2 - screen_width;
		} else xmin = 0;
		if(xmin > xmax) return;

		//dst4 = gpDraw[numsurface].ptbuffer + (x2 + w2 - xmax) * screen_height - h2 - y2 + ymin;
		dst4 = framebuffer + (x2 + w2 - xmax) * screen_height - h2 - y2 + ymin;
		src += mulzoom(xmin) * height2 + mulzoom(ymin);
		ASMZoomTransInvBlit(src, dst4, xmax - xmin, ymax - ymin, height2, zoomf, trans);
	}
}

//void ZoomSolidBlit(const int numsurface, const int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans, const int coul, const int cote) {
void gp_ZoomSolidBlitFlip(void *framebuffer, const int dx, const int dy, const int width, const int height, const unsigned char *src, const int trans, const int coul, const int cote) {
	int x2 = divzoom(dx);
	int y2 = divzoom(dy);
	int w2 = divzoom(width);
	int h2 = divzoom(height);
	int xmin, ymin, xmax, ymax;
	int height2 = ( (height + 3) >> 2) << 2;

	if(y2 < 0) {
		ymax = h2 + y2 - 1;
	} else ymax = h2 - 1;
	if( (y2 + h2) > screen_height) {
		ymin = y2 + h2 - screen_height;
	} else ymin = 0;
	if(ymin > ymax) return;

	unsigned char *dst4;
	if(cote > 0) {
		if(x2 < 0) {
			xmin = -x2;
		} else xmin = 0;
		if( (x2++ + w2) > screen_width) {
			xmax = screen_width - x2;
		} else xmax = w2 - 1;
		if(xmin > xmax) return;

		//dst4 = gpDraw[numsurface].ptbuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
		dst4 = framebuffer + (x2 + xmax) * screen_height - h2 - y2 + 1 + ymin;
		src += mulzoom(xmin) * height2 + mulzoom(ymin);
		ASMZoomSolidBlit(src, dst4, xmax - xmin, ymax - ymin, height2, zoomf, trans, coul);
	} else {
		if(x2 < 0) {
			xmax = w2 + x2 - 1;
		} else xmax = w2 - 1;
		if( (x2 + w2) > screen_width) {
			xmin = x2 + w2 - screen_width;
		} else xmin = 0;
		if(xmin > xmax) return;

		//dst4 = gpDraw[numsurface].ptbuffer + (x2 + w2 - xmax) * screen_height - h2 - y2 + ymin;
		dst4 = framebuffer + (x2 + w2 - xmax) * screen_height - h2 - y2 + ymin;
		src += mulzoom(xmin) * height2 + mulzoom(ymin);
		ASMZoomSolidInvBlit(src, dst4, xmax - xmin, ymax - ymin, height2, zoomf, trans, coul);
	}
}

//Copy from framebuffer to dest

//void SaveBitmap(const int numsurface, int dx, const int dy, const int width, const int height, const unsigned char *dest) { //Sur l'écran
void gp_SaveBitmap(void *framebuffer, int dx, const int dy, const int width, const int height, unsigned char *dest) { //Sur l'icran
	int xmin, ymin, xmax, ymax;
	int height2 = ( (height + 3) >> 2) << 2;

	if(dx < 0) {
		xmin = -dx;
	} else xmin = 0;
	if( (dx++ + width) > screen_width) {
		xmax = screen_width - dx;
	} else xmax = width - 1;
	if(dy < 0) {
		ymax = height + dy - 1;
	} else ymax = height - 1;
	if( (dy + height) > screen_height) {
		ymin = dy + height - screen_height;
	} else ymin = 0;
	if( (xmin > xmax) || (ymin > ymax) ) return;

	//unsigned char *src4 = gpDraw[numsurface].ptbuffer + (dx + xmax) * screen_height - height - dy + ymin;
	unsigned char *src4 = framebuffer + (dx + xmax) * screen_height - height - dy + ymin;
	dest += (xmin * height2 + ymin + 1);
	ASMSaveBitmap(src4, dest, xmax - xmin, ymax - ymin, height2);
}

//Clears area with color #0, should not trigger clicky noise

//void FastClear(int numsurface, int dx, int dy, int width, int height) {
void gp_FastClear(void *framebuffer, int dx, int dy, int width, int height) {
	int xmin, ymin, xmax, ymax;

	if(dx < 0) {
		xmin = -dx;
	} else xmin = 0;
	if( (dx++ + width) > screen_width) {
		xmax = screen_width - dx;
	} else xmax = width - 1;
	if(dy < 0) {
		ymax = height + dy - 1;
	} else ymax = height - 1;
	if( (dy + height) > screen_height) {
		ymin = dy + height - screen_height;
	} else ymin = 0;
	if( (xmin > xmax) || (ymin > ymax) ) return;

//	int decaly = screen_height - height - dy;

	//unsigned char *dst4 = gpDraw[numsurface].ptbuffer + (dx + xmax) * screen_height - height - dy + ymin;
	unsigned char *dst4 = framebuffer + (dx + xmax) * screen_height - height - dy + ymin;
	ASMFastClear(dst4, xmax - xmin, ymax - ymin);
}
