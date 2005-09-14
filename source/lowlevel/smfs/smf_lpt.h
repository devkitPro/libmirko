/**********
 *
 * smf_lpt.h: SmartMedia File System Logical/Physical Transition part
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

#ifndef SMF_LPT_H
#define SMF_LPT_H

#include "smf_cmn.h"


#ifdef __cplusplus
extern "C" {
#endif


/**********
 * Value Definitions
 **********/
#define P_ZONE_MAX			1024
#define L_ZONE_MAX			1000

#define L_SECTOR_SIZE		512
#define P_SECTOR_SIZE		528

/* pLBlock, pPBlock value */
#define UNUSED_LBLOCK		0
#define INVALID_PBLOCK		0xffee
#define UNUSED_PBLOCK		0xffff

/* format mode */
#define LPT_FORMAT_NORMAL	0
#define LPT_FORMAT_RESCUE	1

#define SM_INVALID_BLOCK_MARK	0xf0	/* user defined invalid block mark. it must have 4 ~ 6 0bits. */

/**********
 * Structure Type Definitions
 **********/
typedef struct {
	udword manufacture;	/* MI_SAMSUNG, MI_TOSHIBA */
	udword device;		/* DI_1M, DI_2M, ... DI_128M */
} sDEV_ID;


void sm_LPTInitDefaultValue(void);

/**********
 * Logical/Physical Translation API
 **********/
/*SML_EXPORT ERR_CODE smlGetNewBlock(udword drv_no, udword* p_lblock_no); */
SML_EXPORT ERR_CODE smlGetNewSpecifiedBlock(udword drv_no, udword lblock_no, udword* p_changed_block);
SML_EXPORT ERR_CODE smlDiscardPhysBlock(udword drv_no, udword pba);
SML_EXPORT ERR_CODE smlWriteBlock(udword drv_no, udword lblock_no, udword offset, const void* p_buf, udword write_size);	/* max_size: block size */
SML_EXPORT ERR_CODE smlReadBlock(udword drv_no, udword lblock_no, udword offset, void* p_buf, udword read_size);
SML_EXPORT ERR_CODE smlReadPhysBlock(udword drv_no, udword pba, udword offset, void* p_buf, udword read_size);
SML_EXPORT ERR_CODE smlEraseBlock(udword drv_no, udword lblock_no);
SML_EXPORT ERR_CODE smlReadSector(udword drv_no, udword lsector_no, udword offset, void* p_buf, udword read_size);			/* max_size: L_SECTOR_SIZE */
SML_EXPORT ERR_CODE smlGetDeviceInfo(udword drv_no, const sDEV_INFO** dev_info);
SML_EXPORT ERR_CODE smlFormatVol(udword drv_no, udword format_id, udword* p_bad_count);
SML_EXPORT bool smlIsSpaceAvailable(udword drv_no, udword block, udword offset, udword count);
SML_EXPORT bool smlIsPhysSectorErased(udword drv_no, udword pba, udword sectorOffset);
SML_EXPORT ERR_CODE smlTouchBlock(udword drv_no, udword lblock);
SML_EXPORT ERR_CODE smlOptimizeVol(udword drv_no, udword opt_mode);
SML_EXPORT void SMLCBCardInserted(udword drv_no);
SML_EXPORT void SMLCBCardEjected(udword drv_no);


#ifdef __cplusplus
}
#endif


#endif
