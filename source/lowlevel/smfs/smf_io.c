/**********
 *
 * smf_io.c: SmartMedia File System IO part
 *
 * Portable File System designed for SmartMedia
 *
 * Created by Kim Hyung Gi (06/99) - kimhg@mmrnd.sec.samsung.co.kr
 * Samsung Electronics, M/M R&D Center, Visual Communication Group
 *
 **********
 * Modified by Lee Jin Woo (1999/08/17)
 *
 **********/
#include "smf_cmn.h"
#include "smf_io.h"
#include "smf_lpt.h"

/**********
 * Static Variables
 **********/
static bool s_cardInserted[MAX_DRIVE];
static udword s_ioFlag[MAX_DRIVE];		/* IO layer initialization status flag */
static udword s_deviceSize[MAX_DRIVE];
static udword s_pageSize[MAX_DRIVE];	/* page size of the SmartMedia that is inserted */
static ERR_CODE s_smpErr;

/**********
 * Global Variables
 **********/
ubyte g_mCode[MAX_MI_NUM] = { 0xec, 0x98 };

ubyte g_dCode[MAX_MI_NUM][MAX_DI_NUM][MAX_VI_NUM] = {
	/* value from the source code from NVM Product Planning Team (secsm10.c) */
	/* MI_SAMSUNG */
	{
		{ 0x6e, 0x00 },			/* DI_1M */
		{ 0xea, 0x64 },			/* DI_2M */
		{ 0xe3, 0xe5 },			/* DI_4M */
		{ 0xe6, 0x00 },			/* DI_8M */
		{ 0x73, 0x00 },			/* DI_16M */
		{ 0x75, 0x00 },			/* DI_32M */
		{ 0x76, 0x00 },			/* DI_64M */
		{ 0x79, 0x00 }			/* DI_128M */
	},
	/* MI_TOSHIBA */
	{
		{ 0x00, 0x00 },			/* DI_1M */
		{ 0xea, 0x64 },			/* DI_2M */
		{ 0xe5, 0x6b },			/* DI_4M */
		{ 0xe6, 0x00 },			/* DI_8M */
		{ 0x73, 0x00 },			/* DI_16M */
		{ 0x75, 0x00 },			/* DI_32M */
		{ 0x76, 0x00 },			/* DI_64M */
		{ 0x79, 0x00 }			/* DI_128M */
	}
};


/**********/

/* static funtions */
static ERR_CODE sm_IOInit(udword drv_no);
static ERR_CODE sm_PreIO(udword drv_no);
static ERR_CODE sm_PostIO(udword drv_no);
static ERR_CODE sm_StatusCheck(udword drv_no);
static ERR_CODE sm_CheckDevice(udword drv_no);


/**********
 * IO API definitions
 **********/

SMP_EXPORT ERR_CODE smpFlashWriteSector(udword drv_no, udword sector, const ubyte* p_buf, const ubyte* p_spare)
{
	udword addr = sector * SECTOR_SIZE;

	s_smpErr = smpFlashWritePage(drv_no, addr, p_buf, p_spare);
	if (s_smpErr != SM_OK)
		return s_smpErr;

	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		s_smpErr = smpFlashWritePage(drv_no, addr + PAGE_SIZE_256, p_buf + PAGE_SIZE_256, p_spare + 8);
		if (s_smpErr != SM_OK)
			return s_smpErr;
	}

	return SM_OK;
}


SMP_EXPORT ERR_CODE smpFlashReadSector(udword drv_no, udword sector, ubyte* p_buf)
{
	udword addr = sector * SECTOR_SIZE;

	s_smpErr = smpFlashReadPage(drv_no, addr, p_buf, s_pageSize[drv_no]);
	if (s_smpErr != SM_OK)
		return s_smpErr;

	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		s_smpErr = smpFlashReadPage(drv_no, addr + PAGE_SIZE_256, p_buf + PAGE_SIZE_256, PAGE_SIZE_256);
		if (s_smpErr != SM_OK)
			return s_smpErr;
	}

	return SM_OK;
}


