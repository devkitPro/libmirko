/**********
 *
 * smf_conf.c: SmartMedia File System Configuation Part
 *
 * Portable File System designed for SmartMedia
 *
 * Created by Kim Hyung Gi (06/99) - kimhg@mmrnd.sec.samsung.co.kr
 * Samsung Electronics, M/M R&D Center, Visual Communication Group
 *
 **********
 *
 * Modified by Lee Jin Woo (1999/08/17)
 *	- Added smfsUserInit() (1999/08/18)
 *
 **********/

#include "smf_conf.h"
#include "smf_cmn.h"

#if (SM_COMPILER == MSVC)
#include <time.h>
#endif

#if (SM_OS == MS_WINNT)
#include <stddef.h>
#include <winioctl.h>
#include "../gpioctl.h"
#elif (SM_OS == PSOS)
#include "psos.h"
#endif

/**********
 * Global Variables
 **********/
#if (SM_TARGET == IML_EPP)
#if (SM_OS == MS_WINDOWS)
#define SM_BASE		0x378
volatile unsigned char* g_smStatAddr[MAX_DRIVE] = { (unsigned char*)SM_BASE + 1 };
volatile unsigned char* g_smCtrlAddr[MAX_DRIVE] = { (unsigned char*)SM_BASE + 2 };
volatile unsigned char* g_smAddrAddr[MAX_DRIVE] = { (unsigned char*)SM_BASE + 3 };
volatile unsigned char* g_smDataAddr[MAX_DRIVE] = { (unsigned char*)SM_BASE + 4 };
#endif

#elif (SM_TARGET == WEBVP)
volatile unsigned long* g_smDataAddr[MAX_DRIVE] = { (volatile unsigned long*)0x23800000 };

#elif (SM_TARGET == WEBVP_SM)
volatile unsigned long* g_smDataAddr[MAX_DRIVE] = { (volatile unsigned long*)0x23400014 };

#elif (SM_TARGET == ISA_SM)
unsigned char g_ctl_val;
volatile unsigned char* g_smCtrlAddr[MAX_DRIVE] = { (unsigned char*)0xf301 };
volatile unsigned char* g_smDataAddr[MAX_DRIVE] = { (unsigned char*)0xf300 };

#endif

#if (SM_OS == MS_WINNT)
HANDLE g_ioDev = INVALID_HANDLE_VALUE;
#endif

/**********
 * Platform Dependent Functions
 **********/

/**********
 * Function: smfsUserInit
 * Remarks:
 *	- This function is for performing user initialization.
 *	  This function is called during smInit().
 * Return Value:
 *	- 0 if successful, non-zero otherwise.
 * Parameters:
 *	- None
 **********/
int smfsUserInit()
{
	// **********
	SM_WP_DIS();

	/* perform user initialization */
#if (SM_OS == MS_WINNT)
	if (g_ioDev == INVALID_HANDLE_VALUE)
	{
		g_ioDev = CreateFile("\\\\.\\GpdDev",
				GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, 0, NULL);
	}
	if (g_ioDev == INVALID_HANDLE_VALUE)
		return -1;

	return 0;

#else
	return 0;

#endif
}

/**********
 * Function: smfsGetTime
 * Remarks:
 *	- time function that must be defined according to the present
 *		platform
 * Return Value:
 *	- 1 if success, 0 if fail
 * Parameters
 *	. p_time (result): 8 byte time information
 *		from the lowest address: msec/4(1), sec(1), min(1), hour(1),
 *									day(1), month(1), year(2)
 **********/
