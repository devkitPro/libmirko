/*
 * CPU setup - gp_cpuspeed.c
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


/*
MCLK is the main clock for the CPU                (160Mhz maximum)
HCLK is the clock for the system bus (SDRAM LCD)  (133Mhz maximum)
PCLK is the clock for the peripherals (UARTs, SMC, IIS, timers etc) (66Mhz maximum)

The values for clk_mode are:
0 HCLK = MCLK,     PCLK = MCLK
1 HCLK = MCLK,     PCLK = MCLK / 2
2 HCLK = MCLK / 2, PCLK = MCLK / 2
3 HCLK = MCLK / 2, PCLK = MCLK / 4
*/

void gp_initButton();
void _cpu_speed(int MCLK,int CLKDIV,int CLKMODE);


// 40Mhz mode
static int CLKDIV =0x48013;
static int MCLK   =40000000;
static int CLKMODE=0;
static int HCLK   =40000000;
static int PCLK   =40000000;

int gp_getPCLK(){
   return PCLK;
}

int gp_getHCLK(){
   return HCLK;
}

void gp_setCpuspeed(int freq) {
  switch (freq) {
     // overclocking 
     case 168: { CLKDIV=0x14000; MCLK=168000000; CLKMODE=3; break;}
     case 172: { CLKDIV=0x23010; MCLK=172000000; CLKMODE=3; break;}
     case 176: { CLKDIV=0x24010; MCLK=176000000; CLKMODE=3; break;}
     case 180: { CLKDIV=0x16000; MCLK=180000000; CLKMODE=3; break;}
     case 184: { CLKDIV=0x26010; MCLK=184000000; CLKMODE=3; break;}
     case 188: { CLKDIV=0x27010; MCLK=188000000; CLKMODE=3; break;}
     case 192: { CLKDIV=0x18000; MCLK=192000000; CLKMODE=3; break;}
     case 196: { CLKDIV=0x29010; MCLK=196000000; CLKMODE=3; break;}
     case 200: { CLKDIV=0x2A010; MCLK=200000000; CLKMODE=3; break;}
     case 204: { CLKDIV=0x2b010; MCLK=204000000; CLKMODE=3; break;}
     case 208: { CLKDIV=0x2c010; MCLK=208000000; CLKMODE=3; break;}
     case 212: { CLKDIV=0x2d010; MCLK=212000000; CLKMODE=3; break;}
     case 216: { CLKDIV=0x2e010; MCLK=216000000; CLKMODE=3; break;}
     case 220: { CLKDIV=0x2f010; MCLK=220000000; CLKMODE=3; break;}
     case 224: { CLKDIV=0x30010; MCLK=224000000; CLKMODE=3; break;}
     case 228: { CLKDIV=0x1e000; MCLK=228000000; CLKMODE=3; break;}
     case 232: { CLKDIV=0x32010; MCLK=232000000; CLKMODE=3; break;}
     case 236: { CLKDIV=0x33010; MCLK=236000000; CLKMODE=3; break;}
     case 240: { CLKDIV=0x20000; MCLK=240000000; CLKMODE=3; break;}
     case 244: { CLKDIV=0x35010; MCLK=244000000; CLKMODE=3; break;}
     case 248: { CLKDIV=0x36010; MCLK=248000000; CLKMODE=3; break;}
     case 252: { CLKDIV=0x22000; MCLK=252000000; CLKMODE=3; break;}
     case 256: { CLKDIV=0x38010; MCLK=256000000; CLKMODE=3; break;}
     
     // normal
     case 166: { CLKDIV=0x4B011; MCLK=166000000; CLKMODE=3; break;}
     case 164: { CLKDIV=0x4a011; MCLK=164000000; CLKMODE=3; break;}
     case 160: { CLKDIV=0x48011; MCLK=160000000; CLKMODE=3; break;}
     case 156: { CLKDIV=0x2c001; MCLK=156000000; CLKMODE=3; break;}
     case 144: { CLKDIV=0x28001; MCLK=144000000; CLKMODE=3; break;}
     case 133: { CLKDIV=0x3a011; MCLK=132000000; CLKMODE=3; break;}
     case 100: { CLKDIV=0x2b011; MCLK=102000000; CLKMODE=2; break;}
     case  66: { CLKDIV=0x25002; MCLK= 67500000; CLKMODE=2; break;}
     case  50: { CLKDIV=0x2a012; MCLK= 50000000; CLKMODE=0; break;}
     case  40: { CLKDIV=0x48013; MCLK= 40000000; CLKMODE=0; break;}
     case  33: { CLKDIV=0x25003; MCLK= 33750000; CLKMODE=0; break;}
     case  22: { CLKDIV=0x33023; MCLK= 22125000; CLKMODE=0; break;}
  }
   if (CLKMODE==0) {HCLK=MCLK  ;PCLK=MCLK  ;}
   if (CLKMODE==1) {HCLK=MCLK  ;PCLK=MCLK/2;}
   if (CLKMODE==2) {HCLK=MCLK/2;PCLK=MCLK/2;}
   if (CLKMODE==3) {HCLK=MCLK/2;PCLK=MCLK/4;}
   _cpu_speed(MCLK,CLKDIV,CLKMODE);
   gp_initButton();  // Init the Buttons to input
}

void gp_Reset() {
   gp_setCpuspeed(66);
   asm volatile("swi #4\n");
}

