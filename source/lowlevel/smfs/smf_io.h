/**********
 *
 * smf_io.h: SmartMedia File System IO part
 *
 * Portable File System designed for SmartMedia
 *
 * Created by Kim Hyung Gi (06/99) - kimhg@mmrnd.sec.samsung.co.kr
 * Samsung Electronics, M/M R&D Center, Visual Communication Group
 *
 **********
 * Modified by Lee Jin Woo (1999/09/02)
 *
 **********/

#ifndef SMF_IO_H
#define SMF_IO_H

#include "smf_cmn.h"


#ifdef __cplusplus
extern "C" {
#endif


/**********
 * Macro Definitions
 **********/
#define IO_RETURN(drv_no, ret_val)		do { sm_PostIO((drv_no));  return (ret_val); } while (0)


/**********
 * Value Definitions
 **********/

/* status wait loop count */
/**********
 * 1. this is used in waiting loop after program(max. 500us) / erase(3ms).
 * 2. this value is used by at least 5 instruction loop, and it must be kept looping for 3ms
 * 3. this value does not need to be very precise. It defines only the timeout value.
 *		If ready signal from SmartMedia comes in, it exits from the loop even though the loop count is not reached.
 *		Therefore it is set to the value much larger than is needed. But is must not exceed 32bit value.
 **********/
#define WAIT_STATUS_COUNT	(((CPU_MIPS / 5) * 3000) * 2)	/* twice the value as needed */

/* SmartMedia Command */
#define SEQ_DATA_INPUT_CMD	0x80
#define READ1_CMD			0x00
#define READ1_1_CMD			0x01
#define READ2_CMD			0x50
#define READ_ID_CMD			0x90
#define RESET_CMD			0xFF
#define PAGE_PROGRAM_CMD	0x10
#define BLOCK_ERASE_CMD		0x60
#define BLOCK_ERASE_CFM_CMD	0xD0
#define READ_STATUS_CMD		0x70
#define RESET_PTR_CMD		0x00

/* index for manufacture code */
#define MI_SAMSUNG			0
#define MI_TOSHIBA			1
#define MAX_MI_NUM			2

/* index for device code */
#define DI_1M				0
#define DI_2M				1
#define DI_4M				2
#define DI_8M				3
#define DI_16M				4
#define DI_32M				5
#define DI_64M				6
#define DI_128M				7
#define MAX_DI_NUM			8

/* index for voltage */
#define VI_3_3				0
#define VI_5				1
#define MAX_VI_NUM			2


/* page size */
#define PAGE_SIZE_256		256
#define PAGE_SIZE_512		512

extern ubyte g_mCode[MAX_MI_NUM];
extern ubyte g_dCode[MAX_MI_NUM][MAX_DI_NUM][MAX_VI_NUM];

void sm_IOInitDefaultValue(void);

/**********
 * I/O API
 **********/
SMP_EXPORT ERR_CODE smpFlashWriteSector(udword drv_no, udword sector, const ubyte* p_buf, const ubyte* p_spare);
SMP_EXPORT ERR_CODE smpFlashReadSector(udword drv_no, udword sector, ubyte* p_buf);
SMP_EXPORT ERR_CODE smpFlashReadSectorSpare(udword drv_no, udword sector, ubyte* p_buf);
SMP_EXPORT ERR_CODE smpFlashWritePage(udword drv_no, udword addr, const ubyte* p_buf, const ubyte* p_spare);
SMP_EXPORT ERR_CODE smpFlashReadPage(udword drv_no, udword addr, ubyte* p_buf, udword read_size);
SMP_EXPORT ERR_CODE smpFlashReadPage2(udword drv_no, udword addr, ubyte* p_buf, udword read_size);
SMP_EXPORT ERR_CODE smpFlashReadSpare(udword drv_no, udword addr, ubyte* p_buf, udword read_size);	/* max_size: 8 or 16 */
SMP_EXPORT ERR_CODE smpFlashEraseBlock(udword drv_no, udword addr);
SMP_EXPORT ERR_CODE smpFlashReadID(udword drv_no, ubyte* p_buf, udword read_size);
SMP_EXPORT ERR_CODE smpCheckDevice(udword drv_no);
SMP_EXPORT void SMPCBCardInserted(udword drv_no);
SMP_EXPORT void SMPCBCardEjected(udword drv_no);


#ifdef __cplusplus
}
#endif


#endif
