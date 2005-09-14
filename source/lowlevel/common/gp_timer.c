/*
 * Realtimeclock driver - gp_timer.c
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

static char in_use=0;
volatile u32 rGLOBALCOUNTER;

static void RTCInt(void) __attribute__ ((interrupt ("IRQ")));
static void RTCInt(void) {
   rGLOBALCOUNTER++;
}

static
void setupRTCInt( void ) {
   // enable RTC
   rCLKCON |= 0x800;
   //
   // The CPU clock indepentent RTC Timer
   //
   // The ticnt register 0x15700044 (32 bit) offers:
   //
   // BIT   7: 0=disable 1=enable timer
   // BIT 0-6: tick time count value 1-127
   //
   // %10000000 should result in a 1/128 timer, but is not working
   // %10000001 result in 1/64 timer
   // %11111111 result in 1/1  timer
   
   rTICINT = 0x81;
   rGLOBALCOUNTER = 0;
}

static
void InstallRTC( void ) {
   gp_disableIRQ();
   gp_installSWIIRQ(8,RTCInt);
   gp_enableIRQ();
}

u32 gp_getRTC() {
   return rGLOBALCOUNTER;
}

void gp_initRTC() {
   if (in_use == 0) {
      setupRTCInt();
      InstallRTC();
      in_use=1;
   }
}

void gp_clearRTC() {
   rGLOBALCOUNTER=0;
}
