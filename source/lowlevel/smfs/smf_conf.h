/**********
 *
 * sm_conf.h: SmartMedia File System Header File for Configuration
 *
 * Portable File System designed for SmartMedia
 *
 * Created by Kim Hyung Gi (06/99) - kimhg@mmrnd.sec.samsung.co.kr
 * Samsung Electronics, M/M R&D Center, Visual Communication Group
 *
 * Notes:
 *	1. "SM_WAIT_TR" is very important definition.
 *		This makes great diffrence in the SYSTEM PERFORMANCE !!!
 *		It is best to check if the ready signal of the chip is ON.
 *		If there is no way to check the ready signal in your platform
 *		however, use smfsWaitUSec(10).
 *		But this function has some overhead to operate correctly in
 *		various system, therefore it makes the system slow.
 **********
 * WebVP Porting: Kim Do Wan(99/07/26)
 *		1. Change "Platform Decision"
 *
 **********
 * Modified by Lee Jin Woo (1999/08/17)
 *		1. Removed the #define's for SM_OUTB and SM_INB
 **********/

#ifndef SMF_CONF_H
#define SMF_CONF_H


#ifdef __cplusplus
extern "C" {
#endif


/**********
 * Decision of Platform
 **********/

/* COMPILER */
#define MSVC		1
#define ARMC		2	/* ARM is defined in ARM compiler environment */
#define GNUC		3	// ***************

/* TARGET */
#define IML_EPP		1	/* made by InfoMedia Lab, SEC */
#define WEBVP		2	/* made by InfoMedia Lab, SEC */
#define ISA_SM		3	/* made by Memory Product & Technology, SEC */
#define WEBVP_SM	4	/* made by InfoMedia Lab, SEC */
#define GP32_SM		5	// ***************

/* OS */
#define MS_WINDOWS	1
#define PSOS		2
#define MS_WINNT	3
#define GPOS		4	// ***************

/* memory level */
#define MEMORY_VERY_SMALL	1		/* Data Memory: 2K ~ 4K */
#define MEMORY_SMALL		2		/* Data Memory: 4K ~ 30K */
#define MEMORY_MEDIUM		3		/* Data Memory: 30K ~ 80K */
#define MEMORY_LARGE		4		/* Data Memory: 80K ~ 150K */
#define MEMORY_VERY_LARGE	5		/* Data Memory: 150K ~ */

/* Platform Decision */


// **********
#define SM_COMPILER		GNUC
//#define SM_TARGET		GP32_SM
//#define SM_OS			GPOS
#define MEMORY_LEVEL	MEMORY_MEDIUM

/*#if 0
#define SM_COMPILER		MSVC
#define SM_TARGET		IML_EPP
#define SM_OS			MS_WINDOWS
#define MEMORY_LEVEL	MEMORY_LARGE

#else
#define SM_COMPILER		ARMC
#define SM_TARGET		WEBVP_SM
#define SM_OS			PSOS
#define MEMORY_LEVEL	MEMORY_LARGE

#endif*/

/**********
 * This value must be set larger than or equal to the CPU MIPS
 *	(Million Instructions Per Second).
 *	. PentiumII 400MHz => 1158
 **********/
//#if (SM_TARGET == WEBVP_SM || SM_TARGET == WEBVP)
//#define CPU_MIPS		170
//#else
//#define CPU_MIPS		1200
//#endif

#define CPU_MIPS 100
/*---------------------------------------------------------------------------*/


/**********
 * Decision of Compile Option
 * (default setting is that nothing is defined)
 **********/

/**********
 * Compile Environment
 **********/
//#define NO_STDLIB
//#define NO_MALLOC

/**********
 * Operation Mode
 **********/
#define LONG_FILE_NAME_ENABLE		/* this needs some more code size,
									   and some more time in
									   smReadDirEx() */
/* #define CHANGE_DIR_ENABLE */
/* #define ECC_ENCODE_DISABLE */	/* if this is defined, it could not
									   have interoperability with other
									   systems */
/* #define ECC_DECODE_DISABLE */
#define FAT_UPDATE_WHEN_FILE_CLOSE	/* if not, fat is updated everytime
									   it is changed, so it makes the
									   system a little bit slow */

#if (MEMORY_LEVEL <= MEMORY_MEDIUM)
	#define WRITE_CACHE_DISABLE		/* don't change this line. cache
									   must be disabled if (MEMORY_LEVEL
									   <= MEMORY_MEDIUM) */
#else
	/* #define WRITE_CACHE_DISABLE */		/* user can control cache
											 * enable/disable only if
											 * (MEMORY_LEVEL
											 * >= MEMORY_LARGE)
											 */
#endif


//#define SM_MULTITASK		/* if two or more tasks call SMFS API's
//							 * simultaneously, this must be defined
//							 */

/*--------------------------------------------------------------------*/



/**********
 * ETC
 **********/

/* number of SmartMedia drive that is installed.
 * each is defined in "smf_conf.c" */
#define MAX_DRIVE		1

/* pre-declarator for export functions */
#define SM_EXPORT
#define SML_EXPORT
#define SMP_EXPORT
/*--------------------------------------------------------------------*/



/**********
 * Macro Definition
 **********/

#ifdef	LONG_FILE_NAME_ENABLE
extern int sm_ConvertStrToUnicode(unsigned short *des, unsigned char *src, int len);

/****
* This MACRO must return 2byte-length of converted unicode  as an 'int' type
* Parameters
* 	- p_uni : unicode string pointer (unsigned short*)
* 	- p_mb : multibyte string pointer (unsignd char*)
* 	- mb_len : multibyte string length in bytes.
*				If 0, p_mb is considered to be a NULL terminated string (int)
* Return Value
*	- 2byte-length of converted unicode. It includes NULL character at the end.
*		(eg. If it returns 3, it successfully converted 3 unicode character(6 bytes).
*			The last 2 bytes are all 0)
* Notes
*	- 'p_uni' must be allocated for the (MAX_FILE_NAME_LEN + 2) bytes.
*	- This function must be able to stop the conversion at the maximum length,
*		and add NULL unicode character at the end.
*****/
#define	SM_MBS_TO_UNICODE(p_uni, p_mb, mb_len)	sm_ConvertStrToUnicode(p_uni, p_mb, mb_len)
#endif


/**********
 * Compiler Dependent Definition
 **********/
	extern int rand(void);
	extern void srand(unsigned int seed);
	#define SM_RANDOM()				rand()
	#define SM_SRAND(seed)			srand((seed))

#ifdef NO_STDLIB
	#define SM_MEMCMP		sm_memcmp
	#define SM_MEMCPY		sm_memcpy
	#define SM_MEMSET		sm_memset
	#define SM_STRCMP		sm_strcmp
	#define SM_STRCPY		sm_strcpy
	#define SM_STRLEN		sm_strlen
#else

	#define SM_MEMCMP		memcmp
	#define SM_MEMCPY		memcpy
	#define SM_MEMSET		memset
	#define SM_STRCMP		strcmp
	#define SM_STRCPY		strcpy
	#define SM_STRLEN		strlen
#endif

/**********
 * OS Dependent Definition
 **********/
#ifdef SM_MULTITASK
#if (SM_OS == MS_WINDOWS)
	#define SM_LOCK_INIT(x)
	#define SM_LOCK(x)
	#define SM_UNLOCK(x)
#elif (SM_OS == PSOS)
	extern int sm_LockInit(void);
	extern int sm_Lock(void);
	extern int sm_Unlock(void);

	#define SM_LOCK_INIT(x)		sm_LockInit()
	#define SM_LOCK(x)			sm_Lock()
	#define SM_UNLOCK(x)		sm_Unlock()
#else
	#define SM_LOCK_INIT(x)
	#define SM_LOCK(x)
	#define SM_UNLOCK(x)
#endif
#endif


/**********
 * Target Dependent Definition
 **********/
#if (SM_OS == MS_WINNT)
#include <windows.h>
#endif

#if (SM_TARGET == IML_EPP)
#if (SM_OS == MS_WINDOWS)
	extern volatile unsigned char* g_smStatAddr[MAX_DRIVE];
	extern volatile unsigned char* g_smCtrlAddr[MAX_DRIVE];
	extern volatile unsigned char* g_smAddrAddr[MAX_DRIVE];
	extern volatile unsigned char* g_smDataAddr[MAX_DRIVE];

	extern int _outp(unsigned short port, int databyte);
	extern int _inp(unsigned short port);

	#define SM_ALE_EN(drv_no) 		_outp((unsigned short)g_smAddrAddr[(drv_no)], 0x0a)
	#define SM_ALE_DIS(drv_no)		_outp((unsigned short)g_smAddrAddr[(drv_no)], 0x08)
	#define SM_CLE_EN(drv_no) 		_outp((unsigned short)g_smAddrAddr[(drv_no)], 0x09)
	#define SM_CLE_DIS(drv_no)		_outp((unsigned short)g_smAddrAddr[(drv_no)], 0x08)
	#define SM_CHIP_EN(drv_no)		_outp((unsigned short)g_smAddrAddr[(drv_no)], 0x08)
	#define SM_CHIP_DIS(drv_no)		_outp((unsigned short)g_smAddrAddr[(drv_no)], 0x0c)

	#define SM_WRITE_EN(drv_no)		_outp((unsigned short)g_smCtrlAddr[(drv_no)], 4)
	#define SM_WRITE_DIS(drv_no)	_outp((unsigned short)g_smCtrlAddr[(drv_no)], 0)
	#define SM_READ_EN(drv_no) 		_outp((unsigned short)g_smCtrlAddr[(drv_no)], 0xf4)
	#define SM_READ_DIS(drv_no)		_outp((unsigned short)g_smCtrlAddr[(drv_no)], 0)

	#define SM_WRITE_CMD(drv_no, cmd) 	_outp((unsigned short)g_smDataAddr[(drv_no)], cmd)
	#define SM_WRITE_ADDR(drv_no, addr)	_outp((unsigned short)g_smDataAddr[(drv_no)], addr)
	#define SM_WRITE_DATA(drv_no, data)	_outp((unsigned short)g_smDataAddr[(drv_no)], data)
	#define SM_READ_DATA(drv_no) 		_inp((unsigned short)g_smDataAddr[(drv_no)])

	#define SM_WAIT_TR(drv_no) \
			do { \
				_outp((unsigned short)g_smCtrlAddr[(drv_no)], 0xf4); \
				while (!((_inp((unsigned short)g_smStatAddr[(drv_no)]) \
							& 0x80))) {} \
				_outp((unsigned short)g_smCtrlAddr[(drv_no)], 0); \
			} while (0)
#elif (SM_OS == MS_WINNT)
	extern unsigned char sm_readStatPort(void);
	extern void sm_writeStatPort(unsigned char data);
	extern unsigned char sm_readCtrlPort(void);
	extern void sm_writeCtrlPort(unsigned char data);
	extern unsigned char sm_readAddrPort(void);
	extern void sm_writeAddrPort(unsigned char data);
	extern unsigned char sm_readDataPort(void);
	extern void sm_writeDataPort(unsigned char data);

	#define SM_ALE_EN(drv_no)				sm_writeAddrPort(0x0a)
	#define SM_ALE_DIS(drv_no)				sm_writeAddrPort(0x08)
	#define SM_CLE_EN(drv_no)				sm_writeAddrPort(0x09)
	#define SM_CLE_DIS(drv_no)				sm_writeAddrPort(0x08)
	#define SM_CHIP_EN(drv_no)				sm_writeAddrPort(0x08)
	#define SM_CHIP_DIS(drv_no)				sm_writeAddrPort(0x0c)

	#define SM_WRITE_EN(drv_no)				sm_writeCtrlPort(4)
	#define SM_WRITE_DIS(drv_no)			sm_writeCtrlPort(0)
	#define SM_READ_EN(drv_no)				sm_writeCtrlPort(0xf4)
	#define SM_READ_DIS(drv_no)				sm_writeCtrlPort(0)

	#define SM_WRITE_CMD(drv_no, cmd) 		sm_writeDataPort((unsigned char)(cmd))
	#define SM_WRITE_ADDR(drv_no, addr)		sm_writeDataPort((unsigned char)(addr))
	#define SM_WRITE_DATA(drv_no, data)		sm_writeDataPort((unsigned char)(data))
	#define SM_READ_DATA(drv_no)			sm_readDataPort()

	#define SM_WAIT_TR(drv_no) \
				do { \
					sm_writeCtrlPort(0xf4); \
					while (!((sm_readStatPort() & 0x80))) {} \
					sm_writeCtrlPort(0); \
				} while (0)
#endif	/* SM_OS == MS_WINNT */

#elif (SM_TARGET == WEBVP)
	extern volatile unsigned long* g_smDataAddr[MAX_DRIVE];

	#define SM_ALE_EN(drv_no)				io_SetCPort2(0x4, 0x4)
	#define SM_ALE_DIS(drv_no)				io_SetCPort2(0x4, 0)
	#define SM_CLE_EN(drv_no)				io_SetCPort2(0x2, 0x2)
	#define SM_CLE_DIS(drv_no)				io_SetCPort2(0x2, 0)

	#define SM_CHIP_EN(drv_no)				/* No Codes */

	#define SM_CHIP_DIS(drv_no)				/* No Codes */

	#define SM_WRITE_EN(drv_no)				/* No Codes */
	#define SM_WRITE_DIS(drv_no)			/* No Codes */
	#define SM_READ_EN(drv_no)				/* No Codes */
	#define SM_READ_DIS(drv_no)				/* No Codes */

	#define SM_WRITE_CMD(drv_no, cmd) 		SetD_FastIO(g_smDataAddr[drv_no], cmd)
	#define SM_WRITE_ADDR(drv_no, addr) 	SetD_FastIO(g_smDataAddr[drv_no], addr)
	#define SM_WRITE_DATA(drv_no, data) 	SetD_FastIO(g_smDataAddr[drv_no], data)
	#define SM_READ_DATA(drv_no) 			((unsigned char)GetD_FastIO(g_smDataAddr[drv_no]))

	#define SM_WAIT_TR(drv_no)				smfsWaitUSec(10);
#elif (SM_TARGET == WEBVP_SM)
	extern volatile unsigned long* g_smDataAddr[MAX_DRIVE];

	#define SM_ALE_EN(drv_no)				io_SetSmMas(0x2, 0x2)
	#define SM_ALE_DIS(drv_no)				io_SetSmMas(0x2, 0)
	#define SM_CLE_EN(drv_no)				io_SetSmMas(0x1, 0x1)
	#define SM_CLE_DIS(drv_no)				io_SetSmMas(0x1, 0)

	#define SM_CHIP_EN(drv_no)				/* No Codes */

	#define SM_CHIP_DIS(drv_no)				/* No Codes */

	#define SM_WRITE_EN(drv_no)				/* No Codes */
	#define SM_WRITE_DIS(drv_no)			/* No Codes */
	#define SM_READ_EN(drv_no)				/* No Codes */
	#define SM_READ_DIS(drv_no)				/* No Codes */

	#define SM_WRITE_CMD(drv_no, cmd) 		SetD_FastIO(g_smDataAddr[drv_no], cmd)
	#define SM_WRITE_ADDR(drv_no, addr)		SetD_FastIO(g_smDataAddr[drv_no], addr)
	#define SM_WRITE_DATA(drv_no, data)		SetD_FastIO(g_smDataAddr[drv_no], data)
	#define SM_READ_DATA(drv_no) 			((unsigned char)GetD_FastIO(g_smDataAddr[drv_no]))

	#define SM_WAIT_TR(drv_no) \
				do { \
					smfsWaitUSec(1); \
					while (!(io_GetSmMas() & 1)) {} \
				} while (0)
#elif (SM_TARGET == ISA_SM)
	extern volatile unsigned char* g_smCtrlAddr[MAX_DRIVE];
	extern volatile unsigned char* g_smDataAddr[MAX_DRIVE];
	extern unsigned char g_ctl_val;

	extern int _outp(unsigned short port, int databyte);
	extern int _inp(unsigned short port);

	#define SM_ALE_EN(drv_no) \
				do { \
					g_ctl_val |= 0x01; \
					_outp((unsigned short)g_smCtrlAddr[(drv_no)], \
							g_ctl_val); \
				} while (0)
	#define SM_ALE_DIS(drv_no) \
				do { \
					g_ctl_val &= 0x0e; \
					_outp((unsigned short)g_smCtrlAddr[(drv_no)], \
							g_ctl_val); \
				} while (0)
	#define SM_CLE_EN(drv_no) \
				do { \
					g_ctl_val |= 0x02; \
					_outp((unsigned short)g_smCtrlAddr[(drv_no)], \
							g_ctl_val); \
				} while (0)
	#define SM_CLE_DIS(drv_no) \
				do { \
					g_ctl_val &= 0x0d; \
					_outp((unsigned short)g_smCtrlAddr[(drv_no)], \
							g_ctl_val); \
				} while (0)
	#define SM_CHIP_EN(drv_no) \
				do { \
					g_ctl_val = 0x04; \
					_outp((unsigned short)g_smCtrlAddr[(drv_no)], \
							g_ctl_val); \
					SM_CLE_EN((drv_no)); \
					_outp((unsigned short)g_smDataAddr[(drv_no)], \
							0xff); \
					SM_CLE_DIS((drv_no)); \
				} while (0)
	#define SM_CHIP_DIS(drv_no) \
				do { \
					g_ctl_val = 0xfc; \
					_outp((unsigned short)g_smCtrlAddr[(drv_no)], \
							g_ctl_val); \
				} while (0);

	#define SM_WRITE_EN(drv_no)
	#define SM_WRITE_DIS(drv_no)
	#define SM_READ_EN(drv_no)
	#define SM_READ_DIS(drv_no)

	#define SM_WRITE_CMD(drv_no, cmd) 		_outp((unsigned short)g_smDataAddr[(drv_no)], (cmd))
	#define SM_WRITE_ADDR(drv_no, addr) 	_outp((unsigned short)g_smDataAddr[(drv_no)], (addr))
	#define SM_WRITE_DATA(drv_no, data) 	_outp((unsigned short)g_smDataAddr[(drv_no)], (data))
	#define SM_READ_DATA(drv_no) 			_inp((unsigned short)g_smDataAddr[(drv_no)])

	#define SM_WAIT_TR(drv_no)				smfsWaitUSec(10)
#else


/*	#define SM_ALE_EN(drv_no)
	#define SM_ALE_DIS(drv_no)
	#define SM_CLE_EN(drv_no)
	#define SM_CLE_DIS(drv_no)
	#define SM_CHIP_EN(drv_no)
	#define SM_CHIP_DIS(drv_no)

	#define SM_WRITE_EN(drv_no)
	#define SM_WRITE_DIS(drv_no)
	#define SM_READ_EN(drv_no)
	#define SM_READ_DIS(drv_no)

	#define SM_WRITE_CMD(drv_no, cmd)
	#define SM_WRITE_ADDR(drv_no, addr)
	#define SM_WRITE_DATA(drv_no, data)
	#define SM_READ_DATA(drv_no)

	#define SM_WAIT_TR(drv_no)
*/

	// ***************

	#include "gp32_registers.h"

	// add SM_WP_DIS() to smfsUserInit();
	#define SM_WP_EN()						(rPDDAT &=~ 0x40)	// PD6
	#define SM_WP_DIS()						(rPDDAT |= 0x40)	// PD6

	#define SM_ALE_EN(drv_no)				(rPEDAT |= 0x10)	// PE4
	#define SM_ALE_DIS(drv_no)				(rPEDAT &=~ 0x10)	// PE4

	#define SM_CLE_EN(drv_no)				(rPEDAT |= 0x20)	// PE5
	#define SM_CLE_DIS(drv_no)				(rPEDAT &=~ 0x20)	// PE5

	#define SM_CHIP_EN(drv_no)				(rPDDAT &=~ 0x80)	// PD7
	#define SM_CHIP_DIS(drv_no)				(rPDDAT |= 0x80)	// PD7

	#define SM_READ_EN(drv_no)				(rPBCON &=~ 0xFFFF)
	#define SM_READ_DIS(drv_no)				(rPBCON |= 0x5555)

	#define SM_WRITE_EN(drv_no)				SM_READ_DIS(drv_no)
	#define SM_WRITE_DIS(drv_no)			SM_READ_EN(drv_no)

	#define SM_WRITE_DATA(drv_no, data)		{ rPBDAT = (rPBDAT & 0xff00) | data; rPEDAT &=~ 0x08; rPEDAT |= 0x08; }

	#define SM_READ_DATA(drv_no)			({unsigned char data; rPDDAT &=~ 0x100; data = (rPBDAT & 0xFF); rPDDAT |= 0x100; data; })


	#define SM_WRITE_CMD(drv_no, cmd)		SM_WRITE_DATA(drv_no, cmd)
	#define SM_WRITE_ADDR(drv_no, addr)		SM_WRITE_DATA(drv_no, addr)

	#define SM_WAIT_TR(drv_no)				while (!(rPDDAT & 0x200))
#endif



/**********
 **********/
#ifndef NO_MALLOC
	#define SM_MALLOC(size)			malloc((size))
	#define SM_FREE(addr)			free((addr))
#endif


/**********
 * Global Variables  & Functions defined in smf_conf.c
 **********/

/**********
 * Platform Dependent Functions
 **********/
extern int smfsUserInit(void);
extern void smfsWaitUSec(unsigned long usec);

/*
 * p_time is an array of 8 bytes
 * from lowest address =>
 * 		msec/4(1), sec(1), min(1), hour(1), day(1), month(1), year(2)
 */
extern unsigned long smfsGetTime(unsigned char p_time[]);


/**********
 * Standard Library Functions used
 **********/

/* (if no standard library provided, these functions must be defined) */
#ifdef NO_STDLIB
	extern int sm_memcmp(const void* buf1, const void* buf2, unsigned int count);
	extern void* sm_memcpy(void* dst, const void* src, unsigned int count);
	extern void* sm_memset(void* dst, int val, unsigned int count);
	extern int sm_strcmp(const char* src, const char* dst);
	extern char* sm_strcpy(char* dst, const char* src);
	extern unsigned int sm_strlen(const char* str);
#else	/* NO_STDLIB */
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
#endif


#ifdef NO_MALLOC
	extern void* malloc(int size);
	extern void free(void* memblock);
#else	/* NO_MALLOC */
	#include <stdlib.h>
#endif


#if (SM_OS == MS_WINNT)
	extern HANDLE g_ioDev;
#endif


#ifdef __cplusplus
}
#endif


#endif
