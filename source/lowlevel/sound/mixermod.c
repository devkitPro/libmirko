/*
 * IRQ based mixer  - gp_mixermod.c
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
#include "modplayer.h"

volatile unsigned short playmodfile=0;
volatile struct sample {
   u16 *data;
   u16 freq;
   u8  stereo;
   u8  volume;
   u16 play;
   u32 pos;
   u32 size;
} SAMPLE;

#define SEGMENTSIZE 1920  //Bytes

static void TIMER4IRQ(void) __attribute__ ((interrupt ("IRQ")));
static void TIMER4IRQ(void) {
     u16 *renderbuffer = (u16*)SAMPLEBUFFER1; //One playsegment, 1920 bytes
     int i;
     rTCON = BITCLEAR(rTCON,20); // timer4 off

        if (playmodfile) gp_rendermod(renderbuffer);
        else {
           s16 *rend = (s16*)renderbuffer;
           for (i=0;i<SEGMENTSIZE/2;i++) *rend++=0;
        }

        if (SAMPLE.play == 1) {
             int samples = SEGMENTSIZE;
             int left= SAMPLE.size - SAMPLE.pos;
             s16 *sam= (s16*)((SAMPLE.data)+(SAMPLE.pos/2));
             s16 *buf= (s16*)renderbuffer;
             if (left<=samples) { SAMPLE.play=0;samples=left; }

             if (SAMPLE.stereo == 1) for (i=0;i<samples/2;i++) renderbuffer[i]=((*buf++) + (*sam++))/2;
             if (SAMPLE.stereo == 0) {
               s16 *render = (s16*)renderbuffer;
               for (i=0;i<samples/2/2;i++) {
               *render++=((*buf++) + (*sam))/2;
               *render++=((*buf++) + (*sam++))/2;
               }
             samples/=2;
             }
             SAMPLE.pos+= samples;
        }


     gp_addRingsegment(renderbuffer);
     rTCON = BITSET(rTCON,20); // timer4 on
}

static int initsound=1;
void gp_startSoundmixer( int finetuning ) {
   int pclk;
   pclk = gp_getPCLK();
   pclk/= 16;
   pclk/= 256;
   if (finetuning)
   pclk/= (49-finetuning);  // 48Hz timer, will be enought for some Clockrates (but not all !)
   else
   pclk/= 51;  // 51Hz timer, tested 33 66 133 Mhz

   SAMPLE.play=0;
   if (initsound) {
      gp_initSound(22050,16,SEGMENTSIZE*2);
      initsound=0;
   }
   gp_disableIRQ();
   rTCFG0 |= (0xFF<<8);   // Presacler for timer 2,3,4 = 256
   rTCFG1 |= (0x03<<16);  // timer4  1/16
   rTCNTB4 = (long)pclk;
   rTCON  = (0x1<<22) | (0x1<<20); // start timer4, auto reload
   gp_installSWIIRQ(14,TIMER4IRQ);
   gp_enableIRQ();
}

//static
void gp_stopSoundmixer() {
    gp_disableIRQ();
    rTCON = BITCLEAR(rTCON,20); // timer4 off
    gp_removeSWIIRQ(14,TIMER4IRQ);
    gp_enableIRQ();
    gp_clearRingbuffer();
}

void gp_startModfile(unsigned char *mod) {
   playmodfile=0;
   gp_disableIRQ();
   gp_startmod(mod);
   gp_enableIRQ();
   playmodfile=1;
}

void gp_pauseModfile() {
   playmodfile=0;
}

void gp_resumeModfile() {
   playmodfile=1;
}

void gp_addSample(u16 *data, u16 freq, u16 stereo, u8 volume, u32 size) {
   SAMPLE.size  = size;  //size in Bytes
   SAMPLE.pos   = 0;
   SAMPLE.freq  = freq;
   SAMPLE.data  = data;
   SAMPLE.stereo= stereo;
   SAMPLE.volume= volume;
   SAMPLE.play  = 1;
}