unsigned long smfsGetTime(unsigned char p_time[])
{
#if (SM_COMPILER == MSVC)
	struct tm* newtime;
	time_t long_time;

	time(&long_time);                /* Get time as long integer. */
	newtime = localtime(&long_time); /* Convert to local time. */

	p_time[0] = 0;
	p_time[1] = (unsigned char)newtime->tm_sec;
	p_time[2] = (unsigned char)newtime->tm_min;
	p_time[3] = (unsigned char)newtime->tm_hour;
	p_time[4] = (unsigned char)newtime->tm_mday;
	p_time[5] = (unsigned char)(newtime->tm_mon + 1);
	p_time[6] = (unsigned char)((newtime->tm_year + 1900) & 0xff);
	p_time[7] = (unsigned char)(((newtime->tm_year + 1900) >> 8) & 0xff);

/* kdw add */
#elif (SM_COMPILER == ARMC)
	unsigned long Date, Time, Tick;

	tm_get(&Date, &Time, &Tick);
	p_time[0] = 0;
	p_time[1] = (unsigned char)(Time & 0x000000ff);
	p_time[2] = (unsigned char)((Time & 0x0000ff00) >> 8 );
	p_time[3] = (unsigned char)((Time & 0xffff0000) >> 16);
	p_time[4] = (unsigned char)((Date & 0x000000ff));
	p_time[5] = (unsigned char)((Date & 0x0000ff00) >> 8 );
	p_time[6] = (unsigned char)((Date & 0x00ff0000) >> 16);
	p_time[7] = (unsigned char)((Date & 0xff000000) >> 24);

#endif
	return 1;
}


void smfsWaitUSec(unsigned long usec)
{
#if (SM_COMPILER == MSVC)
	udword i, j;
	udword count;

	count = CPU_MIPS / 2;

	for (i = 0; i < usec; ++i)
	{
		for (j = 0; j < count; ++j)
		{
			/* do nothing */
		}
	}
/*
	clock_t start_clocks, new_clocks, wait_clocks;

	wait_clocks = usec * CLOCKS_PER_SEC / 1000;

	start_clocks = new_clocks = clock();

	if (start_clocks <= start_clocks + wait_clocks) {
		* additional wait until the (start_clocks + wait_clocks) is lager than start_clocks *
		while (start_clocks > start_clocks + wait_clocks) {
			start_clocks = clock();
		}

		new_clocks = clock();
	}

	while ((new_clocks >= start_clocks) &&
			(new_clocks <= start_clocks + wait_clocks)) {
		new_clocks = clock();
	}
*/
#elif (SM_COMPILER == ARMC)
	udword i, j;
	udword temp;
	for (i = 0; i < usec; ++i)
	{
		for (j = 0;  j < 30; ++j)
		{
			temp = 100 % 28;
		}
	}
#else
	udword i, j;
	udword count;

	count = CPU_MIPS / 2;		/* the internal loop is composed of
								   at least 2 instructions */

	for (i = 0;  i < usec; ++i)
	{
		for (j = 0; j < count; ++j)
		{
			/* do nothing */
		}
	}
#endif
}

#if (SM_OS == MS_WINNT)
unsigned char sm_readStatPort()
{
	ULONG port = 1;
	unsigned char data;
	ULONG len;

	DeviceIoControl(g_ioDev, IOCTL_GPD_READ_PORT_UCHAR, &port,
			sizeof(port), &data, sizeof(data), &len, NULL);
	return data;
}


void sm_writeStatPort(unsigned char data)
{
	GENPORT_WRITE_INPUT input;
	ULONG len;

	input.PortNumber = 1;
	input.CharData = data;
	DeviceIoControl(g_ioDev, IOCTL_GPD_WRITE_PORT_UCHAR, &input,
			offsetof(GENPORT_WRITE_INPUT, CharData) +
			sizeof(input.CharData), NULL, 0, &len, NULL);
}

unsigned char sm_readCtrlPort()
{
	ULONG port = 2;
	unsigned char data;
	ULONG len;

	DeviceIoControl(g_ioDev, IOCTL_GPD_READ_PORT_UCHAR, &port,
			sizeof(port), &data, sizeof(data), &len, NULL);
	return data;
}

void sm_writeCtrlPort(unsigned char data)
{
	GENPORT_WRITE_INPUT input;
	ULONG len;

	input.PortNumber = 2;
	input.CharData = data;
	DeviceIoControl(g_ioDev, IOCTL_GPD_WRITE_PORT_UCHAR, &input,
			offsetof(GENPORT_WRITE_INPUT, CharData) +
			sizeof(input.CharData), NULL, 0, &len, NULL);
}

