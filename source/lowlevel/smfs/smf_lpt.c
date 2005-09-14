/**********
 *
 * smf_lpt.c: SmartMedia File System Logical/Physical Transition part
 *
 * Portable File System designed for SmartMedia
 *
 * Created by Kim Hyung Gi (06/99) - kimhg@mmrnd.sec.samsung.co.kr
 * Samsung Electronics, M/M R&D Center, Visual Communication Group
 *
 * Notes:
 *	- This layer uses the IO layer APIs, and provides for APIs that is used by FAT layer.
 *	- In general, input parameters of the APIs are not checked for its validities (it must be checked at FAT layer)
 *
 **********
 * Modified by Lee Jin Woo (1999/08/17)
 *
 **********/

#include "smf_cmn.h"
#include "smf_lpt.h"
#include "smf_io.h"
#include "smf_fat.h"
#include "smf_buf.h"


/**********
 * Macro Definitions
 **********/
#define LPT_RETURN(drv_no, ret)				do { sm_PostLPT((drv_no)); return (ret); } while (0)

#define SET_PB_TBL(drv_no, offset, value)	do { s_pBlock[(drv_no)][(offset)] = (value); } while (0)
#define GET_PB_TBL(drv_no, offset)			s_pBlock[(drv_no)][(offset)]

#define SET_LB_TBL(drv_no, offset, value)	do { s_lBlock[(drv_no)][(offset)] = (value); } while (0)
#define GET_LB_TBL(drv_no, offset)			s_lBlock[(drv_no)][(offset)]


/**********
 * Static Variables
 **********/

static const ubyte s_cis[110] = {	/* 110 Byte CIS data */
	0x01, 0x03, 0xd9, 0x01, 0xff, 0x18, 0x02, 0xdf,
	0x01, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x21,
	0x02, 0x04, 0x01, 0x22, 0x02, 0x01, 0x01, 0x22,
	0x03, 0x02, 0x04, 0x07, 0x1a, 0x05, 0x01, 0x03,
	0x00, 0x02, 0x0f, 0x1b, 0x08, 0xc0, 0xc0, 0xa1,
	0x01, 0x55, 0x08, 0x00, 0x20, 0x1b, 0x0a, 0xc1,
	0x41, 0x99, 0x01, 0x55, 0x64, 0xf0, 0xff, 0xff,
	0x20, 0x1b, 0x0c, 0x82, 0x41, 0x18, 0xea, 0x61,
	0xf0, 0x01, 0x07, 0xf6, 0x03, 0x01, 0xee, 0x1b,
	0x0c, 0x83, 0x41, 0x18, 0xea, 0x61, 0x70, 0x01,
	0x07, 0x76, 0x03, 0x01, 0xee, 0x15, 0x14, 0x05,
	0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x30, 0x2e,
	0x30, 0x00, 0xff, 0x14, 0x00, 0xff
};

static const sDEV_INFO s_devInfo[8] = {
/*    CpV, HpC, SpH,   allS, szS, PBpV, LBpV, SpB, PpB, szP */
	{ 125,   4,   4,   2000, 512,  256,  250,   8,  16, 256},	/* DI_1M */
	{ 125,   4,   8,   4000, 512,  512,  500,   8,  16, 256},	/* DI_2M */
	{ 250,   4,   8,   8000, 512,  512,  500,  16,  16, 512},	/* DI_4M */
	{ 250,   4,  16,  16000, 512, 1024, 1000,  16,  16, 512},	/* DI_8M */
	{ 500,   4,  16,  32000, 512, 1024, 1000,  32,  32, 512},	/* DI_16M */
	{ 500,   8,  16,  64000, 512, 2048, 2000,  32,  32, 512},	/* DI_32M */
	{ 500,   8,  32, 128000, 512, 4096, 4000,  32,  32, 512},	/* DI_64M */
	{ 500,  16,  32, 256000, 512, 8192, 8000,  32,  32, 512}	/* DI_128M */
};

static sDEV_ID s_devID[MAX_DRIVE];

/**********
 * s_pBlock[MAX_DRIVE]: contains BAA in Spare area (index indicates Physical Block Number)
 *	- size: 2 * zone_num * pblock_num per zone (512B <= 1M SM, 16KB <= 128M SM)
 *	- value:
 *		. 0xffff: unused block
 *		. 0xffee: invalid block (checked by Block Status Area in Spare area)
 *		. 0     : CIS block
 *		. else  : Logical Block Number(with parity bit) in the zone => BAA value in Spare area
 **********/
static uword* s_pBlock[MAX_DRIVE] = { NULL, };


/**********
 * s_lBlock[MAX_DRIVE]: contains Physical Block Number (index indicates Logical Block Number)
 *	- size: 2 * zone_num * pblock_num per zone (used only 2 * zone_num * lblock_num)
 *	- value: minimum available value is 2(start of MBR)
 *		. 0      : unused block
 *		. else   : Physical Block Number in the zone
 **********/
static uword* s_lBlock[MAX_DRIVE] = { NULL, };

static udword s_lptFlag[MAX_DRIVE];

static ERR_CODE s_smlErr;

/**********
 * For wear leveling, this variable is set to random value in LPTInit(),
 *  and from then on, it is used for start index of search for the free LBlock.
 * After searched the free LBLock, this variable is set to that block number
 **********/
static udword s_lBlockSearchStart[MAX_DRIVE];

/**********
 * For wear leveling, this variable is set to random value when a zone is
 *	changed in sm_SearchFreePBlock(), and from then on, it is used for start
 *	index of search for the free PBlock.
 * After searched the free PBLock, this variable is set to that block number.
 **********/
static udword s_pBlockSearchStart[MAX_DRIVE];

/********************************************************************/

/* static funtions */
static ERR_CODE sm_LPTInit(udword drv_no);
static ERR_CODE sm_PreLPT(udword drv_no);
static ERR_CODE sm_PostLPT(udword drv_no);
/* static ERR_CODE sm_SearchFreeLBlock(udword drv_no, udword* pLba); */
static ERR_CODE sm_SearchFreePBlock(udword drv_no, udword lba, udword* pPba);
static ERR_CODE sm_ECCEncode(const ubyte* p_buf, ubyte* p_ecc);
static ERR_CODE sm_ECCDecode(const ubyte* p_buf, const ubyte* p_ecc);
static ERR_CODE sm_ReadBlock(udword drv_no, udword lblock_no, udword offset, void* p_buf, udword read_size);
static ERR_CODE sm_ReadPhysBlock(udword drv_no, udword pba, udword offset, void* p_buf, udword read_size);
static ERR_CODE sm_WriteCISBlock(udword drv_no, udword pblock_no);
static uword sm_MakeBAA(udword lblock_no);
static ERR_CODE sm_processBadBlock(udword drv_no, udword lblock_no);
static void sm_markBadBlock(udword drv_no, udword pba);


/**********
 * LPT API definitions
 **********/

#if 0	/* smlGetNewBlock function is obsolete */
/**********
 * Function: smlGetNewBlock
 * Remarks:
 *	- finds unused pLBlock & pPBlock entry , connects them to each other
 *		, and then returns the pLBlock number through the parameter
 * Notes:
 *	- higher layer must call this function to get a new block for use
 *		(before the start of smlWriteBlock()'s in a new block)
 **********/
SML_EXPORT ERR_CODE smlGetNewBlock(udword drv_no, udword* p_lblock_no) 
{
	udword pba, lba;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	/*
	 * in a zone, max LBlock = 1000, max PBlock = 1024,
	 *	and both must be in the same zone => free LBLock is searched first
	 */
	s_smlErr = sm_SearchFreeLBlock(drv_no, &lba);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	s_smlErr = sm_SearchFreePBlock(drv_no, lba, &pba);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	SET_LB_TBL(drv_no, lba, (uword)pba);
	SET_PB_TBL(drv_no, pba, sm_MakeBAA(lba));

	*p_lblock_no = lba;

	LPT_RETURN(drv_no, SM_OK);
}
#endif

/**********
 * Function: smlGetNewSpecifiedBlock
 * Remarks:
 *	- finds unused pPBlock entry, connects it to the specified lblock_no
 *	- updates pPBlock, pLBlock table
 *	- This API is used for the case that higher layer must use the specified lblock
 *		ex) when formatting, MBR must be located in 0th block
 * Parameters
 *	. lblock_no: logical block number that is to be ready
 *	. p_changed_pblock(result): physical block number that was mapped before.
 *			UNUSED_LBLOCK if the specified lblock_no was in free state.
 * Notes:
 *	- If the specified lblock_no is not free, the block is remapped to the
 *		other free block, updates pPBlock, pLBlock according to the new mapping,
 *		and then returns the old physical block with p_changed_pblock
 * *** the higher layer must call smlDiscardPhysBlock() with the parameter of pba
 *		that is returned through p_changed_pblock after updating the contents
 *		if the value is not UNUSED_LBLOCK
 **********/
