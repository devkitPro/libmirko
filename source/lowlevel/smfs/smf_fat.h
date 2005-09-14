/**********
 *
 * smf_fat.h: SmartMedia File System FAT part
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

#ifndef SMF_FAT_H
#define SMF_FAT_H

#include "smf_cmn.h"


#ifdef __cplusplus
extern "C" {
#endif

/**********
 * Variable Ttype Definition
 **********/
typedef long F_HANDLE;  /* MSB is 1, when error. otherwise cluster
                                                 * number
                                                 *      (bits 30 - 24: device number (0 - 128))
                                                 *      (bits 23 - 17: file opened id) =>
                                                 *              used multi-access mode
                                                 */

/**********
 * Value Definitions
 * value of sSmInfo.pbr.file_sys_type
 **********/
#define FS_UNKNOWN                      0
#define FS_FAT12                        1
#define FS_FAT16                        2

/* FAT table contents */
#define UNUSED_CLUSTER          0
#define DEFECTIVE_CLUSTER       0xfff7
#define LAST_CLUSTER            0xffff

/* fcreate mode */
#define NOT_IF_EXIST            0
#define ALWAYS_CREATE           1

/* open mode (can be ORed) */
#define OPEN_R                          1
#define OPEN_W                          2

/* ddel mode */
#define NOT_IF_NOT_EMPTY        0
#define ALWAYS_DELETE           1

/* seek mode */
#define FROM_CURRENT            0
#define FROM_BEGIN                      1
#define FROM_END                        2

/* path_check mode */
#define PATH_FULL                       0
#define PATH_EXCEPT_LAST        1

/* find mode */
#define FIND_FILE_NAME          0
#define FIND_CLUSTER            1
#define FIND_FILE_INDEX         2
#define FIND_SUBDIR_INDEX       3
#define FIND_UNUSED                     4
#define FIND_DELETED            5
#define FIND_PREVIOUS           6
#define FIND_FILE_FAT_NAME      7
#define FIND_NEXT                       8

/* file or directory */
#define ENTRY_FILE                      0
#define ENTRY_DIR                       1

/* format mode */
#define FORMAT_NORMAL           0
#define FORMAT_RESCUE           1

/* callback function id */
#define CALLBACK_CARD_INSERT            0
#define CALLBACK_CARD_EJECT             1

/* File Attribute */
#define ATTRIB_DIR                      0x10
#define ATTRIB_FILE                     0x20


/* etc */
#define ROOT_DIR_ENTRY          256
#define MAX_DRIVE_NAME_LEN      10
#define MAX_PATH_NAME_LEN       256
#define MAX_OPENED_FILE_NUM     10
#define ROOT_HANDLE                     0
#define MAX_VERSION_INFO_LENGTH         40

#ifdef LONG_FILE_NAME_ENABLE
        #define MAX_FILE_NAME_LEN       80
#else
        #define MAX_FILE_NAME_LEN       12
#endif


/**********
 * Structure Type Definitions
 **********/
typedef struct {
        udword def_boot;
        udword start_cyl;
        udword start_head;
        udword start_sector;
        udword pt_type;
        udword end_cyl;
        udword end_head;
        udword end_sector;
        udword start_lba_sector;
        /* total_sectors - start_lba_sector */
        udword total_available_sectors;
} sPARTITION_ENTRY;


typedef struct {
        udword bytes_per_sector;
        udword sectors_per_cluster;
        udword reserved_sectors;
        udword fat_num;
        udword root_entry;
        udword total_sectors;
        udword format_type;
        udword sectors_in_fat;
        udword sectors_per_track;               /* sector / head */
        udword head_num;                                /* head / cylinder */
        udword hidden_sectors;
        udword huge_sectors;
} sBPB;


typedef struct {
        sPARTITION_ENTRY partition_entry[4];
        uword signature;
} sMBR;


typedef struct {
        byte oem_name[12];
        sBPB bpb;
        udword drive_num;
        udword reserved;
        udword ext_boot_signature;
        udword vol_id;
        byte vol_label[12];
        udword file_sys_type;
        udword signature;
} sPBR;


typedef struct {
        sMBR mbr;
        sPBR pbr;
} sSM_INFO;


typedef struct {
     uword year;
     ubyte month;
     ubyte day;
     ubyte hour;
     ubyte min;
     ubyte sec;
     ubyte msec;
}sTIME;


/**********
 * attr
 *      7-6 Reserved. Must be 0's.
 *      5: 1 = ARCHIVE file was modified
 *      4: 1 = DIRECTORY, 0 = file
 *      3: 1 = VOLUME label
 *      2: 1 = SYSTEM file or directory
 *      1: 1 = HIDDEN file or directory
 *      0: 1 = READONLY file
 **********/
typedef struct {
        udword attr;
        udword cluster;
        udword size;
        sTIME time;
} sFILE_STAT;


typedef struct {
        udword f_ptr;
        udword count;
        ubyte* p_buf;
} sCACHE;


typedef struct tagOPENED_LIST {
        struct tagOPENED_LIST* prev;
        struct tagOPENED_LIST* next;
        udword cluster;
        F_HANDLE h_parent;
        udword f_ptr;           /* present file pointer */
        udword cur_cluster;     /* cluster in which f_ptr lies in */
        udword old_cur_cluster; /* previous cur_cluster */
        udword size;            /* file size */
        udword mode;            /* read, write, read_write ... */
        udword id;
#ifndef WRITE_CACHE_DISABLE
        sCACHE cache;
#endif
} sOPENED_LIST;


typedef struct {
        byte name[16];
} sDIR_ENTRY;


typedef struct {
        byte name[16];
        unsigned short long_name[(MAX_FILE_NAME_LEN>>1) + 1];   /* unicode */
        sFILE_STAT stat;
} sDIR_ENTRY_EX;