SMP_EXPORT ERR_CODE smpFlashReadSectorSpare(udword drv_no, udword sector, ubyte* p_buf)
{
	udword addr = sector * SECTOR_SIZE;

	s_smpErr = smpFlashReadSpare(drv_no, addr, p_buf,
			(s_pageSize[drv_no] == PAGE_SIZE_256) ? 8 : 16);
	if (s_smpErr != SM_OK)
		return s_smpErr;

	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		s_smpErr = smpFlashReadSpare(drv_no, addr + PAGE_SIZE_256, p_buf + 8, 8);
		if (s_smpErr != SM_OK)
			return s_smpErr;
	}

	return SM_OK;
}

/**********
 * Function: sm_FlashWritePage
 * Notes:
 *	- write 1 page (maximum)
 *	- cannot write beyond the page boundary
 *	- write_size parameter must be given that write ends on the page boundary
 *	- if status error occurs, it tries one more time
 **********/
ERR_CODE smpFlashWritePage(udword drv_no, udword addr, const ubyte* p_buf, const ubyte* p_spare)
{
	udword i;
	udword addr0, addr1, addr2, addr3=0;
	udword pageSize = s_pageSize[drv_no];
	udword spareSize = (pageSize == PAGE_SIZE_256) ? 8 : 16;

	s_smpErr = sm_PreIO(drv_no);
	if (s_smpErr != SM_OK)
		IO_RETURN(drv_no, s_smpErr);

	SM_CHIP_EN(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, RESET_PTR_CMD);
	SM_WRITE_CMD(drv_no, SEQ_DATA_INPUT_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	SM_ALE_EN(drv_no);
	addr0 = addr & 0xff;
	if (pageSize == PAGE_SIZE_256)
	{
		addr1 = (addr >> 8) & 0xff;
		addr2 = (addr >> 16) & 0xff;
	}
	else
	{
		addr1 = (addr >> 9) & 0xff;
		addr2 = (addr >> 17) & 0xff;
		addr3 = (addr >> 25) & 0x7f;
	}

	SM_WRITE_EN(drv_no);
	SM_WRITE_ADDR(drv_no, addr0);
	SM_WRITE_ADDR(drv_no, addr1);
	SM_WRITE_ADDR(drv_no, addr2);
	if (s_deviceSize[drv_no] >= DI_64M)
	{
		SM_WRITE_ADDR(drv_no, addr3);
	}

	SM_WRITE_DIS(drv_no);
	SM_ALE_DIS(drv_no);

	SM_WRITE_EN(drv_no);

	for (i = 0; i < pageSize; ++i, ++p_buf)
	{
		SM_WRITE_DATA(drv_no, *p_buf);
	}

	for (i = 0; i < spareSize; ++i, ++p_spare)
	{
		SM_WRITE_DATA(drv_no, *p_spare);
	}

	SM_WRITE_DIS(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, PAGE_PROGRAM_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	s_smpErr = sm_StatusCheck(drv_no);

	SM_CHIP_DIS(drv_no);

	IO_RETURN(drv_no, s_smpErr);
}


/**********
 * Function: smpFlashReadPage
 * Notes:
 *	- read 1 page (maximum)
 *	- cannot read beyond the page boundary
 **********/
SMP_EXPORT ERR_CODE smpFlashReadPage(udword drv_no, udword addr, ubyte* p_buf, udword read_size)
{
	udword i;
	udword addr0, addr1, addr2, addr3=0;

	s_smpErr = sm_PreIO(drv_no);
	if (s_smpErr != SM_OK)
		IO_RETURN(drv_no, s_smpErr);

	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		if (read_size > 264 - (addr & 0xff))
			IO_RETURN(drv_no, ERR_INVALID_PARAM);
	}
	else {
		if (read_size > 528 - (addr & 0xff))
			IO_RETURN(drv_no, ERR_INVALID_PARAM);
	}

	SM_CHIP_EN(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, READ1_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	SM_ALE_EN(drv_no);
	addr0 = addr & 0xff;
	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		addr1 = (addr >> 8) & 0xff;
		addr2 = (addr >> 16) & 0xff;
	}
	else
	{
		addr1 = (addr >> 9) & 0xff;
		addr2 = (addr >> 17) & 0xff;
		addr3 = (addr >> 25) & 0x7f;
	}

	SM_WRITE_EN(drv_no);
	SM_WRITE_ADDR(drv_no, addr0);
	SM_WRITE_ADDR(drv_no, addr1);
	SM_WRITE_ADDR(drv_no, addr2);
	if (s_deviceSize[drv_no] >= DI_64M)
	{
		SM_WRITE_ADDR(drv_no, addr3);
	}

	SM_WRITE_DIS(drv_no);
	SM_ALE_DIS(drv_no);

	SM_WAIT_TR(drv_no);

	SM_READ_EN(drv_no);

	for (i = 0; i < read_size; ++i, ++p_buf)
	{
		// FIXME: where is this function suppose to be declared?
		*p_buf = SM_READ_DATA(drv_no);
	}

	SM_READ_DIS(drv_no);

	SM_CHIP_DIS(drv_no);

	SM_WAIT_TR(drv_no);

	IO_RETURN(drv_no, s_smpErr);
}


/**********
 * Function: smpFlashReadPage2
 * Remarks:
 *	- read from the second half page
 * Notes:
 *	- cannot read beyond the page boundary
 **********/
SMP_EXPORT ERR_CODE smpFlashReadPage2(udword drv_no, udword addr, ubyte* p_buf, udword read_size)
{
	udword i;
	udword addr0, addr1, addr2, addr3;

	s_smpErr = sm_PreIO(drv_no);
	if (s_smpErr != SM_OK)
		IO_RETURN(drv_no, s_smpErr);

	if (s_pageSize[drv_no] == PAGE_SIZE_256)
		IO_RETURN(drv_no, ERR_INVALID_PARAM);
	else
	{
		if (read_size > 264 - (addr & 0xff))
			IO_RETURN(drv_no, ERR_INVALID_PARAM);
	}

	SM_CHIP_EN(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, READ1_1_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	SM_ALE_EN(drv_no);
	addr0 = addr & 0xff;
	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		addr1 = (addr >> 8) & 0xff;
		addr2 = (addr >> 16) & 0xff;
	}
	else
	{
		addr1 = (addr >> 9) & 0xff;
		addr2 = (addr >> 17) & 0xff;
		addr3 = (addr >> 25) & 0x7f;
	}

	SM_WRITE_EN(drv_no);
	SM_WRITE_ADDR(drv_no, addr0);
	SM_WRITE_ADDR(drv_no, addr1);
	SM_WRITE_ADDR(drv_no, addr2);
	if (s_deviceSize[drv_no] >= DI_64M)
	{
		SM_WRITE_ADDR(drv_no, addr3);
	}

	SM_WRITE_DIS(drv_no);
	SM_ALE_DIS(drv_no);

	SM_WAIT_TR(drv_no);

	SM_READ_EN(drv_no);

	for (i = 0; i < read_size; ++i, ++p_buf)
	{
		*p_buf = SM_READ_DATA(drv_no);
	}

	SM_READ_DIS(drv_no);

	SM_CHIP_DIS(drv_no);

	SM_WAIT_TR(drv_no);

	IO_RETURN(drv_no, s_smpErr);
}


/**********
 * Function: smpFlashReadSpare
 * Remarks:
 *	- read from the spare part of the page
 * Notes:
 *	- 8 or 16 bytes maximum
 *	- cannot read beyond the page boundary
 **********/
SMP_EXPORT ERR_CODE smpFlashReadSpare(udword drv_no, udword addr, ubyte* p_buf, udword read_size)
{
	udword i;
	udword addr0, addr1, addr2, addr3=0;

	s_smpErr = sm_PreIO(drv_no);
	if (s_smpErr != SM_OK)
		IO_RETURN(drv_no, s_smpErr);

	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		if (read_size > 8 - (addr & 0x07))
			IO_RETURN(drv_no, ERR_INVALID_PARAM);
	}
	else
	{
		if (read_size > 16 - (addr & 0x0f))
			IO_RETURN(drv_no, ERR_INVALID_PARAM);
	}

	SM_CHIP_EN(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, READ2_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	SM_ALE_EN(drv_no);
	addr0 = addr & 0xff;
	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		addr1 = (addr >> 8) & 0xff;
		addr2 = (addr >> 16) & 0xff;
	}
	else
	{
		addr1 = (addr >> 9) & 0xff;
		addr2 = (addr >> 17) & 0xff;
		addr3 = (addr >> 25) & 0x7f;
	}

	SM_WRITE_EN(drv_no);
	SM_WRITE_ADDR(drv_no, addr0);
	SM_WRITE_ADDR(drv_no, addr1);
	SM_WRITE_ADDR(drv_no, addr2);

	if (s_deviceSize[drv_no] >= DI_64M)
	{
		SM_WRITE_ADDR(drv_no, addr3);
	}

	SM_WRITE_DIS(drv_no);
	SM_ALE_DIS(drv_no);

	SM_WAIT_TR(drv_no);

	SM_READ_EN(drv_no);

	for (i = 0; i < read_size; ++i, ++p_buf)
	{
		*p_buf = SM_READ_DATA(drv_no);
	}

	SM_READ_DIS(drv_no);

	SM_CHIP_DIS(drv_no);

	SM_WAIT_TR(drv_no);

	IO_RETURN(drv_no, s_smpErr);
}


