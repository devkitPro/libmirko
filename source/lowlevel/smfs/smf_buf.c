#include <stddef.h>


#include "smf_conf.h"

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS)
#ifdef _DEBUG
#include <stdio.h>
#include <windows.h>
#endif
#endif

#include "smf_cmn.h"
#include "smf_buf.h"


#define BUF_POOL_COUNT		3


typedef struct tagBufNode {
	struct tagBufNode* next;
	ubyte data[SECTOR_SIZE + 2];	/* sm_FATUpdate() needs two more bytes */
} BufNode;


static BufNode s_buf[BUF_POOL_COUNT];
static BufNode* s_freePool;


void smbBufPoolInit() 
{
	int i;

	for (i = 0; i < BUF_POOL_COUNT - 1; ++i) 
	{
		s_buf[i].next = s_buf + i + 1;
	}
	s_buf[i].next = NULL;

	s_freePool = s_buf;
}

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS)
#ifdef _DEBUG
static int s_allocCount;
#endif
#endif

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS) && defined(_DEBUG)
ubyte* smbAllocBuf(const ubyte* file, int line) 
{
#else
ubyte* smbAllocBuf() 
{
#endif
	ubyte* buf = NULL;

	if (s_freePool) 
	{
		buf = s_freePool->data;
		s_freePool = s_freePool->next;

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS)
#ifdef _DEBUG
		++s_allocCount;
		{
			ubyte str[100];
			sprintf(str, "[BUF ALLOC] alloc count: %d\n", s_allocCount);
			OutputDebugString(str);
			sprintf(str, "            file: %s, line: %d\n", file, line);
			OutputDebugString(str);

			if (s_allocCount > 3) 
			{
				DebugBreak();
			}
		}
#endif
#endif
	}

	return buf;
}

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS) && defined(_DEBUG)
void smbFreeBuf(ubyte* buf, const ubyte* file, int line) 
{
#else
void smbFreeBuf(ubyte* buf) 
{
#endif
	if (buf) 
	{
		BufNode* node = (BufNode*)(buf - offsetof(BufNode, data));
		node->next = s_freePool;
		s_freePool = node;

#if (SM_OS == MS_WINNT || SM_OS == MS_WINDOWS)
#ifdef _DEBUG
		--s_allocCount;
		{
			ubyte str[100];
			sprintf(str, "[BUF FREE ] alloc count: %d\n", s_allocCount);
			OutputDebugString(str);
			sprintf(str, "            file: %s, line: %d\n", file, line);
			OutputDebugString(str);
		}
#endif
#endif
	}
}

/*
static udword dbgMaxEspReg;
static udword dbgMinEspReg = (udword)(-1);
static char dbgStr[30];
__declspec(naked) void __cdecl _penter() {
	udword curEsp;

	__asm {
		push	ebp
		mov		ebp, esp
		sub		esp, __LOCAL_SIZE
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
	}

	__asm {
		mov		dword ptr [curEsp], ebp
	}

	if (curEsp > dbgMaxEspReg) {
		dbgMaxEspReg = curEsp;
		sprintf(dbgStr, "max esp: 0x%x\n", dbgMaxEspReg);
		OutputDebugString(dbgStr);
	}

	if (curEsp < dbgMinEspReg) {
		dbgMinEspReg = curEsp;
		sprintf(dbgStr, "min esp: 0x%x\n", dbgMinEspReg);
		OutputDebugString(dbgStr);
	}

	__asm {
		pop		edi
		pop		esi
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
		mov		esp, ebp
		pop		ebp
		ret
	}
}
*/