SML_EXPORT ERR_CODE smlGetNewSpecifiedBlock(udword drv_no, udword lblock_no, udword* p_changed_pblock) 
{
	udword pba;
	udword addr;
	bool done = FALSE;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	*p_changed_pblock = GET_LB_TBL(drv_no, lblock_no);
	SET_LB_TBL(drv_no, lblock_no, UNUSED_LBLOCK);

	while (!done) 
	{
		done = TRUE;

		s_smlErr = sm_SearchFreePBlock(drv_no, lblock_no, &pba);
		if (s_smlErr != SM_OK) 
			LPT_RETURN(drv_no, s_smlErr);

		addr = pba * SECTOR_SIZE * s_devInfo[s_devID[drv_no].device].SpB;
		s_smlErr = smpFlashEraseBlock(drv_no, addr);
		if (s_smlErr != SM_OK) 
		{
			if (s_smlErr == ERR_FLASH_STATUS) 
			{
				sm_markBadBlock(drv_no, pba);
				done = FALSE;
				continue;
			}
			LPT_RETURN(drv_no, s_smlErr);
		}

		SET_LB_TBL(drv_no, lblock_no, (uword)pba);
		SET_PB_TBL(drv_no, pba, sm_MakeBAA(lblock_no));
	}

	LPT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smlDiscardBlock
 * Remarks:
 *	- erases the pba physical block , and makes the entries(in PLBlock & pPBlock) to unused state
 * Notes:
 *	- higher layer must call this function when the block is no more used
 **********/
SML_EXPORT ERR_CODE smlDiscardPhysBlock(udword drv_no, udword pba) 
{
	udword addr;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	if ((pba >= s_devInfo[s_devID[drv_no].device].PBpV) || (pba == UNUSED_LBLOCK)) 
		LPT_RETURN(drv_no, ERR_INVALID_BLOCK);

	addr = pba * SECTOR_SIZE * s_devInfo[s_devID[drv_no].device].SpB;
	smpFlashEraseBlock(drv_no, addr);

	SET_PB_TBL(drv_no, pba, UNUSED_PBLOCK);

	LPT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smlWriteBlock
 * Remarks:
 *	- write data within 1 block
 *	- cannot write beyond the block boundary
 *	- offset and write_size must be on the sector boundary
 * Notes:
 *	- Prior to using this API, higher layer must prepare 1 block using smlGetNewBlock().
 *	- It is strongly recommended that higher layer use this API sequentially from 1st sector to last sector.
 *		* If this API is used to the written sector, it returns error.
 **********/
SML_EXPORT ERR_CODE smlWriteBlock(udword drv_no, udword lblock_no,
		udword offset, const void* p_buf, udword write_size) 
{
	udword i;
	udword pba;
	udword converted_lba;
	udword sector;
	udword page_size;
	ubyte spare[16];
	bool done = FALSE;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	/*
	 * check if the offset and the write_size are on the sector boundary
	if ((offset & (SECTOR_SIZE - 1))
			|| (write_size & (SECTOR_SIZE - 1))) {
		LPT_RETURN(drv_no, ERR_INVALID_PARAM);
	}

	 * check if the (offset + write_size) is beyond the block boundary
	if (offset + write_size >
			SECTOR_SIZE * conDevInfo[sDevID[drv_no].device].SpB) {
		LPT_RETURN(drv_no, ERR_INVALID_PARAM);
	}
	 */

	while (!done) 
	{
		done = TRUE;

		pba = GET_LB_TBL(drv_no, lblock_no);
		if ((pba >= s_devInfo[s_devID[drv_no].device].PBpV)
				|| (pba == UNUSED_LBLOCK)) {
			LPT_RETURN(drv_no, ERR_INVALID_BLOCK);
		}

		converted_lba = sm_MakeBAA(lblock_no);

		sector = pba * s_devInfo[s_devID[drv_no].device].SpB + offset / SECTOR_SIZE;
		page_size = s_devInfo[s_devID[drv_no].device].szP;

		SM_MEMSET(spare, 0xff, 16);
		spare[5] = 0xff;
		spare[6] = spare[11] = (byte)((converted_lba & 0xff00) >> 8);
		spare[7] = spare[12] = (byte)(converted_lba & 0xff);

		for (i = 0; i < write_size; i += SECTOR_SIZE, ++sector,	p_buf = (const ubyte*)p_buf + SECTOR_SIZE) 
		{
#ifndef ECC_ENCODE_DISABLE
			sm_ECCEncode((const ubyte*)p_buf, spare + 13);
			sm_ECCEncode((const ubyte*)p_buf + 256, spare + 8);
#endif
			s_smlErr = smpFlashWriteSector(drv_no, sector, (const ubyte*)p_buf, spare);
			if (s_smlErr != SM_OK) 
			{
				if (s_smlErr == ERR_FLASH_STATUS) 
				{
					s_smlErr = sm_processBadBlock(drv_no, lblock_no);
					if (s_smlErr != SM_OK) 
						LPT_RETURN(drv_no, s_smlErr);

					done = FALSE;
					break;
				}
				LPT_RETURN(drv_no, s_smlErr);
			}
		}
	}

	LPT_RETURN(drv_no, SM_OK);
}

ERR_CODE sm_processBadBlock(udword drv_no, udword lblock_no) 
{
	udword old_pblock;
	udword sectorsPerBlock;
	udword i;
	ubyte* buf;
	udword read_offset;

	// copy the contents
	s_smlErr = smlGetNewSpecifiedBlock(drv_no, lblock_no, &old_pblock);
	if (s_smlErr != SM_OK) 
		return s_smlErr;

	buf = SMB_ALLOC_BUF();
	if (!buf) 
	{
		smlDiscardPhysBlock(drv_no, old_pblock);
		return ERR_OUT_OF_MEMORY;
	}

	s_smlErr = SM_OK;

	sectorsPerBlock = s_devInfo[s_devID[drv_no].device].SpB;
	for (i = 0, read_offset = 0; i < sectorsPerBlock; ++i, read_offset += SECTOR_SIZE) 
	{
		if (!smlIsPhysSectorErased(drv_no, old_pblock, i)) 
		{
			s_smlErr = smlReadPhysBlock(drv_no, old_pblock, read_offset, buf, SECTOR_SIZE);
			if (s_smlErr != SM_OK) 
				break;

			s_smlErr = smlWriteBlock(drv_no, lblock_no, read_offset, buf, SECTOR_SIZE);
			if (s_smlErr != SM_OK) 
				break;
		}
	}

	SMB_FREE_BUF(buf);

	// mark as a bad block
	sm_markBadBlock(drv_no, old_pblock);

	return s_smlErr;
}

void sm_markBadBlock(udword drv_no, udword pba) 
{
	ubyte* buf;
	ubyte spare_buf[16];
	udword sectorsPerBlock;
	udword i;
	udword sector;

	buf = SMB_ALLOC_BUF();
	if (!buf) 
		return;

	SM_MEMSET(buf, 0xff, SECTOR_SIZE);
	SM_MEMSET(spare_buf, 0xff, 16);
	spare_buf[5] = SM_INVALID_BLOCK_MARK;

	sectorsPerBlock = s_devInfo[s_devID[drv_no].device].SpB;
	sector = pba * sectorsPerBlock;
	smpFlashEraseBlock(drv_no, sector * SECTOR_SIZE);

	for (i = 0; i < sectorsPerBlock; ++i) 
	{
		smpFlashWriteSector(drv_no, sector + i, buf, spare_buf);
	}

	SET_PB_TBL(drv_no, pba, INVALID_PBLOCK);

	SMB_FREE_BUF(buf);
}

/**********
 * Function: smlReadBlock
 * Remarks:
 *	- read data within 1 block
 *	- cannot read beyond the block boundary
 *	- if there is an ECC error, p_buf is filled with flash data as in the normal case, but return value is ERR_ECC
 **********/
SML_EXPORT ERR_CODE smlReadBlock(udword drv_no, udword lblock_no, udword offset, void* p_buf, udword read_size) 
{
	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	s_smlErr = sm_ReadBlock(drv_no, lblock_no, offset, p_buf, read_size);

	LPT_RETURN(drv_no, s_smlErr);
}


/**********
 * Function: smlReadPhysBlock
 * Remarks:
 *	- read data within 1 block
 *	- cannot read beyond the block boundary
 *	- if there is an ECC error, p_buf is filled with flash data as in the normal case, but return value is ERR_ECC
 **********/
SML_EXPORT ERR_CODE smlReadPhysBlock(udword drv_no, udword pba, udword offset, void* p_buf, udword read_size) 
{
	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	s_smlErr = sm_ReadPhysBlock(drv_no, pba, offset, p_buf, read_size);

	LPT_RETURN(drv_no, s_smlErr);
}

/**********
 * Function: smlEraseBlock
 * Remarks:
 *	- erase 1 block
 *	- this function is not needed in normal read & write
 *		(normally, erase function is implemented in smlDiscardBlock)
 **********/
SML_EXPORT ERR_CODE smlEraseBlock(udword drv_no, udword lblock_no) 
{
	udword addr;
	udword pba;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	pba = GET_LB_TBL(drv_no, lblock_no);
	if ((pba >= s_devInfo[s_devID[drv_no].device].PBpV) || (pba == UNUSED_LBLOCK)) 
	{
		LPT_RETURN(drv_no, ERR_INVALID_PARAM);
	}

	addr = pba * SECTOR_SIZE * s_devInfo[s_devID[drv_no].device].SpB;
	s_smlErr = smpFlashEraseBlock(drv_no, addr);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	LPT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smlReadSector
 * Remarks:
 *	- calculate (lblock_no, offset) from (lsector_no, offset), and do the same as smlReadBlock
 *	- cannot read beyond the sector boundary
 **********/
SML_EXPORT ERR_CODE smlReadSector(udword drv_no, udword lsector_no, udword offset, void* p_buf, udword read_size) 
{
	udword new_lblock;
	udword new_lsector;
	udword new_offset;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	new_lblock = lsector_no / s_devInfo[s_devID[drv_no].device].SpB;
	new_lsector = lsector_no % s_devInfo[s_devID[drv_no].device].SpB;
	new_offset = new_lsector * SECTOR_SIZE + offset;

	s_smlErr = sm_ReadBlock(drv_no, new_lblock, new_offset, p_buf, read_size);

	LPT_RETURN(drv_no, s_smlErr);
}


/**********
 * Function: smlGetDeviceInfo
 * Remarks:
 *	- returns SmartMedia information
 *	- used when formatting in FAT layer
 **********/
SML_EXPORT ERR_CODE smlGetDeviceInfo(udword drv_no, const sDEV_INFO** dev_info) 
{
	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	*dev_info = &s_devInfo[s_devID[drv_no].device];

	LPT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smlFormatVol
 * Remarks:
 *	- formats the SmartMedia according to the device ID
 *	- This API must be called prior to the FAT API for format (smFormatVol())
 * Parameters
 *	. format_id: LPT_FORMAT_NORMAL, LPT_FORMAT_RESCUE
 *	. p_bad_count (result): number of bad blocks
 **********/
SML_EXPORT ERR_CODE smlFormatVol(udword drv_no, udword format_id, udword* p_bad_count) 
{
	udword i, j, n;
	udword total_blocks;
	udword addr;
	ubyte spare[20];
	udword cnt;
	udword block_size;
	udword page_size;
	ubyte* write_buf;
	ubyte spare_buf[16];
	bool b_error;
	udword err_count=0;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	total_blocks = s_devInfo[s_devID[drv_no].device].PBpV;
	page_size = s_devInfo[s_devID[drv_no].device].szP;
	block_size = s_devInfo[s_devID[drv_no].device].SpB * SECTOR_SIZE;

	if (format_id == LPT_FORMAT_NORMAL) 
	{
		for (i = 0; i < total_blocks; ++i) 
		{
			addr = i * SECTOR_SIZE * s_devInfo[s_devID[drv_no].device].SpB;

			s_smlErr = smpFlashReadSpare(drv_no, addr, spare, 8);
			if (s_smlErr != SM_OK) 
				LPT_RETURN(drv_no, s_smlErr);

			if (spare[5] != 0xff) 
			{
				for (j = 0, cnt = 0; j < 8; ++j) 
				{
					if ((spare[5] >> j) & 0x01) 
					{
						++cnt;
					}
				}

				/* skip invalid block */
				if(cnt <= 6) 
				{
					++err_count;
					continue;
				}
			}

			smpFlashEraseBlock(drv_no, addr);
		}
	}
	else if (format_id == LPT_FORMAT_RESCUE) 
	{
		write_buf = SMB_ALLOC_BUF();
		if (!write_buf) 
			LPT_RETURN(drv_no, ERR_OUT_OF_MEMORY);

		for (i = 0; i < total_blocks; ++i) 
		{
			addr = i * SECTOR_SIZE * s_devInfo[s_devID[drv_no].device].SpB;

			s_smlErr = smpFlashReadSpare(drv_no, addr, spare, 8);
			if (s_smlErr != SM_OK) 
			{
				SMB_FREE_BUF(write_buf);
				LPT_RETURN(drv_no, s_smlErr);
			}

			if (spare[5] != 0xff) 
			{
				for (j = 0, cnt = 0; j < 8; ++j) 
				{
					if ((spare[5] >> j) & 0x01) 
					{
						++cnt;
					}
				}

				/* skip invalid block marked by factory */
				if (cnt <= 1) 
				{
					++err_count;
					continue;
				}
			}

			b_error = FALSE;

			s_smlErr = smpFlashEraseBlock(drv_no, addr);
			if (s_smlErr != SM_OK) 
			{
				if (s_smlErr == ERR_FLASH_STATUS) 
				{
					b_error = TRUE;
				}
				else 
				{
					SMB_FREE_BUF(write_buf);
					LPT_RETURN(drv_no, s_smlErr);
				}
			}

			SM_MEMSET(write_buf, 0x5a, 512);
			SM_MEMSET(spare_buf, 0x5a, 16);

			if (b_error == FALSE) 
			{
				for (j = 0; j < block_size; j += SECTOR_SIZE) 
				{
					s_smlErr = smpFlashWriteSector(drv_no, (addr + j) / SECTOR_SIZE, write_buf, spare_buf);
					if (s_smlErr != SM_OK) 
					{
						if (s_smlErr == ERR_FLASH_STATUS) 
						{
							b_error = TRUE;
							break;
						}
						else 
						{
							SMB_FREE_BUF(write_buf);
							LPT_RETURN(drv_no, s_smlErr);
						}
					}

					s_smlErr = smpFlashReadSector(drv_no, (addr + j) / SECTOR_SIZE, write_buf);
					if (s_smlErr != SM_OK) 
					{
						SMB_FREE_BUF(write_buf);
						LPT_RETURN(drv_no, s_smlErr);
					}
					for (n = 0; n < 512; ++n) 
					{
						if (write_buf[n] != 0x5a) 
						{
							break;
						}
					}

					if (n < 512) 
					{
						b_error = TRUE;
						break;
					}

					s_smlErr = smpFlashReadSectorSpare(drv_no, (addr + j) / SECTOR_SIZE, spare_buf);
					if (s_smlErr != SM_OK) 
					{
						SMB_FREE_BUF(write_buf);
						LPT_RETURN(drv_no, s_smlErr);
					}

					for (n = 0; n < 16; ++n) 
					{
						if (spare_buf[n] != 0x5a) 
						{
							break;
						}
					}

					if (n < 16) 
					{
						b_error = TRUE;
						break;
					}
				}
			}

			if (b_error) 
			{
				++err_count;

				SM_MEMSET(write_buf, 0xff, 512);
				SM_MEMSET(spare_buf, 0xff, 16);
				/*
				 * set block_status_area to 0 => invalid block marking
				 */
				spare_buf[5] = SM_INVALID_BLOCK_MARK;

				for (j = 0; j < block_size; j += SECTOR_SIZE) 
				{
					smpFlashWriteSector(drv_no,	(addr + j) / SECTOR_SIZE, write_buf, spare_buf);
				}
			}
			else 
			{
				smpFlashEraseBlock(drv_no, addr);
			}
		}

		SMB_FREE_BUF(write_buf);
	}

	write_buf = SMB_ALLOC_BUF();
	if (!write_buf) 
		LPT_RETURN(drv_no, ERR_OUT_OF_MEMORY);

	/* bad block marking at block_0 */
	SM_MEMSET(write_buf, 0xff, 512);
	SM_MEMSET(spare_buf, 0xff, 16);
	spare_buf[5] = SM_INVALID_BLOCK_MARK;
	for (j = 0; j < block_size; j += SECTOR_SIZE) 
	{
		smpFlashWriteSector(drv_no, j / SECTOR_SIZE, write_buf, spare_buf);
	}

	SMB_FREE_BUF(write_buf);

	/* write CIS block in the first valid block from block_1 */
	for (i = 1; i < total_blocks; ++i) 
	{
		addr = i * SECTOR_SIZE * s_devInfo[s_devID[drv_no].device].SpB;

		s_smlErr = smpFlashReadSpare(drv_no, addr, spare, 8);
		if (s_smlErr != SM_OK) 
			LPT_RETURN(drv_no, s_smlErr);

		if (spare[5] != 0xff) 
		{
			for (j = 0, cnt = 0; j < 8; ++j) 
			{
				if ((spare[5] >> j) & 0x01) 
				{
					++cnt;
				}
			}

			/* skip invalid block */
			if (cnt <= 6) 
				continue;
		}

		s_smlErr = sm_WriteCISBlock(drv_no, i);
		if (s_smlErr != SM_OK) 
			LPT_RETURN(drv_no, s_smlErr);

		break;
	}

	sm_LPTInit(drv_no);

	*p_bad_count = err_count;

	LPT_RETURN(drv_no, SM_OK);
}

SML_EXPORT bool smlIsSpaceAvailable(udword drv_no, udword block, udword offset, udword count) 
{
	udword pba;
	udword startSector;
	udword endSector;
	udword sectorsPerBlock;
	udword i;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, FALSE);

	pba = GET_LB_TBL(drv_no, block);
	if (pba == UNUSED_LBLOCK) 
		LPT_RETURN(drv_no, FALSE);

	sectorsPerBlock = s_devInfo[s_devID[drv_no].device].SpB;

	startSector = pba * sectorsPerBlock + offset / SECTOR_SIZE;
	endSector = pba * sectorsPerBlock
		+ (offset + count - 1) / SECTOR_SIZE;

	for (i = startSector; i <= endSector; ++i) 
	{
		ubyte spareBuf[16];
		int j;

		s_smlErr = smpFlashReadSectorSpare(drv_no, i, spareBuf);
		if (s_smlErr != SM_OK) 
			LPT_RETURN(drv_no, FALSE);

		for (j = 0; j < sizeof(spareBuf); ++j) 
		{
			if (spareBuf[j] != 0xff) 
				LPT_RETURN(drv_no, FALSE);
		}
	}

	LPT_RETURN(drv_no, TRUE);
}

SML_EXPORT bool smlIsPhysSectorErased(udword drv_no, udword pba, udword sectorOffset) 
{
	udword sector;
	ubyte spareBuf[16];
	int i;
	bool rv;	/* if not use this variable, ARM system misbehaves */

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
	{
		rv = FALSE;
		LPT_RETURN(drv_no, rv);
	}

	sector = pba * s_devInfo[s_devID[drv_no].device].SpB + sectorOffset;

	s_smlErr = smpFlashReadSectorSpare(drv_no, sector, spareBuf);
	if (s_smlErr != SM_OK) 
	{
		rv = FALSE;
		LPT_RETURN(drv_no, rv);
	}

	for (i = 0; i < sizeof(spareBuf); ++i) 
	{
		if (spareBuf[i] != 0xff) 
		{
			rv = FALSE;
			LPT_RETURN(drv_no, rv);
		}
	}

	rv = TRUE;
	LPT_RETURN(drv_no, rv);
}


SML_EXPORT ERR_CODE smlTouchBlock(udword drv_no, udword lblock) 
{
	udword pba;
	ubyte* buf;
	udword i;
	const udword sectorsPerBlock = s_devInfo[s_devID[drv_no].device].SpB;

	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	pba = GET_LB_TBL(drv_no, lblock);
	if (pba == UNUSED_LBLOCK) 
		LPT_RETURN(drv_no, ERR_INVALID_BLOCK);

	buf = SMB_ALLOC_BUF();
	if (!buf) 
		LPT_RETURN(drv_no, ERR_OUT_OF_MEMORY);

	SM_MEMSET(buf, 0xff, SECTOR_SIZE);

	/* if sector's spare area is erased, fill it appropriately */
	for (i = 0; i < sectorsPerBlock; ++i) 
	{
		if (smlIsPhysSectorErased(drv_no, pba, i)) 
		{
			s_smlErr = smlWriteBlock(drv_no, lblock, i * SECTOR_SIZE, buf, SECTOR_SIZE);
			if (s_smlErr != SM_OK) 
			{
				SMB_FREE_BUF(buf);
				LPT_RETURN(drv_no, s_smlErr);
			}
		}
	}

	SMB_FREE_BUF(buf);
	LPT_RETURN(drv_no, SM_OK);
}

/*????????????????????????????????????????????????????????????????????*/
/**********
 * Function: smlOptimizeVol
 * Remarks:
 *	- implements LPT level optimization
 **********/
SML_EXPORT ERR_CODE smlOptimizeVol(udword drv_no, udword opt_mode) 
{
	s_smlErr = sm_PreLPT(drv_no);
	if (s_smlErr != SM_OK) 
		LPT_RETURN(drv_no, s_smlErr);

	LPT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: SMLCBCardInserted
 * Remarks:
 *	- callback function that is called when the card is inserted
 *	- must call the callback function of higher layer
 **********/
SML_EXPORT void SMLCBCardInserted(udword drv_no) 
{
	SMCBCardInserted(drv_no);
}


/**********
 * Function: SMLCBCardEjected
 * Remarks:
 *	- callback function that is called when the card is ejected
 *	- must call the callback function of higher layer
 **********/
SML_EXPORT void SMLCBCardEjected(udword drv_no) 
{
	s_lptFlag[drv_no] = NOT_INITIALIZED;
	SMCBCardEjected(drv_no);
}



/**********
 * Local Function definitions
 **********/

/**********
 * Function: sm_LPTInitDefaultValue
 * Remarks:
 *	- called when the file system is initialized
 **********/
void sm_LPTInitDefaultValue(void) 
{
	udword i;

	for (i = 0; i < MAX_DRIVE; ++i) 
	{
		s_lptFlag[i] = NOT_INITIALIZED;
		if (s_lBlock[i] != NULL) 
		{
			SM_FREE(s_lBlock[i]);
			s_lBlock[i] = NULL;
		}
		if (s_pBlock[i] != NULL) 
		{
			SM_FREE(s_pBlock[i]);
			s_pBlock[i] = NULL;
		}
	}
}


/**********
 * Function: sm_LPTInit
 * Remarks:
 *	- called when the udLPTFlag is not initialized and LPT API is called
 **********/
ERR_CODE sm_LPTInit(udword drv_no) 
{
	udword i, j, k;
	udword cnt;
	udword zone;
	udword total_blocks;
	udword p_num;
	udword l_offset;
	udword p_end;
	udword lba;
	udword parity;
	ubyte acLPTBuf[8];

	s_lptFlag[drv_no] = NOT_INITIALIZED;

	/*
	 * Device Info initial
	 */
	s_smlErr = smpFlashReadID(drv_no, acLPTBuf, 2);
	if (s_smlErr != SM_OK) 
		return s_smlErr;

	/* search manufacture code table */
	for (i = 0; i < MAX_MI_NUM; ++i) 
	{
		if (acLPTBuf[0] == g_mCode[i]) 
			break;
	}
	if (i >= MAX_MI_NUM) 
		return ERR_CARD_NOT_DETECTED;

	s_devID[drv_no].manufacture = i;

	/* search device code table */
	for (j = 0; j < MAX_DI_NUM; ++j) 
	{
		for (k = 0; k < MAX_VI_NUM; ++k) 
		{
			if (acLPTBuf[1] == g_dCode[i][j][k]) 
				break;
		}

		if (k < MAX_VI_NUM) 
			break;
	}

	if (j >= MAX_DI_NUM) 
		return ERR_CARD_NOT_DETECTED;

	s_devID[drv_no].device = j;

	total_blocks = s_devInfo[s_devID[drv_no].device].PBpV;

	/*
	 * s_pBlock[MAX_DRIVE] table initial
	 */
	if (s_pBlock[drv_no] != NULL) 
		SM_FREE(s_pBlock[drv_no]);

	s_pBlock[drv_no] = (uword*)SM_MALLOC(2 * total_blocks);
	if (s_pBlock[drv_no] == NULL) 
		return ERR_INTERNAL;

	for (i = 0; i < total_blocks; ++i) 
	{
		s_smlErr = smpFlashReadSpare(drv_no, i * (SECTOR_SIZE * s_devInfo[s_devID[drv_no].device].SpB),	acLPTBuf, 8);
		if (s_smlErr != SM_OK) 
		{
			SM_FREE(s_pBlock[drv_no]);
			s_pBlock[drv_no] = NULL;
			return (s_smlErr);
		}

		if (acLPTBuf[5] != 0xff) 
		{
			for (j = 0, cnt = 0; j < 8; ++j) 
			{
				if ((acLPTBuf[5] >> j) & 0x01) 
					++cnt;
			}

			if (cnt <= 6) 
			{
				SET_PB_TBL(drv_no, i, INVALID_PBLOCK);
				continue;
			}
		}

		SET_PB_TBL(drv_no, i, ((uword)acLPTBuf[6] << 8) | acLPTBuf[7]);
	}

	/*
	 * pLBlock[MAX_DRIVE] table initial
	 */
	if (s_lBlock[drv_no] != NULL) 
		SM_FREE(s_lBlock[drv_no]);

	s_lBlock[drv_no] = (uword*)SM_MALLOC(2 * total_blocks);
	if (s_lBlock[drv_no] == NULL) 
	{
		SM_FREE(s_pBlock[drv_no]);
		s_pBlock[drv_no] = NULL;
		return ERR_INTERNAL;
	}

	for (i = 0; i < total_blocks; ++i) 
	{
		SET_LB_TBL(drv_no, i, UNUSED_LBLOCK);
	}


	for (zone = 0; zone < total_blocks / P_ZONE_MAX + 1; ++zone) 
	{
		l_offset = zone * L_ZONE_MAX;

		if (total_blocks < P_ZONE_MAX * (zone + 1)) 
			p_end = total_blocks;
		else 
			p_end = P_ZONE_MAX * (zone + 1);

		for (p_num = zone * P_ZONE_MAX; p_num < p_end; ++p_num) 
		{
			if ((GET_PB_TBL(drv_no, p_num) == INVALID_PBLOCK) || (GET_PB_TBL(drv_no, p_num) == UNUSED_PBLOCK)
				|| (GET_PB_TBL(drv_no, p_num) == 0)) 		/* CIS Block is set to 0 */
			{
				continue;
			}

			lba = GET_PB_TBL(drv_no, p_num);

			/* parity check */
			parity = 0;
			for (i = 1; i < 16; ++i) 
			{
				parity ^= (lba >> i) & 0x01;
			}

			if ((lba & 0x01) != parity) 
				continue;

			lba = ((lba >> 1) & 0x3ff) + l_offset;
			if (lba >= total_blocks) 		/* invalid contents */
				continue;

			SET_LB_TBL(drv_no, lba, (uword)p_num);
		}
	}

	/* start of random zone */
	s_lBlockSearchStart[drv_no] = (SM_RANDOM() % s_devInfo[s_devID[drv_no].device].LBpV);
	s_lBlockSearchStart[drv_no] -= (s_lBlockSearchStart[drv_no] % L_ZONE_MAX);

	/* random index of random zone */
	s_pBlockSearchStart[drv_no] = (SM_RANDOM() % s_devInfo[s_devID[drv_no].device].PBpV);

	s_lptFlag[drv_no] = INITIALIZED;

	return SM_OK;
}


/**********
 * Function: sm_PreLPT
 * Remarks:
 *	- called at the first of LPT APIs
 **********/
ERR_CODE sm_PreLPT(udword drv_no) 
{
	if (s_lptFlag[drv_no] != INITIALIZED) 
	{
		s_smlErr = sm_LPTInit(drv_no);
		if (s_smlErr != SM_OK) 
			return s_smlErr;
	}

	return SM_OK;
}


/**********
 * Function: sm_PostLPT
 * Remarks:
 *	- called at the end of LPT APIs
 **********/
ERR_CODE sm_PostLPT(udword drv_no) 
{
	return SM_OK;
}


#if 0	/* sm_SearchFreeLBlock is now obsolete */
/**********
 * Function: sm_SearchFreeLBlock
 * Remark:
 *	- at start, checks one more time the index retunred last
 *		(this is for the case that the value is not used after return)
 **********/
ERR_CODE sm_SearchFreeLBlock(udword drv_no, udword* pLba) 
{
	udword i;

	for (i = 0; i < s_devInfo[s_devID[drv_no].device].LBpV; ++i) 
	{
		if (s_lBlockSearchStart[drv_no] >= s_devInfo[s_devID[drv_no].device].LBpV) 
		{
			s_lBlockSearchStart[drv_no] = 0;
		}

		if (GET_LB_TBL(drv_no, s_lBlockSearchStart[drv_no]) == UNUSED_LBLOCK) 
		{
			*pLba = s_lBlockSearchStart[drv_no];
			return SM_OK;
		}

		++s_lBlockSearchStart[drv_no];
	}

	return ERR_NO_EMPTY_BLOCK;
}
#endif


/**********
 * Function: sm_SearchFreePBlock
 * Remark:
 *	- at start, checks one more time the index retunred last
 *		(this is for the case that the value is not used after return)
 *	- if present udPBlockSearchStart[] is in the different zone from the lba zone,
 *		udPBlockSearchStart[] is set to random value in the same zone as lba's
 **********/
ERR_CODE sm_SearchFreePBlock(udword drv_no, udword lba, udword* pPba) 
{
	udword i;
	udword zone;
	udword count;
	udword min_block, max_block;
	udword start_backup;
	udword pba;

	if (lba >= s_devInfo[s_devID[drv_no].device].LBpV) 
		return ERR_INVALID_PARAM;

	/* if physical block for lba already exists, return it */
	pba = GET_LB_TBL(drv_no, lba);
	if (pba != UNUSED_LBLOCK) 
	{
		*pPba = pba;
		return SM_OK;
	}

	zone = lba / L_ZONE_MAX;

	/* locate MBR at first valid block for convenience */
	if (lba == 0) 
	{
		start_backup = s_pBlockSearchStart[drv_no];
		s_pBlockSearchStart[drv_no] = 3;
	}

	/* if udPBlockSearchStart[] is not in the same zone, this value is newly randomized in the same zone */
	if (s_pBlockSearchStart[drv_no] / P_ZONE_MAX != zone) 
	{
		if (s_devInfo[s_devID[drv_no].device].PBpV < P_ZONE_MAX) 
		{
			s_pBlockSearchStart[drv_no] = SM_RANDOM() % s_devInfo[s_devID[drv_no].device].PBpV;
		}
		else 
		{
			s_pBlockSearchStart[drv_no] = SM_RANDOM() % P_ZONE_MAX;
			s_pBlockSearchStart[drv_no] += (zone * P_ZONE_MAX);
		}
	}

	if (s_devInfo[s_devID[drv_no].device].PBpV < P_ZONE_MAX) 
	{
		count = s_devInfo[s_devID[drv_no].device].PBpV;
		min_block = 0;
		max_block = s_devInfo[s_devID[drv_no].device].PBpV - 1;
	}
	else 
	{
		count = P_ZONE_MAX;
		min_block = P_ZONE_MAX * zone;
		max_block = P_ZONE_MAX * (zone + 1) - 1;
	}

	for (i = 0; i < count; ++i) 
	{
		if (s_pBlockSearchStart[drv_no] > max_block) 
		{
			s_pBlockSearchStart[drv_no] = min_block;
		}

		if(GET_PB_TBL(drv_no, s_pBlockSearchStart[drv_no]) == UNUSED_PBLOCK) 
		{
			*pPba = s_pBlockSearchStart[drv_no];
			break;
		}

		++s_pBlockSearchStart[drv_no];
	}

	/* restore the old search_start block when lba is MBR block */
	if (lba == 0) 
	{
		s_pBlockSearchStart[drv_no] = start_backup;
	}

	if (i < count) 
		return SM_OK;

	return ERR_NO_EMPTY_BLOCK;
}


/**********
 * Function: sm_ECCEncode
 * Remark:
 *	- adopted from "ECC Algorithm for SmartMedia V3.0"
 *		by Memory Product & Technology, Samsung Electronics Co. (ecc30.pdf)
 **********/
ERR_CODE sm_ECCEncode(const ubyte* p_buf, ubyte* p_ecc) 
{
	udword i, j;
	ubyte paritr[256], tmp = 0, tmp2 = 0;
	ubyte data_table0[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };
	ubyte data_table1[16] = { 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 };
	ubyte sum = 0, paritc = 0;
	ubyte parit0c = 0, parit1c = 0, parit2c = 0, parit3c = 0;
	ubyte parit4c = 0, parit5c = 0, parit6c = 0, parit7c = 0;
	ubyte parit1_1, parit1_2, parit2_1, parit2_2, parit4_1, parit4_2;
	ubyte parit8_1 = 0, parit8_2 = 0, parit16_1 = 0, parit16_2 = 0, parit32_1 = 0, parit32_2 = 0;
	ubyte parit64_1 = 0, parit64_2 = 0, parit128_1 = 0, parit128_2 = 0, parit256_1 = 0, parit256_2 = 0;
	ubyte parit512_1 = 0, parit512_2 = 0, parit1024_1 = 0, parit1024_2 = 0;
	ubyte* paritr_ptr;

	paritr_ptr = paritr;
	for (i = 0; i < 256; ++i, ++paritr_ptr, ++p_buf) 
	{
		paritc ^= *p_buf;
		tmp = (*p_buf & 0xf0) >> 4;
		tmp2 = *p_buf & 0x0f;

		switch (tmp) 
		{
			case 0:
			case 3:
			case 5:
			case 6:
			case 9:
			case 10:
			case 12:
			case 15:
				*paritr_ptr = *(data_table0 + tmp2);
				break;

			case 1:
			case 2:
			case 4:
			case 7:
			case 8:
			case 11:
			case 13:
			case 14:
				*paritr_ptr = *(data_table1 + tmp2);
				break;
		}
	}

	parit0c = (paritc & 0x01) ? 1 : 0;
	parit1c = (paritc & 0x02) ? 1 : 0;
	parit2c = (paritc & 0x04) ? 1 : 0;
	parit3c = (paritc & 0x08) ? 1 : 0;
	parit4c = (paritc & 0x10) ? 1 : 0;
	parit5c = (paritc & 0x20) ? 1 : 0;
	parit6c = (paritc & 0x40) ? 1 : 0;
	parit7c = (paritc & 0x80) ? 1 : 0;
	parit1_2 = parit6c ^ parit4c ^ parit2c ^ parit0c;
	parit1_1 = parit7c ^ parit5c ^ parit3c ^ parit1c;
	parit2_2 = parit5c ^ parit4c ^ parit1c ^ parit0c;
	parit2_1 = parit7c ^ parit6c ^ parit3c ^ parit2c;
	parit4_2 = parit3c ^ parit2c ^ parit1c ^ parit0c;
	parit4_1 = parit7c ^ parit6c ^ parit5c ^ parit4c;

	paritr_ptr = paritr;
	for (i = 0; i < 256; ++i, ++paritr_ptr) 
	{
		sum ^= *paritr_ptr;
	}

	paritr_ptr = paritr;
	for (i = 0; i < 256; i += 2, paritr_ptr += 2) 
	{
		parit8_2 ^= *paritr_ptr;
	}

	paritr_ptr = paritr;
	for (i = 0; i < 256; i += 4, paritr_ptr += 4) 
	{
		parit16_2 ^= *paritr_ptr;
		parit16_2 ^= *(paritr_ptr + 1);
	}

	paritr_ptr = paritr;
	for (i = 0; i < 256; i += 8, paritr_ptr += 8) 
	{
		for (j = 0; j <= 3; ++j) 
		{
			parit32_2 ^= *(paritr_ptr + j);
		}
	}

	paritr_ptr = paritr;
	for (i = 0; i < 256; i += 16, paritr_ptr += 16) 
	{
		for (j = 0; j <= 7; ++j) 
		{
			parit64_2 ^= *(paritr_ptr + j);
		}
	}

	paritr_ptr = paritr;
	for (i = 0; i < 256; i += 32, paritr_ptr += 32) 
	{
		for (j = 0; j <= 15; ++j) 
		{
			parit128_2 ^= *(paritr_ptr + j);
		}
	}

	paritr_ptr = paritr;
	for (i = 0; i < 256; i += 64, paritr_ptr += 64) 
	{
		for (j = 0; j <= 31; ++j) 
		{
			parit256_2 ^= *(paritr_ptr + j);
		}
	}

	paritr_ptr = paritr;
	for (i = 0; i < 256; i += 128, paritr_ptr += 128) 
	{
		for (j = 0; j <= 63; ++j) 
		{
			parit512_2 ^= *(paritr_ptr + j);
		}
	}

	paritr_ptr = paritr;
	for (i = 0; i < 256; i += 256, paritr_ptr += 256) 
	{
		for (j = 0; j <= 127; ++j) 
		{
			parit1024_2 ^= *(paritr_ptr + j);
		}
	}

	if (sum==0) 
	{
		parit1024_1 = parit1024_2;
		parit512_1 = parit512_2;
		parit256_1 = parit256_2;
		parit128_1 = parit128_2;
		parit64_1 = parit64_2;
		parit32_1 = parit32_2;
		parit16_1 = parit16_2;
		parit8_1 = parit8_2;
	}
	else 
	{
		parit1024_1 = parit1024_2 ? 0 : 1;
		parit512_1 = parit512_2 ? 0 : 1;
		parit256_1 = parit256_2 ? 0 : 1;
		parit128_1 = parit128_2 ? 0 : 1;
		parit64_1 = parit64_2 ? 0 : 1;
		parit32_1 = parit32_2 ? 0 : 1;
		parit16_1 = parit16_2 ? 0 : 1;
		parit8_1 = parit8_2 ? 0 : 1;
	}

	parit1_2 <<= 2;
	parit1_1 <<= 3;
	parit2_2 <<= 4;
	parit2_1 <<= 5;
	parit4_2 <<= 6;
	parit4_1 <<= 7;
	parit128_1 <<= 1;
	parit256_2 <<= 2;
	parit256_1 <<= 3;
	parit512_2 <<= 4;
	parit512_1 <<= 5;
	parit1024_2 <<= 6;
	parit1024_1 <<= 7;
	parit8_1 <<= 1;
	parit16_2 <<= 2;
	parit16_1 <<= 3;
	parit32_2 <<= 4;
	parit32_1 <<= 5;
	parit64_2 <<= 6;
	parit64_1 <<= 7;

	p_ecc[0] = ~(parit64_1 | parit64_2 | parit32_1 | parit32_2 | parit16_1 | parit16_2 | parit8_1 | parit8_2);
	p_ecc[1] = ~(parit1024_1 |parit1024_2 | parit512_1 | parit512_2 | parit256_1 | parit256_2 | parit128_1 | parit128_2);
	p_ecc[2] = ~(parit4_1 | parit4_2 | parit2_1 | parit2_2 | parit1_1 | parit1_2);

	return SM_OK;
}


/**********
 * Function: sm_ECCCorrect
 * Remarks:
 *	- adopted from source code that was corrected by SeoGyu Kim, Infomedia Lab. SEC.
 **********/
ERR_CODE sm_ECCCorrect(const ubyte *p_data, const ubyte *ecc1, const ubyte *ecc2, ubyte *p_offset, ubyte *p_corrected) 
{
	int i;
	ubyte tmp0_bit[8],tmp1_bit[8],tmp2_bit[8], tmp0, tmp1, tmp2;
	ubyte comp0_bit[8],comp1_bit[8],comp2_bit[8];
	ubyte ecc_bit[22];
	ubyte ecc_gen[3];
	ubyte ecc_sum=0;
	ubyte ecc_value,find_byte,find_bit;

	tmp0 = ~ecc1[0];
	tmp1 = ~ecc1[1];
	tmp2 = ~ecc1[2];

	for (i = 0; i <= 2; ++i) 
	{
		ecc_gen[i] = ~ecc2[i];
	}

	tmp0_bit[0]= tmp0 & 0x01;
	tmp0 >>= 1;
	tmp0_bit[1]= tmp0 & 0x01;
	tmp0 >>= 1;
	tmp0_bit[2]= tmp0 & 0x01;
	tmp0 >>= 1;
	tmp0_bit[3]= tmp0 & 0x01;
	tmp0 >>= 1;
	tmp0_bit[4]= tmp0 & 0x01;
	tmp0 >>= 1;
	tmp0_bit[5]= tmp0 & 0x01;
	tmp0 >>= 1;
	tmp0_bit[6]= tmp0 & 0x01;
	tmp0 >>= 1;
	tmp0_bit[7]= tmp0 & 0x01;

	tmp1_bit[0]= tmp1 & 0x01;
	tmp1 >>= 1;
	tmp1_bit[1]= tmp1 & 0x01;
	tmp1 >>= 1;
	tmp1_bit[2]= tmp1 & 0x01;
	tmp1 >>= 1;
	tmp1_bit[3]= tmp1 & 0x01;
	tmp1 >>= 1;
	tmp1_bit[4]= tmp1 & 0x01;
	tmp1 >>= 1;
	tmp1_bit[5]= tmp1 & 0x01;
	tmp1 >>= 1;
	tmp1_bit[6]= tmp1 & 0x01;
	tmp1 >>= 1;
	tmp1_bit[7]= tmp1 & 0x01;

	tmp2_bit[0]= tmp2 & 0x01;
	tmp2 >>= 1;
	tmp2_bit[1]= tmp2 & 0x01;
	tmp2 >>= 1;
	tmp2_bit[2]= tmp2 & 0x01;
	tmp2 >>= 1;
	tmp2_bit[3]= tmp2 & 0x01;
	tmp2 >>= 1;
	tmp2_bit[4]= tmp2 & 0x01;
	tmp2 >>= 1;
	tmp2_bit[5]= tmp2 & 0x01;
	tmp2 >>= 1;
	tmp2_bit[6]= tmp2 & 0x01;
	tmp2 >>= 1;
	tmp2_bit[7]= tmp2 & 0x01;

	comp0_bit[0]= ecc_gen[0] & 0x01;
	ecc_gen[0] >>= 1;
	comp0_bit[1]= ecc_gen[0] & 0x01;
	ecc_gen[0] >>= 1;
	comp0_bit[2]= ecc_gen[0] & 0x01;
	ecc_gen[0] >>= 1;
	comp0_bit[3]= ecc_gen[0] & 0x01;
	ecc_gen[0] >>= 1;
	comp0_bit[4]= ecc_gen[0] & 0x01;
	ecc_gen[0] >>= 1;
	comp0_bit[5]= ecc_gen[0] & 0x01;
	ecc_gen[0] >>= 1;
	comp0_bit[6]= ecc_gen[0] & 0x01;
	ecc_gen[0] >>= 1;
	comp0_bit[7]= ecc_gen[0] & 0x01;

	comp1_bit[0]= ecc_gen[1] & 0x01;
	ecc_gen[1] >>= 1;
	comp1_bit[1]= ecc_gen[1] & 0x01;
	ecc_gen[1] >>= 1;
	comp1_bit[2]= ecc_gen[1] & 0x01;
	ecc_gen[1] >>= 1;
	comp1_bit[3]= ecc_gen[1] & 0x01;
	ecc_gen[1] >>= 1;
	comp1_bit[4]= ecc_gen[1] & 0x01;
	ecc_gen[1] >>= 1;
	comp1_bit[5]= ecc_gen[1] & 0x01;
	ecc_gen[1] >>= 1;
	comp1_bit[6]= ecc_gen[1] & 0x01;
	ecc_gen[1] >>= 1;
	comp1_bit[7]= ecc_gen[1] & 0x01;

	comp2_bit[0]= ecc_gen[2] & 0x01;
	ecc_gen[2] >>= 1;
	comp2_bit[1]= ecc_gen[2] & 0x01;
	ecc_gen[2] >>= 1;
	comp2_bit[2]= ecc_gen[2] & 0x01;
	ecc_gen[2] >>= 1;
	comp2_bit[3]= ecc_gen[2] & 0x01;
	ecc_gen[2] >>= 1;
	comp2_bit[4]= ecc_gen[2] & 0x01;
	ecc_gen[2] >>= 1;
	comp2_bit[5]= ecc_gen[2] & 0x01;
	ecc_gen[2] >>= 1;
	comp2_bit[6]= ecc_gen[2] & 0x01;
	ecc_gen[2] >>= 1;
	comp2_bit[7]= ecc_gen[2] & 0x01;

	for (i = 0; i <= 5; ++i) 
	{
		ecc_bit[i] = tmp2_bit[i + 2] ^ comp2_bit[i + 2];
	}

	for (i = 0; i <= 7; ++i) 
	{
		ecc_bit[i + 6] = tmp0_bit[i] ^ comp0_bit[i];
	}

	for (i = 0; i <= 7; ++i) 
	{
		ecc_bit[i + 14] = tmp1_bit[i] ^ comp1_bit[i];
	}

	for (i = 0; i <= 21; ++i) 
	{
		ecc_sum += ecc_bit[i];
	}

	if (ecc_sum == 11) 
	{
		find_byte = (ecc_bit[21] << 7) + (ecc_bit[19] << 6) + (ecc_bit[17] << 5) + (ecc_bit[15] << 4) + (ecc_bit[13] << 3) + (ecc_bit[11] << 2) + (ecc_bit[9] << 1) + ecc_bit[7];
		find_bit = (ecc_bit[5] << 2) + (ecc_bit[3] << 1) + ecc_bit[1];
		ecc_value = (p_data[find_byte] >> find_bit) & 0x01;
		if (ecc_value == 0) 
			ecc_value = 1;
		else 
			ecc_value = 0;

		*p_offset = find_byte;
		*p_corrected = p_data[find_byte];

		tmp0_bit[0] = *p_corrected & 0x01;
		*p_corrected >>= 1;
		tmp0_bit[1] = *p_corrected & 0x01;
		*p_corrected >>= 1;
		tmp0_bit[2] = *p_corrected & 0x01;
		*p_corrected >>= 1;
		tmp0_bit[3] = *p_corrected & 0x01;
		*p_corrected >>= 1;
		tmp0_bit[4] = *p_corrected & 0x01;
		*p_corrected >>= 1;
		tmp0_bit[5] = *p_corrected & 0x01;
		*p_corrected >>= 1;
		tmp0_bit[6] = *p_corrected & 0x01;
		*p_corrected >>= 1;
		tmp0_bit[7] = *p_corrected & 0x01;

		tmp0_bit[find_bit] = ecc_value;

		*p_corrected = (tmp0_bit[7] << 7) + (tmp0_bit[6] << 6) + (tmp0_bit[5] << 5) + (tmp0_bit[4] << 4) + (tmp0_bit[3] << 3) + (tmp0_bit[2] << 2) + (tmp0_bit[1] << 1) + tmp0_bit[0];

		return ERR_ECC_CORRECTABLE;
	}
	else if ((ecc_sum == 0) || (ecc_sum == 1)) 
	{
		return SM_OK;
	}
	else 
	{
		return ERR_ECC;
	}

	return SM_OK;
}


/**********
 * Function: sm_ECCDecode
 * Remarks:
 *	- 1 bit error correction is implemented
 **********/
ERR_CODE sm_ECCDecode(const ubyte* p_buf, const ubyte* p_ecc) 
{
	ubyte ecc_data[3];
	ubyte offset;
	ubyte corrected;
	udword ret_val;

	sm_ECCEncode(p_buf, ecc_data);

	if ((p_ecc[0] != ecc_data[0]) || (p_ecc[1] != ecc_data[1]) || (p_ecc[2] != ecc_data[2])) 
	{
		ret_val = sm_ECCCorrect(p_buf, p_ecc, ecc_data, &offset, &corrected);
		if (ret_val == ERR_ECC_CORRECTABLE) 
		{
			*((ubyte*)p_buf + offset) = corrected;
		}
		else if (ret_val == ERR_ECC) 
			return ERR_ECC;
	}

	return SM_OK;
}


/**********
 * Function: sm_ReadBlock
 * Remarks:
 *	- read data within 1 block
 *	- cannot read beyond the block boundary
 *	- if there is an ECC error, p_buf is filled with flash data as in the normal case, but return value is ERR_ECC
 **********/
ERR_CODE sm_ReadBlock(udword drv_no, udword lblock_no, udword offset, void* p_buf, udword read_size) 
{
	udword pba;

	pba = GET_LB_TBL(drv_no, lblock_no);
	return sm_ReadPhysBlock(drv_no, pba, offset, p_buf, read_size);
}

/**********
 * Function: sm_ReadPhysBlock
 * Remarks:
 *	- read data within 1 block
 *	- cannot read beyond the block boundary
 *	- if there is an ECC error, p_buf is filled with flash data as in the normal case, but return value is ERR_ECC
 **********/
ERR_CODE sm_ReadPhysBlock(udword drv_no, udword pba, udword offset, void* p_buf, udword read_size) 
{
	udword i;
	udword addr;
	udword page_size;
#ifndef ECC_DECODE_DISABLE
	ubyte spare[16];
	bool b_ecc_error = FALSE;
#endif
	udword first_read_size = 0;
	udword last_read_size = 0;
	udword temp_read_size;

	/*
	 * check if the (offset + read_size) is beyond the block boundary
	 * if(offset + read_size > SECTOR_SIZE * conDevInfo[sDevID[drv_no].device].SpB)
	 * return ERR_INVALID_PARAM;
	 */

	if ((pba >= s_devInfo[s_devID[drv_no].device].PBpV) || (pba == UNUSED_LBLOCK)) 
	{
		return ERR_INVALID_BLOCK;
	}

	addr = pba * SECTOR_SIZE * s_devInfo[s_devID[drv_no].device].SpB + offset;
	page_size = s_devInfo[s_devID[drv_no].device].szP;

	/* read head side until the first sector boundary or until the read_size */
	if (offset & (SECTOR_SIZE - 1)) 	/* <= (offset % SECTOR_SIZE) */
	{
		first_read_size = SECTOR_SIZE - (offset & (SECTOR_SIZE - 1));	/* <= (offset % SECTOR_SIZE); */

		/* check if the first_read_size is beyond read_size */
		if (first_read_size > read_size) 
		{
			first_read_size = read_size;
		}

		if (page_size == 256) 
		{
			/* check if the start & end are on the different pages */
			if ((offset / 256) != ((offset + first_read_size) / 256)) 
			{
				temp_read_size = first_read_size - ((offset + first_read_size) % 256);

				s_smlErr = smpFlashReadPage(drv_no, addr, (ubyte*)p_buf, temp_read_size);
				if (s_smlErr != SM_OK) 
					return s_smlErr;

				s_smlErr = smpFlashReadPage(drv_no, addr + temp_read_size, (ubyte*)p_buf + temp_read_size, first_read_size - temp_read_size);
				if (s_smlErr != SM_OK) 
					return s_smlErr;
			}
			else 
			{
				s_smlErr = smpFlashReadPage(drv_no, addr, (ubyte*)p_buf, first_read_size);
				if (s_smlErr != SM_OK) 
					return s_smlErr;
			}
		}
		else if (page_size == 512) 
		{
			if ((offset & (SECTOR_SIZE - 1)) < 256) 	/* <= ((offset % SECTOR_SIZE) < 256) */
			{
				s_smlErr = smpFlashReadPage(drv_no, addr, (ubyte*)p_buf, first_read_size);
				if (s_smlErr != SM_OK) 
					return s_smlErr;
			}
			else 
			{
				s_smlErr = smpFlashReadPage2(drv_no, addr, (ubyte*)p_buf, first_read_size);
				if (s_smlErr != SM_OK) 
					return s_smlErr;
			}
		}

		addr += first_read_size;
		p_buf = (ubyte*)p_buf + first_read_size;
		read_size -= first_read_size;
	}

	/* read tail side from the last sector boundary */
	if (read_size & (SECTOR_SIZE - 1)) 	/* <= (read_size % SECTOR_SIZE) */
	{
		last_read_size = read_size & (SECTOR_SIZE - 1);	/* <= read_size % SECTOR_SIZE; */
		if (page_size == 256) 
		{
			if (last_read_size < 256) 
			{
				s_smlErr = smpFlashReadPage(drv_no, addr + read_size - last_read_size, (ubyte*)p_buf + read_size - last_read_size, last_read_size);
				if (s_smlErr != SM_OK) 
					return s_smlErr;
			}
			else 
			{
				s_smlErr = smpFlashReadPage(drv_no, addr + read_size - last_read_size, (ubyte*)p_buf + read_size - last_read_size, 256);
				if (s_smlErr != SM_OK) 
					return s_smlErr;

				s_smlErr = smpFlashReadPage(drv_no, addr + read_size - last_read_size + 256, (ubyte*)p_buf + read_size - last_read_size + 256, last_read_size - 256);
				if (s_smlErr != SM_OK) 
					return s_smlErr;
			}
		}
		else if (page_size == 512) 
		{
			s_smlErr = smpFlashReadPage(drv_no, addr + read_size - last_read_size, (ubyte*)p_buf + read_size - last_read_size, last_read_size);
			if (s_smlErr != SM_OK) 
				return s_smlErr;
		}

		read_size -= last_read_size;
	}

	/*
	 * now, offset and (offset + read_size) is on the sector boundary.
	 *  (if not, there was an error in the upper processing)
	 */
	if (read_size > 0) 
	{
		for (i = 0; i < read_size; i += SECTOR_SIZE, addr += SECTOR_SIZE, p_buf = (ubyte*)p_buf + SECTOR_SIZE) 
		{
			if (page_size == 256) 
			{
				s_smlErr = smpFlashReadPage(drv_no, addr, (ubyte*)p_buf, 256);
				if (s_smlErr != SM_OK) 
					return s_smlErr;

				s_smlErr = smpFlashReadPage(drv_no, addr + 256, (ubyte*)p_buf + 256, 256);
				if (s_smlErr != SM_OK) 
					return s_smlErr;

#ifndef ECC_DECODE_DISABLE
				s_smlErr = smpFlashReadSpare(drv_no, addr + 256, spare, 8);
				if (s_smlErr != SM_OK) 
					return s_smlErr;

				if (sm_ECCDecode((const ubyte*)p_buf, spare + 5) != SM_OK) 
					b_ecc_error = TRUE;

				if (sm_ECCDecode((ubyte*)p_buf + 256, spare) != SM_OK) 
					b_ecc_error = TRUE;
#endif
			}
			else if(page_size == 512) 
			{
				s_smlErr = smpFlashReadPage(drv_no, addr, (ubyte*)p_buf, 512);
				if (s_smlErr != SM_OK) 
					return s_smlErr;

#ifndef ECC_DECODE_DISABLE
				s_smlErr = smpFlashReadSpare(drv_no, addr, spare, 16);
				if (s_smlErr != SM_OK) 
					return s_smlErr;

				if (sm_ECCDecode((const ubyte*)p_buf, spare + 13) != SM_OK) 
					b_ecc_error = TRUE;

				if (sm_ECCDecode((const ubyte*)p_buf + 256, spare + 8) != SM_OK) 
					b_ecc_error = TRUE;
#endif
			}
		}
	}

#ifndef ECC_DECODE_DISABLE
	if (b_ecc_error) 
		return ERR_ECC;
#endif

	return SM_OK;
}

/**********
 * Function: sm_WriteCISBlock
 * Remarks:
 *	- write CIS data to the defined physical block
 **********/
ERR_CODE sm_WriteCISBlock(udword drv_no, udword pblock_no) 
{
	udword sector;
	udword page_size;
	ubyte spare[16];
	ubyte* acLPTBuf = SMB_ALLOC_BUF();

	if (!acLPTBuf) 
		return ERR_OUT_OF_MEMORY;

	sector = pblock_no * s_devInfo[s_devID[drv_no].device].SpB;
	page_size = s_devInfo[s_devID[drv_no].device].szP;

	SM_MEMSET(spare, 0xff, 16);
	spare[5] = 0xff;
	spare[6] = spare[7] = spare[11] = spare[12] = 0;

	/* first sector */
	SM_MEMSET(acLPTBuf, 0, SECTOR_SIZE);
	SM_MEMCPY(acLPTBuf, s_cis, sizeof(s_cis));

#ifndef ECC_ENCODE_DISABLE
	sm_ECCEncode(acLPTBuf, spare + 13);
	sm_ECCEncode(acLPTBuf + 256, spare + 8);
#endif

	s_smlErr = smpFlashWriteSector(drv_no, sector, acLPTBuf, spare);
	if (s_smlErr != SM_OK) 
	{
		SMB_FREE_BUF(acLPTBuf);
		LPT_RETURN(drv_no, s_smlErr);
	}

	SMB_FREE_BUF(acLPTBuf);
	LPT_RETURN(drv_no, SM_OK);
}


/**********
 * Function: sm_MakeBAA
 * Remarks:
 *	- get the value that should be saved to SmartMedia from the logical block number
 **********/
uword sm_MakeBAA(udword lblock_no) 
{
	udword converted_lba;
	udword parity;
	udword i;

	converted_lba = lblock_no % L_ZONE_MAX;
	parity = 1;
	for (i = 0; i < 10; ++i) 
	{
		parity ^= (converted_lba >> i) & 0x01;
	}
	converted_lba = 0x1000 | (converted_lba << 1) | parity;

	return (uword)converted_lba;
}

