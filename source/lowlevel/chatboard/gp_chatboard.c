/*
 * Chatboard driver - gp_chatboard.c
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


// Hardware project from: http://www.deadcoderssociety.tk
// This file is written 2004 Mirko Roller

#include <string.h>

#include <gp32.h>
#include "keydefines.h"


#define CHA_KEY_NUMBER (sizeof(cha01KeyMap) / sizeof(struct threeMap))
// 13 = CR
// 10 = LF

volatile unsigned char  chatboard_buffer[256];
volatile unsigned int   chatboard_buffer_pos=0;
volatile unsigned char  chatboard_tx[32];
volatile unsigned char  seq_ok=0;

static
void ChatBoardSendOKIRQ() {
   int x=0;
   chatboard_tx[0] = 79;  // O
   chatboard_tx[1] = 75;  // K
   chatboard_tx[2] = 13;
   chatboard_tx[3] = 10;
   chatboard_tx[4] =  0;
   //rSRCPND = (1 << 28);  // Start IRQ TX
   //rINTPND = (1 << 28);
   while (1) {
     if (((rUFSTAT0 >> 4) & 0x0f) < 16)  rUTXH0 = chatboard_tx[x++];
     if (x == 5) break;
   }
}

void UART0Rx(void) __attribute__ ((interrupt ("IRQ")));
void UART0Rx(void) {
     volatile u8 chars=0;
     volatile u8 c;
     volatile u8 i;

     if (rUFSTAT0 & 0x100) chars = 16;
     else chars = rUFSTAT0 & 0x0F;   //  Number of data in rx fifo

     for( i=0;i<chars;i++) {
        c = rURXH0;
        if (chatboard_buffer_pos<256) chatboard_buffer[chatboard_buffer_pos++] = c;
        if ( c==13 ) {
           seq_ok=1;
           chatboard_buffer_pos=0;
           ChatBoardSendOKIRQ();
        }
     }
}

// Not used yet....
void UART0Tx(void) __attribute__ ((interrupt ("IRQ")));
void UART0Tx(void) {
}


int gp_initChatboard() {
   int PCLK = 40000000;
   int BOUD = 9600;
   int y=0;

   PCLK = gp_getPCLK();
   chatboard_buffer_pos=0;

   rCLKCON |= 0x100; // Controls PCLK into UART0 block

   {  u16 IRDA = 0;          // No irda mode
      u16 PARITY = 0;        // No parity
      u16 STOPBITS = 0;      // 1 Stop Bit
      u16 DATABITS = 0x3;    // 8 Data Bit
      rULCON0 = (IRDA<<6) | (PARITY<<3) | (STOPBITS<<2) | (DATABITS<<0) ;
   }

   {  u16 TXIT = 0;
      u16 RXIT = 0;
      u16 RXTIMEOUT = 1; // Enable Timeout irq
      u16 RXERROR = 0;
      u16 LOOPBACK = 0;
      u16 SENDBREAK = 0;
      u16 TXMODE = 0x01; // IRQ/Polling mode
      u16 RXMODE = 0x01; // IRQ/Polling mode
      rUCON0 = (TXIT<<9)|(RXIT<<8)|(RXTIMEOUT<<7)|(RXERROR<<6)|(LOOPBACK<<5)|
                      (SENDBREAK<<4)|(TXMODE<<2)|(RXMODE<<0);
   }

   {  u16 TXFIFOTRIGGER = 0; //  0 Byte
      u16 RXFIFOTRIGGER = 3; // 16 Byte
      u16 TXFIFORESET = 1;
      u16 RXFIFIRESET = 1;
      u16 FIFOENABLE  = 1;
      rUFCON0 = (TXFIFOTRIGGER<<6)|(RXFIFOTRIGGER<<4)|(TXFIFORESET<<2)|
                       (RXFIFIRESET<<1)|(FIFOENABLE<<0);
   }

   {  int UBRDIV;
      UBRDIV = (PCLK/(BOUD * 16))-1;
      rUBRDIV0 = UBRDIV;
   }

   gp_disableIRQ();
   gp_installSWIIRQ(23,UART0Rx);
   gp_installSWIIRQ(28,UART0Tx);
   gp_enableIRQ();

   // Check some time if a seq comes, or not.
   while (y++<50000000) {
     if ( seq_ok==1 ) return 1;
   }

   gp_disableIRQ();
   gp_removeSWIIRQ(23,UART0Rx);   // Rx int
   gp_removeSWIIRQ(28,UART0Tx);   // Tx int
//   gp_removeSWIIRQ(23);   // Rx int
//   gp_removeSWIIRQ(28);   // Tx int
   gp_enableIRQ();
   return 0;
}

u8 gp_getChatboard() {
   int x=0,mark;
   char search[256];
   char seq[] = "AT+CKPD=\"";

   if ( seq_ok==1 ) {
      seq_ok=0;
      // Search for seq in chatboard_buffer[]
      for (x=0;x<256;x++) search[x]=0;
      for (x=0;x<250;x++) {
         search[x] = chatboard_buffer[x+9];
         if (search[x]==13) break;
      }
      if ( !strncmp(seq,search,sizeof(seq)) ) return 0;

      mark = x;
      for (x=0;x<CHA_KEY_NUMBER;x++) {
         if (!strncmp (cha01KeyMap[x].map1,search,mark+1) )  return (cha01KeyMap[x].key);
         if (!strncmp (cha01KeyMap[x].map2,search,mark+1) )  return (cha01KeyMap[x].key);
      }
   }
   return 0;
}

