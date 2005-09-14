CC = arm-elf-gcc
LD = arm-elf-gcc
AS = arm-elf-as
AR = arm-elf-ar


INCLUDES = -I../../include

CFLAGS = $(INCLUDES) -O2 -s -Wall -mtune=arm9tdmi
 
OBJS =  gp_setfont.o gp_8x8font.o

all:	$(OBJS)
	$(AR) rcs gp_font8.a $(OBJS)
	cp gp_font8.a ../../lib/
	rm *.o *.a


clean:
	rm -f *.o *~ 