unsigned char sm_readAddrPort()
{
	ULONG port = 3;
	unsigned char data;
	ULONG len;

	DeviceIoControl(g_ioDev, IOCTL_GPD_READ_PORT_UCHAR, &port,
			sizeof(port), &data, sizeof(data), &len, NULL);
	return data;
}

void sm_writeAddrPort(unsigned char data)
{
	GENPORT_WRITE_INPUT input;
	ULONG len;

	input.PortNumber = 3;
	input.CharData = data;
	DeviceIoControl(g_ioDev, IOCTL_GPD_WRITE_PORT_UCHAR, &input,
			offsetof(GENPORT_WRITE_INPUT, CharData) +
			sizeof(input.CharData), NULL, 0, &len, NULL);
}

unsigned char sm_readDataPort()
{
	ULONG port = 4;
	unsigned char data;
	ULONG len;

	DeviceIoControl(g_ioDev, IOCTL_GPD_READ_PORT_UCHAR, &port,
			sizeof(port), &data, sizeof(data), &len, NULL);
	return data;
}

void sm_writeDataPort(unsigned char data)
{
	GENPORT_WRITE_INPUT input;
	ULONG len;

	input.PortNumber = 4;
	input.CharData = data;
	DeviceIoControl(g_ioDev, IOCTL_GPD_WRITE_PORT_UCHAR, &input,
			offsetof(GENPORT_WRITE_INPUT, CharData) +
			sizeof(input.CharData), NULL, 0, &len, NULL);
}
#endif

/**********
 * Standard Library Functions used
 **********/
//#if 0 // NO_STDLIB
int sm_memcmp(const void* buf1, const void* buf2, unsigned int count)
{
	if (!count)
		return 0;

	while (--count && *(char*)buf1 == *(char*)buf2)
	{
		buf1 = (char*)buf1 + 1;
		buf2 = (char*)buf2 + 1;
	}

	return (*((unsigned char*)buf1) - *((unsigned char*)buf2));
}


void* sm_memcpy(void* dst, const void* src, unsigned int count)
{
	void* ret = dst;

	while (count--)
	{
		*(char*)dst = *(char*)src;
		dst = (char*)dst + 1;
		src = (char*)src + 1;
	}

	return ret;
}


void* sm_memset(void* dst, int val, unsigned int count)
{
	void* start = dst;

	while (count--)
	{
		*(char*)dst = (char)val;
		dst = (char*)dst + 1;
	}

	return start;
}


int sm_strcmp(const char* src, const char* dst)
{
	int ret = 0;

	while ((ret = *(unsigned char*)src - *(unsigned char*)dst) == 0 && *dst)
	{
		++src;
		++dst;
	}

	if (ret < 0)
		ret = -1;
	else if (ret > 0)
		ret = 1;

	return ret;
}


char* sm_strcpy(char* dst, const char*src)
{
	char* cp = dst;

	while ((*cp++ = *src++) != '\0')
	{
		/* do nothing */
	}

	return dst;
}


unsigned int sm_strlen(const char* str)
{
	const char* eos = str;

	while (*eos++)
	{
		/* do nothing */
	}

	return (unsigned int)(eos - str - 1);
}
//#endif

/**********
 * NO_MALLOC
 **********/
/*#ifdef NO_MALLOC
void* malloc(int size)
{
}

void* realloc(void* memblock, int size)
{
}

void free(void* memblock)
{
}
#endif
*/

#ifdef SM_MULTITASK
#if (SM_OS == PSOS)
static unsigned long s_smid;

int sm_LockInit(void)
{
	if (sm_ident("wrap", 0, &s_smid) == 0)
	{
		return 0;
	}
	return (sm_create("wrap", 1, (ULONG)SM_PRIOR, &s_smid) == 0) ? 0 : -1;
}

int sm_Lock(void)
{
	return (sm_p(s_smid, (ULONG)SM_WAIT, 0) == 0) ? 0 : -1;
}

int sm_Unlock(void)
{
	return (sm_v(s_smid) == 0) ? 0 : -1;
}

#endif	/* (SM_OS == PSOS) */
#endif	/* SM_MULTITASK */