/**********
 * Function: smpFlashEraseBlock
 * Remarks:
 *	- Erase 1 block
 *	- if status error occurs, it tries one more time
 **********/
SMP_EXPORT ERR_CODE smpFlashEraseBlock(udword drv_no, udword addr)
{
	udword addr1, addr2, addr3=0;

	s_smpErr = sm_PreIO(drv_no);
	if (s_smpErr != SM_OK)
		IO_RETURN(drv_no, s_smpErr);

	SM_CHIP_EN(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, BLOCK_ERASE_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	SM_ALE_EN(drv_no);
	if (s_pageSize[drv_no] == PAGE_SIZE_256)
	{
		addr1 = (addr >> 8) & 0xff;
		addr2 = (addr >> 16) & 0xff;
	}
	else
	{
		addr1 = (addr >> 9) & 0xff;
		addr2 = (addr >> 17) & 0xff;
		addr3 = (addr >> 25) & 0x7f;
	}

	SM_WRITE_EN(drv_no);
	SM_WRITE_ADDR(drv_no, addr1);
	SM_WRITE_ADDR(drv_no, addr2);

	if (s_deviceSize[drv_no] >= DI_64M)
	{
		SM_WRITE_ADDR(drv_no, addr3);
	}

	SM_WRITE_DIS(drv_no);
	SM_ALE_DIS(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, BLOCK_ERASE_CFM_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	s_smpErr = sm_StatusCheck(drv_no);

	SM_CHIP_DIS(drv_no);

	IO_RETURN(drv_no, s_smpErr);
}


/**********
 * Function: smpFlashReadID
 * Remarks:
 *	- Read Flash ID (manufacture & device ID)
 * Notes:
 *	- sm_PreIO() is not called in smpFlashReadID()
 *	- can be called from the same layer(IO layer) functions
 **********/
SMP_EXPORT ERR_CODE smpFlashReadID(udword drv_no, ubyte* p_buf, udword read_size)
{
	udword i;

	s_smpErr = sm_PreIO(drv_no);
	if (s_smpErr != SM_OK)
		IO_RETURN(drv_no, s_smpErr);

	if (read_size < 2)
		IO_RETURN(drv_no, ERR_INVALID_PARAM);

	SM_CHIP_EN(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, READ_ID_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	SM_ALE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_ADDR(drv_no, 0);
	SM_WRITE_DIS(drv_no);
	SM_ALE_DIS(drv_no);

	SM_READ_EN(drv_no);

	for (i = 0; i < read_size; ++i, ++p_buf)
	{
		*p_buf = SM_READ_DATA(drv_no);
	}

	SM_READ_DIS(drv_no);

	SM_CHIP_DIS(drv_no);

	IO_RETURN(drv_no, SM_OK);
}


/**********
 * Function: smpCheckDevice
 * Return value: SM_OK if card is inserted
 * Remarks:
 *	- Check if the SmardMedia card is inserted
 **********/
SMP_EXPORT ERR_CODE smpCheckDevice(udword drv_no)
{
	s_smpErr = sm_PreIO(drv_no);
	if (s_smpErr != SM_OK)
		IO_RETURN(drv_no, s_smpErr);

	s_smpErr = sm_CheckDevice(drv_no);

	IO_RETURN(drv_no, s_smpErr);
}


/**********
 * Function: SMPCBCardInserted
 * Remarks:
 *	- callback function that is called when the card is inserted
 *	- must call the callback function of higher layer
 **********/
SMP_EXPORT void SMPCBCardInserted(udword drv_no)
{
	s_cardInserted[drv_no] = TRUE;
	SMLCBCardInserted(drv_no);
}


/**********
 * Function: SMPCBCardEjected
 * Remarks:
 *	- callback function that is called when the card is ejected
 *	- must call the callback function of higher layer
 **********/
SMP_EXPORT void SMPCBCardEjected(udword drv_no)
{
	s_cardInserted[drv_no] = FALSE;
	s_ioFlag[drv_no] = NOT_INITIALIZED;
	SMLCBCardEjected(drv_no);
}


/**********
 * Local Function definitions
 **********/


/**********
 * Function: sm_IOInitDefaultValue
 * Remarks:
 *	- called when the file system is initialized
 **********/
void sm_IOInitDefaultValue(void)
{
	udword i;

	for (i = 0; i < MAX_DRIVE; ++i)
	{
		s_smpErr = sm_CheckDevice(i);
		if (s_smpErr == SM_OK)
			s_cardInserted[i] = TRUE;
		else
			s_cardInserted[i] = FALSE;

		s_ioFlag[i] = NOT_INITIALIZED;
	}
}


/**********
 * Function: sm_IOInit
 * Remarks:
 *	- called when the udIOFlag is not initialized and IO API is called
 **********/
ERR_CODE sm_IOInit(udword drv_no)
{
	ubyte temp_buf[2];
	udword i, j, k;

	s_ioFlag[drv_no] = NOT_INITIALIZED;

	SM_WP_DIS();

	SM_CHIP_EN(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, READ_ID_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	SM_ALE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_ADDR(drv_no, 0);
	SM_WRITE_DIS(drv_no);
	SM_ALE_DIS(drv_no);

	SM_READ_EN(drv_no);
	temp_buf[0] = SM_READ_DATA(drv_no);
	temp_buf[1] = SM_READ_DATA(drv_no);
	SM_READ_DIS(drv_no);

	SM_CHIP_DIS(drv_no);

	/* search manufacture code table */
	for (i = 0; i < MAX_MI_NUM; ++i)
	{
		if (temp_buf[0] == g_mCode[i])
			break;
	}
	if (i >= MAX_MI_NUM)
		return ERR_CARD_NOT_DETECTED;


	/* search device code table */
	for (j = 0; j < MAX_DI_NUM; ++j)
	{
		for (k = 0; k < MAX_VI_NUM; ++k)
		{
			if (temp_buf[1] == g_dCode[i][j][k])
				break;
		}

		if (k < MAX_VI_NUM)
			break;
	}
	if (j >= MAX_DI_NUM)
		return ERR_CARD_NOT_DETECTED;

	s_deviceSize[drv_no] = j;

	if (j <= DI_2M)
		s_pageSize[drv_no] = PAGE_SIZE_256;
	else
		s_pageSize[drv_no] = PAGE_SIZE_512;

	s_ioFlag[drv_no] = INITIALIZED;
	return SM_OK;
}


/**********
 * Function: sm_PreIO
 * Remarks:
 *	- called at the first of IO APIs
 **********/
ERR_CODE sm_PreIO(udword drv_no)
{
	if (s_ioFlag[drv_no] != INITIALIZED)
	{
		s_smpErr = sm_IOInit(drv_no);
		if (s_smpErr != SM_OK)
			return s_smpErr;
	}

	return SM_OK;
}


/**********
 * Function: sm_PostIO
 * Remarks:
 *	- called at the end of IO APIs
 **********/
ERR_CODE sm_PostIO(udword drv_no)
{
	return SM_OK;
}


/**********
 * Function: sm_StatusCheck
 * Remarks:
 *	- wait until the status register is ready, and read status register
 *	- called after Flash Write/Erase
 **********/
ERR_CODE sm_StatusCheck(udword drv_no)
{
	udword i;
	ubyte status;

	for (i = 0; i < WAIT_STATUS_COUNT; ++i)
	{
		SM_CLE_EN(drv_no);
		SM_WRITE_EN(drv_no);
		SM_WRITE_CMD(drv_no, READ_STATUS_CMD);
		SM_WRITE_DIS(drv_no);
		SM_CLE_DIS(drv_no);
		SM_READ_EN(drv_no);
		status = SM_READ_DATA(drv_no);
		SM_READ_DIS(drv_no);
		if (status & 0x40)
		{
			/* read one more time (it needs 5nsec after 0x40 occurs) */
			SM_CLE_EN(drv_no);
			SM_WRITE_EN(drv_no);
			SM_WRITE_CMD(drv_no, READ_STATUS_CMD);
			SM_WRITE_DIS(drv_no);
			SM_CLE_DIS(drv_no);
			SM_READ_EN(drv_no);
			status = SM_READ_DATA(drv_no);
			SM_READ_DIS(drv_no);
			break;
		}
	}

	if (i >= WAIT_STATUS_COUNT)
		return ERR_SM_TIMEOUT;

	if (status & 0x01)
		return ERR_FLASH_STATUS;

	return SM_OK;
}


/**********
 * Function: sm_CheckDevice
 * Return value: SM_OK if card is inserted
 * Remarks:
 *	- Local function that checks if the SmardMedia card is inserted
 **********/
ERR_CODE sm_CheckDevice(udword drv_no)
{
	ubyte temp_buf[2];

	SM_CHIP_EN(drv_no);

	SM_CLE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_CMD(drv_no, READ_ID_CMD);
	SM_WRITE_DIS(drv_no);
	SM_CLE_DIS(drv_no);

	SM_ALE_EN(drv_no);
	SM_WRITE_EN(drv_no);
	SM_WRITE_ADDR(drv_no, 0);
	SM_WRITE_DIS(drv_no);
	SM_ALE_DIS(drv_no);

	SM_READ_EN(drv_no);
	temp_buf[0] = SM_READ_DATA(drv_no);
	temp_buf[1] = SM_READ_DATA(drv_no);
	SM_READ_DIS(drv_no);

	SM_CHIP_DIS(drv_no);

	if ((temp_buf[0] != 0xec) && (temp_buf[0] != 0x98))
		return ERR_CARD_NOT_DETECTED;

	return SM_OK;
}

