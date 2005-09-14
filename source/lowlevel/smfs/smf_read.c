#include "gp32.h"

unsigned char SM_READ_DATA(unsigned int drv_no) {
   unsigned char data; 
   rPDDAT &=~ 0x100; 
   data = (rPBDAT & 0xFF); 
   rPDDAT |= 0x100; 
   return data;
}
