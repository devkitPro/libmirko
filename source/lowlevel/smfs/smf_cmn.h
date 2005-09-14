/**********
 *
 * smf_cmn.h: SmartMedia File System Header file
 *
 * Portable File System designed for SmartMedia
 *
 * Created by Kim Hyung Gi (06/99) - kimhg@mmrnd.sec.samsung.co.kr
 * Samsung Electronics, M/M R&D Center, Visual Communication Group
 *
 **********
 * Modified by Lee Jin Woo (1999/08/17)
 *
 ***********/


#ifndef SMF_CMN_H
#define SMF_CMN_H

#include "smf_conf.h"


#ifdef __cplusplus
extern "C" {
#endif


/**********
 * Value Definitions
 **********/
#ifndef TRUE
#ifdef __cplusplus
#define TRUE		true		/* C++ has built-in true value */
#else	/* __cplusplus */
#define TRUE		1
#endif	/* __cplusplus */
#endif	/* TRUE */

#ifndef FALSE
#ifdef __cplusplus
#define FALSE		false		/* C++ has buil-in false value */
#else	/* __cplusplus */
#define FALSE		0
#endif	/* __cplusplus */
#endif	/* FALSE */

#ifndef NULL
#ifdef __cplusplus
#define NULL		0			/* C++ doesn't allow NULL
								   to be defined as (void*)0 */
#else	/* __cplusplus */
#define NULL		(void*)0
#endif	/* __cplusplus */
#endif	/* NULL */

#define SECTOR_SIZE			512


/* value of udXXFlag */
#define NOT_INITIALIZED		0
#define INITIALIZED			10


/**********
 * Variable Type Definitions
 **********/
#ifndef __cplusplus		/* C++ has built-in bool type */
#undef bool
#define bool	int
#endif

#undef byte
#define byte	char

#undef ubyte
#define ubyte	unsigned char

#undef word
#define word	short

#undef uword
#define uword	unsigned short

#undef dword
#define dword	int

#undef udword
#define udword	unsigned int

typedef char smchar;

typedef enum {
	SM_OK,					/* 0 */
	ERR_FLASH_STATUS,
	ERR_CARD_NOT_DETECTED,
	ERR_CARD_CHANGED,
	ERR_NOT_ERASED,
	ERR_NOT_FORMATTED,
	ERR_INVALID_MBR,
	ERR_INVALID_PBR,
	ERR_INVALID_FAT,
	ERR_INVALID_NAME,
	ERR_INVALID_HANDLE,		/* 10 */
	ERR_INVALID_PARAM,
	ERR_NO_EMPTY_BLOCK,
	ERR_INVALID_BLOCK,
	ERR_ECC,
	ERR_ECC_CORRECTABLE,
	ERR_FILE_OPENED,
	ERR_FILE_EXIST,
	ERR_DIR_NOT_EMPTY,
	ERR_EOF,
	ERR_FILE_NOT_OPENED,	/* 20 */
	ERR_FILE_NOT_EXIST,
	ERR_NO_LONG_NAME,
	ERR_NOT_FOUND,
	ERR_INCORRECT_FAT,
	ERR_ROOT_DIR,
	ERR_LOCKED,
	ERR_NOT_PERMITTED,
	ERR_OUT_OF_MEMORY,
	ERR_OUT_OF_ROOT_ENTRY,
	ERR_NO_MORE_ENTRY,		/* 30 */
	ERR_SM_TIMEOUT,
	ERR_SYSTEM_PARAMETER,
	ERR_INTERNAL,
	ERR_FILE_NAME_LEN_TOO_LONG,
	ERR_NO_EMPTY_CLUSTER
} ERR_CODE;		/* 0 when success. otherwise error code */


/**********
 * Structure Type Definitions
 **********/
typedef struct {
	udword CpV;
	udword HpC;
	udword SpH;
	udword allS;
	udword szS;
	udword PBpV;
	udword LBpV;
	udword SpB;
	udword PpB;
	udword szP;
} sDEV_INFO;


/**********
 * Global Variables
 **********/


#ifdef __cplusplus
}
#endif


#endif