typedef struct {
        udword total_size;                      /* total size of smartmedia */
        udword used_size;                       /* total used size */
        udword free_size;                       /* available size */
        udword bad_cluster_num;         /* number of FAT bad clusters
                                                                 * (not include physical invalid blocks)
                                                                 */
        udword sector_per_cluster;
        udword sector_size;
} sVOL_INFO;


typedef struct {
        char acInfo[MAX_VERSION_INFO_LENGTH];
        int iMajorVersion;
        int iMinorVersion;
} sVERSION;


/**********
 * Application API
 **********/
SM_EXPORT ERR_CODE smInit(void);
SM_EXPORT ERR_CODE smCreateFile(const smchar* p_file_name, udword fcreate_mode, F_HANDLE* p_handle);
SM_EXPORT ERR_CODE smOpenFile(const smchar* p_file_name, udword fopen_mode, F_HANDLE* p_handle);
SM_EXPORT ERR_CODE smReadFile(F_HANDLE h_file, void* p_buf, udword buf_size, udword* p_read_count);
SM_EXPORT ERR_CODE smWriteFile(F_HANDLE h_file, const void* p_buf, udword count);
SM_EXPORT ERR_CODE smSeekFile(F_HANDLE h_file, udword seek_mode, dword offset, dword* p_old_offset);
SM_EXPORT ERR_CODE smCloseFile(F_HANDLE h_file);
SM_EXPORT ERR_CODE smRemoveFile(const smchar* p_file_name);
SM_EXPORT ERR_CODE smGetSizeFile(const smchar* p_file_name,     udword* p_size);

SM_EXPORT ERR_CODE smCreateDir(const smchar* p_dir_name, udword dcreate_mode);
SM_EXPORT ERR_CODE smRemoveDir(const smchar* p_dir_name, udword ddel_mode);
SM_EXPORT ERR_CODE smGetListNumDir(const smchar* p_dir_name, udword* p_num);
SM_EXPORT ERR_CODE smReadDir(const smchar* p_dir_name, udword entry_start, udword entry_count, sDIR_ENTRY p_buf[], udword* p_read_count);
SM_EXPORT ERR_CODE smReadDirEx(const smchar* p_dir_name, udword entry_start, udword entry_count, sDIR_ENTRY_EX p_buf[], udword* p_read_count);

SM_EXPORT ERR_CODE smReadStat(const smchar* p_name, sFILE_STAT* p_stat);
SM_EXPORT ERR_CODE smWriteStat(const smchar* p_name, const sFILE_STAT* p_stat);

#ifdef  LONG_FILE_NAME_ENABLE
SM_EXPORT ERR_CODE smSetLongName(const smchar* p_file_name, const uword* p_long_name);
SM_EXPORT ERR_CODE smGetLongName(const smchar* p_file_name, uword* p_long_name);
#endif

SM_EXPORT ERR_CODE smGetVolInfo(const smchar* p_vol_name, sVOL_INFO* p_info);
SM_EXPORT ERR_CODE smFormatVol(const smchar* p_vol_name, const smchar* p_label, udword format_id, udword* p_bad_count);
SM_EXPORT ERR_CODE smMountVol(udword drv_no, const smchar* p_vol_name);
SM_EXPORT ERR_CODE smUnmountVol(udword drv_no);

SM_EXPORT ERR_CODE smFileExtend(F_HANDLE h_file, udword size);
SM_EXPORT ERR_CODE smMoveFile(const smchar* oldPath, const smchar* newPath);
SM_EXPORT ERR_CODE smRenameFile(const smchar* old_name, const smchar* new_name);
SM_EXPORT ERR_CODE smNoFATUpdate(const smchar* p_vol_name);
SM_EXPORT ERR_CODE smFATUpdate(const smchar* p_vol_name);

SM_EXPORT ERR_CODE smRegisterCallBack(udword drv_no, udword func_id, void (*pFunc)(udword));
SM_EXPORT ERR_CODE smUnregisterCallBack(udword drv_no, udword func_id);
SM_EXPORT ERR_CODE smGetVersionInfo(sVERSION *p_version);

/*
 * This will call "void appCBCardInserted(udword drv_no)".
 * App must prepare this function
 */
SM_EXPORT void SMCBCardInserted(udword drv_no);
/*
 * This will call "void appCBCardEjected(udword drv_no)".
 * App must prepare this function
 */
SM_EXPORT void SMCBCardEjected(udword drv_no);

/**********
 * extended functions
 **********/
/*
SM_EXPORT ERR_CODE smTruncateFile(F_HANDLE handle, udword size);
SM_EXPORT ERR_CODE smChangeDir(smchar* dir_name);
SM_EXPORT ERR_CODE smCopyFile(smchar* src_file_name, smchar* dst_file_name, udword mode);
SM_EXPORT ERR_CODE smCopyDir(smchar* src_dir_name, smchar* dst_dir_name, udword mode);
SM_EXPORT ERR_CODE smMoveFile(smchar* src_file_name, smchar* dst_file_name, udword mode);
SM_EXPORT ERR_CODE smMoveDir(smchar* src_dir_name, smchar* dst_dir_name, udword mode);
SM_EXPORT ERR_CODE smOptimizeVol(udword drv_no, udword opt_mode);
SM_EXPORT ERR_CODE smCheckVol(udword drv_no, udword chk_mode);
SM_EXPORT ERR_CODE smReadVol(udword drv_no, sSM_INFO* p_sm_info);
SM_EXPORT ERR_CODE smWriteVol(udword drv_no, const sSM_INFO* p_sm_info);
SM_EXPORT ERR_CODE smFlushCache(F_HANDLE h_file);
*/

#ifdef __cplusplus
}
#endif


#endif
